import Foundation
/// Base Incubed errors
public enum IncubedError: Error {

    /// Configuration Error, which is only thrown within the Config or initializer of the In3
    case config(message: String)

    /// error during rpc-execution
    case rpc(message: String)

    /// error during converting the response to a target
    case convert(message: String)
}

/// the result of a Transport operation. 
/// it will only be used internally to report the time and http-status of the response, before verifying the result.
public enum TransportResult {
    /// successful response
    /// - Parameter data : the raw data
    /// - Parameter time : the time in milliseconds to execute the request
    case success(_ data:Data, _ time:Int)

    /// failed response
    /// - Parameter msg : the error-message
    /// - Parameter httpStatus : the http status code
    case error(_ msg:String, _ httpStatus:Int)
}

/// result of a RPC-Request
public enum RequestResult {
    /// success full respons with the data as result.
    case success(_ data:RPCObject)

    /// failed request with the msg describiung the error
    case error(_ msg:String)
}
