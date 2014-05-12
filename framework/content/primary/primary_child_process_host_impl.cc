// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/primary/primary_child_process_host_impl.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/synchronization/waitable_event.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/primary/primary_child_process_host_delegate.h"
#include "content/public/primary/primary_child_process_observer.h"
#include "content/public/primary/primary_thread.h"
#include "content/public/primary/child_process_data.h"
#include "content/public/primary/content_main_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/process_type.h"
#include "content/public/common/result_codes.h"

#if defined(OS_MACOSX)
#include "content/primary/mach_broker_mac.h"
#endif

namespace content {
namespace {

static base::LazyInstance<PrimaryChildProcessHostImpl::PrimaryChildProcessList>
    g_child_process_list = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<ObserverList<PrimaryChildProcessObserver> >
    g_observers = LAZY_INSTANCE_INITIALIZER;

void NotifyProcessHostConnected(const ChildProcessData& data) {
  FOR_EACH_OBSERVER(PrimaryChildProcessObserver, g_observers.Get(),
                    BrowserChildProcessHostConnected(data));
}

void NotifyProcessHostDisconnected(const ChildProcessData& data) {
  FOR_EACH_OBSERVER(PrimaryChildProcessObserver, g_observers.Get(),
                    BrowserChildProcessHostDisconnected(data));
}

void NotifyProcessCrashed(const ChildProcessData& data) {
  FOR_EACH_OBSERVER(PrimaryChildProcessObserver, g_observers.Get(),
                    BrowserChildProcessCrashed(data));
}

}  // namespace

PrimaryChildProcessHost* PrimaryChildProcessHost::Create(
    int process_type,
    PrimaryChildProcessHostDelegate* delegate) {
  return new PrimaryChildProcessHostImpl(process_type, delegate);
}

#if defined(OS_MACOSX)
base::ProcessMetrics::PortProvider* PrimaryChildProcessHost::GetPortProvider() {
  return MachBroker::GetInstance();
}
#endif

// static
PrimaryChildProcessHostImpl::PrimaryChildProcessList*
    PrimaryChildProcessHostImpl::GetIterator() {
  return g_child_process_list.Pointer();
}

// static
void PrimaryChildProcessHostImpl::AddObserver(
    PrimaryChildProcessObserver* observer) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::UI));
  g_observers.Get().AddObserver(observer);
}

// static
void PrimaryChildProcessHostImpl::RemoveObserver(
    PrimaryChildProcessObserver* observer) {
  // TODO(phajdan.jr): Check thread after fixing http://crbug.com/167126.
  g_observers.Get().RemoveObserver(observer);
}

PrimaryChildProcessHostImpl::PrimaryChildProcessHostImpl(
    int process_type,
    PrimaryChildProcessHostDelegate* delegate)
    : data_(process_type),
      delegate_(delegate),
      power_monitor_message_broadcaster_(this) {
  data_.id = ChildProcessHostImpl::GenerateChildProcessUniqueId();

  child_process_host_.reset(ChildProcessHost::Create(this));

  g_child_process_list.Get().push_back(this);
  GetContentClient()->MainClient()->BrowserChildProcessHostCreated(this);
}

PrimaryChildProcessHostImpl::~PrimaryChildProcessHostImpl() {
  g_child_process_list.Get().remove(this);

#if defined(OS_WIN)
  DeleteProcessWaitableEvent(early_exit_watcher_.GetWatchedEvent());
#endif
}

// static
void PrimaryChildProcessHostImpl::TerminateAll() {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  // Make a copy since the PrimaryChildProcessHost dtor mutates the original
  // list.
  PrimaryChildProcessList copy = g_child_process_list.Get();
  for (PrimaryChildProcessList::iterator it = copy.begin();
       it != copy.end(); ++it) {
    delete (*it)->delegate();  // ~*HostDelegate deletes *HostImpl.
  }
}

void PrimaryChildProcessHostImpl::Launch(
#if defined(OS_WIN)
    SandboxedProcessLauncherDelegate* delegate,
#elif defined(OS_POSIX)
    bool use_zygote,
    const base::EnvironmentMap& environ,
#endif
    CommandLine* cmd_line) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));

  GetContentClient()->MainClient()->AppendExtraCommandLineSwitches(
      cmd_line, data_.id);

  const CommandLine& browser_command_line = *CommandLine::ForCurrentProcess();
  static const char* kForwardSwitches[] = {
    switches::kDisableLogging,
    switches::kEnableDCHECK,
    switches::kEnableLogging,
    switches::kLoggingLevel,
    switches::kTraceToConsole,
    switches::kV,
    switches::kVModule,
#if defined(OS_POSIX)
    switches::kChildCleanExit,
#endif
  };
  cmd_line->CopySwitchesFrom(browser_command_line, kForwardSwitches,
                             arraysize(kForwardSwitches));

  child_process_.reset(new ChildProcessLauncher(
#if defined(OS_WIN)
      delegate,
#elif defined(OS_POSIX)
      use_zygote,
      environ,
      child_process_host_->TakeClientFileDescriptor(),
#endif
      cmd_line,
      data_.id,
      this));
}

const ChildProcessData& PrimaryChildProcessHostImpl::GetData() const {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  return data_;
}

ChildProcessHost* PrimaryChildProcessHostImpl::GetHost() const {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  return child_process_host_.get();
}

base::ProcessHandle PrimaryChildProcessHostImpl::GetHandle() const {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  DCHECK(child_process_.get())
      << "Requesting a child process handle before launching.";
  DCHECK(child_process_->GetHandle())
      << "Requesting a child process handle before launch has completed OK.";
  return child_process_->GetHandle();
}

