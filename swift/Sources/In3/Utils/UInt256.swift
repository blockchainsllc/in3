import CIn3
import Foundation


extension mp_int {
    init() {
        self = mp_int(used: 0, alloc: 0, sign: 0, dp: nil)
    }
}
/// a bigint implementation based on tommath to represent big numbers
/// It is used to represent uint256 values
final public class UInt256: CustomStringConvertible, Hashable, Comparable, Decodable, Encodable {
    var value = mp_int()

    /// creates a empt (0)-value
    public init() {
        mp_init_set(&self.value, 0)
    }

    /// i nitializes its value from a uint64 type
    public init(_ v:UInt64) {
        mp_init_set(&self.value, v)
    }

    /// inits its value from a Int
    public required convenience init(_ val: IntegerLiteralType) {
        self.init(UInt64(val))
    }
    

    /// copies the value from another UInt256
    public init(_ value: UInt256) {
        mp_init_copy(&self.value, &value.value)
    }
    
    /// initialze the value from a string.
    /// if the string starts with '0x' it will interpreted as radix 16
    /// otherwise the default for the radix is 10
    /// - Parameter radix : the radix or the base to use when parsing the String (10 - decimal, 16 - hex, 2 - binary ... )
    public init?(_ val: String, radix: Int = 10) {
        var r = radix
        var v = val
        assert(r >= 2 && r <= 36, "Only radix from 2 to 36 are supported")
        
        if v.starts(with: "0x") {
            v = String(v.suffix(from: v.index(v.startIndex, offsetBy: 2)))
            r = 16
        }
        
        if mp_read_radix(&self.value, v, Int32(r)) != 0 {
            return nil
        }
    }

    /// initializes from a decoder
    public init(from decoder: Decoder) throws {
        let v = try decoder.singleValueContainer().decode(String.self)
        if v.starts(with: "0x") {
            if mp_read_radix(&self.value, String(v.suffix(from: v.index(v.startIndex, offsetBy: 2))), Int32(16)) != 0 {
                throw IncubedError.convert(message: "Invalid UInt256 values")
            }
        } else {
            throw IncubedError.convert(message: "UInt256 value must start with '0x'")
        }
    }
    
    /// encodes the value to a decoder
    public func encode(to encoder: Encoder) throws {
        var c = encoder.singleValueContainer()
        try c.encode(self.hexValue)
    }


    /// hash of the value
    public func hash(into hasher: inout Hasher) {
        toString(radix: 16).hash(into: &hasher)
    }
    
    /// cleanup freeing the value
    deinit {
        mp_clear(&self.value);
    }

    /// returns the value as Double (as close as possible)
    public var doubleValue: Double {
        return mp_get_double(&self.value)
    }

    /// the hex representation staring with '0x'
    public var hexValue: String {
        return "0x" + self.toString(radix: 16)
    }

    /// a unsigned Int representation (if possible)
    public var uintValue: UInt {
        return mp_get_int(&self.value)
    }
 
    /// a unsigned UInt64 representation (if possible)
    public var uint64Value: UInt64 {
        return mp_get_long_long(&self.value)
    }
    
    /// a string representation based on the given radix.
    /// - Parameter radix : the radix or the base to use when parsing the String (10 - decimal, 16 - hex, 2 - binary ... )
    public func toString(radix: Int  = 10) -> String {
       assert(radix >= 2 && radix <= 36, "invalid radix")
       
       var len = Int32()
       mp_radix_size(&self.value, Int32(radix), &len)
       
       var buf = [Int8]( repeating: 0, count: Int(len+1))
       mp_toradix(&self.value, &buf, Int32(radix))
       return String(cString: buf).lowercased()
   }
   
    /// String representation as decimals
    public var description: String {
        return toString(radix: 10)
    }

    /// compare 2 UInt256 values
    /// the result is zero if they are equal
    /// negative if the current value is smaller than the given
    /// positive if the current value is higher than the given
    public func compare(other: UInt256) -> Int32 {
        return mp_cmp(&self.value, &other.value)
    }

    /// adds the given number and returns the sum of both
    public func add(_ val:UInt256) -> UInt256 {
        let res:UInt256 = UInt256()
        mp_add(&self.value, &val.value, &res.value)
        return res
    }

    /// substracts the given number and returns the difference of both
    public func sub(_ val:UInt256) -> UInt256 {
        let res:UInt256 = UInt256()
        mp_sub(&self.value, &val.value, &res.value)
        return res
    }

    /// multiplies the current with the given number and returns the product of both
    public func mul(_ val:UInt256) -> UInt256 {
        let res:UInt256 = UInt256()
        mp_mul(&self.value, &val.value, &res.value)
        return res
    }

    /// divides the current number by the given and return the result
    public func div(_ val:UInt256) -> UInt256 {
        let res:UInt256 = UInt256()
        let mod:UInt256 = UInt256()
        mp_div(&self.value, &val.value, &res.value, &mod.value)
        return res
    }

    /// divides the current number by the given and return the rest or module operator
    public func mod(_ val:UInt256) -> UInt256 {
        let res:UInt256 = UInt256()
        mp_mod(&self.value, &val.value, &res.value)
        return res
    }
}

public func == (a: UInt256, b: UInt256) -> Bool {
    return a.compare(other: b) == MP_EQ
}

public func < (a: UInt256, b: UInt256) -> Bool {
    return a.compare(other: b) == MP_LT
}

public func > (a: UInt256, b: UInt256) -> Bool {
    return a.compare(other: b) == MP_GT
}

public func >= (a: UInt256, b: UInt256) -> Bool {
    return a.compare(other: b) != MP_LT
}

public func <= (a: UInt256, b: UInt256) -> Bool {
    return a.compare(other: b) != MP_GT
}

public func + (a: UInt256, b: UInt256) -> UInt256 {
    return a.add(b)
}

public func + (a: UInt256, b: Int) -> UInt256 {
    return b < 0 ? a.sub(UInt256(-b)) : a.add(UInt256(b))
}

public func + (a: UInt256, b: UInt64) -> UInt256 {
    return a.add(UInt256(b))
}

public func - (a: UInt256, b: UInt256) -> UInt256 {
    return a.sub(b)
}

public func - (a: UInt256, b: Int) -> UInt256 {
    return b >= 0 ? a.sub(UInt256(b)) : a.add(UInt256(-b))
}

public func - (a: UInt256, b: UInt64) -> UInt256 {
    return a.sub(UInt256(b))
}

public func / (a: UInt256, b: UInt256) -> UInt256 {
    return a.div(b)
}

public func * (a: UInt256, b: UInt256) -> UInt256 {
    return a.mul(b)
}

public func % (a: UInt256, b: UInt256) -> UInt256 {
    return a.mod(b)
}


extension UInt64 {
    init(_ val:UInt256) {
        self = val.uint64Value
    }
}

