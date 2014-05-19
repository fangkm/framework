#ifndef _MAIN_VIEW_AE037DDF_174D_491E_9667_62447DCFA24F_
#define _MAIN_VIEW_AE037DDF_174D_491E_9667_62447DCFA24F_
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/client_view.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/combobox/combobox.h"
#include "director/views/main_window.h"
#include "director/views/model/video_combo_model.h"

namespace director {
class Director;
class MainFrame;

class MainView  : public MainWindow,
									public views::WidgetDelegateView {
public:
	static const char kViewClassName[];

	explicit MainView(Director* context);
	virtual ~MainView();

	void SetFrame(MainFrame* frame);
	MainFrame* GetFrame() const;

private:
	// Overridden from WidgetDelegate:
	virtual bool CanResize() const OVERRIDE;
	virtual bool CanMaximize() const OVERRIDE;
	virtual string16 GetWindowTitle() const OVERRIDE;
	virtual views::View* GetContentsView() OVERRIDE;
	virtual void WindowClosing() OVERRIDE;

	// Overridden from View:
	virtual void ViewHierarchyChanged(
			const ViewHierarchyChangedDetails& details) OVERRIDE;

	void Initialize();

private:
	Director* context_;
	MainFrame* frame_;

	views::Combobox* camera_devices_;
	views::Combobox* camera_dimension_;
	CameraListModel video_devices_model_;

	views::ImageView* video_render_;
};

}  // namespace director

#endif  // _MAIN_VIEW_AE037DDF_174D_491E_9667_62447DCFA24F_
