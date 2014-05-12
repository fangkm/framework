// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/primary/primary_child_process_observer.h"

#include "content/primary/primary_child_process_host_impl.h"

namespace content {

// static
void PrimaryChildProcessObserver::Add(PrimaryChildProcessObserver* observer) {
  PrimaryChildProcessHostImpl::AddObserver(observer);
}

// static
void PrimaryChildProcessObserver::Remove(
    PrimaryChildProcessObserver* observer) {
  PrimaryChildProcessHostImpl::RemoveObserver(observer);
}

}  // namespace content
