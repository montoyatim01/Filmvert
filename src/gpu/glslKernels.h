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
    uniform float G_temp;
    uniform float G_tint;
    uniform float G_sat;
    uniform int bypass;
    uniform int gradeBypass;
    // For Cropping
    uniform vec2 imageCropMin;  // Top-left corner of crop rectangle
    uniform vec2 imageCropMax;  // Bottom-right corner of crop rectangle
    uniform float arbitraryRotation;  // Rotation in radians
    uniform bool cropEnabled;  // Whether to apply crop
    uniform bool cropVisible;  // Whether to apply crop
    uniform vec2 imageSize;  // Original image dimensions

    // Output
    out vec4 fragColor;
    out vec4 fragColorSm;

    // Calculate UV coordinates for cropped and rotated region
    vec2 getCroppedRotatedUV(vec2 uv) {
        // Apply rotation if either crop is enabled OR crop is visible
        // This allows rotation to be visible during crop adjustment
        if (!cropEnabled && !cropVisible) {
            return uv;
        }

        // Get crop rectangle dimensions and center
        vec2 cropSize = imageCropMax - imageCropMin;
        vec2 cropCenter = (imageCropMin + imageCropMax) * 0.5;

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
        return inputPixel.x * 0.3439664498 + inputPixel.y * 0.7281660966 + inputPixel.z * -0.0721325464;
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

    // Convert your main() to a processing function
    vec4 imgProcess(vec4 inputPixel) {
        vec4 pixIn = inputPixel;

        // Base Color
        vec4 pixOut = (baseColor / max(pixIn, vec4(0.0001))) * 0.1;
        pixOut.a = 1.0;

        // Set White/Black Points
        pixOut = (pixOut - blackPoint) / (whitePoint - blackPoint);

        // Temp/Tint
        vec4 tempPix = pixOut;
        vec4 warm = vec4(2.0, 1.0, 0.0, 1.0);
        vec4 cool = vec4(0.0, 1.0, 2.0, 1.0);
        vec4 green = vec4(0.0, 1.5, 0.0, 1.0);
        vec4 mag = vec4(1.5, 0.0, 1.5, 1.0);
        float temp = (-1.0 * G_temp);
        float tint = (0.75 * G_tint);

        // WB
        tempPix = temp >= 0.0 ?
            (tempPix * cool * temp) + ((1.0 - temp) * tempPix) :
            (tempPix * warm * (-1.0 * temp)) + ((1.0 - (-1.0 * temp)) * tempPix);
        // Tint
        tempPix = tint >= 0.0 ?
            (tempPix * mag * tint) + ((1.0 - tint) * tempPix) :
            (tempPix * green * (-1.0 * tint)) + ((1.0 - (-1.0 * tint)) * tempPix);

        // Process grade bp/wp in linear
        tempPix = (tempPix - G_blackpoint) / (G_whitepoint - G_blackpoint);

        // Lin to Log for grading
        tempPix = LintoJPLog(tempPix);

        // Grade node operation
        vec4 aGrade = G_mult * (G_gain - G_lift) / (1.0 - 0.0);
        vec4 bGrade = G_offset + G_lift - aGrade * 0.0;
        vec4 powBase = aGrade * tempPix + bGrade;
        powBase = max(powBase, vec4(0.0001));
        tempPix = pow(powBase, 1.0/G_gamma);
        tempPix = clamp(tempPix, 0.0, 100.0);

        // Back to lin for output
        tempPix = JPLogtoLin(tempPix);

        // Perform saturation operation
        float lumaPix = luma(tempPix);
        tempPix = lumaPix + (G_sat + 1.0) * (tempPix - lumaPix);

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
                fragColorSm = fragColor;
                return;
            }
        }

        vec4 inputPixel = texture(inputTexture, sampleUV);
        vec4 gradedPixel = imgProcess(inputPixel);
        gradedPixel.w = 1.0f;
        fragColor = OCIOFUNC(gradedPixel);
        fragColorSm = fragColor;
    }
)V0G0N");
#endif
