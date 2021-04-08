import Foundation

/*
 spec from https://www.jsonrpc.org/specification
 -----------------------------------------------
 Request object
 A rpc call is represented by sending a Request object to a Server. The Request object has the following members:
 jsonrpc
 A String specifying the version of the JSON-RPC protocol. MUST be exactly "2.0".
 method
 A String containing the name of the method to be invoked. Method names that begin with the word rpc followed by a period character (U+002E or ASCII 46) are reserved for rpc-internal methods and extensions and MUST NOT be used for anything else.
 params
 A Structured value that holds the parameter values to be used during the invocation of the method. This member MAY be omitted.
 id
 An identifier established by the Client that MUST contain a String, Number, or NULL value if included. If it is not included it is assumed to be a notification. The value SHOULD normally not be Null [1] and Numbers SHOULD NOT contain fractional parts [2]
 The Server MUST reply with the same value in the Response object if included. This member is used to correlate the context between the two objects.
 [1] The use of Null as a value for the id member in a Request object is discouraged, because this specification uses a value of Null for Responses with an unknown id. Also, because JSON-RPC 1.0 uses an id value of Null for Notifications this could cause confusion in handling.
 [2] Fractional parts may be problematic, since many decimal fractions cannot be represented exactly as binary fractions.
 4.1 Notification
 A Notification is a Request object without an "id" member. A Request object that is a Notification signifies the Client's lack of interest in the corresponding Response object, and as such no Response object needs to be returned to the client. The Server MUST NOT reply to a Notification, including those that are within a batch request.
 Notifications are not confirmable by definition, since they do not have a Response object to be returned. As such, the Client would not be aware of any errors (like e.g. "Invalid params","Internal error").
 4.2 Parameter Structures
 If present, parameters for the rpc call MUST be provided as a Structured value. Either by-position through an Array or by-name through an Object.
 by-position: params MUST be an Array, containing the values in the Server expected order.
 by-name: params MUST be an Object, with member names that match the Server expected parameter names. The absence of expected names MAY result in an error being generated. The names MUST match exactly, including case, to the method's expected parameters.
 Response object
 When a rpc call is made, the Server MUST reply with a Response, except for in the case of Notifications. The Response is expressed as a single JSON Object, with the following members:
 jsonrpc
 A String specifying the version of the JSON-RPC protocol. MUST be exactly "2.0".
 result
 This member is REQUIRED on success.
 This member MUST NOT exist if there was an error invoking the method.
 The value of this member is determined by the method invoked on the Server.
 error
 This member is REQUIRED on error.
 This member MUST NOT exist if there was no error triggered during invocation.
 The value for this member MUST be an Object as defined in section 5.1.
 id
 This member is REQUIRED.
 It MUST be the same as the value of the id member in the Request Object.
 If there was an error in detecting the id in the Request object (e.g. Parse error/Invalid Request), it MUST be Null.
 Either the result member or error member MUST be included, but both members MUST NOT be included.
 5.1 Error object
 When a rpc call encounters an error, the Response Object MUST contain the error member with a value that is a Object with the following members:
 code
 A Number that indicates the error type that occurred.
 This MUST be an integer.
 message
 A String providing a short description of the error.
 The message SHOULD be limited to a concise single sentence.
 data
 A Primitive or Structured value that contains additional information about the error.
 This may be omitted.
 The value of this member is defined by the Server (e.g. detailed error information, nested errors etc.).
 The error codes from and including -32768 to -32000 are reserved for pre-defined errors. Any code within this range, but not defined explicitly below is reserved for future use. The error codes are nearly the same as those suggested for XML-RPC at the following url: http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php
 code    message    meaning
 -32700    Parse error    Invalid JSON was received by the server.
 An error occurred on the server while parsing the JSON text.
 -32600    Invalid Request    The JSON sent is not a valid Request object.
 -32601    Method not found    The method does not exist / is not available.
 -32602    Invalid params    Invalid method parameter(s).
 -32603    Internal error    Internal JSON-RPC error.
 -32000 to -32099    Server error    Reserved for implementation-defined server-errors.
 */

