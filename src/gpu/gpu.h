#ifndef _gpu_h
#define _gpu_h

#include "image.h"
#include "structs.h"
#include "gpuStructs.h"
#include "ocioProcessor.h"
#include <OpenColorIO/oglapphelpers/glsl.h>

#include <GL/glew.h>
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

        bool m_rendering = false;

    private:
        gpuStat m_status;
        bool m_initialized = false;
        std::deque<gpuQueue> m_renderQueue;
        std::mutex m_queueLock;

        image* m_prevIm;

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

        void bufferCheck(image* _image);
        void updateUniforms(renderParams params);
        void cacheUniformLocations();
        void setupGeometry();

        GLuint compileShader(const std::string& source, GLenum type);
        bool createShaders(ocioSetting& ocioSet);

        void renderImage(image* _image, ocioSetting ocioSet);

        bool copyToTex(GLuint textureID, int width, int height, float* rgbaData);

        void getHistTexture(image* _img, float*& pixels, int &width, int &height);
        void setHistTexture(float* pixels);

        void tetherTexture(image* _image);

    private:
        OCIO::OpenGLBuilderRcPtr m_ocioBuilder;

        GLuint m_framebuffer = 0;
        GLuint m_inputTexture = 0;
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
                GLint bypass;
                GLint gradeBypass;
                // Crop Variables
                GLint imageCropMin;
                GLint imageCropMax;
                GLint arbitraryRotation;
                GLint cropEnabled;
                GLint cropVisible;
                GLint imageSize;
        } m_uniforms;
};


#endif
