//
//  File.swift
//  
//
//  Created by Simon Jentzsch on 09.04.21.
//

import Foundation

internal func toHex(val:AnyObject, len:Int=0) -> String {
    var s=""
    switch val {
    case let val as UInt64:
        s = String(format: "%1x", arguments: [val])
    case let val as Int:
        s = String(format: "%1x", arguments: [val])
    case let val as Double:
        s = UInt256(String(val))?.toString(radix: 16) ?? ""
    case let val as String:
        s = val.replacingOccurrences(of: "0x", with: "")
    case let val as Bool:
        s = val ? "1" : "0"
    case nil:
        s = ""
    default:
        s = ""
    }
    while s.count < len*2 {
        s = "0" + s
    }
    return "0x" + s
}

/// represents a contract with a defined ABI
/// for the ABI-spec see https://docs.soliditylang.org/en/v0.7.4/abi-spec.html?highlight=JSON#json
public class Contract {
    
    var in3:In3
    var address:String?
    var abi:[ABI]
    var hashes:[String:ABI]

    /// creates a new Contract-Instance
    /// you need to specify either the abi or the abiJson-Property.
    /// - Parameter in3 : the Incubed instance
    /// - Parameter abi : the parsed structuured ABI-Definitions
    /// - Parameter abiJSON : the ABI as JSON-String
    /// - Parameter at : address of the deployed contract
    ///
    public init(in3:In3, abi:[ABI]? = nil, abiJSON:String? = nil, at: String?=nil) throws {
        self.in3=in3
        self.address = at
        self.hashes = [:]
        if let x = abi {
            self.abi=x
        } else if let json = abiJSON {
            self.abi = try JSONDecoder().decode([ABI].self, from: json.data(using: .utf8)!)
        } else {
            throw IncubedError.config(message: "No ABI given!")
        }
        let utils = Utils(in3)
        for var a in self.abi {
            let hash = try utils.keccak(data: a.inputSignature.replacingOccurrences(of: "indexed ", with: ""))
            self.hashes[hash] = a
            a.hash = hash
        }
    }
    
    /// deploys the contract and returns the transactionhash
    /// - Parameter data : the bytes as hex of the code to deploy
    /// - Parameter args : the optional arguments of the constructor
    /// - Parameter account : the account to send the transaction from
    /// - Parameter gas : the amount of gas to be used
    /// - Parameter gasPrice : the gasPrice. If not given the current gasPrice will be used.
    ///
    /// - Returns : The TransactionHash
    public func deploy(data: String,  args: [AnyObject]?=nil,  account:String?=nil, gas: UInt64?=nil, gasPrice: UInt64?=nil) -> Future<String> {
        do {
            return Eth(in3).sendTransaction(tx: try createDeployTx(data: data, args: args, account: account, gas: gas, gasPrice: gasPrice))
        } catch {
            let p = Promise<String>()
            p.reject(with: error)
            return p
        }
    }

    /// deploys the contract and wait until the receipt is available.
    /// - Parameter data : the bytes as hex of the code to deploy
    /// - Parameter args : the optional arguments of the constructor
    /// - Parameter account : the account to send the transaction from
    /// - Parameter gas : the amount of gas to be used
    /// - Parameter gasPrice : the gasPrice. If not given the current gasPrice will be used.
    ///
    /// - Returns : The TransactionReceipt
    public func deployAndWait(data: String,  args: [AnyObject]?=nil,  account:String?=nil, gas: UInt64?=nil, gasPrice: UInt64?=nil) -> Future<EthTransactionReceipt> {
        do {
            return Eth(in3).sendTransactionAndWait(tx: try createDeployTx(data: data, args: args, account: account, gas: gas, gasPrice: gasPrice)).chained(using: {
                self.address = $0.contractAddress
                return Promise<EthTransactionReceipt>(value:$0)
            })
        } catch {
            let p = Promise<EthTransactionReceipt>()
            p.reject(with: error)
            return p
        }
    }

