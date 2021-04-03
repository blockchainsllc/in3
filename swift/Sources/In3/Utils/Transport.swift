import Foundation

func httpTransfer(_ url: String, _ method:String, _ payload:Data, _ headers: [String], cb:(_ data:TransportResult)->Void) {
    cb(TransportResult.error("Not implemented",-7))
}
