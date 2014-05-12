// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/primary/primary_thread_impl.h"

#include <string>

#include "base/atomicops.h"
#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/primary/primary_thread_delegate.h"

namespace content {

namespace {

// Friendly names for the well-known threads.
static const char* g_primary_thread_names[PrimaryThread::ID_COUNT] = {
  "",  // UI (name assembled in browser_main.cc).
  "Chrome_DBThread",  // DB
  "Chrome_FileThread",  // FILE
  "Chrome_FileUserBlockingThread",  // FILE_USER_BLOCKING
  "Chrome_ProcessLauncherThread",  // PROCESS_LAUNCHER
  "Chrome_CacheThread",  // CACHE
  "Chrome_IOThread",  // IO
};

struct PrimaryThreadGlobals {
  PrimaryThreadGlobals()
      : blocking_pool(new base::SequencedWorkerPool(3, "BrowserBlocking")) {
    memset(threads, 0, PrimaryThread::ID_COUNT * sizeof(threads[0]));
    memset(thread_delegates, 0,
           PrimaryThread::ID_COUNT * sizeof(thread_delegates[0]));
  }

  // This lock protects |threads|. Do not read or modify that array
  // without holding this lock. Do not block while holding this lock.
  base::Lock lock;

  // This array is protected by |lock|. The threads are not owned by this
  // array. Typically, the threads are owned on the UI thread by
  // PrimaryMainLoop. PrimaryThreadImpl objects remove themselves from this
  // array upon destruction.
  PrimaryThreadImpl* threads[PrimaryThread::ID_COUNT];

  // Only atomic operations are used on this array. The delegates are not owned
  // by this array, rather by whoever calls PrimaryThread::SetDelegate.
  PrimaryThreadDelegate* thread_delegates[PrimaryThread::ID_COUNT];

