#ifndef renderParams_h
#define renderParams_h

#ifdef __linux__
#include <stdint.h>
#endif

#define KERNELSIZE 5
#define MAX_GREY_LEVEL 65536
#define EPSILON_GREY_LEVEL 0.1
#define BLOCK_DIM 16
#define MINLOG 0.0001f


//const float pi = 3.14159265358979323846f;

/*#define pickerAR = 99.0f/255.0f
#define pickerAG = 76.0f/255.0f
#define pickerAB = 44.0f/255.0f

#define pickerBR = 38.0f/255.0f
#define pickerBG = 54.0f/255.0f
#define pickerBB = 46.0f/255.0f

#define pickerCR = 20.0f/255.0f
#define pickerCG = 22.0f/255.0f
#define pickerCB = 34.0f/255.0f*/

//int iDivUp(int a, int b);

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


};

struct noise_prng
{
    uint32_t state;
};

struct vec2d
{
    float x;
    float y;
};


#endif
