#include "director/main/director.h"

namespace director {

Director::Director() {

}

Director::~Director() {

}

bool Director::Init() {

	window_ = MainWindow::Create(this);
	return true;
}

}  // namespace director
