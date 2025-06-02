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

// --- Base Color Process Only --- //
const std::string glsl_baseColor(R"V0G0N(
    #version 330 core

    // Input from vertex shader
    in vec2 texCoord;

    // Uniforms
    uniform sampler2D inputTexture;     // Input image (imgIn)
    uniform vec3 baseColor;             // Base color RGB values (renderParams->baseColor)

    // Output
    out vec4 fragColor;

    void main()
    {
        // Sample input pixel
        vec4 pixIn = texture(inputTexture, texCoord);

        // Process pixel (equivalent to Metal kernel logic)
        vec4 pixOut;
        pixOut.x = (baseColor.x / pixIn.x) * 0.1;
        pixOut.y = (baseColor.y / pixIn.y) * 0.1;
        pixOut.z = (baseColor.z / pixIn.z) * 0.1;
        pixOut.w = 1.0;

        // Output processed pixel
        fragColor = pixOut;
    }
)V0G0N");

// --- Base Color Process Only --- //
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
    uniform int bypass;
    uniform int gradeBypass;

    // Output
    out vec4 fragColor;

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

        return (bypass == 1 ? pixIn : gradeBypass == 1 ? pixOut : tempPix);
    }

    void main()
    {
        vec4 inputPixel = texture(inputTexture, texCoord);
        vec4 gradedPixel = imgProcess(inputPixel);
        gradedPixel.w = 1.0;
        fragColor = OCIOFUNC(gradedPixel);
    }
)V0G0N");
#endif
