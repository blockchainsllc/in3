import Foundation
import CIn3

internal func httpTransfer(_ url: String, _ method:String, _ payload:Data, _ headers: [String], cb:(_ data:TransportResult)->Void) {
    cb(TransportResult.error("Not implemented",-7))
}


internal class In3Request {
    var req:UnsafeMutablePointer<in3_req_t>
    var in3:In3
    var cb: (_ result:RequestResult)->Void
    
    init(_ method: String, _ params: [RPCObject],_ _in3:In3, _ _cb: @escaping  (_ result:RequestResult)->Void) throws {
        let r = req_new(_in3.in3, String(decoding:  try JSONEncoder().encode(JSONRequest(id: 1, method: method, params: JSONObject(RPCObject(params)))), as: UTF8.self))
        if let r = r {
            req = r
        }
        else {
            throw IncubedError.rpc(message: "Invalid JSON")
        }
        in3 = _in3
        cb = _cb
    }
    
    deinit {
        req_free(req)
    }
    
    func reportError(req:UnsafeMutablePointer<in3_req_t>) {
        if let error = req.pointee.error {
            DispatchQueue.main.async {
                self.cb(RequestResult.error(String(cString: error)))
            }
        }
        else if let required = req.pointee.required {
            reportError(req: required)
        }
        else {
            DispatchQueue.main.async {
                self.cb(RequestResult.error("Unknown Error"))
            }
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
            }
            _free_(result)
        case REQ_ERROR :
            reportError(req: req)

        case REQ_WAITING_TO_SEND:
            // create request
            let http_req = in3_create_request(req)
            guard  let http = http_req else {
                reportError(req: req)
                return
            }
            for i in 0..<Int(http.pointee.urls_len) {
                let cbResult = {(_ res:TransportResult) -> Void in 
                    switch res {
                    case let .success(data, time):
                        let ptr = data.withUnsafeBytes { ptr in return ptr.baseAddress?.assumingMemoryBound(to: Int8.self) }
                        if let p = ptr {
                            in3_req_add_response(http,Int32(i),0,p,Int32(data.count), UInt32(time))
                        }
                    case let .error(msg,httpStatus):
                        let ptr = msg.data(using: .utf8)!.withUnsafeBytes { ptr in return ptr.baseAddress?.assumingMemoryBound(to: Int8.self) }
                        if let p = ptr {
                            in3_req_add_response(http,Int32(i),Int32(-httpStatus),p,Int32(-1), UInt32(0))
                        }
                    }
                    self.exec()
                }


                // do we need before sending the request?
                if http.pointee.wait > 0 {
                    DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(Int(http.pointee.wait)) ) {
                        self.sendHttp(http: http, index:i, cb:cbResult)
                    }
                } else {
                    self.sendHttp(http: http, index:i, cb: cbResult)
                }
            }
           
        case REQ_WAITING_FOR_RESPONSE:
            // create request
            // for i=0 i< urls_len
              // send it with callbacks
              // in3.transport
            DispatchQueue.main.async {
                self.cb(RequestResult.error( "not implemented yet :"))
            }
        default:
            DispatchQueue.main.async {
                self.cb(RequestResult.error( "not expected"))
            }
        }
    }
    
    func sendHttp(http:UnsafeMutablePointer<in3_http_request_t>, index:Int, cb:(_ data:TransportResult)->Void) {
        
        
    }
    
    
}
