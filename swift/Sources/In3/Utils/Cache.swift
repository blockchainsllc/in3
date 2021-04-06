import Foundation
import CIn3

/// Protocol for Cache-Implementation.
/// The cache is used to store data like nodelists and their reputations, contract codes and more.
/// those calls a synchronous calls and should be fast.
/// in order to set the cache, use the `In3.cache`-property.
public protocol In3Cache {
    
    /// find the data for the given cache-key or `nil`if not found.
    func getEntry(key:String)->Data?

    /// write the data to the cache using the given key..
    func setEntry(key:String,value:Data)->Void

    /// clears all cache entries
    func clear()->Void
}

/// File-Implementation for the cache.
public class FileCache : In3Cache {
    var dir:URL
    
    /// creates the cache-directory in the USers home-directory with hte name `.in3`
    convenience init() throws {
        try self.init(URL(fileURLWithPath: ".in3", relativeTo: FileManager.default.homeDirectoryForCurrentUser))
    }

    /// caches data in the given directory
    convenience init(_ dir:String) throws {
        try self.init(URL(fileURLWithPath: dir, isDirectory: true))
    }

    /// caches data in the given directory
    init(_ dir:URL) throws {
        try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
        self.dir = dir
    }
    
    /// write the data to the cache using the given key..
    public func setEntry(key: String, value: Data) {
         try? value.write(to: URL(fileURLWithPath: key, relativeTo: dir))
    }
    
    /// find the data for the given cache-key or `nil`if not found.
    public func getEntry(key: String) -> Data? {
          return try? Data(contentsOf: URL(fileURLWithPath: key, relativeTo: dir))
    }
    
    /// clears all cache entries
    public func clear() {
           try? FileManager.default.removeItem(at: dir)
           try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
    }
}

internal var defaultCache:In3Cache?

internal func  registerCache(_ in3:In3) throws {
    if defaultCache == nil {
       defaultCache = try FileCache()
    }
    var cbs = swift_cb_t(
        cache_get:  { ctx -> in3_ret_t in
            let key = String(cString: ctx.pointee.key)
            if let cache = defaultCache,
               let data = cache.getEntry(key: key) {
                    data.withUnsafeBytes { (ptr: UnsafeRawBufferPointer )  in
                        ctx.pointee.content = b_new(ptr.baseAddress?.assumingMemoryBound(to: UInt8.self), UInt32(data.count))
                    }
                    return IN3_OK
                } else {
                    return IN3_EIGNORE
                }
        },
        cache_set:  {  ctx -> in3_ret_t in
            let key = String(cString: ctx.pointee.key)
            if let cache = defaultCache {
                cache.setEntry(key: key, value: Data(bytes: ctx.pointee.content.pointee.data, count: Int(ctx.pointee.content.pointee.len)))
                return IN3_OK
            }
            return IN3_EIGNORE
        },
        cache_clear: {
            if let cache = defaultCache {
                cache.clear()
            }
            return IN3_OK
        }
    )
    
    if let in3ptr = in3.in3 {
        in3_register_swift(in3ptr, &cbs)
    }
    
}

