#include "director/views/model/video_combo_model.h"

#include <string>
#include "base/strings/stringprintf.h"
#include "base/memory/scoped_vector.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ui_base_paths.h"

namespace director {
using namespace views;

CameraListModel::CameraListModel() {
}

CameraListModel::~CameraListModel() {
}

int CameraListModel::GetItemCount() const {
	return 10;
}

string16 CameraListModel::GetItemAt(int index) {
	return UTF8ToUTF16(base::StringPrintf("Item %d", index));
}

}  // namespace director
