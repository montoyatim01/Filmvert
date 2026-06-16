#ifndef _glslkernels_h
#define _glslkernels_h
#include <string>

// --- GL Vertex Shader --- ///
const std::string glsl_vertex(R"V0G0N(
    #version 330 core

    // Input vertex attributes
    layout (location = 0) in vec3 position;   // Vertex position
    layout (location = 1) in vec2 uv;         // Texture coordinates

    // Output to fragment shader
    out vec2 texCoord;

    void main()
    {
        texCoord = uv;
        gl_Position = vec4(position, 1.0);
    }
)V0G0N");

// --- Full kernel --- //
const std::string glsl_process(R"V0G0N(

    // Input from vertex shader
    in vec2 texCoord;

    // Uniforms
    uniform sampler2D inputTexture;     // Input image (imgIn)
    uniform vec4 baseColor;             // Base color RGB values
    uniform vec4 blackPoint;
    uniform vec4 whitePoint;
    uniform vec4 G_blackpoint;
    uniform vec4 G_whitepoint;
    uniform vec4 G_lift;
    uniform vec4 G_gain;
    uniform vec4 G_mult;
    uniform vec4 G_offset;
    uniform vec4 G_gamma;
    uniform vec3 G_matrixR;
    uniform vec3 G_matrixG;
    uniform vec3 G_matrixB;
    uniform float G_temp;
    uniform float G_tint;
    uniform float G_sat;
    uniform float G_sharpen;
    uniform float G_sharpenRadius;
    uniform vec2  G_curveW[16]; // Master/luminance curve
    uniform int   G_curveW_n;
    uniform vec2  G_curveR[16]; // Red channel curve
    uniform int   G_curveR_n;
    uniform vec2  G_curveG[16]; // Green channel curve
    uniform int   G_curveG_n;
    uniform vec2  G_curveB[16]; // Blue channel curve
    uniform int   G_curveB_n;
    uniform int bypass;
    uniform int gradeBypass;
    uniform int secEnable; // 1 = wb, 2 = tone, 4 = curves, 8 = matrix
    uniform int showClip;
    uniform int channelView;
    // For Cropping
    uniform vec2 imageCropMin;  // Top-left corner of crop rectangle
    uniform vec2 imageCropMax;  // Bottom-right corner of crop rectangle
    uniform float arbitraryRotation;  // Rotation in radians
    uniform bool cropEnabled;  // Whether to apply crop
    uniform bool cropVisible;  // Whether to apply crop
    uniform vec2 imageSize;  // Original image dimensions
    uniform int proxyPass;

    // Output
    //out vec4 fragColor;
    //out vec4 fragColorSm;

    layout(location = 0) out vec4 fragColor;
    //layout(location = 1) out vec4 fragColorSm;

    // Calculate UV coordinates for cropped and rotated region
    vec2 getCroppedRotatedUV(vec2 uv) {
        // Apply rotation if either crop is enabled OR crop is visible
        // This allows rotation to be visible during crop adjustment
        if (!cropEnabled && !cropVisible) {
            return uv;
        }

        // Get crop rectangle dimensions and center
        vec2 cropSize = imageCropMax - imageCropMin;

        // If crop is enabled, map output UV to crop region
        vec2 workingUV = uv;
        if (cropEnabled) {
            // Map output UV (0-1) to crop rectangle coordinates
            workingUV = imageCropMin + cropSize * uv;
        }

        // Apply rotation around image center (0.5, 0.5)
        vec2 imageCenter = vec2(0.5, 0.5);

        // Convert to centered coordinates for rotation
        vec2 centeredUV = workingUV - imageCenter;

        // Calculate aspect ratio and apply it to maintain proper proportions
        float aspectRatio = imageSize.x / imageSize.y;

        // Scale by aspect ratio before rotation to work in square coordinates
        centeredUV.x *= aspectRatio;

        // Apply inverse rotation (positive angle because we're going backwards)
        float cosR = cos(arbitraryRotation);
        float sinR = sin(arbitraryRotation);
        vec2 rotatedUV = vec2(
            centeredUV.x * cosR - centeredUV.y * sinR,
            centeredUV.x * sinR + centeredUV.y * cosR
        );

        // Scale back by inverse aspect ratio after rotation
        rotatedUV.x /= aspectRatio;

        // Convert back to texture coordinates
        vec2 finalUV = rotatedUV + imageCenter;

        return finalUV;
    }

    //---LUMA---//
    float luma(vec4 inputPixel)
    {
        return inputPixel.x * 0.2722287168f + inputPixel.y * 0.6740817658f + inputPixel.z * 0.0536895174f;
    }

    //---LOG---//
    vec4 JPLogtoLin(vec4 inputPixel)
    {
        vec4 outPixel;

        const float ALOGSM1_LIN_BRKPNT = 0.006801176276;
        const float ALOGSM1_LOG_BRKPNT = 0.16129032258064516129;
        const float ALOGSM1_LINTOLOG_SLOPE = 10.36773919972907075549;
        const float ALOGSM1_LINTOLOG_YINT = 0.09077750069969257965;

        outPixel.x = inputPixel.x <= ALOGSM1_LOG_BRKPNT ? (inputPixel.x - ALOGSM1_LINTOLOG_YINT) / ALOGSM1_LINTOLOG_SLOPE : pow(2.0, inputPixel.x * 20.46 - 10.5);
        outPixel.y = inputPixel.y <= ALOGSM1_LOG_BRKPNT ? (inputPixel.y - ALOGSM1_LINTOLOG_YINT) / ALOGSM1_LINTOLOG_SLOPE : pow(2.0, inputPixel.y * 20.46 - 10.5);
        outPixel.z = inputPixel.z <= ALOGSM1_LOG_BRKPNT ? (inputPixel.z - ALOGSM1_LINTOLOG_YINT) / ALOGSM1_LINTOLOG_SLOPE : pow(2.0, inputPixel.z * 20.46 - 10.5);

        return outPixel;
    }

    vec4 LintoJPLog(vec4 inputPixel)
    {
        vec4 outPixel;

        const float ALOGSM1_LIN_BRKPNT = 0.006801176276;
        const float ALOGSM1_LOG_BRKPNT = 0.16129032258064516129;
        const float ALOGSM1_LINTOLOG_SLOPE = 10.36773919972907075549;
        const float ALOGSM1_LINTOLOG_YINT = 0.09077750069969257965;

        outPixel.x = inputPixel.x <= ALOGSM1_LIN_BRKPNT ? ALOGSM1_LINTOLOG_SLOPE * inputPixel.x + ALOGSM1_LINTOLOG_YINT : (log(inputPixel.x)/log(2.0) + 10.5) / 20.46;
        outPixel.y = inputPixel.y <= ALOGSM1_LIN_BRKPNT ? ALOGSM1_LINTOLOG_SLOPE * inputPixel.y + ALOGSM1_LINTOLOG_YINT : (log(inputPixel.y)/log(2.0) + 10.5) / 20.46;
        outPixel.z = inputPixel.z <= ALOGSM1_LIN_BRKPNT ? ALOGSM1_LINTOLOG_SLOPE * inputPixel.z + ALOGSM1_LINTOLOG_YINT : (log(inputPixel.z)/log(2.0) + 10.5) / 20.46;

        return outPixel;
    }


    // --- Tone Curve Evaluation -----------------------------------------------
    // Monotone cubic Hermite spline through n control points (2 … 16).
    // Each point is (input_x, output_y) with both axes in [0, 1].
    // Tangents use the non-uniform Catmull-Rom formula, then clamped via
    // the Fritsch-Carlson condition so the curve never overshoots.
    float evalCurve(vec2 pts[16], int n, float v) {
        // --- Linear extrapolation below black point ---
        if (v < pts[0].x) {
            float dx0 = max(pts[1].x - pts[0].x, 0.0001);
            float s0  = (pts[1].y - pts[0].y) / dx0;
            return pts[0].y + s0 * (v - pts[0].x);
        }

        // --- Linear extrapolation above white point ---
        if (v > pts[n - 1].x) {
            float dxN = max(pts[n-1].x - pts[n-2].x, 0.0001);
            float sN  = (pts[n-1].y - pts[n-2].y) / dxN;
            return pts[n-1].y + sN * (v - pts[n-1].x);
        }

        // Find the segment [seg, seg+1] that contains v
        int seg = n - 2;
        for (int i = 0; i < n - 1; i++) {
            if (v < pts[i + 1].x) { seg = i; break; }
        }
        int pprev = max(seg - 1, 0);
        int pnext = min(seg + 2, n - 1);

        vec2 pa = pts[seg],    pb = pts[seg + 1];
        vec2 pp = pts[pprev],  pn = pts[pnext];

        float dx = pb.x - pa.x;
        if (dx < 0.0001) return pa.y;

        // Non-uniform Catmull-Rom slopes (in output/input units)
        float dxPrev = max(pb.x - pp.x, 0.0001);
        float dxNext = max(pn.x - pa.x, 0.0001);
        float m0 = (pb.y - pp.y) / dxPrev;
        float m1 = (pn.y - pa.y) / dxNext;

        // Fritsch-Carlson monotonicity: clamp slope ratio to [-3, 3]
        float delta = (pb.y - pa.y) / dx;
        if (abs(delta) < 0.0001) {
            m0 = 0.0; m1 = 0.0;
        } else {
            m0 = clamp(m0 / delta, -3.0, 3.0) * delta;
            m1 = clamp(m1 / delta, -3.0, 3.0) * delta;
        }

        // Cubic Hermite basis
        float u  = (v - pa.x) / dx;
        float u2 = u  * u;
        float u3 = u2 * u;
        float h00 =  2.0*u3 - 3.0*u2 + 1.0;
        float h10 =      u3 - 2.0*u2 + u;
        float h01 = -2.0*u3 + 3.0*u2;
        float h11 =      u3 -     u2;

        return h00*pa.y + h10*dx*m0 + h01*pb.y + h11*dx*m1;
    }

    vec4 imgProcess(vec4 inputPixel) {
        vec4 pixIn = inputPixel;

        // Base Color Inversion
        vec4 pixOut = (baseColor / max(pixIn, vec4(0.0001))) * 0.1;
        pixOut.a = 1.0;

        // Set White/Black Points
        pixOut = (pixOut - blackPoint) / (whitePoint - blackPoint);

        // Temp/Tint Values
        vec4 tempPix = pixOut;
        vec4 warm = vec4(2.0, 1.0, 0.0, 1.0);
        vec4 cool = vec4(0.0, 1.0, 2.0, 1.0);
        vec4 green = vec4(0.0, 1.5, 0.0, 1.0);
        vec4 mag = vec4(1.5, 0.0, 1.5, 1.0);
        float temp = (-1.0 * G_temp);
        float tint = (0.75 * G_tint);

        // WB & Tint
        if ((secEnable & 1) != 0) {
            // Temp
            tempPix = temp >= 0.0 ?
                (tempPix * cool * temp) + ((1.0 - temp) * tempPix) :
                (tempPix * warm * (-1.0 * temp)) + ((1.0 - (-1.0 * temp)) * tempPix);
            // Tint
            tempPix = tint >= 0.0 ?
                (tempPix * mag * tint) + ((1.0 - tint) * tempPix) :
                (tempPix * green * (-1.0 * tint)) + ((1.0 - (-1.0 * tint)) * tempPix);
        }


        // Process grade bp/wp in linear
        if ((secEnable & 2) != 0)
            tempPix = (tempPix - G_blackpoint) / (G_whitepoint - G_blackpoint);

        // Color Matrix
        vec4 inPix = tempPix;
        if ((secEnable & 8) != 0) {
            tempPix.x = (inPix.x * G_matrixR.x + inPix.y * G_matrixR.y + inPix.z * G_matrixR.z);
            tempPix.y = (inPix.x * G_matrixG.x + inPix.y * G_matrixG.y + inPix.z * G_matrixG.z);
            tempPix.z = (inPix.x * G_matrixB.x + inPix.y * G_matrixB.y + inPix.z * G_matrixB.z);
        }




        // Grade node operation
        if ((secEnable & 2) != 0) {
            // Lin to Log for grading
            tempPix = LintoJPLog(tempPix);

            vec4 aGrade = G_mult * (G_gain - G_lift) / (1.0 - 0.0);
            vec4 bGrade = G_offset + G_lift - aGrade * 0.0;
            vec4 powBase = aGrade * tempPix + bGrade;
            powBase = max(powBase, vec4(0.0001));
            tempPix = pow(powBase, 1.0/G_gamma);
            tempPix = clamp(tempPix, 0.0, 100.0);

            // Back to lin for output
            tempPix = JPLogtoLin(tempPix);
        }



        // Perform saturation operation
        float lumaPix = luma(tempPix);
        if ((secEnable & 1) != 0) {
            tempPix = lumaPix + (G_sat + 1.0) * (tempPix - lumaPix);
        }

        return (bypass == 1 ? pixIn : gradeBypass == 1 ? pixOut : tempPix);
    }

    void main()
    {
        vec2 sampleUV = texCoord;
        if (cropEnabled || cropVisible) {
            sampleUV = getCroppedRotatedUV(texCoord);

            // Check if we're outside the valid texture bounds
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
                sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                // Output black for areas outside the image
                fragColor = vec4(0.0, 0.0, 0.0, 1.0);
                //fragColorSm = fragColor;
                return;
            }
        }

        vec4 inputPixel = texture(inputTexture, sampleUV);
        vec4 gradedPixel = imgProcess(inputPixel);
        gradedPixel.w = 1.0f;
        vec4 outPixel = OCIOFUNC(gradedPixel);
        vec4 ODTPixel = outPixel;

        // CURVES - Do in final ODT space for better feel
        if ((secEnable & 4) != 0) {
            // RGB Curves
            outPixel.x = evalCurve(G_curveR, G_curveR_n, outPixel.x);
            outPixel.y = evalCurve(G_curveG, G_curveG_n, outPixel.y);
            outPixel.z = evalCurve(G_curveB, G_curveB_n, outPixel.z);
            // Master (W/luminance) curve applied after per-channel RGB
            outPixel.x = evalCurve(G_curveW, G_curveW_n, outPixel.x);
            outPixel.y = evalCurve(G_curveW, G_curveW_n, outPixel.y);
            outPixel.z = evalCurve(G_curveW, G_curveW_n, outPixel.z);
        }
        outPixel = bypass == 1 || gradeBypass == 1 ? ODTPixel : outPixel;

        //fragColorSm = outPixel;

        vec4 soloColor = outPixel;

        float solo = channelView == 1 ? outPixel.x : channelView == 2 ? outPixel.y : outPixel.z;
        soloColor.x = channelView == 0 ? outPixel.x : solo;
        soloColor.y = channelView == 0 ? outPixel.y : solo;
        soloColor.z = channelView == 0 ? outPixel.z : solo;

        // Clipping
        if (showClip == 1) {
            if (soloColor.x > 0.999 || soloColor.y > 0.999 || soloColor.z > 0.999) {
                if (soloColor.x < 0.001 || soloColor.y < 0.001 || soloColor.z < 0.001)
                    soloColor = vec4(1.0, 0.0, 1.0, 1.0);
                else
                    soloColor = vec4(1.0, 0.0, 0.0, 1.0);
            }

            if (soloColor.x < 0.001 || soloColor.y < 0.001 || soloColor.z < 0.001){
                if (soloColor.x > 0.999 || soloColor.y > 0.999 || soloColor.z > 0.999)
                    soloColor = vec4(1.0, 0.0, 1.0, 1.0);
                else
                    soloColor = vec4(0.0, 0.0, 1.0, 1.0);
            }

        }

        if (proxyPass == 1)
            fragColor = outPixel;
        else
            fragColor = soloColor;

    }
)V0G0N");
#endif
