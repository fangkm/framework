// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_main_loop.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/pending_task.h"
#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "base/process/process_metrics.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/system_monitor/system_monitor.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/hi_res_timer_manager.h"
#include "content/browser/browser_thread_impl.h"
#include "content/browser/startup_task_runner.h"
#include "content/browser/gpu/browser_gpu_channel_host_factory.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/gpu/gpu_data_manager_impl.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/browser/browser_shutdown.h"
#include "content/public/browser/content_main_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/result_codes.h"
#include "crypto/nss_util.h"
#include "net/base/network_change_notifier.h"
#include "net/socket/client_socket_factory.h"
#include "net/ssl/ssl_config_service.h"
#include "ui/base/clipboard/clipboard.h"

#if defined(USE_AURA)
#include "content/browser/aura/image_transport_factory.h"
#endif

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "content/browser/android/browser_startup_controller.h"
#include "content/browser/android/surface_texture_peer_browser_impl.h"
#endif

#if defined(OS_WIN)
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include "base/win/text_services_message_filter.h"
#include "content/browser/system_message_window_win.h"
#include "content/common/sandbox_win.h"
#include "net/base/winsock_init.h"
#include "ui/base/l10n/l10n_util_win.h"
#endif

#if defined(OS_LINUX) || defined(OS_OPENBSD)
#include <glib-object.h>
#endif

#if defined(OS_LINUX)
#include "content/browser/device_monitor_linux.h"
#elif defined(OS_MACOSX) && !defined(OS_IOS)
#include "content/browser/device_monitor_mac.h"
#endif

#if defined(TOOLKIT_GTK)
#include "ui/gfx/gtk_util.h"
#endif

#if defined(OS_POSIX) && !defined(OS_MACOSX)
#include <sys/stat.h>

#include "content/browser/renderer_host/render_sandbox_host_linux.h"
#include "content/browser/zygote_host/zygote_host_impl_linux.h"
#endif

#if defined(TCMALLOC_TRACE_MEMORY_SUPPORTED)
#include "third_party/tcmalloc/chromium/src/gperftools/heap-profiler.h"
#endif

#if defined(USE_X11)
#include <X11/Xlib.h>
#endif

// One of the linux specific headers defines this as a macro.
#ifdef DestroyAll
#undef DestroyAll
#endif

namespace content {

// The currently-running BrowserMainLoop.  There can be one or zero.
BrowserMainLoop* g_current_browser_main_loop = NULL;

// This is just to be able to keep ShutdownThreadsAndCleanUp out of
// the public interface of BrowserMainLoop.
class BrowserShutdownImpl {
 public:
  static void ImmediateShutdownAndExitProcess() {
    DCHECK(g_current_browser_main_loop);
    g_current_browser_main_loop->ShutdownThreadsAndCleanUp();

#if defined(OS_WIN)
    // At this point the message loop is still running yet we've shut everything
    // down. If any messages are processed we'll likely crash. Exit now.
    ExitProcess(RESULT_CODE_NORMAL_EXIT);
#elif defined(OS_POSIX) && !defined(OS_MACOSX)
    _exit(RESULT_CODE_NORMAL_EXIT);
#else
    NOTIMPLEMENTED();
#endif
  }
};

void ImmediateShutdownAndExitProcess() {
  BrowserShutdownImpl::ImmediateShutdownAndExitProcess();
}

// For measuring memory usage after each task. Behind a command line flag.
class BrowserMainLoop::MemoryObserver : public base::MessageLoop::TaskObserver {
 public:
  MemoryObserver() {}
  virtual ~MemoryObserver() {}

  virtual void WillProcessTask(const base::PendingTask& pending_task) OVERRIDE {
  }

