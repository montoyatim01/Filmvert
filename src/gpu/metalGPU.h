#ifndef _metalGPU_h
#define _metalGPU_h

#include <Metal/Metal.hpp>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <deque>
#include <queue>
#include "Metal/MTLSampler.hpp"
#include "Metal/MTLTexture.hpp"
#include "image.h"
#include "structs.h"

struct gpuTimer {
    float renderTime;
    float fps;
};
enum renderType {
    r_sdt = 0,
    r_blr = 1,
    r_full = 2
};
struct gpuQueue {
    image* _img;
    renderType _type;
    ocioSetting _ocioSet;


    gpuQueue(image* img, renderType type, ocioSetting ocioSet)
    {_img = img;
    _type = type;
    _ocioSet = ocioSet;}


};

class metalGPU {
    public:
        metalGPU();
        ~metalGPU();

        void addToRender(image* _image, renderType type, ocioSetting ocioSet);

        bool initOCIOKernels(ocioSetting& ocioSet);
        bool isInQueue(image* _image);
        gpuTimer rdTimer;
        bool rendering = false;

    private:

        bool enableQueue = false;
        std::thread queueThread;
        std::queue<gpuQueue> renderQueue;
        std::mutex queueLock;

        image* prevIm;
        ocioSetting prevOCIO;
        std::string ocioKernelText1;
        std::string ocioKernelText2;
        std::string ocioKernelText2a;
        std::string ocioKernelText3;
        std::string ocioKernelText3a;

        MTL::Device *m_device;
        MTL::CommandQueue *m_command_queue;

        MTL::Library *default_library;

        MTL::ComputePipelineState *_recursive;
        MTL::ComputePipelineState *_transpose;
        MTL::ComputePipelineState *_baseColor;
        MTL::ComputePipelineState *_mainProcess;
        MTL::ComputePipelineState *_ocioProcess;
        MTL::ComputePipelineState *_ocioProcessFull;

        unsigned long long bufferSize;
        MTL::Buffer *m_src;
        MTL::Buffer *m_dst;
        MTL::Buffer *m_disp;

        MTL::Buffer *workingA;
        MTL::Buffer *workingB;

        MTL::Buffer *renderParamBuf;

        // OCIO
        MTL::Texture *ocioTex;
        MTL::SamplerState *ocioSample;

        void initPipelineState();



        void initBuffers();

        void processQueue();

        void bufferCheck(unsigned int width, unsigned int height);

        void loadOCIOTex(ocioSetting ocioSet);

        void computeKernels(float strength, float* kernels);

        void renderImage(image* _image, ocioSetting ocioSet);
        void renderBlurPass(image* _image);

};

#endif
