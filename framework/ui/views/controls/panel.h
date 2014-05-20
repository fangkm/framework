#ifndef _PANEL_E74966E8_9D6E_4DB7_89FA_2526C6565028_
#define _PANEL_E74966E8_9D6E_4DB7_89FA_2526C6565028_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/view.h"

namespace views {

/////////////////////////////////////////////////////////////////////////////
//
// Panel class
//
// A Panel is a view subclass that can display a nine image or color.
//
/////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT Panel : public View {
public:
  // Internal class name.
  static const char kViewClassName[];

  Panel();
  virtual ~Panel();

  void SetBackgroundImage(const gfx::ImageSkia& img);
  const gfx::ImageSkia& GetBackgroundImage() const;

  void SetBackgroundColor(SkColor color);
  SkColor GetBackgroundColor() const;

protected:
  // Overridden from View:
  virtual const char* GetClassName() const OVERRIDE;
  virtual bool HitTestRect(const gfx::Rect& rect) const OVERRIDE;
  virtual void OnBoundsChanged(const gfx::Rect& previous_bounds) OVERRIDE;
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;

private:
	// ±≥æ∞Õº∫Õ±≥æ∞—’…´
	// ªÊ÷∆À≥–Ú: ”≈œ»ªÊ÷∆Õº∆¨
	gfx::ImageSkia background_image_;
  SkColor background_color_;

  DISALLOW_COPY_AND_ASSIGN(Panel);
};

}  // namespace views

#endif  // _PANEL_E74966E8_9D6E_4DB7_89FA_2526C6565028_