  virtual void DidProcessTask(const base::PendingTask& pending_task) OVERRIDE {
#if !defined(OS_IOS)  // No ProcessMetrics on IOS.
    scoped_ptr<base::ProcessMetrics> process_metrics(
        base::ProcessMetrics::CreateProcessMetrics(
#if defined(OS_MACOSX)
            base::GetCurrentProcessHandle(), NULL));
#else
            base::GetCurrentProcessHandle()));
#endif
    size_t private_bytes;
    process_metrics->GetMemoryBytes(&private_bytes, NULL);
    HISTOGRAM_MEMORY_KB("Memory.BrowserUsed", private_bytes >> 10);
#endif
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(MemoryObserver);
};


// BrowserMainLoop construction / destruction =============================

BrowserMainLoop* BrowserMainLoop::GetInstance() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return g_current_browser_main_loop;
}

BrowserMainLoop::BrowserMainLoop(const MainFunctionParams& parameters)
    : parameters_(parameters),
      parsed_command_line_(parameters.command_line),
      result_code_(RESULT_CODE_NORMAL_EXIT),
      created_threads_(false) {
  DCHECK(!g_current_browser_main_loop);
  g_current_browser_main_loop = this;
}

BrowserMainLoop::~BrowserMainLoop() {
  DCHECK_EQ(this, g_current_browser_main_loop);
#if !defined(OS_IOS)
  ui::Clipboard::DestroyClipboardForCurrentThread();
#endif  // !defined(OS_IOS)
  g_current_browser_main_loop = NULL;
}

void BrowserMainLoop::Init() {
  TRACE_EVENT0("startup", "BrowserMainLoop::Init")
  parts_.reset(
      GetContentClient()->MainClient()->CreateBrowserMainParts(parameters_));
}

// BrowserMainLoop stages ==================================================

void BrowserMainLoop::EarlyInitialization() {
  TRACE_EVENT0("startup", "BrowserMainLoop::EarlyInitialization");
#if defined(USE_X11)
  if (parsed_command_line_.HasSwitch(switches::kSingleProcess) ||
      parsed_command_line_.HasSwitch(switches::kInProcessGPU)) {
    if (!XInitThreads()) {
      LOG(ERROR) << "Failed to put Xlib into threaded mode.";
    }
  }
#endif

  if (parts_)
    parts_->PreEarlyInitialization();

#if defined(OS_WIN)
  net::EnsureWinsockInit();
#endif

#if !defined(USE_OPENSSL)
  // We want to be sure to init NSPR on the main thread.
  crypto::EnsureNSPRInit();
#endif  // !defined(USE_OPENSSL)

#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_ANDROID)
  SetupSandbox(parsed_command_line_);
#endif

  if (parsed_command_line_.HasSwitch(switches::kEnableSSLCachedInfo))
    net::SSLConfigService::EnableCachedInfo();

  if (parts_)
    parts_->PostEarlyInitialization();
}

