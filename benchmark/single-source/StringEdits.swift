//===--- StringEdits.swift ------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - current Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

import TestsUtils
#if os(Linux)
import Glibc
#else
import Darwin
#endif

public let StringEdits = BenchmarkInfo(
  name: "StringEdits",
  runFunction: run_StringEdits,
  tags: [.validation, .api, .String])

var editWords: [String] = [
  "woodshed",
  "lakism",
  "gastroperiodynia",
]

let alphabet = "abcdefghijklmnopqrstuvwxyz"
/// All edits that are one edit away from `word`
func edits(_ word: String) -> Set<String> {
  let splits = word.indices.map {
    (String(word[..<$0]), String(word[$0...]))
  }
  
  var result: Array<String> = []
  
  for (left, right) in splits {
    // drop a character
    result.append(left + right.dropFirst())
    
    // transpose two characters
    if let fst = right.first {
      let drop1 = right.dropFirst()
      if let snd = drop1.first {
        result.append(left + [snd,fst] + drop1.dropFirst())
      }
    }
    
    // replace each character with another
    for letter in alphabet {
      result.append(left + [letter] + right.dropFirst())
    }
    
    // insert rogue characters
    for letter in alphabet {
      result.append(left + [letter] + right)
    }
  }
  
  // have to map back to strings right at the end
  return Set(result)
}

@inline(never)
public func run_StringEdits(_ N: Int) {
  for _ in 1...N*100 {
    for word in editWords {
      _ = edits(word)      
    }
  }
}

