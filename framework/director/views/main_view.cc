#include "director/views/main_view.h"


namespace director {
using namespace views;

// static
const char MainView::kViewClassName[] = "MainView";

MainView::MainView()
		: ClientView(NULL, NULL) {

}

MainView::~MainView() {

}

void MainView::Init(Director* director) {

}

void MainView::SetFrame(MainFrame* frame) { 
	frame_ = frame; 
}

MainFrame* MainView::GetFrame() const { 
	return frame_; 
}

}  // namespace director
