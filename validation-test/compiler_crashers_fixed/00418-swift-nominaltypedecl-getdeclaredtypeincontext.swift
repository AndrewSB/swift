// This source file is part of the Swift.org open source project
// Copyright (c) 2014 - current Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors

// RUN: not %target-swift-frontend %s -typecheck
if true {
func f: NSObject {
}
struct S<T where T: NSObject {
class A
