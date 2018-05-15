//===--- Thread.cpp ----------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2018 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file contains wrappers for cross platform cpp::thread APIs.
//
//===----------------------------------------------------------------------===//

#include <thread>
#include <condition_variable>

template <typename Result, typename Argument>
struct RaceThreadContext {
  std::function<Result(Argument)> code;

  T result;

 :unsigned numThreads;
  unsigned &numThreadsReady;
  std::mutex &sharedMutex;
  std::condition_variable &start_condition;
};

template <typename T>
void RaceThunk(RaceThreadContext<T> &ctx) {
  // update shared state
  std::unique_lock<std::mutex> lk(ctx.sharedMutex);
  ++ctx.numThreadsReady;
  bool isLastThread = ctx.numThreadsReady == ctx.numThreads;

  // wait until the rest of the thunks are ready
  ctx.start_condition.wait(lk, [&ctx]{ // waiting releases the lock
    return ctx.numThreadsReady == ctx.numThreads;
  });
  lk.unlock();

  // The last thread will signal the condition_variable to kick off the rest
  // of the waiting threads to start.
  if (isLastThread) ctx.start_condition.notify_all();

  ctx.result = ctx.code();
}


