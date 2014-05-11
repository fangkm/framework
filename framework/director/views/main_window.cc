#include "director/views/main_window.h"

#include <string>

#include "base/memory/scoped_vector.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ui_base_paths.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace director {
using namespace views;
	
class MainWindow : public WidgetDelegateView {
 public:
  MainWindow(Operation operation)
      : example_shown_(new View),
        status_label_(new Label),
        operation_(operation) {
    instance_ = this;
  }
  virtual ~MainWindow() {}

  // Prints a message in the status area, at the bottom of the window.
  void SetStatus(const std::string& status) {
    status_label_->SetText(UTF8ToUTF16(status));
  }

  static MainWindow* instance() { return instance_; }

 private:
  // Overridden from WidgetDelegateView:
  virtual bool CanResize() const OVERRIDE { return true; }
  virtual bool CanMaximize() const OVERRIDE { return true; }
  virtual string16 GetWindowTitle() const OVERRIDE {
    return ASCIIToUTF16("Video Render");
  }
  virtual View* GetContentsView() OVERRIDE { return this; }
  virtual void WindowClosing() OVERRIDE {
    instance_ = NULL;
    if (operation_ == QUIT_ON_CLOSE)
      base::MessageLoopForUI::current()->Quit();
  }

  // Overridden from View:
  virtual void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) OVERRIDE {
    if (details.is_add && details.child == this)
      InitMainWindow();
  }

  // Creates the layout within the examples window.
  void InitMainWindow() {
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
    layout->AddView(status_label_);
    layout->AddPaddingRow(0, 5);
  }

  static MainWindow* instance_;
  View* example_shown_;
  Label* status_label_;
  const Operation operation_;

  DISALLOW_COPY_AND_ASSIGN(MainWindow);
};

// static
MainWindow* MainWindow::instance_ = NULL;

void ShowMainWindow(Operation operation, DirectorContext* context) {
  if (MainWindow::instance()) {
    MainWindow::instance()->GetWidget()->Activate();
  } else {
    Widget* widget = new Widget;
    Widget::InitParams params;
    params.delegate = new MainWindow(operation);
    //params.bounds = gfx::Rect(0, 0, 700, 500);
    params.top_level = true;
    widget->Init(params);
		widget->CenterWindow(gfx::Size(700, 500));
    widget->Show();		
  }
}

void LogStatus(const std::string& string) {
  MainWindow::instance()->SetStatus(string);
}

}  // namespace director
