#ifndef _DIRECTOR_MAIN_DELEGATE_22F6AB5B_BA8D_4752_9A7B_8B2630A01528_
#define _DIRECTOR_MAIN_DELEGATE_22F6AB5B_BA8D_4752_9A7B_8B2630A01528_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/app/content_main_delegate.h"
#include "content/public/common/content_client.h"

namespace director {

class DirectorContentMainClient;

class DirectorMainDelegate : public content::ContentMainDelegate {
 public:
  DirectorMainDelegate();
  virtual ~DirectorMainDelegate();

  // content::ContentMainDelegate implementation
  virtual bool BasicStartupComplete(int* exit_code) OVERRIDE;
  virtual void PreSandboxStartup() OVERRIDE;
  virtual content::ContentMainClient* CreateContentMainClient() OVERRIDE;

 private:
  void InitializeResourceBundle();

  scoped_ptr<DirectorContentMainClient> main_client_;
  content::ContentClient content_client_;

  DISALLOW_COPY_AND_ASSIGN(DirectorMainDelegate);
};

}  // namespace director

#endif  // _DIRECTOR_MAIN_DELEGATE_22F6AB5B_BA8D_4752_9A7B_8B2630A01528_
