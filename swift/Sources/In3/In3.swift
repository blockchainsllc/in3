import CIn3
import Foundation


public class In3 {
  internal var in3: UnsafeMutablePointer<in3_t>? = nil
  public var transport: (_ url: String, _ method:String, _ payload:Data, _ headers: [String], _ cb:(_ data:TransportResult)->Void) -> Void


  public init(_ config: String) throws {
    transport = httpTransfer
    in3 = in3_for_chain_auto_init(1)
    try configure(config)
  }

  deinit {
    in3_free(in3)
  }

  public func configure(_ config: String) throws {
    let error = in3_configure(in3, config)
    if let msg = error {
      throw IncubedError.config(message: String(cString: msg))
    }
  }

  /// Execute a request directly and local.
  /// This works only for requests which do not need to be send to a server.
  public func execLocal(_ method: String, _ params: RPCObject...) throws -> RPCObject {
    let jsonReqData = try JSONEncoder().encode(JSONRequest(id: 1, method: method, params: JSONObject(RPCObject(params))))
    let rawResult = execute(String(decoding: jsonReqData, as: UTF8.self))
    let response = try JSONDecoder().decode(JSONResponse.self, from: rawResult.data(using: .utf8)!)
    if let error = response.error {
      throw IncubedError.rpc(message: error.message)
    }
    else if let result = response.result {
      return RPCObject(result)
    }
    else {
      throw IncubedError.rpc(message: "No Result in response")
    }
  }

  public func exec(_ method: String, _ params: RPCObject..., cb: @escaping  (_ result:RequestResult)->Void) throws {
    try In3Request(method,params,self,cb).exec()
  }
    
  public func execute(_ rpc: String) -> String {
    return String(cString: in3_client_exec_req(in3, makeCString(from: rpc)))
  }
}

