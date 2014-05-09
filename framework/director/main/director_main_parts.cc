#include "director/main/director_main_parts.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/common/content_switches.h"
#include "ui/views/focus/accelerator_handler.h"
#include "ui/views/test/desktop_test_views_delegate.h"
#include "url/gurl.h"

#if defined(USE_AURA)
#include "ui/aura/env.h"
#include "ui/gfx/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/native_widget_aura.h"
#endif

namespace director {

DirectorMainParts::DirectorMainParts(
    const content::MainFunctionParams& parameters) {
}

DirectorMainParts::~DirectorMainParts() {
}

void DirectorMainParts::PreMainMessageLoopRun() {
  //browser_context_.reset(new content::BrowserContext(false, NULL));

//#if !defined(OS_CHROMEOS) && defined(USE_AURA)
//  gfx::Screen::SetScreenInstance(
//      gfx::SCREEN_TYPE_NATIVE, CreateDesktopScreen());
//#endif
  //views_delegate_.reset(new views::DesktopTestViewsDelegate);

  //ShowExamplesWindowWithContent(QUIT_ON_CLOSE, NULL);
}

void DirectorMainParts::PostMainMessageLoopRun() {
  views_delegate_.reset();
#if defined(USE_AURA)
  aura::Env::DeleteInstance();
#endif
}

bool DirectorMainParts::MainMessageLoopRun(int* result_code) {
#if !defined(USE_AURA)
  AcceleratorHandler accelerator_handler;
  base::RunLoop run_loop(&accelerator_handler);
#else
  base::RunLoop run_loop;
#endif
  run_loop.Run();
  return true;
}

}  // namespace director
