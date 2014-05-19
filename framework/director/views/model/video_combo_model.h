#ifndef _VIDEO_COMBO_MODEL_996B63A1_8A93_49E2_ABA5_7EC8B98CB634_
#define _VIDEO_COMBO_MODEL_996B63A1_8A93_49E2_ABA5_7EC8B98CB634_

#include "ui/views/widget/widget_delegate.h"
#include "ui/base/models/combobox_model.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "director/views/main_window.h"

namespace director {

class CameraListModel : public ui::ComboboxModel {
public:
	CameraListModel();
	virtual ~CameraListModel();

	// Overridden from ui::ComboboxModel:
	virtual int GetItemCount() const OVERRIDE;
	virtual string16 GetItemAt(int index) OVERRIDE;

private:
	DISALLOW_COPY_AND_ASSIGN(CameraListModel);
};


}  // namespace director

#endif  // _VIDEO_COMBO_MODEL_996B63A1_8A93_49E2_ABA5_7EC8B98CB634_
