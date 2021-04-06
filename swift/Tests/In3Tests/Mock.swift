
import XCTest
import Foundation
import In3

internal func createIn3(mockdata:[String:String]) throws -> In3 {
//    let in1 = try In3(In3Config(chainId: "mainnet"))
    let in3 = try In3Config(chainId: "mainnet").createClient()
    return in3
}
