// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_CONTENT_BROWSER_CLIENT_H_
#define CONTENT_PUBLIC_BROWSER_CONTENT_BROWSER_CLIENT_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/values.h"
#include "content/public/common/content_client.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/common/window_container_type.h"
#include "ui/base/window_open_disposition.h"

#if defined(OS_POSIX) && !defined(OS_MACOSX)
#include "base/posix/global_descriptors.h"
#endif

class CommandLine;

namespace base {
class DictionaryValue;
class FilePath;
}
namespace crypto {
class CryptoModuleBlockingPasswordDelegate;
}

namespace gfx {
class ImageSkia;
}

namespace sandbox {
class TargetPolicy;
}

namespace ui {
class SelectFilePolicy;
}

namespace content {

class AccessTokenStore;
class BrowserChildProcessHost;
class BrowserContext;

struct MainFunctionParams;
struct Referrer;
struct ShowDesktopNotificationHostMsgParams;

// Embedder API (or SPI) for participating in browser logic, to be implemented
// by the client of the content browser. See ChromeContentBrowserClient for the
// principal implementation. The methods are assumed to be called on the UI
// thread unless otherwise specified. Use this "escape hatch" sparingly, to
// avoid the embedder interface ballooning and becoming very specific to Chrome.
// (Often, the call out to the client can happen in a different part of the code
// that either already has a hook out to the embedder, or calls out to one of
// the observer interfaces.)
class CONTENT_EXPORT ContentMainClient {
 public:
  virtual ~ContentMainClient() {}

  // Allows the embedder to set any number of custom MainParts
  // implementations for the browser startup code. See comments in
  // browser_main_parts.h.
  virtual MainParts* CreateBrowserMainParts(
      const MainFunctionParams& parameters);

  // Notifies that a BrowserChildProcessHost has been created.
  virtual void BrowserChildProcessHostCreated(BrowserChildProcessHost* host) {}

  // Allows the embedder to pass extra command line flags.
  // switches::kProcessType will already be set at this point.
  virtual void AppendExtraCommandLineSwitches(CommandLine* command_line,
                                              int child_process_id) {}

  // Returns the locale used by the application.
  // This is called on the UI and IO threads.
  virtual std::string GetApplicationLocale();

  // Returns the languages used in the Accept-Languages HTTP header.
  // (Not called GetAcceptLanguages so it doesn't clash with win32).
  virtual std::string GetAcceptLangs(BrowserContext* context);

  // Returns the default favicon.  The callee doesn't own the given bitmap.
  virtual gfx::ImageSkia* GetDefaultFavicon();

};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_CONTENT_BROWSER_CLIENT_H_
