#include <stdint.h>
#include <thread>

#include "logger.h"
#include "preferences.h"
#include "window.h"

#if defined (WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(void)
#endif
{
    printf("Starting Filmvert\n");
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", VERMAJOR, VERMINOR, VERPATCH);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Load preferences
    appPrefs.loadFromFile();

    // Setup Threadpool
    int numThreadsI = appPrefs.prefs.maxSimExports;
    if (numThreadsI < 1) {
        numThreadsI = std::thread::hardware_concurrency();
        numThreadsI = numThreadsI < 1 ? 2 : numThreadsI;
    }
    unsigned int numThreads = numThreadsI;
    LOG_INFO("Starting thread pool with {} threads", numThreads);
    tPool = new ThreadPool(numThreads);

    // Start Window
    mainWindow window;
    return window.openWindow();

}
