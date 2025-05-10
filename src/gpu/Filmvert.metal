#include <metal_math>
#include <metal_common>
#include <metal_compute>
#include <metal_geometric>
#include <metal_stdlib>

#include "../renderParams.h"
using namespace metal;


//---OCIO DISPLAY---//
// Declaration of class wrapper

struct ocioOCIOMain
{
ocioOCIOMain(
)
{
}


// Declaration of all helper methods


const int ocio_grading_rgbcurve_knotsOffsets_0[8] = {-1, 0, -1, 0, -1, 0, 0, 9};
const float ocio_grading_rgbcurve_knots_0[9] = {-5.26017761, -3.75502753, -2.24987745, -0.744727492, 1.06145251, 1.96573484, 2.86763239, 3.77526045, 4.67381239};
const int ocio_grading_rgbcurve_coefsOffsets_0[8] = {-1, 0, -1, 0, -1, 0, 0, 24};
const float ocio_grading_rgbcurve_coefs_0[24] = {0.185970441, 0.403778881, -0.0748505071, -0.185833707, -0.192129433, -0.19314684, -0.0501050949, -0.0511224195, 0., 0.559826851, 1.77532244, 1.54999995, 0.878701687, 0.531223178, 0.182825878, 0.0918722972, -4., -3.57868838, -1.82131326, 0.681241214, 2.87457752, 3.51206255, 3.8340621, 3.95872402};

float ocio_grading_rgbcurve_evalBSplineCurve_0(int curveIdx, float x)
{
  int knotsOffs = ocio_grading_rgbcurve_knotsOffsets_0[curveIdx * 2];
  int knotsCnt = ocio_grading_rgbcurve_knotsOffsets_0[curveIdx * 2 + 1];
  int coefsOffs = ocio_grading_rgbcurve_coefsOffsets_0[curveIdx * 2];
  int coefsCnt = ocio_grading_rgbcurve_coefsOffsets_0[curveIdx * 2 + 1];
  int coefsSets = coefsCnt / 3;
  if (coefsSets == 0)
  {
    return x;
  }
  float knStart = ocio_grading_rgbcurve_knots_0[knotsOffs];
  float knEnd = ocio_grading_rgbcurve_knots_0[knotsOffs + knotsCnt - 1];
  if (x <= knStart)
  {
    float B = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets];
    float C = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 2];
    return (x - knStart) * B + C;
  }
  else if (x >= knEnd)
  {
    float A = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets - 1];
    float B = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 2 - 1];
    float C = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 3 - 1];
    float kn = ocio_grading_rgbcurve_knots_0[knotsOffs + knotsCnt - 2];
    float t = knEnd - kn;
    float slope = 2. * A * t + B;
    float offs = ( A * t + B ) * t + C;
    return (x - knEnd) * slope + offs;
  }
  int i = 0;
  for (i = 0; i < knotsCnt - 2; ++i)
  {
    if (x < ocio_grading_rgbcurve_knots_0[knotsOffs + i + 1])
    {
      break;
    }
  }
  float A = ocio_grading_rgbcurve_coefs_0[coefsOffs + i];
  float B = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets + i];
  float C = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 2 + i];
  float kn = ocio_grading_rgbcurve_knots_0[knotsOffs + i];
  float t = x - kn;
  return ( A * t + B ) * t + C;
}

const int ocio_grading_rgbcurve_knotsOffsets_1[8] = {-1, 0, -1, 0, -1, 0, 0, 15};
const float ocio_grading_rgbcurve_knots_1[15] = {-2.54062366, -2.08035731, -1.62009084, -1.15982437, -0.69955802, -0.239291579, 0.220974833, 0.681241214, 1.01284635, 1.34445143, 1.6760565, 2.00766158, 2.33926654, 2.67087173, 3.00247669};
const int ocio_grading_rgbcurve_coefsOffsets_1[8] = {-1, 0, -1, 0, -1, 0, 0, 42};
const float ocio_grading_rgbcurve_coefs_1[42] = {0.521772683, 0.0654487088, 0.272604734, 0.123911291, 0.0858645961, -0.0171162505, 0.0338416733, -0.194834962, -0.201688975, -0.476983279, -0.276004612, -0.139139131, -0.0922630876, -0.0665909499, 0., 0.480308801, 0.54055649, 0.791498125, 0.90556252, 0.984603703, 0.968847632, 1., 0.870783448, 0.737021267, 0.420681119, 0.237632066, 0.145353615, 0.0841637775, -1.69896996, -1.58843505, -1.35350001, -1.04694998, -0.656400025, -0.221410006, 0.22814402, 0.681241214, 0.991421878, 1.25800002, 1.44994998, 1.55910003, 1.62259996, 1.66065454};

