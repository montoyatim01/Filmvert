#include <metal_math>
#include <metal_common>
#include <metal_compute>
#include <metal_geometric>
#include <metal_stdlib>

#include "../renderParams.h"
using namespace metal;



//---XYZ---//
float4 AP1toXYZ(float4 ACESAP1) {

    float4 XYZ = {0.0f, 0.0f, 0.0f, ACESAP1.w};  // preserve alpha channel

    const float matrix[9] = {0.691093846956809, 0.154408804238822, 0.154497348804368, 0.287610000124518, 0.658999962830599, 0.053390037044883, -0.005603358770650, 0.004305687238724, 1.001297671531927};
    XYZ.x = matrix[0]*ACESAP1.x + matrix[1]*ACESAP1.y + matrix[2]*ACESAP1.z;
    XYZ.y = matrix[3]*ACESAP1.x + matrix[4]*ACESAP1.y + matrix[5]*ACESAP1.z;
    XYZ.z = matrix[6]*ACESAP1.x + matrix[7]*ACESAP1.y + matrix[8]*ACESAP1.z;

    return XYZ;
}
float4 XYZtoAP1(float4 XYZ) {

    float4 ACESAP1 = {0.0f, 0.0f, 0.0f, XYZ.w};  // preserve alpha channel
    const float matrix[9] = {1.600599241442365, -0.373549352063788, -0.227049889378577, -0.699525331442052, 1.681235481150607, 0.018289850291445, 0.011965142266092, -0.009319911011437, 0.997354768745345};
    ACESAP1.x = matrix[0]*XYZ.x + matrix[1]*XYZ.y + matrix[2]*XYZ.z;
    ACESAP1.y = matrix[3]*XYZ.x + matrix[4]*XYZ.y + matrix[5]*XYZ.z;
    ACESAP1.z = matrix[6]*XYZ.x + matrix[7]*XYZ.y + matrix[8]*XYZ.z;

    return ACESAP1;
}


//---BLUR KERNELS---//
kernel void m_recursiveGaussian_rgba(device float4 *id [[buffer(0)]],
                                     device float4 *od [[buffer(1)]],
                                     constant int& w [[buffer(2)]],
                                     constant int& h [[buffer(3)]],
                                     constant float& a0 [[buffer(4)]],
                                     constant float& a1 [[buffer(5)]],
                                     constant float& a2 [[buffer(6)]],
                                     constant float& a3 [[buffer(7)]],
                                     constant float& b1 [[buffer(8)]],
                                     constant float& b2 [[buffer(9)]],
                                     constant float& coefp [[buffer(10)]],
                                     constant float& coefn [[buffer(11)]],
                                     uint pos [[thread_position_in_grid]])
{
  unsigned int x = pos;

  if (x >= w) return;

  device float4* inColumn = id + x;
  device float4* outColumn = od + x;

  id += x;  // advance pointers to correct column
  od += x;

  // forward pass
  float4 xp = {0.0f, 0.0f, 0.0f, 0.0f};  // previous input
  float4 yp = {0.0f, 0.0f, 0.0f, 0.0f};  // previous output
  float4 yb = {0.0f, 0.0f, 0.0f, 0.0f};  // previous output by 2
//#if CLAMP_TO_EDGE
  xp = *inColumn;
  yb = coefp * xp;
  yp = yb;
//#endif

  for (int y = 0; y < h; y++) {
    float4 xc = *inColumn;
    float4 yc = a0 * xc + a1 * xp - b1 * yp - b2 * yb;
    *outColumn = yc;
    inColumn += w;
    outColumn += w;  // move to next row
    xp = xc;
    yb = yp;
    yp = yc;
  }

  // reset pointers to point to last element in column
  inColumn -= w;
  outColumn -= w;

  // reverse pass
  // ensures response is symmetrical
  float4 xn = {0.0f, 0.0f, 0.0f, 0.0f};
  float4 xa = {0.0f, 0.0f, 0.0f, 0.0f};
  float4 yn = {0.0f, 0.0f, 0.0f, 0.0f};
  float4 ya = {0.0f, 0.0f, 0.0f, 0.0f};
//#if CLAMP_TO_EDGE
  xn = xa = *inColumn;
  yn = coefn * xn;
  ya = yn;
//#endif

  for (int y = h - 1; y >= 0; y--) {
    float4 xc = *inColumn;
    float4 yc = a2 * xn + a3 * xa - b1 * yn - b2 * ya;
    xa = xn;
    xn = xc;
    ya = yn;
    yn = yc;
    float4 oldValue = *outColumn;
    *outColumn = oldValue + yc;
    inColumn -= w;
    outColumn -= w;  // move to previous row
  }
}

kernel void m_transpose(device float4 *odata [[buffer(0)]],
                        device float4 *idata [[buffer(1)]],
                        constant int& width [[buffer(2)]],
                        constant int& height [[buffer(3)]],
                        uint2 gid [[ thread_position_in_grid ]],
                        uint2 tid [[ thread_position_in_threadgroup ]],
                        uint2 tgid [[ threadgroup_position_in_grid ]],
                        uint2 tgsize [[ threads_per_threadgroup ]])
{
  // Shared memory declaration
  threadgroup float4 block[BLOCK_DIM][BLOCK_DIM + 1];

  // Calculate global indices
  uint xIndex = tgid.x * BLOCK_DIM + tid.x;
  uint yIndex = tgid.y * BLOCK_DIM + tid.y;

  // Read the matrix tile into shared memory
  if ((xIndex < width) && (yIndex < height)) {
      uint index_in = yIndex * width + xIndex;
      block[tid.y][tid.x] = idata[index_in];
  }

  // Synchronize the threads in the threadgroup
  threadgroup_barrier(mem_flags::mem_threadgroup);

  // Calculate transposed indices
  xIndex = tgid.y * BLOCK_DIM + tid.x;
  yIndex = tgid.x * BLOCK_DIM + tid.y;

  // Write the transposed matrix tile to global memory
  if ((xIndex < height) && (yIndex < width)) {
      uint index_out = yIndex * height + xIndex;
      odata[index_out] = block[tid.x][tid.y];
  }
}

