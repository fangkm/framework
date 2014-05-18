#ifndef _MAIN_WINDOW_IMPL_A78907C2_2F32_45FC_974D_FBDF9C20639A_
#define _MAIN_WINDOW_IMPL_A78907C2_2F32_45FC_974D_FBDF9C20639A_

#include "ui/views/widget/widget_delegate.h"
#include "ui/base/models/combobox_model.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "director/views/main_window.h"

namespace director {

	class ComboboxModelExample : public ui::ComboboxModel {
	public:
		ComboboxModelExample();
		virtual ~ComboboxModelExample();

		// Overridden from ui::ComboboxModel:
		virtual int GetItemCount() const OVERRIDE;
		virtual string16 GetItemAt(int index) OVERRIDE;

	private:
		DISALLOW_COPY_AND_ASSIGN(ComboboxModelExample);
	};

class MainWindowImpl : public MainWindow, 
											 public views::WidgetDelegateView,
											 public views::ComboboxListener {
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

	// Overridden from ComboboxListener:
	virtual void OnSelectedIndexChanged(views::Combobox* combobox) OVERRIDE;

	// Creates the layout within the examples window.
	void Initialize();

	DirectorContext* context_;

	ComboboxModelExample combobox_model_;
	views::Combobox* combobox_camera_;
	views::Combobox* combobox_dimension_;

	DISALLOW_COPY_AND_ASSIGN(MainWindowImpl);
};

}  // namespace director

#endif  // _MAIN_WINDOW_IMPL_A78907C2_2F32_45FC_974D_FBDF9C20639A_