float ocio_grading_rgbcurve_evalBSplineCurve_1(int curveIdx, float x)
{
  int knotsOffs = ocio_grading_rgbcurve_knotsOffsets_1[curveIdx * 2];
  int knotsCnt = ocio_grading_rgbcurve_knotsOffsets_1[curveIdx * 2 + 1];
  int coefsOffs = ocio_grading_rgbcurve_coefsOffsets_1[curveIdx * 2];
  int coefsCnt = ocio_grading_rgbcurve_coefsOffsets_1[curveIdx * 2 + 1];
  int coefsSets = coefsCnt / 3;
  if (coefsSets == 0)
  {
    return x;
  }
  float knStart = ocio_grading_rgbcurve_knots_1[knotsOffs];
  float knEnd = ocio_grading_rgbcurve_knots_1[knotsOffs + knotsCnt - 1];
  if (x <= knStart)
  {
    float B = ocio_grading_rgbcurve_coefs_1[coefsOffs + coefsSets];
    float C = ocio_grading_rgbcurve_coefs_1[coefsOffs + coefsSets * 2];
    return (x - knStart) * B + C;
  }
  else if (x >= knEnd)
  {
    float A = ocio_grading_rgbcurve_coefs_1[coefsOffs + coefsSets - 1];
    float B = ocio_grading_rgbcurve_coefs_1[coefsOffs + coefsSets * 2 - 1];
    float C = ocio_grading_rgbcurve_coefs_1[coefsOffs + coefsSets * 3 - 1];
    float kn = ocio_grading_rgbcurve_knots_1[knotsOffs + knotsCnt - 2];
    float t = knEnd - kn;
    float slope = 2. * A * t + B;
    float offs = ( A * t + B ) * t + C;
    return (x - knEnd) * slope + offs;
  }
  int i = 0;
  for (i = 0; i < knotsCnt - 2; ++i)
  {
    if (x < ocio_grading_rgbcurve_knots_1[knotsOffs + i + 1])
    {
      break;
    }
  }
  float A = ocio_grading_rgbcurve_coefs_1[coefsOffs + i];
  float B = ocio_grading_rgbcurve_coefs_1[coefsOffs + coefsSets + i];
  float C = ocio_grading_rgbcurve_coefs_1[coefsOffs + coefsSets * 2 + i];
  float kn = ocio_grading_rgbcurve_knots_1[knotsOffs + i];
  float t = x - kn;
  return ( A * t + B ) * t + C;
}

// Declaration of the OCIO shader function

