#include "ui/views/layout/relative_layout.h"

#include "base/logging.h"

namespace views {

// Component
Component::Component()
		: align_(kAlignLeft),
			start_(0),
			end_(0) {

}

Component::Component(Alignment align, uint32 start, uint32 end)
		: align_(align),
			start_(start),
			end_(end){

}

void Component::GetPosition(uint32 parent_size, uint32& start, uint32& end) const {
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
		start = parent_size - start_;
		end   = start - end_;
		break;
	case kAlignSize:
		start = start_;
		end   = parent_size - end_;
		break;
	}
}

// RelativeAdapter
RelativeAdapter::RelativeAdapter() 
		: view_(NULL) {

}

RelativeAdapter::RelativeAdapter(View* view)
		: view_(view) {

}

RelativeAdapter::~RelativeAdapter() {

}

View* RelativeAdapter::GetView() const {
	return view_;
}

gfx::Rect RelativeAdapter::GetViewRect(const gfx::Size& parent_size) const {
	uint32 left(0), top(0), right(0), bottom(0);
	xcomponent_.GetPosition(parent_size.width(), left, right);
	ycomponent_.GetPosition(parent_size.height(), top, bottom);
	return gfx::Rect(left, top, right - left, bottom - top);
}

RelativeAdapter& RelativeAdapter::LeftPos(uint32 xpos, uint32 width) {
	xcomponent_ = Component(Component::kAlignLeft, xpos, width);
	return *this;
}

RelativeAdapter& RelativeAdapter::TopPos(uint32 ypos, uint32 height) {
	ycomponent_ = Component(Component::kAlignTop, ypos, height);
	return *this;
}

RelativeAdapter& RelativeAdapter::RightPos(uint32 xpos, uint32 width) {
	xcomponent_ = Component(Component::kAlignRight, xpos, width);
	return *this;
}

RelativeAdapter& RelativeAdapter::BottomPos(uint32 ypos, uint32 height) {
	ycomponent_ = Component(Component::kAlignBottom, ypos, height);
	return *this;
}

RelativeAdapter& RelativeAdapter::HSizePos(uint32 left, uint32 right) {
	xcomponent_ = Component(Component::kAlignSize, left, right);
	return *this;
}

RelativeAdapter& RelativeAdapter::VSizePos(uint32 top, uint32 bottom) {
	ycomponent_ = Component(Component::kAlignSize, top, bottom);
	return *this;
}

RelativeAdapter& RelativeAdapter::SizePos() {
	return HSizePos().VSizePos();
}

RelativeAdapter& RelativeAdapter::HCenterPos(uint32 width, uint32 delta) {
	xcomponent_ = Component(Component::kAlignCenter, delta, width);
	return *this;
}

RelativeAdapter& RelativeAdapter::VCenterPos(uint32 height, uint32 delta) {
	ycomponent_ = Component(Component::kAlignCenter, delta, height);
	return *this;
}

// RelativeLayout
RelativeLayout::RelativeLayout(View* host)
		: host_(host) {}

RelativeLayout::~RelativeLayout() {}

RelativeAdapter& RelativeLayout::AddView(View* view) {
	if (!view->parent())
		host_->AddChildView(view);

	RelativeAdapter& relative_view = relatives_[view];
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

		RelativeAdapter& relative = relatives_[child];
		child->SetBoundsRect(relative.GetViewRect(rect.size()));
	}
}

gfx::Size RelativeLayout::GetPreferredSize(View* host) {
	return host->size();

	DCHECK_EQ(1, host->child_count());
	gfx::Rect rect(host->child_at(0)->GetPreferredSize());
	rect.Inset(-host->GetInsets());
	return rect.size();
}

}  // namespace views
