#ifndef renderParams_h
#define renderParams_h

#include <cstdint>
#ifdef __linux__
#include <stdint.h>
#endif


#define BLOCK_DIM 16
#define MINLOG 0.0001f
#define CURVE_MAX_PTS 16   // maximum control points per curve channel



struct renderParams
{
  unsigned int width;
  unsigned int height;
  unsigned int bypass;
  unsigned int gradeBypass;
  uint32_t secEnable;
  unsigned int showClip;
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
  float G_matrixR[3];
  float G_matrixG[3];
  float G_matrixB[3];
  float sharpen;
  float sharpenRadius;
  float curveW[CURVE_MAX_PTS * 2]; // interleaved x,y pairs — Master / luminance
  float curveR[CURVE_MAX_PTS * 2];
  float curveG[CURVE_MAX_PTS * 2];
  float curveB[CURVE_MAX_PTS * 2];
  int   curveW_n;  // active point count for each channel
  int   curveR_n;
  int   curveG_n;
  int   curveB_n;

  float arbitraryRotation;
  float imageCropMinX;
  float imageCropMinY;
  float imageCropMaxX;
  float imageCropMaxY;
  int cropEnable;
  int cropVisible;
  int channelView;


};


#endif
