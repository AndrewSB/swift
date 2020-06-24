// RUN: %target-swift-frontend %s -typecheck
import CoreFoundation

open class NSNull: NSObject {}

let json: [AnyHashable: Any]? = .none
assert(json?["nextPage"] == NSNull.null)
