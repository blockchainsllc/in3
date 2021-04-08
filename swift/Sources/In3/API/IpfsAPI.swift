/// this is generated file don't edit it manually!

import Foundation

/// A Node supporting IPFS must support these 2 RPC-Methods for uploading and downloading IPFS-Content. The node itself will run a ipfs-client to handle them.
/// 
/// Fetching ipfs-content can be easily verified by creating the ipfs-hash based on the received data and comparing it to the requested ipfs-hash. Since there is no chance of manipulating the data, there is also no need to put a deposit or convict a node. That's why the registry-contract allows a zero-deposit fot ipfs-nodes.
/// 
public class IpfsAPI {
    internal var in3: In3

    /// initialiazes the Ipfs API
    /// - Parameter in3 : the incubed Client
    init(_ in3: In3) {
       self.in3 = in3
    }

    /// Fetches the data for a requested ipfs-hash. If the node is not able to resolve the hash or find the data a error should be reported.
    /// - Parameter ipfshash : the ipfs multi hash
    /// - Parameter encoding : the encoding used for the response. ( `hex` , `base64` or `utf8`)
    /// - Returns: the content matching the requested hash encoded in the defined encoding.
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// IpfsAPI(in3).get(ipfshash: "QmSepGsypERjq71BSm4Cjq7j8tyAUnCw6ZDTeNdE8RUssD", encoding: "utf8") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = I love Incubed
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func get(ipfshash: String, encoding: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "ipfs_get", params:RPCObject( ipfshash), RPCObject( encoding), convertWith: toString )
    }

    /// Stores ipfs-content to the ipfs network.
    /// Important! As a client there is no garuantee that a node made this content available. ( just like `eth_sendRawTransaction` will only broadcast it). 
    /// Even if the node stores the content there is no gurantee it will do it forever. 
    /// 
    /// - Parameter data : the content encoded with the specified encoding.
    /// - Parameter encoding : the encoding used for the request. ( `hex` , `base64` or `utf8`)
    /// - Returns: the ipfs multi hash
    /// 
    /// **Example**
    /// 
    /// ```swift
    /// IpfsAPI(in3).put(data: "I love Incubed", encoding: "utf8") .observe(using: {
    ///     switch $0 {
    ///        case let .failure(err):
    ///          print("Failed because : \(err.localizedDescription)")
    ///        case let .success(val):
    ///          print("result : \(val)")
    /// //              result = QmSepGsypERjq71BSm4Cjq7j8tyAUnCw6ZDTeNdE8RUssD
    ///      }
    /// }
    /// 
    /// ```
    /// 
    public func put(data: String, encoding: String) -> Future<String> {
        return execAndConvert(in3: in3, method: "ipfs_put", params:RPCObject( data), RPCObject( encoding), convertWith: toString )
    }


}
