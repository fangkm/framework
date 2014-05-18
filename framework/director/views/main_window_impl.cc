#include "director/views/main_window_impl.h"

#include <string>
#include "base/strings/stringprintf.h"
#include "base/memory/scoped_vector.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ui_base_paths.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"


namespace director {
using namespace views;

ComboboxModelExample::ComboboxModelExample() {
}

ComboboxModelExample::~ComboboxModelExample() {
}

int ComboboxModelExample::GetItemCount() const {
	return 10;
}

string16 ComboboxModelExample::GetItemAt(int index) {
	return UTF8ToUTF16(base::StringPrintf("Item %d", index));
}
	
// MainWindowImpl
MainWindowImpl::MainWindowImpl(DirectorContext* context)
      : context_(context) {

}

MainWindowImpl::~MainWindowImpl() {

}

bool MainWindowImpl::CanResize() const { 
	return true; 
}

bool MainWindowImpl::CanMaximize() const { 
	return true; 
}

string16 MainWindowImpl::GetWindowTitle() const {
	return ASCIIToUTF16("Video Render");
}

View* MainWindowImpl::GetContentsView() { 
	return this; 
}

void MainWindowImpl::WindowClosing() {
	base::MessageLoop::current()->Quit();
}

// Overridden from View:
void MainWindowImpl::ViewHierarchyChanged(
		const ViewHierarchyChangedDetails& details) {
	if (details.is_add && details.child == this)
		Initialize();
}

// Creates the layout within the examples window.
void MainWindowImpl::Initialize() {

	set_background(Background::CreateStandardPanelBackground());
	GridLayout* layout = new GridLayout(this);
	SetLayoutManager(layout);
	ColumnSet* column_set = layout->AddColumnSet(0);
	column_set->AddPaddingColumn(0, 5);
	column_set->AddColumn(GridLayout::CENTER, GridLayout::CENTER, 0,
												GridLayout::FIXED, 150, 0);

	column_set->AddColumn(GridLayout::LEADING, GridLayout::CENTER, 1,
												GridLayout::FIXED, 200, 200);

	layout->AddPaddingRow(0, 5);

	combobox_camera_ = new Combobox(&combobox_model_);
	combobox_camera_->set_listener(this);
	combobox_camera_->SetSelectedIndex(3);

	combobox_dimension_ = new Combobox(&combobox_model_);
	combobox_dimension_->set_listener(this);
	combobox_dimension_->SetSelectedIndex(3);

	Label* label = new Label(L"选择摄像头:");

	layout->StartRow(0, 0);
	layout->AddView(label);
	layout->AddView(combobox_camera_, 1, 1, GridLayout::LEADING, GridLayout::CENTER, 200, 25);
	layout->AddPaddingRow(0, 10);

	layout->StartRow(0, 0);
	label = new Label(L"选择分辨率:");
	layout->AddView(label);
	layout->AddView(combobox_dimension_, 1, 1, GridLayout::LEADING, GridLayout::CENTER, 200, 25);
}

void MainWindowImpl::OnSelectedIndexChanged(Combobox* combobox) {

}

// static
MainWindow* MainWindow::Create(DirectorContext* context) {
	MainWindowImpl* wnd = new MainWindowImpl(context);
	Widget* widget = new Widget;
	Widget::InitParams params;
	params.delegate = wnd;
	//params.bounds = gfx::Rect(0, 0, 700, 500);
	params.top_level = true;
	widget->Init(params);
	widget->CenterWindow(gfx::Size(700, 500));
	widget->Show();	
	return wnd;
}

}  // namespace director