void BrowserMainLoop::MainMessageLoopStart() {
  TRACE_EVENT0("startup", "BrowserMainLoop::MainMessageLoopStart")
  if (parts_) {
    TRACE_EVENT0("startup",
        "BrowserMainLoop::MainMessageLoopStart:PreMainMessageLoopStart");
    parts_->PreMainMessageLoopStart();
  }

#if defined(OS_WIN)
  // If we're running tests (ui_task is non-null), then the ResourceBundle
  // has already been initialized.
  if (!parameters_.ui_task) {
    // Override the configured locale with the user's preferred UI language.
    l10n_util::OverrideLocaleWithUILanguageList();
  }
#endif

  // Create a MessageLoop if one does not already exist for the current thread.
  if (!base::MessageLoop::current())
    main_message_loop_.reset(new base::MessageLoop(base::MessageLoop::TYPE_UI));

  InitializeMainThread();

  {
    TRACE_EVENT0("startup", "BrowserMainLoop::Subsystem:SystemMonitor")
    system_monitor_.reset(new base::SystemMonitor);
  }
  {
    TRACE_EVENT0("startup", "BrowserMainLoop::Subsystem:PowerMonitor")
    scoped_ptr<base::PowerMonitorSource> power_monitor_source(
      new base::PowerMonitorDeviceSource());
    power_monitor_.reset(new base::PowerMonitor(power_monitor_source.Pass()));
  }
  {
    TRACE_EVENT0("startup", "BrowserMainLoop::Subsystem:HighResTimerManager")
    hi_res_timer_manager_.reset(new base::HighResolutionTimerManager);
  }
  {
    TRACE_EVENT0("startup", "BrowserMainLoop::Subsystem:NetworkChangeNotifier")
    network_change_notifier_.reset(net::NetworkChangeNotifier::Create());
  }

#if defined(OS_WIN)
  system_message_window_.reset(new SystemMessageWindowWin);

  if (base::win::IsTSFAwareRequired()) {
    // Create a TSF message filter for the message loop. MessageLoop takes
    // ownership of the filter.
    scoped_ptr<base::win::TextServicesMessageFilter> tsf_message_filter(
      new base::win::TextServicesMessageFilter);
    if (tsf_message_filter->Init()) {
      base::MessageLoopForUI::current()->SetMessageFilter(
        tsf_message_filter.PassAs<base::MessageLoopForUI::MessageFilter>());
    }
  }
#endif

  if (parts_)
    parts_->PostMainMessageLoopStart();

#if defined(OS_ANDROID)
  {
    TRACE_EVENT0("startup", "BrowserMainLoop::Subsystem:SurfaceTexturePeer")
    SurfaceTexturePeer::InitInstance(new SurfaceTexturePeerBrowserImpl());
  }
#endif

  if (parsed_command_line_.HasSwitch(switches::kMemoryMetrics)) {
    TRACE_EVENT0("startup", "BrowserMainLoop::Subsystem:MemoryObserver")
    memory_observer_.reset(new MemoryObserver());
    base::MessageLoop::current()->AddTaskObserver(memory_observer_.get());
  }

#if defined(TCMALLOC_TRACE_MEMORY_SUPPORTED)
  trace_memory_controller_.reset(new base::debug::TraceMemoryController(
      base::MessageLoop::current()->message_loop_proxy(),
      ::HeapProfilerWithPseudoStackStart,
      ::HeapProfilerStop,
      ::GetHeapProfile));
#endif
}

int BrowserMainLoop::PreCreateThreads() {
  if (parts_) {
    TRACE_EVENT0("startup",
        "BrowserMainLoop::CreateThreads:PreCreateThreads");
    result_code_ = parts_->PreCreateThreads();
  }

  return result_code_;
}

void BrowserMainLoop::CreateStartupTasks() {
  TRACE_EVENT0("startup", "BrowserMainLoop::CreateStartupTasks");

  // First time through, we really want to create all the tasks
  if (!startup_task_runner_.get()) {
#if defined(OS_ANDROID)
    startup_task_runner_ = make_scoped_ptr(new StartupTaskRunner(
        base::Bind(&BrowserStartupComplete),
        base::MessageLoop::current()->message_loop_proxy()));
#else
    startup_task_runner_ = make_scoped_ptr(new StartupTaskRunner(
        base::Callback<void(int)>(),
        base::MessageLoop::current()->message_loop_proxy()));
#endif
    StartupTask pre_create_threads =
        base::Bind(&BrowserMainLoop::PreCreateThreads, base::Unretained(this));
    startup_task_runner_->AddTask(pre_create_threads);

    StartupTask create_threads =
        base::Bind(&BrowserMainLoop::CreateThreads, base::Unretained(this));
    startup_task_runner_->AddTask(create_threads);

    StartupTask browser_thread_started = base::Bind(
        &BrowserMainLoop::BrowserThreadsStarted, base::Unretained(this));
    startup_task_runner_->AddTask(browser_thread_started);

    StartupTask pre_main_message_loop_run = base::Bind(
        &BrowserMainLoop::PreMainMessageLoopRun, base::Unretained(this));
    startup_task_runner_->AddTask(pre_main_message_loop_run);

#if defined(OS_ANDROID)
    if (BrowserMayStartAsynchronously()) {
      startup_task_runner_->StartRunningTasksAsync();
    }
#endif
  }
#if defined(OS_ANDROID)
  if (!BrowserMayStartAsynchronously()) {
    // A second request for asynchronous startup can be ignored, so
    // StartupRunningTasksAsync is only called first time through. If, however,
    // this is a request for synchronous startup then it must override any
    // previous call for async startup, so we call RunAllTasksNow()
    // unconditionally.
    startup_task_runner_->RunAllTasksNow();
  }
#else
  startup_task_runner_->RunAllTasksNow();
#endif
}

