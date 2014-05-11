#include "director/main/director_views_delegate.h"

#include "base/command_line.h"
#include "base/logging.h"

#if defined(USE_AURA) && !defined(OS_CHROMEOS)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/native_widget_aura.h"
#endif

namespace director {
using namespace views;

DirectorViewsDelegate::DirectorViewsDelegate()
    : use_transparent_windows_(false) {
  DCHECK(!ViewsDelegate::views_delegate);
  ViewsDelegate::views_delegate = this;
}

DirectorViewsDelegate::~DirectorViewsDelegate() {
  ViewsDelegate::views_delegate = NULL;
}

void DirectorViewsDelegate::SetUseTransparentWindows(bool transparent) {
  use_transparent_windows_ = transparent;
}

void DirectorViewsDelegate::SaveWindowPlacement(const Widget* window,
                                            const std::string& window_name,
                                            const gfx::Rect& bounds,
                                            ui::WindowShowState show_state) {
}

bool DirectorViewsDelegate::GetSavedWindowPlacement(
    const std::string& window_name,
    gfx::Rect* bounds,
    ui:: WindowShowState* show_state) const {
  return false;
}

NonClientFrameView* DirectorViewsDelegate::CreateDefaultNonClientFrameView(
    Widget* widget) {
  return NULL;
}

bool DirectorViewsDelegate::UseTransparentWindows() const {
  return use_transparent_windows_;
}

void DirectorViewsDelegate::OnBeforeWidgetInit(
    Widget::InitParams* params,
    internal::NativeWidgetDelegate* delegate) {
#if defined(USE_AURA) && !defined(OS_CHROMEOS)
	// If we already have a native_widget, we don't have to try to come
	// up with one.
	if (params->native_widget)
		return;

	if (params->parent && params->type != views::Widget::InitParams::TYPE_MENU) {
		params->native_widget = new views::NativeWidgetAura(delegate);
	} else if (!params->parent && !params->context) {
		params->native_widget = new views::DesktopNativeWidgetAura(delegate);
	}
#endif
}

base::TimeDelta DirectorViewsDelegate::GetDefaultTextfieldObscuredRevealDuration() {
  return base::TimeDelta();
}

}  // namespace director
