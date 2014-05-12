// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BROWSER_MAIN_LOOP_H_
#define CONTENT_BROWSER_BROWSER_MAIN_LOOP_H_

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/primary/primary_process_sub_thread.h"
#include "content/public/primary/primary_main_runner.h"

class CommandLine;

namespace base {
class HighResolutionTimerManager;
class MessageLoop;
class PowerMonitor;
class SystemMonitor;
namespace debug {
class TraceMemoryController;
}  // namespace debug
}  // namespace base


namespace net {
class NetworkChangeNotifier;
}  // namespace net

namespace content {
class MainParts;
class PrimaryShutdownImpl;
class PrimaryThreadImpl;
class StartupTaskRunner;
class SystemMessageWindowWin;
struct MainFunctionParams;

#if defined(OS_LINUX)
class DeviceMonitorLinux;
#elif defined(OS_MACOSX)
class DeviceMonitorMac;
#endif

// Implements the main browser loop stages called from PrimaryMainRunner.
// See comments in primary_main_parts.h for additional info.
class CONTENT_EXPORT PrimaryMainLoop {
 public:
  // Returns the current instance. This is used to get access to the getters
  // that return objects which are owned by this class.
  static PrimaryMainLoop* GetInstance();

  explicit PrimaryMainLoop(const MainFunctionParams& parameters);
  virtual ~PrimaryMainLoop();

  void Init();

  void EarlyInitialization();
  void InitializeToolkit();
  void MainMessageLoopStart();

  // Create and start running the tasks we need to complete startup. Note that
  // this can be called more than once (currently only on Android) if we get a
  // request for synchronous startup while the tasks created by asynchronous
  // startup are still running.
  void CreateStartupTasks();

  // Perform the default message loop run logic.
  void RunMainMessageLoopParts();

  // Performs the shutdown sequence, starting with PostMainMessageLoopRun
  // through stopping threads to PostDestroyThreads.
  void ShutdownThreadsAndCleanUp();

  int GetResultCode() const { return result_code_; }

 private:
  class MemoryObserver;
  // For ShutdownThreadsAndCleanUp.
  friend class PrimaryShutdownImpl;

  void InitializeMainThread();

  // Called just before creating the threads
  int PreCreateThreads();

  // Create all secondary threads.
  int CreateThreads();

  // Called right after the browser threads have been started.
  int PrimaryThreadsStarted();

  int PreMainMessageLoopRun();

  void MainMessageLoopRun();

  // Members initialized on construction ---------------------------------------
  const MainFunctionParams& parameters_;
  const CommandLine& parsed_command_line_;
  int result_code_;
  // True if the non-UI threads were created.
  bool created_threads_;

  // Members initialized in |MainMessageLoopStart()| ---------------------------
  scoped_ptr<base::MessageLoop> main_message_loop_;
  scoped_ptr<base::SystemMonitor> system_monitor_;
  scoped_ptr<base::PowerMonitor> power_monitor_;
  scoped_ptr<base::HighResolutionTimerManager> hi_res_timer_manager_;
  scoped_ptr<net::NetworkChangeNotifier> network_change_notifier_;

#if defined(OS_WIN)
  scoped_ptr<SystemMessageWindowWin> system_message_window_;
#elif defined(OS_LINUX)
  scoped_ptr<DeviceMonitorLinux> device_monitor_linux_;
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  scoped_ptr<DeviceMonitorMac> device_monitor_mac_;
#endif
  // The startup task runner is created by CreateStartupTasks()
  scoped_ptr<StartupTaskRunner> startup_task_runner_;

  // Destroy parts_ before main_message_loop_ (required) and before other
  // classes constructed in content (but after main_thread_).
  scoped_ptr<MainParts> parts_;

  // Members initialized in |InitializeMainThread()| ---------------------------
  // This must get destroyed before other threads that are created in parts_.
  scoped_ptr<PrimaryThreadImpl> main_thread_;

  // Members initialized in |RunMainMessageLoopParts()| ------------------------
  scoped_ptr<PrimaryProcessSubThread> db_thread_;
  scoped_ptr<PrimaryProcessSubThread> file_user_blocking_thread_;
  scoped_ptr<PrimaryProcessSubThread> file_thread_;
  scoped_ptr<PrimaryProcessSubThread> process_launcher_thread_;
  scoped_ptr<PrimaryProcessSubThread> cache_thread_;
  scoped_ptr<PrimaryProcessSubThread> io_thread_;
  scoped_ptr<MemoryObserver> memory_observer_;
  scoped_ptr<base::debug::TraceMemoryController> trace_memory_controller_;

  DISALLOW_COPY_AND_ASSIGN(PrimaryMainLoop);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BROWSER_MAIN_LOOP_H_
