#include "ui/views/layout/relative_layout.h"

#include "base/logging.h"

namespace views {

RelativeLayout::RelativeLayout() {}

RelativeLayout::~RelativeLayout() {}

void RelativeLayout::Layout(View* host) {
  if (!host->has_children())
    return;

  View* frame_view = host->child_at(0);
  frame_view->SetBoundsRect(host->GetContentsBounds());
}

gfx::Size RelativeLayout::GetPreferredSize(View* host) {
  DCHECK_EQ(1, host->child_count());
  gfx::Rect rect(host->child_at(0)->GetPreferredSize());
  rect.Inset(-host->GetInsets());
  return rect.size();
}

}  // namespace views