int BrowserMainLoop::CreateThreads() {
  TRACE_EVENT0("startup", "BrowserMainLoop::CreateThreads");

  base::Thread::Options default_options;
  base::Thread::Options io_message_loop_options;
  io_message_loop_options.message_loop_type = base::MessageLoop::TYPE_IO;
  base::Thread::Options ui_message_loop_options;
  ui_message_loop_options.message_loop_type = base::MessageLoop::TYPE_UI;

  // Start threads in the order they occur in the BrowserThread::ID
  // enumeration, except for BrowserThread::UI which is the main
  // thread.
  //
  // Must be size_t so we can increment it.
  for (size_t thread_id = BrowserThread::UI + 1;
       thread_id < BrowserThread::ID_COUNT;
       ++thread_id) {
    scoped_ptr<BrowserProcessSubThread>* thread_to_start = NULL;
    base::Thread::Options* options = &default_options;

    switch (thread_id) {
      case BrowserThread::DB:
        TRACE_EVENT_BEGIN1("startup",
            "BrowserMainLoop::CreateThreads:start",
            "Thread", "BrowserThread::DB");
        thread_to_start = &db_thread_;
        break;
      case BrowserThread::FILE_USER_BLOCKING:
        TRACE_EVENT_BEGIN1("startup",
            "BrowserMainLoop::CreateThreads:start",
            "Thread", "BrowserThread::FILE_USER_BLOCKING");
        thread_to_start = &file_user_blocking_thread_;
        break;
      case BrowserThread::FILE:
        TRACE_EVENT_BEGIN1("startup",
            "BrowserMainLoop::CreateThreads:start",
            "Thread", "BrowserThread::FILE");
        thread_to_start = &file_thread_;
#if defined(OS_WIN)
        // On Windows, the FILE thread needs to be have a UI message loop
        // which pumps messages in such a way that Google Update can
        // communicate back to us.
        options = &ui_message_loop_options;
#else
        options = &io_message_loop_options;
#endif
        break;
      case BrowserThread::PROCESS_LAUNCHER:
        TRACE_EVENT_BEGIN1("startup",
            "BrowserMainLoop::CreateThreads:start",
            "Thread", "BrowserThread::PROCESS_LAUNCHER");
        thread_to_start = &process_launcher_thread_;
        break;
      case BrowserThread::CACHE:
        TRACE_EVENT_BEGIN1("startup",
            "BrowserMainLoop::CreateThreads:start",
            "Thread", "BrowserThread::CACHE");
        thread_to_start = &cache_thread_;
        options = &io_message_loop_options;
        break;
      case BrowserThread::IO:
        TRACE_EVENT_BEGIN1("startup",
            "BrowserMainLoop::CreateThreads:start",
            "Thread", "BrowserThread::IO");
        thread_to_start = &io_thread_;
        options = &io_message_loop_options;
        break;
      case BrowserThread::UI:
      case BrowserThread::ID_COUNT:
      default:
        NOTREACHED();
        break;
    }

    BrowserThread::ID id = static_cast<BrowserThread::ID>(thread_id);

    if (thread_to_start) {
      (*thread_to_start).reset(new BrowserProcessSubThread(id));
      (*thread_to_start)->StartWithOptions(*options);
    } else {
      NOTREACHED();
    }

    TRACE_EVENT_END0("startup", "BrowserMainLoop::CreateThreads:start");

  }
  created_threads_ = true;
  return result_code_;
}

