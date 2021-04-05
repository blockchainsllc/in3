
import Foundation



internal func toUInt64Promise(_ data:RPCObject, _ promise:Promise<UInt64>) {
    switch data {
    case let .integer(val):
       promise.resolve(with: UInt64(val))
    case let .string(val):
        if let intVal = UInt64(val) {
            promise.resolve(with: intVal)
        } else if val.hasPrefix("0x"), let intVal = UInt64(val.suffix(from: val.index(  val.startIndex, offsetBy: 2)), radix: 16) {
            promise.resolve(with: intVal)
        } else {
            promise.reject(with: IncubedError.rpc(message: "Invalid integer value " + val))
        }
    default:
       promise.reject(with: IncubedError.rpc(message: "Invalid returntype"))
    }
}


internal func execUInt64(_ in3:In3, _ method: String, _ params: RPCObject...) -> Future<UInt64> {
    let promise = Promise<UInt64>()
    do {
        try In3Request(method,params,in3,{
            switch $0 {
            case let .error(msg):
                promise.reject(with: IncubedError.rpc(message: msg))
            case let .success(data):
                toUInt64Promise(data,promise)
            }
        }).exec()
    } catch {
        promise.reject(with: error)
    }
    return promise
}




