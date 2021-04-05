import Foundation

public enum IncubedError: Error {
    case config(message: String)
    case rpc(message: String)
    case convert(message: String)
}

public enum TransportResult {
    case success(_ data:Data, _ time:Int)
    case error(_ msg:String, _ httpStatus:Int)
}

public enum RequestResult {
    case success(_ data:RPCObject)
    case error(_ msg:String)
}
