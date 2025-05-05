#ifndef _metalGPU_h
#define _metalGPU_h

#include <Metal/Metal.hpp>
#include <vector>
#include <chrono>
#include "imageIO.h"

struct gpuTimer {
    float renderTime;
    float fps;
};

class metalGPU {
    public:
        metalGPU();
        ~metalGPU(){};

        void renderImage(image* _image);
        void renderBlurPass(image* _image);
        gpuTimer rdTimer;

    private:


        image* prevIm;

        MTL::Device *m_device;
        MTL::CommandQueue *m_command_queue;

        MTL::Library *default_library;

        MTL::ComputePipelineState *_recursive;
        MTL::ComputePipelineState *_transpose;
        MTL::ComputePipelineState *_baseColor;
        MTL::ComputePipelineState *_mainProcess;

        unsigned long long bufferSize;
        MTL::Buffer *m_src;
        MTL::Buffer *m_dst;

        MTL::Buffer *workingA;
        MTL::Buffer *workingB;

        MTL::Buffer *renderParamBuf;

        void initPipelineState();

        void initBuffers();

        void bufferCheck(unsigned int width, unsigned int height);

        void computeKernels(float strength, float* kernels);

};

#endif
