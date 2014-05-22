#ifndef _RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
#define _RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
#include <hash_map>
#include "base/compiler_specific.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/view.h"

namespace views {

namespace internal {

class Orientation {
public:
	enum Alignment {
		kAlignCenter   = 0,
		kAlignLeft,
		kAlignRight,
		kAlignTop,
		kAlignBottom,
		kAlignSize,
	};

public:
	Orientation();
	Orientation(Alignment align, uint32 start, uint32 end);

	void GetPosition(uint32 parent_size, uint32& start, uint32& end) const;

private:
	Alignment align_;
	uint32		start_;
	uint32		end_;
};

}  // namespace internal

class RelativeItem {
public:
	RelativeItem();
	explicit RelativeItem(View* view);
	~RelativeItem();

	View* GetView() const;
	gfx::Rect GetViewRect(const gfx::Size& parent_size) const;

	RelativeItem& LeftPos(uint32 xpos, uint32 width);
	RelativeItem& TopPos(uint32 ypos, uint32 height);
	RelativeItem& RightPos(uint32 xpos, uint32 width);
	RelativeItem& BottomPos(uint32 ypos, uint32 height);
	RelativeItem& HSizePos(uint32 left = 0, uint32 right = 0);
	RelativeItem& VSizePos(uint32 top = 0, uint32 bottom = 0);
	RelativeItem& SizePos();
	RelativeItem& HCenterPos(uint32 width, uint32 delta = 0);
	RelativeItem& VCenterPos(uint32 height, uint32 delta = 0);

private:
	View* view_;

	internal::Orientation x_orient_;
	internal::Orientation y_orient_;
};

///////////////////////////////////////////////////////////////////////////////
//
// RelativeLayout: 相对布局, 使用示例
//	class MyView : public View {
//	 public:
//		MyView() {
//			Initialize();
//		}

//		void Initialize() {
//			RelativeLayout* layout = CreateRelativeLayout(this);

//			Label* label = new Label;
//			layout->AddView(label).HSizePos(10, 10).TopPos(10, 25);

//			label = new Label;
//			layout->AddView(label).RightPos(10, 100).BottomPos(10, 25);
//		}
//	};
///////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT RelativeLayout : public LayoutManager {
 public:
  explicit RelativeLayout(View* host);
  virtual ~RelativeLayout();

	RelativeItem& AddView(View* view);

  // Overridden from LayoutManager:
  virtual void Layout(View* host) OVERRIDE;
  virtual gfx::Size GetPreferredSize(View* host) OVERRIDE;

 private:
	View* host_;

	typedef stdext::hash_map<View*, RelativeItem> RelativeMap;
	RelativeMap relatives_;

  DISALLOW_COPY_AND_ASSIGN(RelativeLayout);
};

RelativeLayout* CreateRelativeLayout(View* host);

}  // namespace views

#endif//_RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
