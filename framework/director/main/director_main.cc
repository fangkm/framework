// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/app/content_main.h"
#include "sandbox/win/src/sandbox_types.h"
#include "director/main/director_main_delegate.h"
#include "content/app/startup_helper_win.h"


int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  director::DirectorMainDelegate delegate;
  return content::ContentMain(instance, &sandbox_info, &delegate);
}
