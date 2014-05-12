// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/primary/browser_child_process_host_iterator.h"

#include "base/logging.h"
#include "content/primary/primary_child_process_host_impl.h"
#include "content/public/primary/primary_thread.h"

namespace content {

BrowserChildProcessHostIterator::BrowserChildProcessHostIterator()
    : all_(true), process_type_(PROCESS_TYPE_UNKNOWN) {
  CHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO)) <<
        "BrowserChildProcessHostIterator must be used on the IO thread.";
  iterator_ = PrimaryChildProcessHostImpl::GetIterator()->begin();
}

BrowserChildProcessHostIterator::BrowserChildProcessHostIterator(int type)
    : all_(false), process_type_(type) {
  CHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO)) <<
        "BrowserChildProcessHostIterator must be used on the IO thread.";
  iterator_ = PrimaryChildProcessHostImpl::GetIterator()->begin();
  if (!Done() && (*iterator_)->GetData().process_type != process_type_)
    ++(*this);
}

bool BrowserChildProcessHostIterator::operator++() {
  CHECK(!Done());
  do {
    ++iterator_;
    if (Done())
      break;

    if (!all_ && (*iterator_)->GetData().process_type != process_type_)
      continue;

    return true;
  } while (true);

  return false;
}

bool BrowserChildProcessHostIterator::Done() {
  return iterator_ == PrimaryChildProcessHostImpl::GetIterator()->end();
}

const ChildProcessData& BrowserChildProcessHostIterator::GetData() {
  CHECK(!Done());
  return (*iterator_)->GetData();
}

bool BrowserChildProcessHostIterator::Send(IPC::Message* message) {
  CHECK(!Done());
  return (*iterator_)->Send(message);
}

PrimaryChildProcessHostDelegate*
    BrowserChildProcessHostIterator::GetDelegate() {
  return (*iterator_)->delegate();
}

}  // namespace content