    /// create a TransactionDefinition which cqan be used to deploy the contract
    /// - Parameter data : the bytes as hex of the code to deploy
    /// - Parameter args : the optional arguments of the constructor
    /// - Parameter account : the account to send the transaction from
    /// - Parameter gas : the amount of gas to be used
    /// - Parameter gasPrice : the gasPrice. If not given the current gasPrice will be used.
    ///
    /// - Returns : The Transaction Definition
    public func createDeployTx(data: String, args: [AnyObject]?=nil, account:String?=nil, gas: UInt64?=nil, gasPrice: UInt64?=nil) throws -> EthTransaction  {
        if let x = args, x.count>0 {
            var tx = try createTx(name: "", args: x, account: account, gas: gas, gasPrice: gasPrice)
            tx.data = data + String(tx.data!.suffix(from: tx.data!.index(tx.data!.startIndex, offsetBy: 10)))
            return tx
        } else {
            return EthTransaction( from: account, gas: gas, gasPrice: gasPrice,data : data)
        }
    }
    
    /// calls a function of the contract by running the code in a local evm.
    /// - Parameter name : the name of the function
    /// - Parameter args : the arguments.
    /// - Parameter account : the account to be used as sender
    /// - Parameter gas : the amount of gas to be used as limit
    ///
    /// - Returns : a array witht the return values of the function
    public func call(name: String, args: [AnyObject], block: UInt64? = nil, account:String?=nil,  gas: UInt64?=nil) -> Future<[Any]> {
        do {
            let tx = try createTx(name: name, args: args, account: account, gas: gas)
            return Eth(in3).call(tx: tx, block: block).chained(using: {
                let res = try Utils(self.in3).abiDecode(signature: self[name]!.signature, data : $0)
                return Promise(value: res.map({ $0.asObject() }) as [Any])
            })
        } catch {
            let p = Promise<[Any]>()
            p.reject(with: error)
            return p
        }
    }
    
    /// estimates the gas used to send a transaction to the specified function of the contract.
    /// - Parameter name : the name of the function
    /// - Parameter args : the arguments.
    /// - Parameter account : the account to be used as sender
    ///
    /// - Returns :the gas needed to run a tx
    public func estimateGas(name: String, args: [AnyObject], account:String?=nil) -> Future<UInt64> {
        do {
            let tx = try createTx(name: name, args: args, account: account)
            return Eth(in3).estimateGas(tx: tx)
        } catch {
            let p = Promise<UInt64>()
            p.reject(with: error)
            return p
        }
    }
    
    /// sends a transaction to a function of the contract and returns the transactionHash
    /// - Parameter name : the name of the function
    /// - Parameter args : the arguments.
    /// - Parameter account : the account to be used as sender
    /// - Parameter gas : the amount of gas to be used as limit
    /// - Parameter gasPrice : the gasPrice. if not set, the current average gasPrice will be used.
    ///
    /// - Returns : the TransactionHash
    public func sendTx(name: String,  args: [AnyObject],  account:String?=nil, gas: UInt64?=nil, gasPrice: UInt64?=nil) -> Future<String> {
        do {
            return Eth(in3).sendTransaction(tx: try createTx(name: name, args: args, account: account, gas: gas, gasPrice: gasPrice))
        } catch {
            let p = Promise<String>()
            p.reject(with: error)
            return p
        }
    }

    /// sends a transaction to a function of the contract and waits for the receipt.
    /// - Parameter name : the name of the function
    /// - Parameter args : the arguments.
    /// - Parameter account : the account to be used as sender
    /// - Parameter gas : the amount of gas to be used as limit
    /// - Parameter gasPrice : the gasPrice. if not set, the current average gasPrice will be used.
    ///
    /// - Returns : the TransactionReceipt
    public func sendTxAndWait(name: String,  args: [AnyObject],  account:String?=nil, gas: UInt64?=nil, gasPrice: UInt64?=nil) -> Future<EthTransactionReceipt> {
        do {
            return Eth(in3).sendTransactionAndWait(tx: try createTx(name: name, args: args, account: account, gas: gas, gasPrice: gasPrice))
        } catch {
            let p = Promise<EthTransactionReceipt>()
            p.reject(with: error)
            return p
        }
    }

