#ifndef _DIRECTOR_BROWSER_MAIN_PARTS_8CB2C2FD_6F8A_4396_9D9F_7415FDFE68B2_
#define _DIRECTOR_BROWSER_MAIN_PARTS_8CB2C2FD_6F8A_4396_9D9F_7415FDFE68B2_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_main_parts.h"

namespace content {
struct MainFunctionParams;
}

namespace views {
class ViewsDelegate;
}

namespace director {

class DirectorBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit DirectorBrowserMainParts(
      const content::MainFunctionParams& parameters);
  virtual ~DirectorBrowserMainParts();

  // Overridden from content::BrowserMainParts:
  virtual void PreMainMessageLoopRun() OVERRIDE;
  virtual bool MainMessageLoopRun(int* result_code) OVERRIDE;
  virtual void PostMainMessageLoopRun() OVERRIDE;

 private:
  scoped_ptr<views::ViewsDelegate> views_delegate_;

  DISALLOW_COPY_AND_ASSIGN(DirectorBrowserMainParts);
};

}  // namespace director

#endif  // _DIRECTOR_BROWSER_MAIN_PARTS_8CB2C2FD_6F8A_4396_9D9F_7415FDFE68B2_
