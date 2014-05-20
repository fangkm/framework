#include "director/views/main_view.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/label.h"
#include "director/views/main_frame.h"

namespace director {
using namespace views;

// static
const char MainView::kViewClassName[] = "MainView";

MainView::MainView(Director* context)
		: context_(context) {
	set_owned_by_client();
}

MainView::~MainView() {

}

void MainView::SetFrame(MainFrame* frame) { 
	frame_ = frame; 
}

MainFrame* MainView::GetFrame() const { 
	return frame_; 
}

bool MainView::CanResize() const { 
	return true; 
}

bool MainView::CanMaximize() const { 
	return true; 
}

string16 MainView::GetWindowTitle() const {
	return ASCIIToUTF16("Video Render");
}

View* MainView::GetContentsView() { 
	return this; 
}

void MainView::WindowClosing() {
	base::MessageLoop::current()->Quit();
}

// Overridden from View:
void MainView::ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) {
	if (details.is_add && details.child == this)
		Initialize();
}

// Creates the layout within the examples window.
void MainView::Initialize() {
	set_background(Background::CreateStandardPanelBackground());

	Label* label = new Label(L"选择摄像头:");
	label->SetBounds(10, 10, 120, 25);
	AddChildView(label);

	label = new Label(L"选择分辨率:");
	label->SetBounds(10, 50, 120, 25);
	AddChildView(label);

	camera_devices_ = new Combobox(&video_devices_model_);
	camera_devices_->SetBounds(130, 10, 250, 25);
	AddChildView(camera_devices_);
	//combobox_camera_->set_listener(this);
	//combobox_camera_->SetSelectedIndex(3);

	camera_dimension_ = new Combobox(&video_devices_model_);
	camera_dimension_->SetBounds(130, 50, 250, 25);
	AddChildView(camera_dimension_);
	//combobox_dimension_->set_listener(this);
	//combobox_dimension_->SetSelectedIndex(3);

	video_render_ = new ImageView;
	video_render_->SetBounds(10, 100, 640, 480);
	AddChildView(video_render_);

	gfx::Canvas  canvas(gfx::Size(640, 480), ui::SCALE_FACTOR_200P, false);
	canvas.DrawColor(SkColorSetRGB(155, 100, 155));
	
	video_render_->SetImage(gfx::ImageSkia(canvas.ExtractImageRep()));
}

// static
MainWindow* MainWindow::Create(Director* context) {
	MainView* view = new MainView(context);
	(new MainFrame(view))->InitFrame();
	return view;
}

}  // namespace director
;