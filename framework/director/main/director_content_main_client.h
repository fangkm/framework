#ifndef _DIRECTOR_CONTENT_BROWSER_CLIENT_24DE9E02_EFE8_4F18_A9F3_333D4395641E_
#define _DIRECTOR_CONTENT_BROWSER_CLIENT_24DE9E02_EFE8_4F18_A9F3_333D4395641E_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/content_main_client.h"

namespace director {

class DirectorMainParts;

class DirectorContentMainClient : public content::ContentMainClient {
 public:
  DirectorContentMainClient();
  virtual ~DirectorContentMainClient();

  // Overridden from content::ContentMainClient:
  virtual content::MainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) OVERRIDE;

 private:
  DirectorMainParts* director_main_parts_;

  DISALLOW_COPY_AND_ASSIGN(DirectorContentMainClient);
};

}  // namespace director

#endif  // _DIRECTOR_CONTENT_BROWSER_CLIENT_24DE9E02_EFE8_4F18_A9F3_333D4395641E_
