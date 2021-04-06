import Foundation

/// Ethereum API
/// a collection of ethereum-specific functions.
public class Eth {
    internal var  in3: In3;

    /// initializer with Incubed client
    init(_ in3:In3) {
        self.in3 = in3
    }
    
    /// returns the current blockNumber
    public func blockNumber()  -> Future<UInt64> {
        return execAndConvert(in3: in3, method:"eth_blockNumber", convertWith: toUInt64)
    }

    /// returns the TransactionReceipt for the given transsaction hash
    /// - Paramater hash : the transaction hash
    /// the result is an optional value depending on the existence of such a Receipt.
    public func getTransactionReceipt(hash:String)  -> Future<EthTransactionReceipt?> {
        return execAndConvertOptional(in3: in3,  method:"eth_getTransactionReceipt", params:RPCObject(hash),convertWith: { try EthTransactionReceipt($0,$1) } )
    }
    

}

/// a TransactionReceipt
public struct EthTransactionReceipt {

    /// the transactionhash
    public var hash: String
    
    internal init?(_ rpc:RPCObject?, _ optional: Bool = true) throws {
        guard let obj = try toObject(rpc) else {
            return nil
        }
        
        hash = try toString(obj["transactionHash"], false)!
    }
}
