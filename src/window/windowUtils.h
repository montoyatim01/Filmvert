#ifndef _windowutils_h
#define _windowutils_h

void transformCoordinates(int& x, int& y, int rotation, int width, int height);
void inverseTransformCoordinates(int& x, int& y, int rotation, int width, int height);
#endif
