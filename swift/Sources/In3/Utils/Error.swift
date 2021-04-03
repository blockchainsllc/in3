import Foundation

enum IncubedError: Error {
    case config(message: String)
    case rpc(message: String)
}

enum TransportResult {
    case success(_ data:Data, _ time:Int)
    case error(_ msg:String, _ httpStatus:Int)
}