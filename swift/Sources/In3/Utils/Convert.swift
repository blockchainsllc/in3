
import Foundation


/// converts a RPC-Object to UInt64 or throws
internal func toUInt64(_ data:RPCObject?, _ optional:Bool = true) throws -> UInt64?{
    if let data = data {
        switch data {
        case let .integer(val):
           return UInt64(val)
        case let .string(val):
            if let intVal = UInt64(val) {
                return intVal
            } else if val.hasPrefix("0x"), let intVal = UInt64(val.suffix(from: val.index(  val.startIndex, offsetBy: 2)), radix: 16) {
               return intVal
            } else {
                throw IncubedError.config(message: "Can not convert '\(val)' to int")
            }
        case .none:
            if !optional {
                throw IncubedError.config(message: "missing value")
            }
             return nil
        default:
            throw IncubedError.config(message: "Invalid type for Int")
        }
    } else if !optional {
        throw IncubedError.config(message: "missing value")
    }
    return nil
}

/// converts a RPC-Object to UInt64 or throws
internal func toInt(_ data:RPCObject?, _ optional:Bool = true) throws -> Int?{
    if let data = data {
        switch data {
        case let .integer(val):
           return val
        case let .string(val):
            if let intVal = Int(val) {
                return intVal
            } else if val.hasPrefix("0x"), let intVal = Int(val.suffix(from: val.index(  val.startIndex, offsetBy: 2)), radix: 16) {
               return intVal
            } else {
                throw IncubedError.config(message: "Can not convert '\(val)' to int")
            }
        case .none:
            if !optional {
                throw IncubedError.config(message: "missing value")
            }
             return nil
        default:
            throw IncubedError.config(message: "Invalid type for Int")
        }
    } else if !optional {
        throw IncubedError.config(message: "missing value")
    }
    return nil
}

/// converts a RPC-Object to Double or throws
internal func toDouble(_ data:RPCObject?, _ optional:Bool = true) throws -> Double?{
    if let data = data {
        switch data {
        case let .integer(val):
           return Double(val)
        case let .double(val):
           return val
        case let .string(val):
            if let intVal = Double(val) {
                return intVal
            } else {
                throw IncubedError.config(message: "Can not convert '\(val)' to int")
            }
        case .none:
            if !optional {
                throw IncubedError.config(message: "missing value")
            }
             return nil
        default:
            throw IncubedError.config(message: "Invalid type for Double")
        }
    } else if !optional {
        throw IncubedError.config(message: "missing value")
    }
    return nil
}

/// converts a RPC-Object to Bool or throws
internal func toBool(_ data:RPCObject?, _ optional:Bool = true) throws -> Bool?{
    if let data = data {
        switch data {
        case let .integer(val):
            return val != 0;
        case let .string(val):
            if val == "true" || val=="0x1"  {
                return true
            } else if val == "false" || val == "0x0" {
               return false
            } else {
                throw IncubedError.config(message: "Invalid string to convert '\(val)' to Bool")
            }
        case let .bool(val):
            return val
        case .none:
            if !optional {
                throw IncubedError.config(message: "missing value")
            }
            return nil
        default:
            throw IncubedError.config(message: "Invalid type for bool")
        }
    } else if !optional {
        throw IncubedError.config(message: "missing value")
    }
    return nil
}

/// converts a RPC-Object to String or throws
internal func toString(_ data:RPCObject?, _ optional:Bool = true) throws -> String?{
    if let data = data {
        switch data {
        case let .integer(val):
            return String(val)
        case let .string(val):
            return val
        case let .bool(val):
            return val ? "true":"false"
        case .none:
            if !optional {
                throw IncubedError.config(message: "missing value")
            }
            return nil
        default:
            throw IncubedError.config(message: "Invalid type for String")
        }
    } else if !optional {
        throw IncubedError.config(message: "missing value")
    }
    return nil
}

/// converts a RPC-Object to a Dictory or throws
internal func toObject(_ data:RPCObject?,_  optional:Bool = true) throws -> [String: RPCObject]?{
    if let data = data {
        switch data {
        case let .dictionary(val):
            return val
        case .none:
            return nil
        default:
            throw IncubedError.config(message: "Invalid type for Object")
        }
    }
    return nil
}


/// converts a RPC-Object to a List or throws
internal func toArray(_ data:RPCObject?, _ optional:Bool = true) throws -> [RPCObject]?{
    if let data = data {
        switch data {
        case let .list(val):
            return val
        case .none:
            return nil
        default:
            throw IncubedError.config(message: "Invalid type for Array")
        }
    }
    return nil
}

/// executes a rpc-request and converts the result as non optional. (will return a rejected promise if it is `nil`)
internal func execAndConvert<Type>(in3:In3, method: String,  params: RPCObject..., convertWith: @escaping (_ data:RPCObject?, _ optional:Bool) throws -> Type?)  -> Future<Type> {
    let promise = Promise<Type>()
    do {
        try In3Request(method,params,in3,{
            switch $0 {
            case let .error(msg):
                promise.reject(with: IncubedError.rpc(message: msg))
            case let .success(data):
                do {
                   if let v = try convertWith(data,false) {
                        promise.resolve(with: v)
                   } else {
                        promise.reject(with:  IncubedError.rpc(message: "Null Value is not allowed here"))
                   }
                } catch {
                   promise.reject(with: error)
                }
            }
        }).exec()
    } catch {
        promise.reject(with: error)
    }
    return promise
}

/// executes a rpc-request and converts the result as  optional allowing `nil`as valid result.
internal func execAndConvertOptional<Type>(in3:In3, method: String,  params: RPCObject..., convertWith: @escaping (_ data:RPCObject?, _ optional:Bool) throws -> Type?)  -> Future<Type?> {
    let promise = Promise<Type?>()
    do {
        try In3Request(method,params,in3,{
            switch $0 {
            case let .error(msg):
                promise.reject(with: IncubedError.rpc(message: msg))
            case let .success(data):
                do {
                   let val = try convertWith(data, true)
                   promise.resolve(with: val)
                } catch {
                   promise.reject(with: error)
                }
            }
        }).exec()
    } catch {
        promise.reject(with: error)
    }
    return promise
}




/// executes a rpc-request and converts the result as non optional. (will return a rejected promise if it is `nil`)
internal func execLocalAndConvert<Type>(in3:In3, method: String,  params: RPCObject..., convertWith: @escaping (_ data:RPCObject?, _ optional:Bool) throws -> Type?) throws -> Type {
    let res = try in3.execLocal(method, params)
    return try convertWith(res,false)!
}
