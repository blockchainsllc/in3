import XCTest
import Foundation
@testable import In3

final class In3Tests: XCTestCase {
    func testLocal() throws {
        let in3 = try In3(Config(chainId: "mainnet"))
        let hash = try in3.execLocal("keccak",RPCObject("simon"))
        switch hash {
            case let .string(value):
              XCTAssertEqual(value , "0x12c66c32d34a85291ac705641fb4d8cdf784dd6f84ecec01170f8d0735d54a4a")
            default:
              XCTFail("Invalid return type")
        }
    }

    func testJSON() throws {
        let in3 = try In3(Config(chainId: "mainnet"))
        let res = in3.executeJSON("{\"method\":\"keccak\",\"params\":[\"simon\"]}")
        XCTAssertEqual(res , "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"0x12c66c32d34a85291ac705641fb4d8cdf784dd6f84ecec01170f8d0735d54a4a\"}")
    }

    func testExec() throws {
        let expect = XCTestExpectation(description: "Should get a hash-value")
        let in3 = try In3(Config(chainId: "mainnet"))
        try in3.exec("keccak", RPCObject("simon"), cb: {
            switch $0 {
            case let .error(msg):
                XCTFail(msg)
            case let .success(hash):
                switch hash {
                    case let .string(value):
                      XCTAssertEqual(value , "0x12c66c32d34a85291ac705641fb4d8cdf784dd6f84ecec01170f8d0735d54a4a")
                        expect.fulfill()
                    default:
                      XCTFail("Invalid return type")
                }

            }
        })
        wait(for: [expect], timeout: 10)
    }

    func testAPI() throws {
        let expect = XCTestExpectation(description: "Should get a hash-value")
        let in3 = try In3(Config(rpc: "https://rpc.slock.it/mainnet"))
        Eth(in3).getTransactionReceipt(hash: "0xe3f6f3a73bccd73b77a7b9e9096fe07b9341e7d1d8f1ad8b8e5207f2fe349fa0") .observe(using: {
            switch $0 {
            case let .failure(err):
                print(err.localizedDescription)
            case let .success( val ):
                if let tx = val {
                    print("txhash : ",tx.hash)
                } else {
                    print("on tx found ")
                }
                expect.fulfill()
            }
        })
        wait(for: [expect], timeout: 100)

    }


    static var allTests = [
        ("execlocal", testLocal),
        ("execJSON", testJSON),
        ("exec", testExec),
    ]
}
