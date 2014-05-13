#include "director/content/director_main_parts.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/common/content_switches.h"
#include "ui/views/focus/accelerator_handler.h"
#include "url/gurl.h"

#if defined(USE_AURA)
#include "ui/aura/env.h"
#include "ui/gfx/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/native_widget_aura.h"
#endif

#include "director/main/director_views_delegate.h"
#include "director/main/director_context.h"
#include "director/views/main_window.h"

namespace director {

DirectorMainParts::DirectorMainParts(
    const content::MainFunctionParams& parameters) {
}

DirectorMainParts::~DirectorMainParts() {
}

void DirectorMainParts::PreMainMessageLoopRun() {
  context_.reset(new DirectorContext);
	views_delegate_.reset(new DirectorViewsDelegate);

  gfx::Screen::SetScreenInstance(
      gfx::SCREEN_TYPE_NATIVE, views::CreateDesktopScreen());

	MainWindow::Create(context_.get());
}

void DirectorMainParts::PostMainMessageLoopRun() {
  views_delegate_.reset();
	aura::Env::DeleteInstance();
}

bool DirectorMainParts::MainMessageLoopRun(int* result_code) {
  base::RunLoop run_loop;
  run_loop.Run();
  return true;
}

}  // namespace director
