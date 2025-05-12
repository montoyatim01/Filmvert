#ifndef _metalGPU_h
#define _metalGPU_h

#include <Metal/Metal.hpp>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <deque>
#include <queue>
#include "imageIO.h"

struct gpuTimer {
    float renderTime;
    float fps;
};
enum renderType {
    r_sdt = 0,
    r_blr = 1
};
struct gpuQueue {
    image* _img;
    renderType _type;
    gpuQueue(image* img, renderType type)
    {_img = img;
    _type = type;}


};

class metalGPU {
    public:
        metalGPU();
        ~metalGPU();

        void addToRender(image* _image, renderType type);
        void renderBlurPass(image* _image);
        bool initOCIOKernels(int csSetting, int csDisp, int view);
        bool isInQueue(image* _image);
        gpuTimer rdTimer;
        bool rendering = false;

    private:

        bool enableQueue = false;
        std::thread queueThread;
        std::queue<gpuQueue> renderQueue;
        std::mutex queueLock;

        image* prevIm;
        int prevCSOpt;
        int prevDisp;
        int prevView;
        std::string ocioKernelText1;
        std::string ocioKernelText2;
        std::string ocioKernelText3;

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

        void initPipelineState();



        void initBuffers();

        void processQueue();

        void bufferCheck(unsigned int width, unsigned int height);

        void computeKernels(float strength, float* kernels);

        void renderImage(image* _image);

};

#endif
