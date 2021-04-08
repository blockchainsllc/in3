import XCTest
import Foundation
@testable import In3

final class In3Tests: XCTestCase {
    
    func testUInt256() throws {
        let val: UInt256  = UInt256(65535)
        XCTAssertEqual(val.description,"65535")
        XCTAssertEqual(val.uint64Value,UInt64(65535))
        XCTAssertEqual(val.toString(radix: 16),"ffff")
        let r = val + UInt256(1)
        XCTAssertEqual(r.description,"65536")
        if let v = UInt256("0xffff") {
            XCTAssertEqual(v.description,"65535")
        }
        else {
            XCTFail("Could not parse UINt256")
        }
        if let v = UInt256("0x1") {
            XCTAssertEqual(v.description,"1")
            XCTAssertEqual(v.hexValue,"0x1")
        } else {
            XCTFail("Could not parse UINt256")
        }
        if let v = UInt256("0x0") {
            XCTAssertEqual(v.description,"0")
            XCTAssertEqual(v.hexValue,"0x0")
        } else {
            XCTFail("Could not parse UINt256")
        }
        
        let a = UInt256(20)
        let b = a + 10
        XCTAssertEqual(b.uintValue,30)
        

    }
    
    func testLocal() throws {
        let in3 = try In3(In3Config(chainId: "mainnet"))
        let hash = try in3.execLocal("keccak",RPCObject("simon"))
        switch hash {
            case let .string(value):
              XCTAssertEqual(value , "0x12c66c32d34a85291ac705641fb4d8cdf784dd6f84ecec01170f8d0735d54a4a")
            default:
              XCTFail("Invalid return type")
        }
    }

    func testJSON() throws {
        let in3 = try In3(In3Config(chainId: "mainnet"))
        let res = in3.executeJSON("{\"method\":\"keccak\",\"params\":[\"simon\"]}")
        XCTAssertEqual(res , "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"0x12c66c32d34a85291ac705641fb4d8cdf784dd6f84ecec01170f8d0735d54a4a\"}")
    }

    func testExec() throws {
        let expect = XCTestExpectation(description: "Should get a hash-value")
        let in3 = try In3(In3Config(chainId: "mainnet"))
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
        wait(for: [expect], timeout: 1000)
    }

    func testAPI() throws {
        let expect = XCTestExpectation(description: "Should get a hash-value")
//        let in3 = try In3(Config(rpc: "https://rpc.slock.it/mainnet"))
        let in3 = try In3(In3Config(chainId: "mainnet"))
        let eth = EthAPI(in3)
        eth.getBlock().observe(using: {
            switch $0 {
            case let .failure(err):
                print(err.localizedDescription)
            case let .success( val ):
                if let b = val {
                    print("block : ",b.miner)
                } else {
                    print("on tx found ")
                }
                expect.fulfill()
            }
        })
        wait(for: [expect], timeout: 10)

    }


    static var allTests = [
        ("UInt256", testUInt256),
        ("execlocal", testLocal),
        ("execJSON", testJSON),
        ("exec", testExec),
    ]
}