int BrowserMainLoop::PreMainMessageLoopRun() {
  if (parts_) {
    TRACE_EVENT0("startup",
        "BrowserMainLoop::CreateThreads:PreMainMessageLoopRun");
    parts_->PreMainMessageLoopRun();
  }

  // If the UI thread blocks, the whole UI is unresponsive.
  // Do not allow disk IO from the UI thread.
  base::ThreadRestrictions::SetIOAllowed(false);
  base::ThreadRestrictions::DisallowWaiting();
  return result_code_;
}

void BrowserMainLoop::RunMainMessageLoopParts() {
  TRACE_EVENT_BEGIN_ETW("BrowserMain:MESSAGE_LOOP", 0, "");

  bool ran_main_loop = false;
  if (parts_)
    ran_main_loop = parts_->MainMessageLoopRun(&result_code_);

  if (!ran_main_loop)
    MainMessageLoopRun();

  TRACE_EVENT_END_ETW("BrowserMain:MESSAGE_LOOP", 0, "");
}

void BrowserMainLoop::ShutdownThreadsAndCleanUp() {

  if (!created_threads_) {
    // Called early, nothing to do
    return;
  }
  TRACE_EVENT0("shutdown", "BrowserMainLoop::ShutdownThreadsAndCleanUp")

  // Teardown may start in PostMainMessageLoopRun, and during teardown we
  // need to be able to perform IO.
  base::ThreadRestrictions::SetIOAllowed(true);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(base::IgnoreResult(&base::ThreadRestrictions::SetIOAllowed),
                 true));

  if (parts_) {
    TRACE_EVENT0("shutdown",
                 "BrowserMainLoop::Subsystem:PostMainMessageLoopRun");
    parts_->PostMainMessageLoopRun();
  }

  trace_memory_controller_.reset();

#if !defined(OS_IOS)

#if defined(USE_AURA)
  {
    TRACE_EVENT0("shutdown",
                 "BrowserMainLoop::Subsystem:ImageTransportFactory");
    ImageTransportFactory::Terminate();
  }
#endif

  // The device monitors are using |system_monitor_| as dependency, so delete
  // them before |system_monitor_| goes away.
  // On Mac and windows, the monitor needs to be destroyed on the same thread
  // as they were created. On Linux, the monitor will be deleted when IO thread
  // goes away.
#if defined(OS_WIN)
  system_message_window_.reset();
#elif defined(OS_MACOSX)
  device_monitor_mac_.reset();
