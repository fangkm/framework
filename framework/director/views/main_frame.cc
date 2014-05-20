#include "director/views/main_frame.h"

#include "director/views/main_view.h"

namespace director {
using namespace views;

MainFrame::MainFrame(MainView* view)
		: main_view_(view) {
	main_view_->SetFrame(this);
}

MainFrame::~MainFrame() {

}

void MainFrame::InitFrame() {
	Widget::InitParams params;
	params.delegate   =  main_view_;
	params.top_level  =  true;
	Init(params);
	CenterWindow(gfx::Size(676, 630));
	Show();	
}

}  // namespace director
