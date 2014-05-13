#ifndef _MAIN_WINDOW_IMPL_A78907C2_2F32_45FC_974D_FBDF9C20639A_
#define _MAIN_WINDOW_IMPL_A78907C2_2F32_45FC_974D_FBDF9C20639A_

#include "ui/views/widget/widget_delegate.h"
#include "director/views/main_window.h"

namespace director {

class MainWindowImpl : public MainWindow, public views::WidgetDelegateView {
public:
	MainWindowImpl(DirectorContext* context);
	virtual ~MainWindowImpl();

private:
	// Overridden from WidgetDelegateView:
	virtual bool CanResize() const OVERRIDE;
	virtual bool CanMaximize() const OVERRIDE;
	virtual string16 GetWindowTitle() const OVERRIDE;
	virtual View* GetContentsView() OVERRIDE;
	virtual void WindowClosing() OVERRIDE;

	// Overridden from View:
	virtual void ViewHierarchyChanged(
			const ViewHierarchyChangedDetails& details) OVERRIDE;

	// Creates the layout within the examples window.
	void Initialize();

	DirectorContext* context_;

	DISALLOW_COPY_AND_ASSIGN(MainWindowImpl);
};

}  // namespace director

#endif  // _MAIN_WINDOW_IMPL_A78907C2_2F32_45FC_974D_FBDF9C20639A_
