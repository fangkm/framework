#include "director/views/main_view.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/panel.h"
#include "ui/views/layout/relative_layout.h"
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

	RelativeLayout* layout = new RelativeLayout(this);
	SetLayoutManager(layout);

	Label* label = new Label(L"选择摄像头:");
	layout->AddView(label).LeftPos(10, 120).TopPos(10, 25);

	label = new Label(L"选择分辨率:");
	layout->AddView(label).LeftPos(10, 120).TopPos(50, 25);

	camera_devices_ = new Combobox(&video_devices_model_);
	layout->AddView(camera_devices_).RightPos(10, 250).TopPos(10, 25);
	//combobox_camera_->set_listener(this);
	//combobox_camera_->SetSelectedIndex(3);

	camera_dimension_ = new Combobox(&video_devices_model_);
	layout->AddView(camera_dimension_).RightPos(10, 250).TopPos(50, 25);
	//combobox_dimension_->set_listener(this);
	//combobox_dimension_->SetSelectedIndex(3);

	Panel* panel = new Panel;
	layout->AddView(camera_dimension_).HSizePos(10, 10).VSizePos(100, 10);

	return;

	video_render_ = new ImageView;
	video_render_->SetBounds(10, 100, 640, 480);
	AddChildView(video_render_);

	gfx::Canvas  canvas(gfx::Size(640, 480), ui::SCALE_FACTOR_200P, false);
	canvas.DrawColor(SkColorSetRGB(0, 0, 0));
	
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