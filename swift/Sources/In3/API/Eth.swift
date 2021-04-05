import Foundation

public class Eth {
    internal var  in3: In3;
    
    init(_ in3:In3) {
        self.in3 = in3
    }
    
    public func blockNumber()  -> Future<UInt64> {
        return execUInt64(in3,"eth_blockNumber")
    }
}
