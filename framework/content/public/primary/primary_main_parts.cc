// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/primary/primary_main_parts.h"

namespace content {

int MainParts::PreCreateThreads() {
  return 0;
}

bool MainParts::MainMessageLoopRun(int* result_code) {
  return false;
}

}  // namespace content
