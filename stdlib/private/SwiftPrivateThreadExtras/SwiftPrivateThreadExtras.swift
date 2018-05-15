//===--- SwiftPrivateThreadExtras.swift ----------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file contains wrappers for pthread APIs that are less painful to use
// than the C APIs.
//
//===----------------------------------------------------------------------===//

#if os(macOS) || os(iOS) || os(watchOS) || os(tvOS)
import Darwin
#elseif os(Linux) || os(FreeBSD) || os(PS4) || os(Android) || os(Cygwin) || os(Haiku)
import Glibc
#elseif os(Windows)
import MSVCRT
#endif

public class _Thread<Argument, Result> {
	public typealias ReturnCode = Int

	let block: (Argument) -> Result
	let argument: Argument

	public init(argument: Argument, block: @escaping (Argument) -> Result) {
		self.argument = argument
		self.block = block
	}

	public func join() -> (ReturnCode, Result) {
		return (0, block(argument))
	}
}

public class _ThreadBarrier {
	public typealias ReturnCode = Int

	public static let BARRIER_SERIAL_THREAD_CODE: ReturnCode = 1

	public init?(withNumThreads threads: Int) {}

	public func wait() -> ReturnCode {
		return 0;
	}

	public func destroy() -> ReturnCode {
		return 0;
	}
}
