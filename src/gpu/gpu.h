#ifndef _gpu_h
#define _gpu_h

#include "image.h"
#include "structs.h"
#include "gpuStructs.h"
#include "ocioProcessor.h"
#include <OpenColorIO/oglapphelpers/glsl.h>

#include <GL/glew.h>
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
    int errorCode;
};

struct histFrame {
    float* imgData = nullptr;
    float* histData = nullptr;
    int imgW = 0;
    int imgH = 0;
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
        void processQueue();

        bool getStatus();
        void clearError();
        std::string getError();

        void getMipMapTexture(image* _img, float*& pixels, int &width, int &height);
        void setHistTexture(float* pixels);
        void histoCheck();

        long long unsigned int histoTex(){return m_histoTex;}

        gpuTimer rdTimer;
        bool rendering = false;
        histFrame histObj;

    private:
        gpuStat status;
        bool m_initialized = false;
        bool enableQueue = false;
        std::thread queueThread;
        std::deque<gpuQueue> renderQueue;
        std::mutex queueLock;
        std::mutex histLock;

        image* prevIm;

        ocioSetting prevOCIO;

        unsigned long long imBufferSize;
        unsigned int m_width = 0;
        unsigned int m_height = 0;



        void initBuffers();

        void checkError(std::string location);

        void bufferCheck(image* _image);
        void updateUniforms(renderParams params);
        void cacheUniformLocations();
        void setupGeometry();
        void loadOCIOTex(ocioSetting ocioSet);

        void allocateAllTextures(OCIO::GpuShaderDescRcPtr &gpuDesc);

        GLuint compileShader(const std::string& source, GLenum type);
        GLuint createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
        bool createShaders(ocioSetting& ocioSet);

        void renderImage(image* _image, ocioSetting ocioSet);
        void renderBlurPass(image* _image);

        void copyToTex(GLuint textureID, int width, int height, float* rgbaData);
        void copyFromTexFull(GLuint textureID, int width, int height, float* rgbaData);


        OCIO::OpenGLBuilderRcPtr ocioBuilder;

        GLuint m_framebuffer = 0;
        GLuint m_inputTexture = 0;
        GLuint m_histoTex = 0;
        //GLuint m_outputTexture = 0;

        GLuint m_vertexArray = 0;
        GLuint m_vertexBuffer = 0;
        GLuint m_indexBuffer = 0;


        GLuint m_shaderProgram = 0;
        GLuint quadVAO = 0;
        GLuint inputTex = 0;
        GLuint outputTex = 0;
        GLuint fbo = 0;

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
                GLint bypass;
                GLint gradeBypass;
        } m_uniforms;
};


#endif