  const scoped_refptr<base::SequencedWorkerPool> blocking_pool;
};

base::LazyInstance<PrimaryThreadGlobals>::Leaky
    g_globals = LAZY_INSTANCE_INITIALIZER;

}  // namespace

PrimaryThreadImpl::PrimaryThreadImpl(ID identifier)
    : Thread(g_primary_thread_names[identifier]),
      identifier_(identifier) {
  Initialize();
}

PrimaryThreadImpl::PrimaryThreadImpl(ID identifier,
                                     base::MessageLoop* message_loop)
    : Thread(message_loop->thread_name().c_str()), identifier_(identifier) {
  set_message_loop(message_loop);
  Initialize();
}

// static
void PrimaryThreadImpl::ShutdownThreadPool() {
  // The goal is to make it impossible for chrome to 'infinite loop' during
  // shutdown, but to reasonably expect that all BLOCKING_SHUTDOWN tasks queued
  // during shutdown get run. There's nothing particularly scientific about the
  // number chosen.
  const int kMaxNewShutdownBlockingTasks = 1000;
  PrimaryThreadGlobals& globals = g_globals.Get();
  globals.blocking_pool->Shutdown(kMaxNewShutdownBlockingTasks);
}

// static
void PrimaryThreadImpl::FlushThreadPoolHelper() {
  // We don't want to create a pool if none exists.
  if (g_globals == NULL)
    return;
  g_globals.Get().blocking_pool->FlushForTesting();
}

void PrimaryThreadImpl::Init() {
  PrimaryThreadGlobals& globals = g_globals.Get();

  using base::subtle::AtomicWord;
  AtomicWord* storage =
      reinterpret_cast<AtomicWord*>(&globals.thread_delegates[identifier_]);
  AtomicWord stored_pointer = base::subtle::NoBarrier_Load(storage);
  PrimaryThreadDelegate* delegate =
      reinterpret_cast<PrimaryThreadDelegate*>(stored_pointer);
  if (delegate) {
    delegate->Init();
    message_loop()->PostTask(FROM_HERE,
                             base::Bind(&PrimaryThreadDelegate::InitAsync,
                                        // Delegate is expected to exist for the
                                        // duration of the thread's lifetime
                                        base::Unretained(delegate)));
  }
}

// We disable optimizations for this block of functions so the compiler doesn't
// merge them all together.
MSVC_DISABLE_OPTIMIZE()
MSVC_PUSH_DISABLE_WARNING(4748)

NOINLINE void PrimaryThreadImpl::UIThreadRun(base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

NOINLINE void PrimaryThreadImpl::DBThreadRun(base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

NOINLINE void PrimaryThreadImpl::FileThreadRun(
    base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

NOINLINE void PrimaryThreadImpl::FileUserBlockingThreadRun(
    base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

NOINLINE void PrimaryThreadImpl::ProcessLauncherThreadRun(
    base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

NOINLINE void PrimaryThreadImpl::CacheThreadRun(
    base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

NOINLINE void PrimaryThreadImpl::IOThreadRun(base::MessageLoop* message_loop) {
  volatile int line_number = __LINE__;
  Thread::Run(message_loop);
  CHECK_GT(line_number, 0);
}

MSVC_POP_WARNING()
MSVC_ENABLE_OPTIMIZE();

void PrimaryThreadImpl::Run(base::MessageLoop* message_loop) {
  PrimaryThread::ID thread_id;
  if (!GetCurrentThreadIdentifier(&thread_id))
    return Thread::Run(message_loop);

  switch (thread_id) {
    case PrimaryThread::UI:
      return UIThreadRun(message_loop);
    case PrimaryThread::DB:
      return DBThreadRun(message_loop);
    case PrimaryThread::FILE:
      return FileThreadRun(message_loop);
    case PrimaryThread::FILE_USER_BLOCKING:
      return FileUserBlockingThreadRun(message_loop);
    case PrimaryThread::PROCESS_LAUNCHER:
      return ProcessLauncherThreadRun(message_loop);
    case PrimaryThread::CACHE:
      return CacheThreadRun(message_loop);
    case PrimaryThread::IO:
      return IOThreadRun(message_loop);
    case PrimaryThread::ID_COUNT:
      CHECK(false);  // This shouldn't actually be reached!
      break;
  }
  Thread::Run(message_loop);
}

void PrimaryThreadImpl::CleanUp() {
  PrimaryThreadGlobals& globals = g_globals.Get();

  using base::subtle::AtomicWord;
  AtomicWord* storage =
      reinterpret_cast<AtomicWord*>(&globals.thread_delegates[identifier_]);
  AtomicWord stored_pointer = base::subtle::NoBarrier_Load(storage);
  PrimaryThreadDelegate* delegate =
      reinterpret_cast<PrimaryThreadDelegate*>(stored_pointer);

  if (delegate)
    delegate->CleanUp();
}

void PrimaryThreadImpl::Initialize() {
  PrimaryThreadGlobals& globals = g_globals.Get();

  base::AutoLock lock(globals.lock);
  DCHECK(identifier_ >= 0 && identifier_ < ID_COUNT);
  DCHECK(globals.threads[identifier_] == NULL);
  globals.threads[identifier_] = this;
}

PrimaryThreadImpl::~PrimaryThreadImpl() {
  // All Thread subclasses must call Stop() in the destructor. This is
  // doubly important here as various bits of code check they are on
  // the right PrimaryThread.
  Stop();

  PrimaryThreadGlobals& globals = g_globals.Get();
  base::AutoLock lock(globals.lock);
  globals.threads[identifier_] = NULL;
#ifndef NDEBUG
  // Double check that the threads are ordered correctly in the enumeration.
  for (int i = identifier_ + 1; i < ID_COUNT; ++i) {
    DCHECK(!globals.threads[i]) <<
        "Threads must be listed in the reverse order that they die";
  }
#endif
}

// static
bool PrimaryThreadImpl::PostTaskHelper(
    PrimaryThread::ID identifier,
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay,
    bool nestable) {
  DCHECK(identifier >= 0 && identifier < ID_COUNT);
  // Optimization: to avoid unnecessary locks, we listed the ID enumeration in
  // order of lifetime.  So no need to lock if we know that the target thread
  // outlives current thread.
  // Note: since the array is so small, ok to loop instead of creating a map,
  // which would require a lock because std::map isn't thread safe, defeating
  // the whole purpose of this optimization.
  PrimaryThread::ID current_thread;
  bool target_thread_outlives_current =
      GetCurrentThreadIdentifier(&current_thread) &&
      current_thread >= identifier;

  PrimaryThreadGlobals& globals = g_globals.Get();
  if (!target_thread_outlives_current)
    globals.lock.Acquire();

  base::MessageLoop* message_loop =
      globals.threads[identifier] ? globals.threads[identifier]->message_loop()
                                  : NULL;
  if (message_loop) {
    if (nestable) {
      message_loop->PostDelayedTask(from_here, task, delay);
    } else {
      message_loop->PostNonNestableDelayedTask(from_here, task, delay);
    }
  }

  if (!target_thread_outlives_current)
    globals.lock.Release();

  return !!message_loop;
}

// An implementation of MessageLoopProxy to be used in conjunction
// with PrimaryThread.
class BrowserThreadMessageLoopProxy : public base::MessageLoopProxy {
 public:
  explicit BrowserThreadMessageLoopProxy(PrimaryThread::ID identifier)
      : id_(identifier) {
  }

  // MessageLoopProxy implementation.
  virtual bool PostDelayedTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task, base::TimeDelta delay) OVERRIDE {
    return PrimaryThread::PostDelayedTask(id_, from_here, task, delay);
  }

  virtual bool PostNonNestableDelayedTask(
      const tracked_objects::Location& from_here,
      const base::Closure& task,
      base::TimeDelta delay) OVERRIDE {
    return PrimaryThread::PostNonNestableDelayedTask(id_, from_here, task,
                                                     delay);
  }

  virtual bool RunsTasksOnCurrentThread() const OVERRIDE {
    return PrimaryThread::CurrentlyOn(id_);
  }

 protected:
  virtual ~BrowserThreadMessageLoopProxy() {}

 private:
  PrimaryThread::ID id_;
  DISALLOW_COPY_AND_ASSIGN(BrowserThreadMessageLoopProxy);
};

// static
bool PrimaryThread::PostBlockingPoolTask(
    const tracked_objects::Location& from_here,
    const base::Closure& task) {
  return g_globals.Get().blocking_pool->PostWorkerTask(from_here, task);
}

bool PrimaryThread::PostBlockingPoolTaskAndReply(
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    const base::Closure& reply) {
  return g_globals.Get().blocking_pool->PostTaskAndReply(
      from_here, task, reply);
}

// static
bool PrimaryThread::PostBlockingPoolSequencedTask(
    const std::string& sequence_token_name,
    const tracked_objects::Location& from_here,
    const base::Closure& task) {
  return g_globals.Get().blocking_pool->PostNamedSequencedWorkerTask(
      sequence_token_name, from_here, task);
}

// static
base::SequencedWorkerPool* PrimaryThread::GetBlockingPool() {
  return g_globals.Get().blocking_pool.get();
}

// static
bool PrimaryThread::IsThreadInitialized(ID identifier) {
  if (g_globals == NULL)
    return false;

  PrimaryThreadGlobals& globals = g_globals.Get();
  base::AutoLock lock(globals.lock);
  DCHECK(identifier >= 0 && identifier < ID_COUNT);
  return globals.threads[identifier] != NULL;
}

// static
bool PrimaryThread::CurrentlyOn(ID identifier) {
  // We shouldn't use MessageLoop::current() since it uses LazyInstance which
  // may be deleted by ~AtExitManager when a WorkerPool thread calls this
  // function.
  // http://crbug.com/63678
  base::ThreadRestrictions::ScopedAllowSingleton allow_singleton;
  PrimaryThreadGlobals& globals = g_globals.Get();
  base::AutoLock lock(globals.lock);
  DCHECK(identifier >= 0 && identifier < ID_COUNT);
  return globals.threads[identifier] &&
         globals.threads[identifier]->message_loop() ==
             base::MessageLoop::current();
}

// static
bool PrimaryThread::IsMessageLoopValid(ID identifier) {
  if (g_globals == NULL)
    return false;

  PrimaryThreadGlobals& globals = g_globals.Get();
  base::AutoLock lock(globals.lock);
  DCHECK(identifier >= 0 && identifier < ID_COUNT);
  return globals.threads[identifier] &&
         globals.threads[identifier]->message_loop();
}

// static
bool PrimaryThread::PostTask(ID identifier,
                             const tracked_objects::Location& from_here,
                             const base::Closure& task) {
  return PrimaryThreadImpl::PostTaskHelper(
      identifier, from_here, task, base::TimeDelta(), true);
}

// static
bool PrimaryThread::PostDelayedTask(ID identifier,
                                    const tracked_objects::Location& from_here,
                                    const base::Closure& task,
                                    base::TimeDelta delay) {
  return PrimaryThreadImpl::PostTaskHelper(
      identifier, from_here, task, delay, true);
}

// static
bool PrimaryThread::PostNonNestableTask(
    ID identifier,
    const tracked_objects::Location& from_here,
    const base::Closure& task) {
  return PrimaryThreadImpl::PostTaskHelper(
      identifier, from_here, task, base::TimeDelta(), false);
}

// static
bool PrimaryThread::PostNonNestableDelayedTask(
    ID identifier,
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    base::TimeDelta delay) {
  return PrimaryThreadImpl::PostTaskHelper(
      identifier, from_here, task, delay, false);
}

// static
bool PrimaryThread::PostTaskAndReply(
    ID identifier,
    const tracked_objects::Location& from_here,
    const base::Closure& task,
    const base::Closure& reply) {
  return GetMessageLoopProxyForThread(identifier)->PostTaskAndReply(from_here,
                                                                    task,
                                                                    reply);
}

// static
bool PrimaryThread::GetCurrentThreadIdentifier(ID* identifier) {
  if (g_globals == NULL)
    return false;

  // We shouldn't use MessageLoop::current() since it uses LazyInstance which
  // may be deleted by ~AtExitManager when a WorkerPool thread calls this
  // function.
  // http://crbug.com/63678
  base::ThreadRestrictions::ScopedAllowSingleton allow_singleton;
  base::MessageLoop* cur_message_loop = base::MessageLoop::current();
  PrimaryThreadGlobals& globals = g_globals.Get();
  for (int i = 0; i < ID_COUNT; ++i) {
    if (globals.threads[i] &&
        globals.threads[i]->message_loop() == cur_message_loop) {
      *identifier = globals.threads[i]->identifier_;
      return true;
    }
  }

  return false;
}

// static
scoped_refptr<base::MessageLoopProxy>
PrimaryThread::GetMessageLoopProxyForThread(ID identifier) {
  return make_scoped_refptr(new BrowserThreadMessageLoopProxy(identifier));
}

// static
base::MessageLoop* PrimaryThread::UnsafeGetMessageLoopForThread(ID identifier) {
  if (g_globals == NULL)
    return NULL;

  PrimaryThreadGlobals& globals = g_globals.Get();
  base::AutoLock lock(globals.lock);
  base::Thread* thread = globals.threads[identifier];
  DCHECK(thread);
  base::MessageLoop* loop = thread->message_loop();
  return loop;
}

// static
void PrimaryThread::SetDelegate(ID identifier,
                                PrimaryThreadDelegate* delegate) {
  using base::subtle::AtomicWord;
  PrimaryThreadGlobals& globals = g_globals.Get();
  AtomicWord* storage = reinterpret_cast<AtomicWord*>(
      &globals.thread_delegates[identifier]);
  AtomicWord old_pointer = base::subtle::NoBarrier_AtomicExchange(
      storage, reinterpret_cast<AtomicWord>(delegate));

  // This catches registration when previously registered.
  DCHECK(!delegate || !old_pointer);
}

}  // namespace content
