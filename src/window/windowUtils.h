#ifndef _windowutils_h
#define _windowutils_h


bool ImRightAlign(const char* str_id);
#define ImEndRightAlign ImGui::EndTable


void transformCoordinates(int& x, int& y, int rotation, int width, int height);
void inverseTransformCoordinates(int& x, int& y, int rotation, int width, int height);
#endif
