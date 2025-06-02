#ifndef _windowhistogram_h
#define _windowhistogram_h

#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>

#include "image.h"
#include "gpu.h"


class winHistogram {
    public:
    winHistogram(){};
    ~winHistogram(){};

    void startHistogram();
    void stopHistogram();

    void processImage(image* _img, openglGPU* _gpu);

private:
    std::thread histThread;
    std::mutex mtx;
    std::condition_variable cv;
    bool procHist = false;
    bool histStop = true;

    image* _imgProc;
    openglGPU* _gpuProc;

    void workThread();
    void updateHistogram();
    void updateHistPixels(image* img, float* imgPixels, float* histPixels, int width, int height, float intensityMultiplier);
};

#endif
