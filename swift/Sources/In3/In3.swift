import CIn3
import Foundation

/// The Incubed client
public class In3 {
  internal var in3: UnsafeMutablePointer<in3_t>? = nil
  /// the transport function
  public var transport: (_ url: String, _ method:String, _ payload:Data?, _ headers: [String], _ cb: @escaping (_ data:TransportResult)->Void) -> Void
  
  /// set the cache impementation.
  ///
  /// the default is usually the FileCache storing data in the users home-directory, but by implementing the IN3Cache Protocol any custom storage is supported.
  var cache: In3Cache? {
        get {
            return defaultCache
        }
        set(value) {
            defaultCache = value
        }
    }

  /// initialize with a Configurations
  public init(_ config: In3Config) throws {
    transport = httpTransfer
    in3 = in3_for_chain_auto_init(1)
    try configure(config)
    try registerCache(self)
  }

  deinit {
    in3_free(in3)
  }

  /// change the configuration.
  /// - Paramater config : the partial or full Configuration to change.
  public func configure(_ config: In3Config) throws {
    let jsonConfig = try JSONEncoder().encode(config)
    let error = in3_configure(in3, String(decoding: jsonConfig, as: UTF8.self))
    if let msg = error {
      throw IncubedError.config(message: String(cString: msg))
    }
  }

    /// Execute a request directly and local.
    /// This works only for requests which do not need to be send to a server.
    public func execLocal(_ method: String, _ params: RPCObject...) throws -> RPCObject {
        return try execLocal(method, params)
    }

  /// Execute a request directly and local.
  /// This works only for requests which do not need to be send to a server.
  public func execLocal(_ method: String, _ params: [RPCObject]) throws -> RPCObject {
    let jsonReqData = try JSONEncoder().encode(JSONRequest(id: 1, method: method, params: JSONObject(RPCObject(params))))
    let rawResult = executeJSON(String(decoding: jsonReqData, as: UTF8.self))
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

    
  /// executes a asnychronous request
  ///
  /// This requires a transport to be set
  /// - Parameter method : the rpc-method to call
  /// - Parameter params : the paramas as ROCPobjects
  /// - Parameter cb : the callback which will be called with a Result (either success or error ) when done.
  public func exec(_ method: String, _ params: RPCObject..., cb: @escaping  (_ result:RequestResult)->Void) throws {
    try In3Request(method,params,self,cb).exec()
  }

  /// executes a json-rpc encoded request synchonously and returns the result as json-string
  public func executeJSON(_ rpc: String) -> String {
    return rpc.withCString { (baseAddress)->String in
        let count = rpc.utf8.count + 1
        let cstr = UnsafeMutablePointer<Int8>.allocate(capacity: count)
        defer {
            cstr.deallocate()
        }
        cstr.initialize(from: baseAddress, count: count)
        return String(cString: in3_client_exec_req(in3, cstr))
    }
  }
}

