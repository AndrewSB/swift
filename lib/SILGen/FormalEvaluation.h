//===--- FormalEvaluation.h -------------------------------------*- C++ -*-===//
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

#ifndef SWIFT_SILGEN_FORMALEVALUATION_H
#define SWIFT_SILGEN_FORMALEVALUATION_H

#include "Cleanup.h"
#include "swift/Basic/DiverseStack.h"
#include "swift/SIL/SILValue.h"
#include "llvm/ADT/Optional.h"

namespace swift {
namespace Lowering {

class SILGenFunction;
class LogicalPathComponent;

class FormalAccess {
public:
  enum Kind { Shared, Exclusive, Owned };

private:
  unsigned allocatedSize;
  Kind kind;

protected:
  SILLocation loc;
  CleanupHandle cleanup;
  bool finished;

  FormalAccess(unsigned allocatedSize, Kind kind, SILLocation loc,
               CleanupHandle cleanup)
      : allocatedSize(allocatedSize), kind(kind), loc(loc), cleanup(cleanup),
        finished(false) {}

public:
  virtual ~FormalAccess() {}

  // This anchor method serves three purposes: it aligns the class to
  // a pointer boundary, it makes the class a primary base so that
  // subclasses will be at offset zero, and it anchors the v-table
  // to a specific file.
  virtual void _anchor();

  /// Return the allocated size of this object.  This is required by
  /// DiverseStack for iteration.
  size_t allocated_size() const { return allocatedSize; }

  CleanupHandle getCleanup() const { return cleanup; }

  Kind getKind() const { return kind; }

  void finish(SILGenFunction &SGF) { finishImpl(SGF); }

  void setFinished() { finished = true; }

  bool isFinished() const { return finished; }

  void verify(SILGenFunction &SGF) const;

protected:
  virtual void finishImpl(SILGenFunction &SGF) = 0;
};

class SharedBorrowFormalAccess : public FormalAccess {
  SILValue originalValue;
  SILValue borrowedValue;

public:
  SharedBorrowFormalAccess(SILLocation loc, CleanupHandle cleanup,
                           SILValue originalValue, SILValue borrowedValue)
      : FormalAccess(sizeof(*this), FormalAccess::Shared, loc, cleanup),
        originalValue(originalValue), borrowedValue(borrowedValue) {}

  SILValue getBorrowedValue() const { return borrowedValue; }
  SILValue getOriginalValue() const { return originalValue; }

private:
  void finishImpl(SILGenFunction &SGF) override;
};

class OwnedFormalAccess : public FormalAccess {
  SILValue value;

public:
  OwnedFormalAccess(SILLocation loc, CleanupHandle cleanup, SILValue value)
      : FormalAccess(sizeof(*this), FormalAccess::Owned, loc, cleanup),
        value(value) {}

  SILValue getValue() const { return value; }

private:
  void finishImpl(SILGenFunction &SGF) override;
};

class FormalEvaluationContext {
  DiverseStack<FormalAccess, 128> stack;

public:
  using stable_iterator = decltype(stack)::stable_iterator;
  using iterator = decltype(stack)::iterator;

  FormalEvaluationContext() : stack() {}

  // This is a type that can only be embedded in other types, it can not be
  // moved or copied.
  FormalEvaluationContext(const FormalEvaluationContext &) = delete;
  FormalEvaluationContext(FormalEvaluationContext &&) = delete;
  FormalEvaluationContext &operator=(const FormalEvaluationContext &) = delete;
  FormalEvaluationContext &operator=(FormalEvaluationContext &&) = delete;

  ~FormalEvaluationContext() {
    assert(stack.empty() &&
           "entries remaining on formal evaluation cleanup stack at end of function!");
  }

  iterator begin() { return stack.begin(); }
  iterator end() { return stack.end(); }
  stable_iterator stabilize(iterator iter) const {
    return stack.stabilize(iter);
  }
  stable_iterator stable_begin() { return stabilize(begin()); }
  iterator find(stable_iterator iter) { return stack.find(iter); }

  template <class U, class... ArgTypes> void push(ArgTypes &&... args) {
    stack.push<U>(std::forward<ArgTypes>(args)...);
  }

  void pop() { stack.pop(); }

  /// Pop objects off of the stack until \p the object pointed to by stable_iter
  /// is the top element of the stack.
  void pop(stable_iterator stable_iter) { stack.pop(stable_iter); }

  void dump(SILGenFunction &SGF);
};

/// A scope associated with the beginning of the formal evaluation of an lvalue.
///
/// A formal evaluation of an lvalue occurs when emitting:
///
///   1. accessors.
///   2. getters.
///   3. materializeForSets.
///
/// for lvalues. The general form of such an evaluation is:
///
///   formally evaluate the lvalue "x" into memory
///   begin formal access to "x"
///   end formal access to "x"
///   ... *more formal access*
///   begin formal access to "x"
///   end formal access to "x"
///   end formal evaluation of lvalue into memory
///
/// *NOTE* All formal access contain a pointer to a cleanup in the normal
/// cleanup stack. This is to ensure that when SILGen calls
/// Cleanups.emitBranchAndCleanups (and other special cleanup code along error
/// edges), writebacks are properly created. What is key to notice is that all
/// of these cleanup emission types are non-destructive. Contrast this with
/// normal scope popping. In such a case, the scope pop is destructive. This
/// means that any pointers from the formal access to the cleanup stack is now
/// invalid.
///
/// In order to avoid this issue, it is important to /never/ create a formal
/// access cleanup when the "top level" scope is not a formal evaluation scope.
class FormalEvaluationScope {
  SILGenFunction &SGF;
  llvm::Optional<FormalEvaluationContext::stable_iterator> savedDepth;
  bool wasInFormalEvaluationScope;
  bool wasInInOutConversionScope;

public:
  FormalEvaluationScope(SILGenFunction &SGF);
  ~FormalEvaluationScope() {
    if (!savedDepth.hasValue())
      return;
    popImpl();
  }

  bool isPopped() const { return !savedDepth.hasValue(); }

  void pop() {
    if (wasInInOutConversionScope)
      return;

    assert(!isPopped() && "popping an already-popped scope!");
    popImpl();
    savedDepth.reset();
  }

  FormalEvaluationScope(const FormalEvaluationScope &) = delete;
  FormalEvaluationScope &operator=(const FormalEvaluationScope &) = delete;

  FormalEvaluationScope(FormalEvaluationScope &&o);
  FormalEvaluationScope &operator=(FormalEvaluationScope &&o) = delete;

  /// Verify that we can successfully access all of the inner lexical scopes
  /// that would be popped by this scope.
  void verify() const;

private:
  void popImpl();
};

} // namespace Lowering
} // namespace swift

#endif
