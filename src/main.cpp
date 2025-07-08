#include <stdint.h>

#include "logger.h"
#include "window.h"

#if defined (WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(void)
#endif
{
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", VERMAJOR, VERMINOR, VERPATCH);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Setup Threadpool
    unsigned int numThreads = std::thread::hardware_concurrency();
    LOG_INFO("Starting thread pool with {} threads", numThreads);
    tPool = new ThreadPool(numThreads);
    // Initialize Metal/CUDA Subsystem
    //metalGPU metalSubsystem;

    // Start Window
    mainWindow window;
    //window.setGPU(&metalSubsystem);
    return window.openWindow();

}