float4 OCIOMain(float4 inPixel)
{
  float4 outColor = inPixel;

  // Add Matrix processing

  {
    float4 res = float4(outColor.rgb.r, outColor.rgb.g, outColor.rgb.b, outColor.a);
    float4 tmp = res;
    res = float4x4(float4(0.69545224135745187, 0.044794563372037716, -0.0055258825581135443, 0.), float4(0.14067869647029416, 0.85967111845642163, 0.0040252103059786595, 0.), float4(0.16386906217225405, 0.0955343181715404, 1.0015006722521349, 0.), float4(0., 0., 0., 1.)) * tmp;
    outColor.rgb = float3(res.x, res.y, res.z);
    outColor.a = res.w;
  }

  // Add FixedFunction 'ACES_Glow10 (Forward)' processing

  {
    float chroma = sqrt( outColor.rgb.b * (outColor.rgb.b - outColor.rgb.g) + outColor.rgb.g * (outColor.rgb.g - outColor.rgb.r) + outColor.rgb.r * (outColor.rgb.r - outColor.rgb.b) );
    float YC = (outColor.rgb.b + outColor.rgb.g + outColor.rgb.r + 1.75 * chroma) / 3.;
    float maxval = max( outColor.rgb.r, max( outColor.rgb.g, outColor.rgb.b));
    float minval = min( outColor.rgb.r, min( outColor.rgb.g, outColor.rgb.b));
    float sat = ( max(1e-10, maxval) - max(1e-10, minval) ) / max(1e-2, maxval);
    float x = (sat - 0.4) * 5.;
    float t = max( 0., 1. - 0.5 * abs(x));
    float s = 0.5 * (1. + sign(x) * (1. - t * t));
    float GlowGain = 0.0500000007 * s;
    float GlowMid = 0.0799999982;
    float glowGainOut = mix(GlowGain, GlowGain * (GlowMid / YC - 0.5), float( YC > GlowMid * 2. / 3. ));
    glowGainOut = mix(glowGainOut, 0., float( YC > GlowMid * 2. ));
    outColor.rgb = outColor.rgb * glowGainOut + outColor.rgb;
  }

  // Add FixedFunction 'ACES_RedMod10 (Forward)' processing

  {
    float a = 2.0 * outColor.rgb.r - (outColor.rgb.g + outColor.rgb.b);
    float b = 1.7320508075688772 * (outColor.rgb.g - outColor.rgb.b);
    float hue = atan2(b, a);
    float knot_coord = clamp(2. + hue * float(1.6976527), 0., 4.);
    int j = int(min(knot_coord, 3.));
    float t = knot_coord - float(j);
    float4 monomials = float4(t*t*t, t*t, t, 1.);
    float4 m0 = float4(0.25, 0., 0., 0.);
    float4 m1 = float4(-0.75, 0.75, 0.75, 0.25);
    float4 m2 = float4(0.75, -1.5, 0., 1.);
    float4 m3 = float4(-0.25, 0.75, -0.75, 0.25);
    float4 coefs = mix(m0, m1, float(j == 1));
    coefs = mix(coefs, m2, float(j == 2));
    coefs = mix(coefs, m3, float(j == 3));
    float f_H = dot(coefs, monomials);
    float maxval = max( outColor.rgb.r, max( outColor.rgb.g, outColor.rgb.b));
    float minval = min( outColor.rgb.r, min( outColor.rgb.g, outColor.rgb.b));
    float f_S = ( max(1e-10, maxval) - max(1e-10, minval) ) / max(1e-2, maxval);
    outColor.rgb.r = outColor.rgb.r + f_H * f_S * (0.0299999993 - outColor.rgb.r) * 0.180000007;
  }

  // Add Range processing

  {
    outColor.rgb = max(float3(0., 0., 0.), outColor.rgb);
  }

  // Add Matrix processing

  {
    float4 res = float4(outColor.rgb.r, outColor.rgb.g, outColor.rgb.b, outColor.a);
    float4 tmp = res;
    res = float4x4(float4(1.4514393161456653, -0.07655377339602043, 0.008316148425697719, 0.), float4(-0.23651074689374019, 1.1762296998335731, -0.0060324497910210278, 0.), float4(-0.21492856925192524, -0.099675926437552201, 0.9977163013653233, 0.), float4(0., 0., 0., 1.)) * tmp;
    outColor.rgb = float3(res.x, res.y, res.z);
    outColor.a = res.w;
  }

  // Add Range processing

  {
    outColor.rgb = max(float3(0., 0., 0.), outColor.rgb);
  }

  // Add Matrix processing

  {
    float4 res = float4(outColor.rgb.r, outColor.rgb.g, outColor.rgb.b, outColor.a);
    float4 tmp = res;
    res = float4x4(float4(0.97088914867099996, 0.010889148671, 0.010889148671, 0.), float4(0.026963270631999998, 0.98696327063199996, 0.026963270631999998, 0.), float4(0.0021475806959999999, 0.0021475806959999999, 0.96214758069600004, 0.), float4(0., 0., 0., 1.)) * tmp;
    outColor.rgb = float3(res.x, res.y, res.z);
    outColor.a = res.w;
  }

  // Add Log processing

  {
    outColor.rgb = max( float3(1.17549435e-38, 1.17549435e-38, 1.17549435e-38), outColor.rgb);
    outColor.rgb = log(outColor.rgb) * float3(0.434294462, 0.434294462, 0.434294462);
  }

  // Add GradingRGBCurve 'log' forward processing

  {
    outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve_0(0, outColor.rgb.r);
    outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve_0(1, outColor.rgb.g);
    outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve_0(2, outColor.rgb.b);
    outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve_0(3, outColor.rgb.r);
    outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve_0(3, outColor.rgb.g);
    outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve_0(3, outColor.rgb.b);
  }

  // Add GradingRGBCurve 'log' forward processing

  {
    outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve_1(0, outColor.rgb.r);
    outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve_1(1, outColor.rgb.g);
    outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve_1(2, outColor.rgb.b);
    outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve_1(3, outColor.rgb.r);
    outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve_1(3, outColor.rgb.g);
    outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve_1(3, outColor.rgb.b);
  }

  // Add Log 'Anti-Log' processing

  {
    outColor.rgb = pow( float3(10., 10., 10.), outColor.rgb);
  }

  // Add Matrix processing

  {
    float4 res = float4(outColor.rgb.r, outColor.rgb.g, outColor.rgb.b, outColor.a);
    res = float4(0.0208420176, 0.0208420176, 0.0208420176, 1.) * res;
    res = float4(-0.000416840339, -0.000416840339, -0.000416840339, 0.) + res;
    outColor.rgb = float3(res.x, res.y, res.z);
    outColor.a = res.w;
  }

  // Add FixedFunction 'ACES_DarkToDim10 (Forward)' processing

  {
    float Y = max( 1e-10, 0.27222871678091454 * outColor.rgb.r + 0.67408176581114831 * outColor.rgb.g + 0.053689517407937051 * outColor.rgb.b );
    float Ypow_over_Y = pow( Y, -0.0188999772);
    outColor.rgb = outColor.rgb * Ypow_over_Y;
  }

  // Add Matrix processing

  {
    float4 res = float4(outColor.rgb.r, outColor.rgb.g, outColor.rgb.b, outColor.a);
    float4 tmp = res;
    res = float4x4(1.604753433346922, -0.10208245810655031, -0.0032671116532946819, 0., -0.531080948604018, 1.1081341286221253, -0.072755424133422703, 0., -0.073672484741910349, -0.0060516705145729488, 1.0760225357877193, 0., 0., 0., 0., 1.) * tmp;
    outColor.rgb = float3(res.x, res.y, res.z);
    outColor.a = res.w;
  }

  // Add Gamma 'monCurveRev' processing

  {
    float4 breakPnt = float4(0.00303993467, 0.00303993467, 0.00303993467, 1.);
    float4 slope = float4(12.9232101, 12.9232101, 12.9232101, 1.);
    float4 scale = float4(1.05499995, 1.05499995, 1.05499995, 1.00000095);
    float4 offset = float4(0.0549999997, 0.0549999997, 0.0549999997, 9.99999997e-07);
    float4 gamma = float4(0.416666657, 0.416666657, 0.416666657, 0.999998987);
    float4 isAboveBreak = float4((outColor[0] > breakPnt[0]) ? 1.0 : 0.0, (outColor[1] > breakPnt[1]) ? 1.0 : 0.0, (outColor[2] > breakPnt[2]) ? 1.0 : 0.0, (outColor[3] > breakPnt[3]) ? 1.0 : 0.0);
    float4 linSeg = outColor * slope;
    float4 powSeg = pow( max( float4(0., 0., 0., 0.), outColor ), gamma ) * scale - offset;
    float4 res = isAboveBreak * powSeg + ( float4(1., 1., 1., 1.) - isAboveBreak ) * linSeg;
    outColor.rgb = float3(res.x, res.y, res.z);
    outColor.a = res.w;
  }

  return outColor;
}

// Close class wrapper


};
float4 OCIOMain(
  float4 inPixel)
{
  return ocioOCIOMain(
  ).OCIOMain(inPixel);
}

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
