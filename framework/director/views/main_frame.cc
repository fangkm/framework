#include "director/views/main_frame.h"

#include "director/views/main_view.h"

namespace director {
using namespace views;

MainFrame::MainFrame(MainView* main_view)
		: main_view_(main_view) {
	main_view_->SetFrame(this);
}

MainFrame::~MainFrame() {

}

void MainFrame::InitFrame() {
	Widget::InitParams params;
	Init(params);
}

}  // namespace director
