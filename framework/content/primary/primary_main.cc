// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/primary/primary_main.h"

#include "base/debug/trace_event.h"
#include "content/common/content_constants_internal.h"
#include "content/public/primary/browser_main_runner.h"

namespace content {

// Main routine for running as the Browser process.
int PrimaryMain(const MainFunctionParams& parameters) {
  TRACE_EVENT_BEGIN_ETW("PrimaryMain", 0, "");
  base::debug::TraceLog::GetInstance()->SetProcessName("Browser");
  base::debug::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventBrowserProcessSortIndex);

  scoped_ptr<BrowserMainRunner> main_runner(BrowserMainRunner::Create());

  int exit_code = main_runner->Initialize(parameters);
  if (exit_code >= 0)
    return exit_code;

  exit_code = main_runner->Run();

  main_runner->Shutdown();

  TRACE_EVENT_END_ETW("PrimaryMain", 0, 0);

  return exit_code;
}

}  // namespace content
