#ifndef _MAIN_VIEW_AE037DDF_174D_491E_9667_62447DCFA24F_
#define _MAIN_VIEW_AE037DDF_174D_491E_9667_62447DCFA24F_
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/client_view.h"

#include "director/views/main_window.h"

namespace director {
class Director;
class MainFrame;

class MainView  : public MainWindow,
									public views::WidgetDelegate,
									public views::WidgetObserver,
									public views::ClientView {
public:
	static const char kViewClassName[];

	MainView();
	virtual ~MainView();

	void Init(Director* director);

	void SetFrame(MainFrame* frame);
	MainFrame* GetFrame() const;

private:
	MainFrame* frame_;
};

}  // namespace director

#endif  // _MAIN_VIEW_AE037DDF_174D_491E_9667_62447DCFA24F_