void PrimaryChildProcessHostImpl::SetName(const string16& name) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  data_.name = name;
}

void PrimaryChildProcessHostImpl::SetHandle(base::ProcessHandle handle) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  data_.handle = handle;
}

void PrimaryChildProcessHostImpl::ForceShutdown() {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  g_child_process_list.Get().remove(this);
  child_process_host_->ForceShutdown();
}

void PrimaryChildProcessHostImpl::SetBackgrounded(bool backgrounded) {
  child_process_->SetProcessBackgrounded(backgrounded);
}

void PrimaryChildProcessHostImpl::SetTerminateChildOnShutdown(
    bool terminate_on_shutdown) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  child_process_->SetTerminateChildOnShutdown(terminate_on_shutdown);
}

void PrimaryChildProcessHostImpl::NotifyProcessInstanceCreated(
    const ChildProcessData& data) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::UI));
  FOR_EACH_OBSERVER(PrimaryChildProcessObserver, g_observers.Get(),
                    BrowserChildProcessInstanceCreated(data));
}

base::TerminationStatus PrimaryChildProcessHostImpl::GetTerminationStatus(
    bool known_dead, int* exit_code) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  if (!child_process_)  // If the delegate doesn't use Launch() helper.
    return base::GetTerminationStatus(data_.handle, exit_code);
  return child_process_->GetChildTerminationStatus(known_dead,
                                                   exit_code);
}

bool PrimaryChildProcessHostImpl::OnMessageReceived(
    const IPC::Message& message) {
  return delegate_->OnMessageReceived(message);
}

void PrimaryChildProcessHostImpl::OnChannelConnected(int32 peer_pid) {
#if defined(OS_WIN)
  // From this point onward, the exit of the child process is detected by an
  // error on the IPC channel.
  DeleteProcessWaitableEvent(early_exit_watcher_.GetWatchedEvent());
  early_exit_watcher_.StopWatching();
#endif

  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  PrimaryThread::PostTask(PrimaryThread::UI, FROM_HERE,
                          base::Bind(&NotifyProcessHostConnected, data_));

  delegate_->OnChannelConnected(peer_pid);
}

void PrimaryChildProcessHostImpl::OnChannelError() {
  delegate_->OnChannelError();
}

bool PrimaryChildProcessHostImpl::CanShutdown() {
  return delegate_->CanShutdown();
}

void PrimaryChildProcessHostImpl::OnChildDisconnected() {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));
  if (child_process_.get() || data_.handle) {
    DCHECK(data_.handle != base::kNullProcessHandle);
    int exit_code;
    base::TerminationStatus status = GetTerminationStatus(
        true /* known_dead */, &exit_code);
    switch (status) {
      case base::TERMINATION_STATUS_PROCESS_CRASHED:
      case base::TERMINATION_STATUS_ABNORMAL_TERMINATION: {
        delegate_->OnProcessCrashed(exit_code);
        PrimaryThread::PostTask(PrimaryThread::UI, FROM_HERE,
                                base::Bind(&NotifyProcessCrashed, data_));
        UMA_HISTOGRAM_ENUMERATION("ChildProcess.Crashed2",
                                  data_.process_type,
                                  PROCESS_TYPE_MAX);
        break;
      }
      case base::TERMINATION_STATUS_PROCESS_WAS_KILLED: {
        delegate_->OnProcessCrashed(exit_code);
        // Report that this child process was killed.
        UMA_HISTOGRAM_ENUMERATION("ChildProcess.Killed2",
                                  data_.process_type,
                                  PROCESS_TYPE_MAX);
        break;
      }
      case base::TERMINATION_STATUS_STILL_RUNNING: {
        UMA_HISTOGRAM_ENUMERATION("ChildProcess.DisconnectedAlive2",
                                  data_.process_type,
                                  PROCESS_TYPE_MAX);
      }
      default:
        break;
    }
    UMA_HISTOGRAM_ENUMERATION("ChildProcess.Disconnected2",
                              data_.process_type,
                              PROCESS_TYPE_MAX);
  }
  PrimaryThread::PostTask(PrimaryThread::UI, FROM_HERE,
                          base::Bind(&NotifyProcessHostDisconnected, data_));
  delete delegate_;  // Will delete us
}

bool PrimaryChildProcessHostImpl::Send(IPC::Message* message) {
  return child_process_host_->Send(message);
}

void PrimaryChildProcessHostImpl::OnProcessLaunched() {
  base::ProcessHandle handle = child_process_->GetHandle();
  if (!handle) {
    delete delegate_;  // Will delete us
    return;
  }

#if defined(OS_WIN)
  // Start a WaitableEventWatcher that will invoke OnProcessExitedEarly if the
  // child process exits. This watcher is stopped once the IPC channel is
  // connected and the exit of the child process is detecter by an error on the
  // IPC channel thereafter.
  DCHECK(!early_exit_watcher_.GetWatchedEvent());
  early_exit_watcher_.StartWatching(
      new base::WaitableEvent(handle),
      base::Bind(&PrimaryChildProcessHostImpl::OnProcessExitedEarly,
                 base::Unretained(this)));
#endif

  data_.handle = handle;
  delegate_->OnProcessLaunched();
}

#if defined(OS_WIN)

void PrimaryChildProcessHostImpl::DeleteProcessWaitableEvent(
    base::WaitableEvent* event) {
  if (!event)
    return;

  // The WaitableEvent does not own the process handle so ensure it does not
  // close it.
  event->Release();

  delete event;
}

void PrimaryChildProcessHostImpl::OnProcessExitedEarly(
    base::WaitableEvent* event) {
  DeleteProcessWaitableEvent(event);
  OnChildDisconnected();
}

#endif

}  // namespace content
