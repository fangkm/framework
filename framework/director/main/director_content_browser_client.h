#ifndef _DIRECTOR_CONTENT_BROWSER_CLIENT_24DE9E02_EFE8_4F18_A9F3_333D4395641E_
#define _DIRECTOR_CONTENT_BROWSER_CLIENT_24DE9E02_EFE8_4F18_A9F3_333D4395641E_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/content_browser_client.h"

namespace director {

class DirectorBrowserMainParts;

class DirectorContentBrowserClient : public content::ContentBrowserClient {
 public:
  DirectorContentBrowserClient();
  virtual ~DirectorContentBrowserClient();

  // Overridden from content::ContentBrowserClient:
  virtual content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) OVERRIDE;

 private:
  DirectorBrowserMainParts* examples_browser_main_parts_;

  DISALLOW_COPY_AND_ASSIGN(DirectorContentBrowserClient);
};

}  // namespace director

#endif  // _DIRECTOR_CONTENT_BROWSER_CLIENT_24DE9E02_EFE8_4F18_A9F3_333D4395641E_
