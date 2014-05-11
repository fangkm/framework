#ifndef _PRIMARY_MAIN_7314BF23_284F_4D96_AB89_0040A8088A72_
#define _PRIMARY_MAIN_7314BF23_284F_4D96_AB89_0040A8088A72_

#include "base/basictypes.h"
#include "content/common/content_export.h"

namespace content {

struct MainFunctionParams;

bool ExitedMainMessageLoop();

CONTENT_EXPORT int PrimaryMain(const content::MainFunctionParams& parameters);

}  // namespace content

#endif  // _PRIMARY_MAIN_7314BF23_284F_4D96_AB89_0040A8088A72_
