// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/primary/content_main_client.h"

#include "base/files/file_path.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace content {

MainParts* ContentMainClient::CreateBrowserMainParts(
    const MainFunctionParams& parameters) {
  return NULL;
}

std::string ContentMainClient::GetApplicationLocale() {
  return "en-US";
}

gfx::ImageSkia* ContentMainClient::GetDefaultFavicon() {
  static gfx::ImageSkia* empty = new gfx::ImageSkia();
  return empty;
}


}  // namespace content
