// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/app/content_main_delegate.h"

#include "content/public/browser/content_main_client.h"

namespace content {

bool ContentMainDelegate::BasicStartupComplete(int* exit_code) {
  return false;
}

int ContentMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  return -1;
}

#if defined(OS_MACOSX) && !defined(OS_IOS)

bool ContentMainDelegate::ProcessRegistersWithSystemProcess(
    const std::string& process_type) {
  return false;
}

bool ContentMainDelegate::ShouldSendMachPort(const std::string& process_type) {
  return true;
}

bool ContentMainDelegate::DelaySandboxInitialization(
    const std::string& process_type) {
  return false;
}

#elif defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_IOS)

ZygoteForkDelegate* ContentMainDelegate::ZygoteStarting() {
  return NULL;
}

#endif

ContentMainClient* ContentMainDelegate::CreateContentMainClient() {
#if defined(CHROME_MULTIPLE_DLL_CHILD)
  return NULL;
#else
  return new ContentMainClient();
#endif
}

}  // namespace content
