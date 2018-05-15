// RUN: %target-run-simple-swift
// REQUIRES: executable_test
// REQUIRES: stress_test

import StdlibUnittest
import SwiftPrivateThreadExtras
#if os(OSX) || os(iOS)
import Darwin
#elseif os(Linux) || os(Cygwin) || os(Android)
import Glibc
#elseif os(Windows)
import MSVCRT
#endif


var StringTestSuite = TestSuite("String")

extension String {
  var capacity: Int {
    return _guts.capacity
  }
}

// Swift.String used to hsve an optimization that allowed us to append to a
// shared string buffer.  However, as lock-free programming invariably does, it
// introduced a race condition [rdar://25398370 Data Race in StringBuffer.append
// (found by TSan)].
//
// These tests verify that it works correctly when two threads try to append to
// different non-shared strings that point to the same shared buffer.  They used
// to verify that the first append could succeed without reallocation even if
// the string was held by another thread, but that has been removed.  This could
// still be an effective thread-safety test, though.

enum ThreadID {
  case Primary
  case Secondary
}

var barrierVar: _ThreadBarrier?
var sharedString: String = ""
var secondaryString: String = ""

func barrier() {
  var ret = barrierVar!.wait()
  expectTrue(ret == 0 || ret == _ThreadBarrier.BARRIER_SERIAL_THREAD_CODE)
}

func sliceConcurrentAppendThread(_ tid: ThreadID) {
  for i in 0..<100 {
    barrier()
    if tid == .Primary {
      // Get a fresh buffer.
      sharedString = ""
      sharedString.append("abc")
      sharedString.reserveCapacity(16)
      expectLE(16, sharedString.capacity)
    }

    barrier()

    // Get a private string.
    var privateString = sharedString

    barrier()

    // Append to the private string.
    if tid == .Primary {
      privateString.append("def")
    } else {
      privateString.append("ghi")
    }

    barrier()

    // Verify that contents look good.
    if tid == .Primary {
      expectEqual("abcdef", privateString)
    } else {
      expectEqual("abcghi", privateString)
    }
    expectEqual("abc", sharedString)

    // Verify that only one thread took ownership of the buffer.
    if tid == .Secondary {
      secondaryString = privateString
    }
    barrier()
  }
}

StringTestSuite.test("SliceConcurrentAppend") {
  barrierVar = _ThreadBarrier(withNumThreads: 2)!

  let primaryThread = _Thread(.Primary, sliceConcurrentAppendThread)
  let secondaryThread = _Thread(.Secondary, sliceConcurrentAppendThread)

  let (joinRet1, _) = primaryThread.join()
  let (joinRet2, _) = secondaryThread.join()

  expectEqual(0, joinRet1)
  expectEqual(0, joinRet2)

  expectEqual(0, barrierVar!.destroy())
}

runAllTests()
