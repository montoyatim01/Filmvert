#ifndef _windowutils_h
#define _windowutils_h
//#include "window.h"
#include "imgui.h"
#include "image.h"

void setTempColor(float tempVal);
void setTintColor(float tintVal);

bool ColorEdit4WithFineTune(const char* label, float col[4], bool cmyk, bool slow = false, ImGuiColorEditFlags flags = 0);
bool ColorEdit3WithFineTune(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
bool ColorEdit1WithFineTune(const char* label, float col[4], ImGuiColorEditFlags flags = 0);
bool TreeNodeWithLine(const char* label, bool* checked = nullptr, bool altHeld = false, float rightPad = 0.0f);
void transformCoordinates(int& x, int& y, int rotation, int width, int height);
void inverseTransformCoordinates(int& x, int& y, int rotation, int width, int height);

ImVec2 transformPointToCroppedDisplay(float origX, float origY, image* img, float dispScale, ImVec2 imagePos);
bool inverseTransformFromCroppedDisplay(ImVec2 displayPos, ImVec2 imagePos, image* img, float dispScale, float& origX, float& origY);

bool is_hidden(const std::filesystem::path& p);
#endif
