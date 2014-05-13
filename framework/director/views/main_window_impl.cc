#include "director/views/main_window_impl.h"

#include <string>

#include "base/memory/scoped_vector.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ui_base_paths.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"


namespace director {
using namespace views;
	
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
	column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 1,
		GridLayout::USE_PREF, 0, 0);
	column_set->AddPaddingColumn(0, 5);
	layout->AddPaddingRow(0, 5);
	layout->StartRow(0 /* no expand */, 0);

	layout->StartRow(0 /* no expand */, 0);
	//layout->AddView(status_label_);
	layout->AddPaddingRow(0, 5);
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
