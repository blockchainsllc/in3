import Foundation
import CIn3

public protocol In3Cache {
    func getEntry(key:String)->Data?
    func setEntry(key:String,value:Data)->Void
    func clear()->Void
}


public class FileCache : In3Cache {
    var dir:URL
    
    convenience init() throws {
        try self.init(URL(fileURLWithPath: ".in3", relativeTo: FileManager.default.homeDirectoryForCurrentUser))
    }
    convenience init(_ dir:String) throws {
        try self.init(URL(fileURLWithPath: dir, isDirectory: true))
    }

    init(_ dir:URL) throws {
        try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
        self.dir = dir
    }
    
    public func setEntry(key: String, value: Data) {
        do {
         try value.write(to: URL(fileURLWithPath: key, relativeTo: dir))
        } catch {
        }
    }
    
    public func getEntry(key: String) -> Data? {
        do {
            return try Data(contentsOf: URL(fileURLWithPath: key, relativeTo: dir))
        } catch {
            return nil
        }
    }
    
    public func clear() {
        do {
           try FileManager.default.removeItem(at: dir)
           try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
        } catch {
        }
    }
}

internal var defaultCache:In3Cache?

internal func  registerCache(_ in3:In3) {
    if defaultCache == nil {
        do {
            defaultCache = try FileCache()
        } catch {
            print("Could not init the cache : \(error)")
        }
    }
    var cbs = swift_cb_t(
        cache_get:  { ctx -> in3_ret_t in
            if let cache = defaultCache {
                let key = String(cString: ctx.pointee.key)
                let value = cache.getEntry(key: key)
                if let data = value {
                    data.withUnsafeBytes { (ptr: UnsafeRawBufferPointer )  in
                        ctx.pointee.content = b_new(ptr.baseAddress?.assumingMemoryBound(to: UInt8.self), UInt32(data.count))
                    }
                    return IN3_OK
                } else {
                    return IN3_EIGNORE
                }
            }
            return IN3_OK
        },
        cache_set:  {  ctx -> in3_ret_t in
            if let cache = defaultCache {
                let key = String(cString: ctx.pointee.key)
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
    
    
    /*
    let callbacks = callbacks(
        cl
        
        printGreeting: { (modifier) in
            printGreeting(modifier: modifier)
        }
    )
    */
}

/*

private func printGreeting(modifier: UnsafePointer<CChar>) {
    print("Hello \(String(cString: modifier))World!")
}

var callbacks = SomeCLibCallbacks(
    printGreeting: { (modifier) in
        printGreeting(modifier: modifier)
    }
)
SomeCLibSetup(&callbacks)
*/