//---BASE COLOR ONLY---//
kernel void baseColorProcess(device float4 *imgIn [[buffer(0)]],
                    device float4 *imgOut [[buffer(1)]],
                    device renderParams *renderParams [[buffer(2)]],
                    uint2 pos [[thread_position_in_grid]])
{
    unsigned int index = (pos.y * renderParams->width) + pos.x;
    float4 pixIn = imgIn[index];
    float4 pixOut;
    pixOut.x = (renderParams->baseColor[0] / pixIn.x) * 0.1f;
    pixOut.y = (renderParams->baseColor[1] / pixIn.y) * 0.1f;
    pixOut.z = (renderParams->baseColor[2] / pixIn.z) * 0.1f;
    pixOut.w = 1.0f;
    imgOut[index] = pixOut;
}

//---MAIN KERNEL---//

kernel void mainProcess(device float4 *imgIn [[buffer(0)]],
                    device float4 *imgOut [[buffer(1)]],
                    device renderParams *renderParams [[buffer(2)]],
                    uint2 pos [[thread_position_in_grid]])
{
    unsigned int index = (pos.y * renderParams->width) + pos.x;
    float4 pixIn = imgIn[index];
    // Order of Operations
    // 1. Base / Img
    // Set White/Black points
    // Grade "node" operations
    //output = (input - blackpoint) / (whitepoint - blackpoint)

    //TODO: Fix this mess
    const float4 baseColor = float4(renderParams->baseColor[0], renderParams->baseColor[1], renderParams->baseColor[2], renderParams->baseColor[3]);
    const float4 blackPoint = float4(renderParams->blackPoint[0], renderParams->blackPoint[1], renderParams->blackPoint[2], renderParams->blackPoint[3]);
    const float4 whitePoint = float4(renderParams->whitePoint[0], renderParams->whitePoint[1], renderParams->whitePoint[2], renderParams->whitePoint[3]);

    const float4 G_blackpoint = float4(renderParams->G_blackpoint[0], renderParams->G_blackpoint[1], renderParams->G_blackpoint[2], renderParams->G_blackpoint[3]);
    const float4 G_whitepoint = float4(renderParams->G_whitepoint[0], renderParams->G_whitepoint[1], renderParams->G_whitepoint[2], renderParams->G_whitepoint[3]);
    const float4 G_lift = float4(renderParams->G_lift[0], renderParams->G_lift[1], renderParams->G_lift[2], renderParams->G_lift[3]);
    const float4 G_gain = float4(renderParams->G_gain[0], renderParams->G_gain[1], renderParams->G_gain[2], renderParams->G_gain[3]);
    const float4 G_mult = float4(renderParams->G_mult[0], renderParams->G_mult[1], renderParams->G_mult[2], renderParams->G_mult[3]);
    const float4 G_offset = float4(renderParams->G_offset[0], renderParams->G_offset[1], renderParams->G_offset[2], renderParams->G_offset[3]);
    const float4 G_gamma = float4(renderParams->G_gamma[0], renderParams->G_gamma[1], renderParams->G_gamma[2], renderParams->G_gamma[3]);



    // Base Color
    // *((float4*)&renderParams->baseColor)
    float4 pixOut = (baseColor / pixIn) * 0.1f;

    // Set White/Black Points
    pixOut = (pixOut - blackPoint) / (whitePoint - blackPoint);

    // Temp/Tint
    float4 tempPix = pixOut;
    float4 warm = {1.0f, 0.5f, 0.0f, 1.0f};
    float4 cool = {0.0f, 0.5f, 1.0f, 1.0f};
    float4 green = {0.0f, 1.0f, 0.0f, 1.0f};
    float4 mag = {1.0f, 0.0f, 1.0f, 1.0f};
    float temp = (0.75f * renderParams->temp);
    float tint = (0.25f * renderParams->tint);
    float4 ttXYZIn = AP1toXYZ(tempPix);
    ttXYZIn.b *= temp + 1.0f;
    ttXYZIn.g *= (-1.0f * tint) + 1.0f;
    tempPix = XYZtoAP1(ttXYZIn);

    // Grade node operation
    float4 aGrade = G_mult * (G_gain - G_lift) / (G_whitepoint - G_blackpoint);
    float4 bGrade = G_offset + G_lift - aGrade * G_blackpoint;
    tempPix = pow(aGrade * tempPix + bGrade, 1.0f/G_gamma);
    tempPix = clamp(tempPix, 0.0f, 100.0f);
//OCIOMain
    float4 out = (renderParams->bypass == 1 ? pixIn : renderParams->gradeBypass == 1 ? pixOut : tempPix);
    imgOut[index] = out;
    //dispImg[index] = (uchar4)(clamp(out * 255.0f, 0.0f, 255.0f));
}
