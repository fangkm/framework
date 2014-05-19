#ifndef _MAIN_FRAME_81328314_1D00_4F53_A1AF_859153316B28_
#define _MAIN_FRAME_81328314_1D00_4F53_A1AF_859153316B28_

#include "ui/views/widget/widget.h"

namespace director {

class MainView;

class MainFrame : public views::Widget {
public:
	explicit MainFrame(MainView* main_view);
	~MainFrame();

	void InitFrame();

private:
	MainView* main_view_;
};

}  // namespace director

#endif  // _MAIN_FRAME_81328314_1D00_4F53_A1AF_859153316B28_
