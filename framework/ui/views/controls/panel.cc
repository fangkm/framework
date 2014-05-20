#include "ui/views/controls/panel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/accessibility/accessible_view_state.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/text/text_elider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/insets.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/text_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"


namespace views {

// static
const char Panel::kViewClassName[] = "Panel";

Panel::Panel() {
	background_color_  =  SkColorSetRGB(100, 100, 100);
}

Panel::~Panel() {
}

void Panel::SetBackgroundImage(const gfx::ImageSkia& img) {
	background_image_ = img;
	SchedulePaint();
}

const gfx::ImageSkia& Panel::GetBackgroundImage() const {
	return background_image_;
}

void Panel::SetBackgroundColor(SkColor color) {
	background_color_ = color;
	SchedulePaint();
}

SkColor Panel::GetBackgroundColor() const {
	return background_color_;
}

const char* Panel::GetClassName() const {
  return kViewClassName;
}

bool Panel::HitTestRect(const gfx::Rect& rect) const {
  return false;
}

void Panel::OnBoundsChanged(const gfx::Rect& previous_bounds) {

}

void Panel::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);

	canvas->DrawColor(background_color_);

	if (!background_image_.isNull()) {
		canvas->DrawImageInt(background_image_, 0, 0);
	}
}

}  // namespace views