#endif
#endif  // !defined(OS_IOS)

  // Must be size_t so we can subtract from it.
  for (size_t thread_id = BrowserThread::ID_COUNT - 1;
       thread_id >= (BrowserThread::UI + 1);
       --thread_id) {
    // Find the thread object we want to stop. Looping over all valid
    // BrowserThread IDs and DCHECKing on a missing case in the switch
    // statement helps avoid a mismatch between this code and the
    // BrowserThread::ID enumeration.
    //
    // The destruction order is the reverse order of occurrence in the
    // BrowserThread::ID list. The rationale for the order is as
    // follows (need to be filled in a bit):
    //
    //
    // - The IO thread is the only user of the CACHE thread.
    //
    // - The PROCESS_LAUNCHER thread must be stopped after IO in case
    //   the IO thread posted a task to terminate a process on the
    //   process launcher thread.
    //
    // - (Not sure why DB stops last.)
    switch (thread_id) {
      case BrowserThread::DB: {
          TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:DBThread");
          db_thread_.reset();
        }
        break;
      case BrowserThread::FILE_USER_BLOCKING: {
          TRACE_EVENT0("shutdown",
                       "BrowserMainLoop::Subsystem:FileUserBlockingThread");
          file_user_blocking_thread_.reset();
        }
        break;
      case BrowserThread::FILE: {
          TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:FileThread");
          file_thread_.reset();
        }
        break;
      case BrowserThread::PROCESS_LAUNCHER: {
          TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:LauncherThread");
          process_launcher_thread_.reset();
        }
        break;
      case BrowserThread::CACHE: {
          TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:CacheThread");
          cache_thread_.reset();
        }
        break;
      case BrowserThread::IO: {
          TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:IOThread");
          io_thread_.reset();
        }
        break;
      case BrowserThread::UI:
      case BrowserThread::ID_COUNT:
      default:
        NOTREACHED();
        break;
    }
  }

  // Close the blocking I/O pool after the other threads. Other threads such
  // as the I/O thread may need to schedule work like closing files or flushing
  // data during shutdown, so the blocking pool needs to be available. There
  // may also be slow operations pending that will blcok shutdown, so closing
  // it here (which will block until required operations are complete) gives
  // more head start for those operations to finish.
  {
    TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:ThreadPool");
    BrowserThreadImpl::ShutdownThreadPool();
  }

#if !defined(OS_IOS)
  // Must happen after the IO thread is shutdown since this may be accessed from
  // it.
  {
    TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:GPUChannelFactory");
    BrowserGpuChannelHostFactory::Terminate();
  }

#endif  // !defined(OS_IOS)

  if (parts_) {
    TRACE_EVENT0("shutdown", "BrowserMainLoop::Subsystem:PostDestroyThreads");
    parts_->PostDestroyThreads();
  }
}

void BrowserMainLoop::InitializeMainThread() {
  TRACE_EVENT0("startup", "BrowserMainLoop::InitializeMainThread")
  const char* kThreadName = "CrBrowserMain";
  base::PlatformThread::SetName(kThreadName);
  if (main_message_loop_)
    main_message_loop_->set_thread_name(kThreadName);

  // Register the main thread by instantiating it, but don't call any methods.
  main_thread_.reset(
      new BrowserThreadImpl(BrowserThread::UI, base::MessageLoop::current()));
}

int BrowserMainLoop::BrowserThreadsStarted() {
  TRACE_EVENT0("startup", "BrowserMainLoop::BrowserThreadsStarted")

#if defined(OS_ANDROID)
  // Up the priority of anything that touches with display tasks
  // (this thread is UI thread, and io_thread_ is for IPCs).
  io_thread_->SetPriority(base::kThreadPriority_Display);
  base::PlatformThread::SetThreadPriority(
      base::PlatformThread::CurrentHandle(),
      base::kThreadPriority_Display);
#endif

#if !defined(OS_IOS)

  BrowserGpuChannelHostFactory::Initialize();
#if defined(USE_AURA)
  ImageTransportFactory::Initialize();
#endif

#if defined(OS_LINUX)
  device_monitor_linux_.reset(new DeviceMonitorLinux());
#elif defined(OS_MACOSX)
  device_monitor_mac_.reset(new DeviceMonitorMac());
#endif

  // Initialize the GpuDataManager before we set up the MessageLoops because
  // otherwise we'll trigger the assertion about doing IO on the UI thread.
  GpuDataManagerImpl::GetInstance()->Initialize();

  // Alert the clipboard class to which threads are allowed to access the
  // clipboard:
  std::vector<base::PlatformThreadId> allowed_clipboard_threads;
  // The current thread is the UI thread.
  allowed_clipboard_threads.push_back(base::PlatformThread::CurrentId());
#if defined(OS_WIN)
  // On Windows, clipboards are also used on the File or IO threads.
  allowed_clipboard_threads.push_back(file_thread_->thread_id());
  allowed_clipboard_threads.push_back(io_thread_->thread_id());
#endif
  ui::Clipboard::SetAllowedThreads(allowed_clipboard_threads);

  // When running the GPU thread in-process, avoid optimistically starting it
  // since creating the GPU thread races against creation of the one-and-only
  // ChildProcess instance which is created by the renderer thread.
  bool always_uses_gpu = IsForceCompositingModeEnabled();
#if defined(USE_AURA)
  if (!GpuDataManagerImpl::GetInstance()->CanUseGpuBrowserCompositor())
    always_uses_gpu = false;
#endif
  if (GpuDataManagerImpl::GetInstance()->GpuAccessAllowed(NULL) &&
      always_uses_gpu &&
      !parsed_command_line_.HasSwitch(switches::kDisableGpuProcessPrelaunch) &&
      !parsed_command_line_.HasSwitch(switches::kSingleProcess) &&
      !parsed_command_line_.HasSwitch(switches::kInProcessGPU)) {
    TRACE_EVENT_INSTANT0("gpu", "Post task to launch GPU process",
                         TRACE_EVENT_SCOPE_THREAD);
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE, base::Bind(
            base::IgnoreResult(&GpuProcessHost::Get),
            GpuProcessHost::GPU_PROCESS_KIND_SANDBOXED,
            CAUSE_FOR_GPU_LAUNCH_BROWSER_STARTUP));
  }
