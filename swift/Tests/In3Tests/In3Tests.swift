import XCTest
@testable import In3

final class In3Tests: XCTestCase {
    func testExample() throws {
        let in3 = try In3( "{\"chainId\":\"mainnet\"}")
        let hash = try in3.execLocal("keccak",RPCObject("simon"))
        switch hash {
            case let .string(value):
              XCTAssertEqual(value , "0x12c66c32d34a85291ac705641fb4d8cdf784dd6f84ecec01170f8d0735d54a4a")
            default:
              XCTAssertTrue(false, "Invalid return type")
        }

        let res = in3.executeJSON("{\"method\":\"keccak\",\"params\":[\"simon\"]}")
        // This is an example of a functional test case.
        // Use XCTAssert and related functions to verify your tests produce the correct
        // results.
        XCTAssertEqual(res , "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"0x12c66c32d34a85291ac705641fb4d8cdf784dd6f84ecec01170f8d0735d54a4a\"}")
    }

    static var allTests = [
        ("testExample", testExample),
    ]
}
