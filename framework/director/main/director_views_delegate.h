#ifndef _DIRECTOR_VIEWS_DELEGATE_4C391C3E_EE2A_4334_959C_E8EC67EBE0AF_
#define _DIRECTOR_VIEWS_DELEGATE_4C391C3E_EE2A_4334_959C_E8EC67EBE0AF_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "build/build_config.h"
#include "ui/base/accessibility/accessibility_types.h"
#include "ui/views/views_delegate.h"

namespace ui {
class Clipboard;
}

namespace views {
class View;
class Widget;
}

namespace director {

class DirectorViewsDelegate : public views::ViewsDelegate {
 public:
  DirectorViewsDelegate();
  virtual ~DirectorViewsDelegate();

  void SetUseTransparentWindows(bool transparent);

  // Overridden from ViewsDelegate:
  virtual void SaveWindowPlacement(const views::Widget* window,
                                   const std::string& window_name,
                                   const gfx::Rect& bounds,
                                   ui::WindowShowState show_state) OVERRIDE;
  virtual bool GetSavedWindowPlacement(
      const std::string& window_name,
      gfx::Rect* bounds,
      ui::WindowShowState* show_state) const OVERRIDE;

  virtual void NotifyAccessibilityEvent(
      views::View* view, ui::AccessibilityTypes::Event event_type) OVERRIDE {}

  virtual void NotifyMenuItemFocused(const string16& menu_name,
                                     const string16& menu_item_name,
                                     int item_index,
                                     int item_count,
                                     bool has_submenu) OVERRIDE {}
#if defined(OS_WIN)
  virtual HICON GetDefaultWindowIcon() const OVERRIDE {
    return NULL;
  }
#endif
  virtual views::NonClientFrameView* CreateDefaultNonClientFrameView(
      views::Widget* widget) OVERRIDE;
  virtual bool UseTransparentWindows() const OVERRIDE;
  virtual void AddRef() OVERRIDE {}
  virtual void ReleaseRef() OVERRIDE {}
  virtual void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) OVERRIDE;
  virtual base::TimeDelta GetDefaultTextfieldObscuredRevealDuration() OVERRIDE;

 private:
  bool use_transparent_windows_;

  DISALLOW_COPY_AND_ASSIGN(DirectorViewsDelegate);
};

}  // namespace director

#endif  // _DIRECTOR_VIEWS_DELEGATE_4C391C3E_EE2A_4334_959C_E8EC67EBE0AF_
