//
//  File.swift
//  
//
//  Created by Simon Jentzsch on 09.04.21.
//

import Foundation

public class Contract {
    
    var in3:In3
    var address:String?
    var abi:[ABI]
    
    public init(in3:In3, abi:[ABI],  at: String?=nil) throws {
        self.in3=in3
        self.abi=abi
        self.address = at
    }
    
    
    public func deploy(data: String, args: [AnyObject])  {
    }
    
    public func call(name: String, args: [AnyObject]) -> Future<[Any]> {
        let p = Promise<[Any]>()
        guard let adr = address,  let def = self[name] else {
            p.reject(with: IncubedError.config(message: "The Contract has no address!"))
            return p
        }
        do {
            let data = try encodeCall(name: name, args: args)
            return Eth(in3).call(tx: EthTx(to: adr, data: data)).chained(using: {
                let res = try Utils(self.in3).abiDecode(signature: def.signature, data : $0)
                return Promise(value: res.map({ $0.asObject() }) as [Any])
            })
        } catch {
            p.reject(with: error)
            return p
        }
    }

    /// returns the abi encoded arguments as hex string
    public func encodeCall(name: String, args: [AnyObject]) throws -> String{
        if let abi = self[name] {
            return try Utils(in3).abiEncode(signature: abi.signature, params: args)
        } else {
            throw IncubedError.rpc(message: "The method \(name) does not exist in the contract!")
        }
    }
    
    subscript(name: String) -> ABI? {
        get {
            return self.abi.first(where:{ $0.name == name })
        }
    }
}



/// a function, event or a
public struct ABI : Codable {
    public var hash:String?
    public var anonymous: Bool?
    public var constant: Bool?
    public var payable: Bool?
    public var stateMutability: String?
    public var components: [ABIField]?
    public var inputs: [ABIField]?
    public var outputs: [ABIField]?
    public var name: String?
    public var type: String
    public var internalType: String?
    public var signature:String {
        guard let inp = inputs, let name = self.name else {
            return "\(self.name ?? "_")()"
        }
        var sig = "\(name)(\( inp.map({ $0.signature }).joined(separator: ",")  ))"
        if let r = outputs {
            sig += ":(\( r.map({ $0.signature }).joined(separator: ",")  ))"
        }
        return sig
    }
}


/// configure the Bitcoin verification
public struct ABIField : Codable {
    public var internalType: String?
    public var components: [ABIField]?
    public var indexed: Bool?
    public var name: String
    public var type: String
    public var signature:String {
        let parts = type.split(separator: "[", maxSplits: 2)
        var baseType = String(parts[0])
        let array = parts.count == 2 ? String(parts[1]) : ""
        
        if baseType == "tuple", let cs = components {
            baseType = "(\(cs.map({ $0.signature }).joined(separator: ",")))"
        }
        return baseType + array
    }
}


