#ifndef _RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
#define _RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
#include <hash_map>
#include "base/compiler_specific.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/view.h"

namespace views {

class Component {
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
	Component();
	Component(Alignment align, uint32 start, uint32 end);

	void GetPosition(uint32 parent_size, uint32& start, uint32& end) const;

private:
	Alignment align_;
	uint32		start_;
	uint32		end_;
};

class RelativeAdapter {
public:
	RelativeAdapter();
	explicit RelativeAdapter(View* view);
	~RelativeAdapter();

	View* GetView() const;
	gfx::Rect GetViewRect(const gfx::Size& parent_size) const;

	RelativeAdapter& LeftPos(uint32 xpos, uint32 width);
	RelativeAdapter& TopPos(uint32 ypos, uint32 height);
	RelativeAdapter& RightPos(uint32 xpos, uint32 width);
	RelativeAdapter& BottomPos(uint32 ypos, uint32 height);
	RelativeAdapter& HSizePos(uint32 left = 0, uint32 right = 0);
	RelativeAdapter& VSizePos(uint32 top = 0, uint32 bottom = 0);
	RelativeAdapter& SizePos();
	RelativeAdapter& HCenterPos(uint32 width, uint32 delta = 0);
	RelativeAdapter& VCenterPos(uint32 height, uint32 delta = 0);

private:
	View* view_;

	Component xcomponent_;
	Component ycomponent_;
};

///////////////////////////////////////////////////////////////////////////////
//
// RelativeLayout
//  
//
///////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT RelativeLayout : public LayoutManager {
 public:
  explicit RelativeLayout(View* host);
  virtual ~RelativeLayout();

	RelativeAdapter& AddView(View* view);

  // Overridden from LayoutManager:
  virtual void Layout(View* host) OVERRIDE;
  virtual gfx::Size GetPreferredSize(View* host) OVERRIDE;

 private:
	View* host_;

	typedef stdext::hash_map<View*, RelativeAdapter> RelativeMap;
	RelativeMap relatives_;

  DISALLOW_COPY_AND_ASSIGN(RelativeLayout);
};

}  // namespace views

#endif//_RELATIVE_LAYOUT_B408E1FC_BE2C_402C_B277_93B0EEB96C62_
