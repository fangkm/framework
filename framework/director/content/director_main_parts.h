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
class DirectorContext;

class DirectorMainParts : public content::MainParts {
 public:
  explicit DirectorMainParts(
      const content::MainFunctionParams& parameters);
  virtual ~DirectorMainParts();

  // Overridden from content::MainParts:
  virtual void PreMainMessageLoopRun() OVERRIDE;
  virtual bool MainMessageLoopRun(int* result_code) OVERRIDE;
  virtual void PostMainMessageLoopRun() OVERRIDE;

 private:
  scoped_ptr<views::ViewsDelegate> views_delegate_;
	scoped_ptr<DirectorContext> context_;

  DISALLOW_COPY_AND_ASSIGN(DirectorMainParts);
};

}  // namespace director

#endif  // _DIRECTOR_BROWSER_MAIN_PARTS_8CB2C2FD_6F8A_4396_9D9F_7415FDFE68B2_
