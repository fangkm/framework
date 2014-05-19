#ifndef _DIRECTOR_CONTEXT_B325D5FD_0816_4B9C_9D1C_32E540F8D5A0_
#define _DIRECTOR_CONTEXT_B325D5FD_0816_4B9C_9D1C_32E540F8D5A0_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "director/views/main_window.h"

namespace director {

class Director {
public:
	Director();
	~Director();

	bool Init();

private:
	MainWindow* window_;
};

}  // namespace director

#endif  // _DIRECTOR_CONTEXT_B325D5FD_0816_4B9C_9D1C_32E540F8D5A0_
