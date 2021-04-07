import CIn3
import Foundation


extension mp_int {
    init() {
        self = mp_int(used: 0, alloc: 0, sign: 0, dp: nil)
    }
}
/// a bigint implementation based on tommath to represent big numbers
public class UInt256: CustomStringConvertible, Hashable, Comparable {
    var value = mp_int()

    public init() {
        mp_init_set(&self.value, 0)
    }
    
    public init(_ v:UInt64) {
        mp_init_set(&self.value, v)
    }
    
    public required convenience init(integerLiteral value: IntegerLiteralType) {
        self.init(UInt64(value))
    }

    public init(_ value: UInt256) {
        mp_init_copy(&self.value, &value.value)
    }
    
    public func hash(into hasher: inout Hasher) {
        toString(radix: 16).hash(into: &hasher)
    }
    
    public init?(_ val: String, radix: Int = 10) {
        var r = radix
        var v = val
        assert(r >= 2 && r <= 36, "Only radix from 2 to 36 are supported")
        
        if (v.starts(with: "0x")) {
            v = String(v.suffix(from: v.index(v.startIndex, offsetBy: 2)))
            r = 16
        }
        
        if mp_read_radix(&self.value, v, Int32(r)) != 0 {
            return nil
        }
    }

    deinit {
        mp_clear(&self.value);
    }
    
    public var doubleValue: Double {
        return mp_get_double(&self.value)
    }
    
    public var uintValue: UInt {
        return mp_get_int(&self.value)
    }
 
    public var uint64Value: UInt64 {
        return mp_get_long_long(&self.value)
    }
    
    public func toString(radix: Int  = 10) -> String {
       assert(radix >= 2 && radix <= 36, "invalid radix")
       
       var len = Int32()
       mp_radix_size(&self.value, Int32(radix), &len)
       
       var buf = [Int8]( repeating: 0, count: Int(len+1))
       mp_toradix(&self.value, &buf, Int32(radix))
       return String(cString: buf).lowercased()
   }
   
    public var description: String {
        return toString(radix: 10)
    }

    public func compare(other: UInt256) -> Int32 {
        return mp_cmp(&self.value, &other.value)
    }
    
    public func add(_ val:UInt256) -> UInt256 {
        let res:UInt256 = UInt256()
        mp_add(&self.value, &val.value, &res.value)
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
