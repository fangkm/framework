#ifndef _MAIN_WINDOW_1C7F4253_1EBE_4B20_95EE_15CB7D6E43F5_
#define _MAIN_WINDOW_1C7F4253_1EBE_4B20_95EE_15CB7D6E43F5_

namespace director {
class Director;

class MainWindow {
public:
	virtual ~MainWindow() { }

	static MainWindow* Create(Director* context);
};

}  // namespace director

#endif  // _MAIN_WINDOW_1C7F4253_1EBE_4B20_95EE_15CB7D6E43F5_