private let jsonrpcVersion = "2.0"

internal struct JSONRequest: Codable {
    var jsonrpc: String
    var id: Int
    var method: String
    var params: JSONObject

    init(id: Int, method: String, params: JSONObject) {
        self.jsonrpc = jsonrpcVersion
        self.id = id
        self.method = method
        self.params = params
    }
}

internal struct JSONResponse: Codable {
    var jsonrpc: String
    var id: Int
    var result: JSONObject?
    var error: JSONError?

    init(id: Int, result: JSONObject) {
        self.jsonrpc = jsonrpcVersion
        self.id = id
        self.result = result
        self.error = nil
    }

    init(id: Int, error: JSONError) {
        self.jsonrpc = jsonrpcVersion
        self.id = id
        self.result = nil
        self.error = error
    }

    init(id: Int, errorCode: JSONErrorCode, error: Error) {
        self.init(id: id, error: JSONError(code: errorCode, error: error))
    }

    init(id: Int, result: RPCObject) {
        self.init(id: id, result: JSONObject(result))
    }

    init(id: Int, error: RPCError) {
        self.init(id: id, error: JSONError(error))
    }
}

internal struct JSONError: Codable {
    var code: Int
    var message: String
    var data: Dictionary<String, String>?

    init(code: Int, message: String) {
        self.code = code
        self.message = message
        self.data = nil
    }

    init(code: JSONErrorCode, message: String) {
        self.init(code: code.rawValue, message: message)
    }

    init(code: JSONErrorCode, error: Error) {
        self.init(code: code, message: String(describing: error))
    }

    init(_ error: RPCError) {
        switch error.kind {
        case .invalidMethod:
            self.init(code: .methodNotFound, message: error.description ?? "invalid method")
        case .invalidParams:
            self.init(code: .invalidParams, message: error.description ?? "invalid params")
        case .invalidRequest:
            self.init(code: .invalidRequest, message: error.description ?? "invalid request")
        case .applicationError(let description):
            self.init(code: .other, message: error.description ?? description)
        }
    }
}

internal enum JSONErrorCode: Int, Codable {
    case parseError = -32700
    case invalidRequest = -32600
    case methodNotFound = -32601
    case invalidParams = -32602
    case internalError = -32603
    case other = -32000
}

internal enum JSONObject: Codable {
    case none
    case string(String)
    case integer(Int)
    case double(Double)
    case bool(Bool)
    case list([JSONObject])
    case dictionary([String: JSONObject])

    init(_ object: RPCObject) {
        switch object {
        case .none:
            self = .none
        case .string(let value):
            self = .string(value)
        case .integer(let value):
            self = .integer(value)
        case .double(let value):
            self = .double(value)
        case .bool(let value):
            self = .bool(value)
        case .list(let value):
            self = .list(value.map { JSONObject($0) })
        case .dictionary(let value):
            self = .dictionary(value.mapValues { JSONObject($0) })
        }
    }
}

internal extension JSONObject {
    enum CodingKeys: CodingKey {
        case string
        case integer
        case double
        case bool
        case list
        case dictionary
    }

    // FIXME: is there a more elegant way?
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        do {
            let value = try container.decode(String.self)
            self = .string(value)
        } catch {
            do {
                let value = try container.decode(Int.self)
                self = .integer(value)
            } catch {
                do {
                    let value = try container.decode(Double.self)
                    self = .double(value)
                } catch {
                    do {
                        let value = try container.decode(Bool.self)
                        self = .bool(value)
                    } catch {
                        do {
                            let value = try container.decode([JSONObject].self)
                            self = .list(value)
                        } catch {
                            do {
                                let value = try container.decode([String: JSONObject].self)
                                self = .dictionary(value)
                            } catch {
                                self = .none
                            }
                        }
                    }
                }
            }
        }
    }

    func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        switch self {
        case .none:
            break
        case .string(let value):
            try container.encode(value)
        case .integer(let value):
            try container.encode(value)
        case .double(let value):
            try container.encode(value)
        case .bool(let value):
            try container.encode(value)
        case .list(let value):
            try container.encode(value)
        case .dictionary(let value):
            try container.encode(value)
        }
    }
}