#endif  // !defined(OS_IOS)
  return result_code_;
}

void BrowserMainLoop::InitializeToolkit() {
  TRACE_EVENT0("startup", "BrowserMainLoop::InitializeToolkit")
  // TODO(evan): this function is rather subtle, due to the variety
  // of intersecting ifdefs we have.  To keep it easy to follow, there
  // are no #else branches on any #ifs.
  // TODO(stevenjb): Move platform specific code into platform specific Parts
  // (Need to add InitializeToolkit stage to BrowserParts).
#if defined(OS_LINUX) || defined(OS_OPENBSD)
  // g_type_init will be deprecated in 2.36. 2.35 is the development
  // version for 2.36, hence do not call g_type_init starting 2.35.
  // http://developer.gnome.org/gobject/unstable/gobject-Type-Information.html#g-type-init
#if !GLIB_CHECK_VERSION(2, 35, 0)
  // Glib type system initialization. Needed at least for gconf,
  // used in net/proxy/proxy_config_service_linux.cc. Most likely
  // this is superfluous as gtk_init() ought to do this. It's
  // definitely harmless, so retained as a reminder of this
  // requirement for gconf.
  g_type_init();
#endif

#if !defined(USE_AURA)
  gfx::GtkInitFromCommandLine(parsed_command_line_);
#endif

  SetUpGLibLogHandler();
#endif

#if defined(TOOLKIT_GTK)
  // It is important for this to happen before the first run dialog, as it
  // styles the dialog as well.
  gfx::InitRCStyles();
#endif

#if defined(OS_WIN)
  // Init common control sex.
  INITCOMMONCONTROLSEX config;
  config.dwSize = sizeof(config);
  config.dwICC = ICC_WIN95_CLASSES;
  if (!InitCommonControlsEx(&config))
    LOG_GETLASTERROR(FATAL);
#endif

  if (parts_)
    parts_->ToolkitInitialized();
}

void BrowserMainLoop::MainMessageLoopRun() {
#if defined(OS_ANDROID)
  // Android's main message loop is the Java message loop.
  NOTREACHED();
#else
  DCHECK_EQ(base::MessageLoop::TYPE_UI, base::MessageLoop::current()->type());
  if (parameters_.ui_task)
    base::MessageLoopForUI::current()->PostTask(FROM_HERE,
                                                *parameters_.ui_task);

  base::RunLoop run_loop;
  run_loop.Run();
#endif
}

}  // namespace content