    /// returns the abi encoded arguments as hex string
    /// - Parameter name : the name of the function
    /// - Parameter args : the arguments.
    ///
    /// - Returns : the abi encoded arguments as hex string
    public func encodeCall(name: String, args: [AnyObject]) throws -> String{
        if let abi = self[name] {
            return try Utils(in3).abiEncode(signature: abi.signature, params: args)
        } else {
            throw IncubedError.rpc(message: "The method \(name) does not exist in the contract!")
        }
    }
    
    
    /// creates the transaction parameter for a tx to the given function.
    /// - Parameter name : the name of the function
    /// - Parameter args : the arguments.
    /// - Parameter account : the account to be used as sender
    /// - Parameter gas : the amount of gas to be used as limit
    /// - Parameter gasPrice : the gasPrice. if not set, the current average gasPrice will be used.
    ///
    /// - Returns : the EthTransaction with the set parameters
    public func createTx(name: String, args: [AnyObject], account:String?=nil, gas: UInt64?=nil, gasPrice: UInt64?=nil) throws -> EthTransaction {
        if let adr = address {
            let data = try encodeCall(name: name, args: args)
            return EthTransaction(to: adr, from: account, gas: gas ?? 100000, gasPrice: gasPrice, data: data)
        } else {
            throw  IncubedError.config(message: "The Contract has no address!")
        }
    }
    
    /// reads events for the given contract
    /// if the eventName is omitted all events will be returned. ( in this case  filter must be nil ! )
    /// - Parameter eventName : the name of the event  or null if all events should be fetched
    /// - Parameter filter : the dictionary with values to search for. Only valid if the eventName is set and the all values must be indexed arguments!
    /// - Parameter fromBlock : the BlockNumber to start searching for events. If nil the latest block is used.
    /// - Parameter toBlock : the BlockNumber to end searching for events. If nil the latest block is used.
    /// - Parameter topics : the topics of the block as search criteria.
    public func getEvents(eventName:String? = nil, filter: [String:AnyObject]? = nil, fromBlock: UInt64? = nil, toBlock: UInt64? = nil, topics: [String?]?) -> Future<[EthEvent]> {
        do {
            var t = [String?].init(repeating: nil, count: 4)
            if let x = topics {
                var i = 0
                for n in x {
                    t[i] = n
                    i += 1
                }
            }
            if let name = eventName {
                if filter != nil {
                    throw IncubedError.config(message: "filter is not allowed when fetiching all event!")
                }
                guard let def = self[name], let inp = def.inputs else {
                    throw  IncubedError.config(message: "The event \(name) does not exist in the abi definition!")
                }
                t[0] = def.hash
                
                if let f = filter {
                    var i = 0
                    for d in inp {
                        if let x = d.indexed, x {
                            i += 1
                            if let val = f[d.name] {
                                t[i] = toHex(val: val, len:32)
                            }
                        }
                    }
                }
            }
            
            let ef = EthFilter(fromBlock: fromBlock, toBlock: toBlock, address: self.address, topics: t)
            return Eth(in3).getLogs(filter: ef).chained(using: {
                Promise<[EthEvent]>(value: try $0.map({ (log) -> EthEvent in
                    if let abi = self.hashes[log.topics[0]]  {
                        return try EthEvent(in3: self.in3, abi: abi, log: log)
                    } else {
                        throw IncubedError.config(message: "The eventhash \(log.topics[0]) can not be found in the config!")
                    }
                }))
            })
        } catch {
            let p = Promise<[EthEvent]>()
            p.reject(with: error)
            return p
        }
    }
    
    
    /// accesses the ABI for the given function or event
    /// - Parameter name : the name to search for
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
        if let r = outputs {
           return inputSignature + ":(\( r.map({ $0.signature }).joined(separator: ",")  ))"
        } else {
            return inputSignature
        }
    }
    public var inputSignature:String {
        guard let inp = inputs, let name = self.name else {
            return "\(self.name ?? "_")()"
        }
        return "\(name == "" ? "_" : name)(\( inp.map({ $0.signature }).joined(separator: ",")  ))"
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
        if let x = indexed, x {
            return "indexed " + baseType
        }
        return baseType + array
    }
}


public struct EthEvent {
    public var log:Ethlog
    public var event:String
    public var values:[String:AnyObject]
    
    init(in3:In3, abi:ABI, log:Ethlog) throws {
        self.log = log
        self.event = abi.name ?? ""
        self.values = [:]
        if let inp = abi.inputs {
            let vals = try Utils(in3).abiDecode(signature: abi.inputSignature, data: log.data, topics: "0x"+log.topics.map({ $0.replacingOccurrences(of: "0x", with: "")}).joined(separator: ""))
            var i = 0
            for a in inp {
                let r = vals[i]
                self.values[a.name] = r.asObject()
                i += 1
            }
        }
    }
    
}
