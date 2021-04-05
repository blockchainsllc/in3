import Foundation
import CIn3

internal func httpTransfer(_ surl: String, _ method:String, _ payload:Data?, _ headers: [String], cb:@escaping (_ data:TransportResult)->Void) {
    
    guard let url = URL(string:surl) else {
        DispatchQueue.main.async {
           cb(TransportResult.error("no valid url",-7))
        }
        return
    }
    
    let reqStart = Date()
    var request = URLRequest(url: url)
    request.httpMethod = method
    request.httpBody = payload
    if method == "POST" {
        request.addValue("application/json", forHTTPHeaderField: "Content-Type")
        request.addValue("application/json", forHTTPHeaderField: "Accept")
    }
    for h in headers {
        let tokens:[String] = h.split(separator: ":", maxSplits: 2).map { String($0).trimmingCharacters(in: NSCharacterSet.whitespacesAndNewlines) }
        if tokens.count == 2 {
            request.addValue(tokens[1], forHTTPHeaderField: tokens[0])
        }
    }
    URLSession.shared.dataTask(with: request,completionHandler: { data, response, error in

        if let err = error {
            DispatchQueue.main.async {
                 cb(TransportResult.error("Could not send request : \(err)", 500))
            }
            return
        }
        guard let resp = response as? HTTPURLResponse else {
            DispatchQueue.main.async {
                 cb(TransportResult.error("Invalid response", 500))
            }
            return
        }
        guard let d = data else {
            DispatchQueue.main.async {
                 cb(TransportResult.error("No data", 500))
            }
            return
        }
        if resp.statusCode < 400 && resp.statusCode >= 200 {
            DispatchQueue.main.async {
                cb(TransportResult.success(d, Int(Date().timeIntervalSince(reqStart)*1000)))
            }
        } else {
            DispatchQueue.main.async {
                cb(TransportResult.error("Invalid Status :\(resp.statusCode)", resp.statusCode))
            }
        }
    }).resume()

    
    
}


internal class In3Request {
    var req:UnsafeMutablePointer<in3_req_t>
    var in3:In3
    var cb: (_ result:RequestResult)->Void
    var freed: Bool
    
    init(_ method: String, _ params: [RPCObject],_ _in3:In3, _ _cb: @escaping  (_ result:RequestResult)->Void) throws {
       let r = req_new_clone(_in3.in3, String(decoding:  try JSONEncoder().encode(JSONRequest(id: 1, method: method, params: JSONObject(RPCObject(params)))), as: UTF8.self))
        if let r = r {
            req = r
        }
        else {
            throw IncubedError.rpc(message: "Invalid JSON")
        }
        in3 = _in3
        cb = _cb
        freed = false
    }
    
    func free() {
        if !freed {
            freed = true
            req_free(req)
        }
    }
    
    deinit {
        self.free()
    }
    
    func reportError(req:UnsafeMutablePointer<in3_req_t>) {
        if let error = req.pointee.error {
            DispatchQueue.main.async {
                self.cb(RequestResult.error(String(cString: error)))
            }
            self.free()
        }
        else if let required = req.pointee.required {
            reportError(req: required)
        }
        else {
            DispatchQueue.main.async {
                self.cb(RequestResult.error("Unknown Error"))
            }
            self.free()
        }
    }
    
    func exec() {
        switch in3_req_exec_state(req) {
        case REQ_SUCCESS :
            let result = req_get_result_json(req,0)
            if let res = result {
                let resultString = String(cString: res)
                do {
                    let response = try JSONDecoder().decode(JSONObject.self, from: resultString.data(using: .utf8)!)
                    DispatchQueue.main.async {
                        self.cb(RequestResult.success(RPCObject(response)))
                    }
                } catch {
                    DispatchQueue.main.async {
                       self.cb(RequestResult.error( "Error parsing the result '\(resultString)' : \(error)"))
                    }
                }
            } else {
                DispatchQueue.main.async {
                   self.cb(RequestResult.error( "The response was null"))
                }
            }
            
            _free_(result)
            self.free()

        case REQ_ERROR :
            reportError(req: req)


        case REQ_WAITING_TO_SEND:
            // create request
            let http_req = in3_create_request(req)
            guard  let http = http_req else {
                reportError(req: req)
                return
            }
            let req_ptr = http.pointee.req
            guard  let req = req_ptr else {
                reportError(req: self.req)
                return
            }

            for i in 0..<Int(http.pointee.urls_len) {
                let cbResult = {(_ res:TransportResult) -> Void in
                    // in case the response is not needed, we simply discard it.
                    if self.freed || in3_req_state(req) != REQ_WAITING_FOR_RESPONSE {
                       return
                    }
                    
                    // set the response...
                    switch res {
                    case let .success(data, time):
                        data.withUnsafeBytes {
                            in3_ctx_add_response(req,Int32(i),0,$0.baseAddress?.assumingMemoryBound(to: Int8.self),Int32(data.count), UInt32(time))
                        }
                    case let .error(msg,httpStatus):
                        msg.data(using: .utf8)!.withUnsafeBytes {
                            in3_ctx_add_response(req,Int32(i),Int32(-httpStatus),$0.baseAddress?.assumingMemoryBound(to: Int8.self),Int32(-1), UInt32(0))
                        }
                    }
                    // now try to verify the response
                    self.exec()
                }

                // do we need to wait before sending the request?
                if http.pointee.wait > 0 {
                    DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(Int(http.pointee.wait)) ) {
                        self.sendHttp(http: http, index:i, cb:cbResult)
                    }
                } else {
                    self.sendHttp(http: http, index:i, cb: cbResult)
                }
            }
           
        case REQ_WAITING_FOR_RESPONSE:
            // here we do nothing, but simply wait for the next event
            return
        default:
            DispatchQueue.main.async {
                self.cb(RequestResult.error( "not expected"))
            }
        }
    }
    
    func sendHttp(http:UnsafeMutablePointer<in3_http_request_t>, index:Int, cb:@escaping (_ data:TransportResult)->Void) {
        let payload:Data? = http.pointee.payload_len == 0 ? nil : Data(buffer:UnsafeMutableBufferPointer(start: http.pointee.payload, count: Int(http.pointee.payload_len)))
        var headers:[String] = []
        var p = http.pointee.headers
        let url = http.pointee.urls + index
        while let ph = p {
             headers.append(String(cString: ph.pointee.value))
             p = ph.pointee.next
        }
        if let url = url.pointee {
           in3.transport( String(cString:url) ,   String(cString: http.pointee.method), payload, headers, cb)
        } else {
             DispatchQueue.main.async {
                cb(TransportResult.error("No URL specified",500))
            }
        }
        
        // if this was the last request we sent out, we clean up the request.
        if index == http.pointee.urls_len - 1 {
           request_free(http)
        }
    }
    
    
}
