import CIn3

enum IncubedError: Error {
    case config(message: String)
    case rpc(message: String)
}


public class In3 {
  var in3: UnsafeMutablePointer<in3_t>? = nil

  func makeCString(from str: String) -> UnsafeMutablePointer<Int8> {
    let count = str.utf8.count + 1
    let result = UnsafeMutablePointer<Int8>.allocate(capacity: count)
    str.withCString { (baseAddress) in
        // func initialize(from: UnsafePointer<Pointee>, count: Int) 
        result.initialize(from: baseAddress, count: count)
    }
    return result
  }

  public init(_ config: String) throws {
    in3 = in3_for_chain_auto_init(1)
    let error = in3_configure(in3, config)
    if let msg = error {
      throw IncubedError.config(message: String(cString: msg))
    }
  }

  deinit {
    in3_free(in3)
  }

  public func execute(_ rpc: String) -> String {
    return String(cString: in3_client_exec_req(in3, makeCString(from: rpc)))
  }
}

