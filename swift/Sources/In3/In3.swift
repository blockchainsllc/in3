import CIn3
import Foundation

enum IncubedError: Error {
    case config(message: String)
    case rpc(message: String)
}


public class In3 {
  var in3: UnsafeMutablePointer<in3_t>? = nil

  internal func makeCString(from str: String) -> UnsafeMutablePointer<Int8> {
    let count = str.utf8.count + 1
    let result = UnsafeMutablePointer<Int8>.allocate(capacity: count)
    str.withCString { (baseAddress) in
        result.initialize(from: baseAddress, count: count)
    }
    return result
  }

  public init(_ config: String) throws {
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

  public func execLocal(_ method: String, _ params: RPCObject) throws -> RPCObject {
    let jsonReqData = try JSONEncoder().encode(JSONRequest(id: 1, method: method, params: JSONObject(params)))
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

  public func execute(_ rpc: String) -> String {
    return String(cString: in3_client_exec_req(in3, makeCString(from: rpc)))
  }
}

