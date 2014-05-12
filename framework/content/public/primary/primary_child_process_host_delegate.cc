// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/primary/primary_child_process_host_delegate.h"

namespace content {

bool PrimaryChildProcessHostDelegate::CanShutdown() {
  return true;
}

}  // namespace content
