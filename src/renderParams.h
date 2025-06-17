#ifndef renderParams_h
#define renderParams_h

#ifdef __linux__
#include <stdint.h>
#endif


#define BLOCK_DIM 16
#define MINLOG 0.0001f



struct renderParams
{
  unsigned int width;
  unsigned int height;
  unsigned int bypass;
  unsigned int gradeBypass;
  unsigned int align;

  float sigmaFilter;
  float temp;
  float tint;
  float saturation;

  float baseColor[4];

  float blackPoint[4];
  float whitePoint[4];

  float G_blackpoint[4];
  float G_whitepoint[4];
  float G_lift[4];
  float G_gain[4];
  float G_mult[4];
  float G_offset[4];
  float G_gamma[4];

  float arbitraryRotation;
  float imageCropMinX;
  float imageCropMinY;
  float imageCropMaxX;
  float imageCropMaxY;
  int cropEnable;
  int cropVisible;


};


#endif
