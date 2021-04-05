import Foundation

public class Eth {
    internal var  in3: In3;
    
    init(_ in3:In3) {
        self.in3 = in3
    }
    
    public func blockNumber()  -> Future<UInt64> {
        return execAndConvert(in3: in3, method:"eth_blockNumber", convertWith: toUInt64)
    }
    
    public func getTransactionReceipt(hash:String)  -> Future<EthTransactionReceipt?> {
        return execAndConvertOptional(in3: in3,  method:"eth_getTransactionReceipt", params:RPCObject(hash),convertWith: { try EthTransactionReceipt($0,$1) } )
    }
    

}

public struct EthTransactionReceipt {
    public var hash: String
    
    public init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc) else {
            return nil
        }
        
        hash = try toString(obj["transactionHash"], false)!
    }
}
