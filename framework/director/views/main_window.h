#ifndef _MAIN_WINDOW_51486125_5D3B_4748_B342_6E326475CC1F_
#define _MAIN_WINDOW_51486125_5D3B_4748_B342_6E326475CC1F_

namespace director {
class DirectorContext;

enum Operation {
  DO_NOTHING_ON_CLOSE = 0,
  QUIT_ON_CLOSE,
};

// Shows a window with the views examples in it.
void ShowMainWindow(Operation operation, DirectorContext* context);

}  // namespace director

#endif  // _MAIN_WINDOW_51486125_5D3B_4748_B342_6E326475CC1F_
