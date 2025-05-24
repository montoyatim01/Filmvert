#ifndef _windowutils_h
#define _windowutils_h
#include "window.h"

bool ColorEdit4WithFineTune(const char* label, float col[4], ImGuiColorEditFlags flags = 0);
void transformCoordinates(int& x, int& y, int rotation, int width, int height);
void inverseTransformCoordinates(int& x, int& y, int rotation, int width, int height);
#endif