/// Wrapper enum for a rpc-object, which could be different kinds
public enum RPCObject: Equatable {
    /// a JSON `null` value.
    case none

    /// a String value
    case string(String)

    /// a Integer
    case integer(Int)

    /// a Doucle-Value
    case double(Double)

    /// a Boolean
    case bool(Bool)

    /// a Array or List of RPC-Objects
    case list([RPCObject])

    /// a JSON-Object represented as Dictionary with properties as keys and their values as RPCObjects
    case dictionary([String: RPCObject])

    /// Wrap a String as Value
    public init(_ value: AnyObject) {
        switch value {
        case let val as UInt64:
            self = .integer(Int(val))
        case let val as Int:
            self = .integer(val)
        case let val as Double:
            self = .double(val)
        case let val as String:
            self = .string(val)
        case let val as Bool:
            self = .bool(val)
        case nil:
            self = .none
        case let val as [AnyObject]:
            self = .list(val.map { RPCObject($0) })
        case let val as [String]:
            self = .list(val.map { RPCObject($0) })
        case let val as [String:AnyObject]:
            self = .dictionary(val.mapValues({ RPCObject($0) }))
        default:
            self = .none
        }
    }

    /// Wrap a String as Value
    public init(_ array: [AnyObject]) {
        self = .list(array.map({ RPCObject($0)}))
    }

    /// Wrap a String as Value
    public init(_ value: String) {
        self = .string(value)
    }

    /// Wrap a Integer as Value
    public init(_ value: Int) {
        self = .integer(value)
    }

    /// Wrap a Integer as Value
    public init(_ value: UInt64) {
        self = .integer(Int(value))
    }

    /// Wrap a UInt256 as Value
    public init(_ value: UInt256) {
        self = .string("0x" + value.toString(radix: 16))
    }

    /// Wrap a Double as Value
    public init(_ value: Double) {
        self = .double(value)
    }

    /// Wrap a Bool as Value
    public init(_ value: Bool) {
        self = .bool(value)
    }

    /// Wrap a String -Array as Value
    public init(_ value: [String]) {
        self = .list(value.map { RPCObject($0) })
    }

    /// Wrap a Integer -Array as Value
    public init(_ value: [Int]) {
        self = .list(value.map { RPCObject($0) })
    }

    /// Wrap a Object with String values as Value
    public init(_ value: [String: String]) {
        self = .dictionary(value.mapValues { RPCObject($0) })
    }

    /// Wrap a Object with Integer values as Value
    public init(_ value: [String: Int]) {
        self = .dictionary(value.mapValues { RPCObject($0) })
    }

    /// Wrap a Object with RPCObject values as Value
    public init(_ value: [String: RPCObject]) {
        self = .dictionary(value)
    }

    /// Wrap a Array or List of RPCObjects as Value
    public init(_ value: [RPCObject]) {
        self = .list(value)
    }

    internal init(_ object: JSONObject) {
        switch object {
        case .none:
            self = .none
        case .string(let value):
            self = .string(value)
        case .integer(let value):
            self = .integer(value)
        case .double(let value):
            self = .double(value)
        case .bool(let value):
            self = .bool(value)
        case .list(let value):
            self = .list(value.map { RPCObject($0) })
        case .dictionary(let value):
            self = .dictionary(value.mapValues { RPCObject($0) })
        }
    }
}

/// Error of a RPC-Request
public struct RPCError {
    /// initializer
    public init(_ kind: Kind, description: String? = nil) {
        self.kind = kind
        self.description = description
    }

    /// the error-type
    public let kind: Kind

    /// the error description
    public let description: String?

    /// the error type
    public enum Kind {
        /// invalid Method
        case invalidMethod
        /// invalid Params
        case invalidParams(String)
        /// invalid Request
        case invalidRequest(String)
        /// application Error
        case applicationError(String)
    }
}

