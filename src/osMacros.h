#ifndef _OS_MACROS
#define _OS_MACROS

#ifdef __APPLE__
//On macOS, special functions are behind the modifier called "CMD"
#define FV_USE_CMD_KEY 1
#define FV_MOD_C       "⌘"
#else

//On other platforms, special functions are behind the modifier called "Control"
#define FV_USE_CMD_KEY 0
#define FV_MOD_C       "Ctrl"
#endif

#endif //_OS_MACROS