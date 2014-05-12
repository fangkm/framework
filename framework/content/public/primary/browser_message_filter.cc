// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/primary/browser_message_filter.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/process/kill.h"
#include "base/process/process_handle.h"
#include "base/task_runner.h"
#include "content/public/primary/user_metrics.h"
#include "content/public/common/result_codes.h"
#include "ipc/ipc_sync_message.h"

using content::BrowserMessageFilter;

namespace content {

BrowserMessageFilter::BrowserMessageFilter()
    : channel_(NULL),
#if defined(OS_WIN)
      peer_handle_(base::kNullProcessHandle),
#endif
      peer_pid_(base::kNullProcessId) {
}

void BrowserMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  channel_ = channel;
}

void BrowserMessageFilter::OnChannelClosing() {
  channel_ = NULL;
}

void BrowserMessageFilter::OnChannelConnected(int32 peer_pid) {
  peer_pid_ = peer_pid;
}

bool BrowserMessageFilter::OnMessageReceived(const IPC::Message& message) {
  PrimaryThread::ID thread = PrimaryThread::IO;
  OverrideThreadForMessage(message, &thread);

  if (thread == PrimaryThread::IO) {
    scoped_refptr<base::TaskRunner> runner =
        OverrideTaskRunnerForMessage(message);
    if (runner.get()) {
      runner->PostTask(
          FROM_HERE,
          base::Bind(base::IgnoreResult(&BrowserMessageFilter::DispatchMessage),
                     this,
                     message));
      return true;
    }
    return DispatchMessage(message);
  }

  if (thread == PrimaryThread::UI && !CheckCanDispatchOnUI(message, this))
    return true;

  PrimaryThread::PostTask(
      thread, FROM_HERE,
      base::Bind(base::IgnoreResult(&BrowserMessageFilter::DispatchMessage),
                 this, message));
  return true;
}

base::ProcessHandle BrowserMessageFilter::PeerHandle() {
#if defined(OS_WIN)
  base::AutoLock lock(peer_handle_lock_);
  if (peer_handle_ == base::kNullProcessHandle)
    base::OpenPrivilegedProcessHandle(peer_pid_, &peer_handle_);

  return peer_handle_;
#else
  base::ProcessHandle result = base::kNullProcessHandle;
  base::OpenPrivilegedProcessHandle(peer_pid_, &result);
  return result;
#endif
}

bool BrowserMessageFilter::Send(IPC::Message* message) {
  if (message->is_sync()) {
    // We don't support sending synchronous messages from the browser.  If we
    // really needed it, we can make this class derive from SyncMessageFilter
    // but it seems better to not allow sending synchronous messages from the
    // browser, since it might allow a corrupt/malicious renderer to hang us.
    NOTREACHED() << "Can't send sync message through BrowserMessageFilter!";
    return false;
  }

  if (!PrimaryThread::CurrentlyOn(PrimaryThread::IO)) {
    PrimaryThread::PostTask(
        PrimaryThread::IO,
        FROM_HERE,
        base::Bind(base::IgnoreResult(&BrowserMessageFilter::Send), this,
                   message));
    return true;
  }

  if (channel_)
    return channel_->Send(message);

  delete message;
  return false;
}

void BrowserMessageFilter::OverrideThreadForMessage(const IPC::Message& message,
                                                    PrimaryThread::ID* thread) {
}

base::TaskRunner* BrowserMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  return NULL;
}

bool BrowserMessageFilter::CheckCanDispatchOnUI(const IPC::Message& message,
                                                IPC::Sender* sender) {
#if defined(OS_WIN)
  // On Windows there's a potential deadlock with sync messsages going in
  // a circle from browser -> plugin -> renderer -> browser.
  // On Linux we can avoid this by avoiding sync messages from browser->plugin.
  // On Mac we avoid this by not supporting windowed plugins.
  if (message.is_sync() && !message.is_caller_pumping_messages()) {
    // NOTE: IF YOU HIT THIS ASSERT, THE SOLUTION IS ALMOST NEVER TO RUN A
    // NESTED MESSAGE LOOP IN THE RENDERER!!!
    // That introduces reentrancy which causes hard to track bugs.  You should
    // find a way to either turn this into an asynchronous message, or one
    // that can be answered on the IO thread.
    NOTREACHED() << "Can't send sync messages to UI thread without pumping "
        "messages in the renderer or else deadlocks can occur if the page "
        "has windowed plugins! (message type " << message.type() << ")";
    IPC::Message* reply = IPC::SyncMessage::GenerateReply(&message);
    reply->set_reply_error();
    sender->Send(reply);
    return false;
  }
#endif
  return true;
}

void BrowserMessageFilter::BadMessageReceived() {
  base::KillProcess(PeerHandle(), content::RESULT_CODE_KILLED_BAD_MESSAGE,
                    false);
}

BrowserMessageFilter::~BrowserMessageFilter() {
#if defined(OS_WIN)
  if (peer_handle_ != base::kNullProcessHandle)
    base::CloseProcessHandle(peer_handle_);
#endif
}

bool BrowserMessageFilter::DispatchMessage(const IPC::Message& message) {
  bool message_was_ok = true;
  bool rv = OnMessageReceived(message, &message_was_ok);
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO) || rv) <<
      "Must handle messages that were dispatched to another thread!";
  if (!message_was_ok) {
    content::RecordAction(UserMetricsAction("BadMessageTerminate_BMF"));
    BadMessageReceived();
  }

  return rv;
}

}  // namespace content