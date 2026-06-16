#ifndef _gpu_h
#define _gpu_h

#include "image.h"
#include "structs.h"
#include "gpuStructs.h"
#include "ocioProcessor.h"
#include <OpenColorIO/oglapphelpers/glsl.h>

#include <GL/glew.h>
#include <array>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <deque>

#define HISTWIDTH 512
#define HISTHEIGHT 256

struct gpuStat {
    bool error = false;
    std::string errString;
    int errorCode = 0;
};

struct histFrame {
    float* imgData = nullptr;
    float* histData = nullptr;
    int imgW = 0;
    int imgH = 0;
    int64_t bufferSize = 0;
    image* imgPtr = nullptr;
    bool get = false;
    bool set = false;
};

class openglGPU {
    public:
        openglGPU();
        ~openglGPU();

        void addToRender(image* _image, renderType type, ocioSetting ocioSet);
        bool initialize(ocioSetting &ocioSet);
        bool isInQueue(image* _image);
        void removeFromQueue(image* _image);
        void processQueue();
        void clearImBuffer(image* img);
        void clearSmBuffer(image* img);
        void copyFromTexFull(GLuint textureID, int width, int height, float* rgbaData);


        bool getStatus();
        void clearError();
        std::string getError();

        long long unsigned int histoTex(){return m_histoTex;}
        long long unsigned int dispTex(){return m_displayTexture;}

        uint64_t activeBytes(){return histoBytes + activeInputBytes + activeDisplayBytes + cleanActiveBytes;}
        image* dispBufIm(){return m_dispBufIm;}

        bool m_rendering = false;
        const int64_t histoBytes = (HISTWIDTH * HISTHEIGHT * 4 * 4);
        uint64_t activeInputBytes = 0;
        uint64_t activeDisplayBytes = 0;
        uint64_t cleanActiveBytes = 0;

    private:
        gpuStat m_status;
        bool m_initialized = false;
        std::deque<gpuQueue> m_renderQueue;
        std::mutex m_queueLock;

        image* m_prevIm;
        image* m_dispBufIm;

        ocioSetting m_prevOCIO;

        unsigned int m_width = 0;
        unsigned int m_height = 0;

        // Histogram
        uint64_t m_histBufSize = 0;
        float* m_histPixels = nullptr;
        float* m_imgPixels = nullptr;

        // histogram.cpp
        void procHistIm(image* img);
        void updateHistPixels(image* img, float* imgPixels, float* histPixels, int width, int height, float intensityMultiplier);

        // gpu.cpp
        void checkError(std::string location);

        bool bufferCheck(image* _image);
        void updateUniforms(renderParams params);
        void cacheUniformLocations();
        void setupGeometry();

        GLuint compileShader(const std::string& source, GLenum type);
        bool createShaders(ocioSetting& ocioSet);

        void renderImage(image* _image, ocioSetting ocioSet);

        bool copyToTex(GLuint textureID, int width, int height, float* rgbaData);

        void getHistTexture(image* _img, float*& pixels, int &width, int &height);
        void setHistTexture(float* pixels);

    private:
        OCIO::OpenGLBuilderRcPtr m_ocioBuilder;

        GLuint m_framebuffer = 0;
        GLuint m_smallFBO    = 0;
        GLuint m_inputTexture = 0;
        GLuint m_displayTexture = 0;
        GLuint m_cleanOutTex = 0;
        GLuint m_histoTex = 0;

        GLuint m_vertexArray = 0;
        GLuint m_vertexBuffer = 0;
        GLuint m_indexBuffer = 0;
        GLuint m_shaderProgram = 0;


        struct UniformLocations {
                GLint inputTexture;
                GLint baseColor;
                GLint blackPoint;
                GLint whitePoint;
                GLint G_blackpoint;
                GLint G_whitepoint;
                GLint G_lift;
                GLint G_gain;
                GLint G_mult;
                GLint G_offset;
                GLint G_gamma;
                GLint G_temp;
                GLint G_tint;
                GLint G_sat;
                GLint G_matrixR;
                GLint G_matrixG;
                GLint G_matrixB;
                GLint G_sharpen;
                GLint G_sharpenRadius;
                GLint G_curveW;   GLint G_curveW_n;
                GLint G_curveR;   GLint G_curveR_n;
                GLint G_curveG;   GLint G_curveG_n;
                GLint G_curveB;   GLint G_curveB_n;
                GLint bypass;
                GLint gradeBypass;
                GLint secEnable;
                GLint showClip;
                GLint channelView;
                // Crop Variables
                GLint imageCropMin;
                GLint imageCropMax;
                GLint arbitraryRotation;
                GLint cropEnabled;
                GLint cropVisible;
                GLint imageSize;
                GLint proxyPass;
        } m_uniforms;
};


#endif
