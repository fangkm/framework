#ifndef _RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
#define _RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_

#include "base/compiler_specific.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/view.h"

namespace views {

///////////////////////////////////////////////////////////////////////////////
//
// RelativeLayout
//  
//
///////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT RelativeLayout : public LayoutManager {
 public:
  RelativeLayout();
  virtual ~RelativeLayout();

  // Overridden from LayoutManager:
  virtual void Layout(View* host) OVERRIDE;
  virtual gfx::Size GetPreferredSize(View* host) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(RelativeLayout);
};

}  // namespace views

#endif//_RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
