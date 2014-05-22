#include "ui/views/layout/relative_layout.h"

#include "base/logging.h"

namespace views {

namespace internal {

// Orientation
Orientation::Orientation()
		: align_(kAlignLeft),
			start_(0),
			end_(0) {
}

Orientation::Orientation(Alignment align, uint32 start, uint32 end)
		: align_(align),
			start_(start),
			end_(end){
}

void Orientation::GetPosition(uint32 parent_size, uint32& start, uint32& end) const {
	switch (align_) {
	case kAlignCenter:
		start = (parent_size + 2 * start_ - end_) / 2;
		end		= start + end_;
		break;
	case kAlignLeft:
	case kAlignTop:
		// start_为left或top值, end_为控件的宽度或高度
		start = start_;
		end   = start + end_;
		break;
	case kAlignRight:
	case kAlignBottom:
		// start_为right或bottom值, end_为控件的宽度或高度
		end    = parent_size - start_;
		start  = end - end_;
		break;
	case kAlignSize:
		start = start_;
		end   = parent_size - end_;
		break;
	}
}

} // namespace internal

using namespace internal;

// RelativeItem
RelativeItem::RelativeItem() 
		: view_(NULL) {

}

RelativeItem::RelativeItem(View* view)
		: view_(view) {

}

RelativeItem::~RelativeItem() {

}

View* RelativeItem::GetView() const {
	return view_;
}

gfx::Rect RelativeItem::GetViewRect(const gfx::Size& parent_size) const {
	uint32 left(0), top(0), right(0), bottom(0);
	x_orient_.GetPosition(parent_size.width(), left, right);
	y_orient_.GetPosition(parent_size.height(), top, bottom);
	return gfx::Rect(left, top, right - left, bottom - top);
}

RelativeItem& RelativeItem::LeftPos(uint32 xpos, uint32 width) {
	x_orient_ = Orientation(Orientation::kAlignLeft, xpos, width);
	return *this;
}

RelativeItem& RelativeItem::TopPos(uint32 ypos, uint32 height) {
	y_orient_ = Orientation(Orientation::kAlignTop, ypos, height);
	return *this;
}

RelativeItem& RelativeItem::RightPos(uint32 xpos, uint32 width) {
	x_orient_ = Orientation(Orientation::kAlignRight, xpos, width);
	return *this;
}

RelativeItem& RelativeItem::BottomPos(uint32 ypos, uint32 height) {
	y_orient_ = Orientation(Orientation::kAlignBottom, ypos, height);
	return *this;
}

RelativeItem& RelativeItem::HSizePos(uint32 left, uint32 right) {
	x_orient_ = Orientation(Orientation::kAlignSize, left, right);
	return *this;
}

RelativeItem& RelativeItem::VSizePos(uint32 top, uint32 bottom) {
	y_orient_ = Orientation(Orientation::kAlignSize, top, bottom);
	return *this;
}

RelativeItem& RelativeItem::SizePos() {
	return HSizePos().VSizePos();
}

RelativeItem& RelativeItem::HCenterPos(uint32 width, uint32 delta) {
	x_orient_ = Orientation(Orientation::kAlignCenter, delta, width);
	return *this;
}

RelativeItem& RelativeItem::VCenterPos(uint32 height, uint32 delta) {
	y_orient_ = Orientation(Orientation::kAlignCenter, delta, height);
	return *this;
}

// RelativeLayout
RelativeLayout::RelativeLayout(View* host)
		: host_(host) {}

RelativeLayout::~RelativeLayout() {}

RelativeItem& RelativeLayout::AddView(View* view) {
	if (!view->parent())
		host_->AddChildView(view);

	RelativeItem& relative_view = relatives_[view];
	return relative_view;
}

void RelativeLayout::Layout(View* host) {
  if (!host->has_children())
    return;

  gfx::Rect rect = host->GetContentsBounds();

	for (int i = 0; i < host->child_count(); ++i) {
		View* child = host->child_at(i);
		if (!child->visible())
			continue;

		RelativeItem& relative = relatives_[child];
		child->SetBoundsRect(relative.GetViewRect(rect.size()));
	}
}

gfx::Size RelativeLayout::GetPreferredSize(View* host) {
	return gfx::Size();
}

RelativeLayout* CreateRelativeLayout(View* host) {
	RelativeLayout* layout = new RelativeLayout(host);
	host->SetLayoutManager(layout);
	return layout;
}

}  // namespace views
