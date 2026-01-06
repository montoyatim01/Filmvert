#include <stdint.h>

#include "logger.h"
#include "preferences.h"
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

    // Load preferences
    appPrefs.loadFromFile();

    // Setup Threadpool
    unsigned int numThreads = appPrefs.prefs.maxSimExports;

    if (numThreads == 0 || numThreads > 4096) {
        // It's unlikely anyone has set this high, so cap it back to a sensible number
        // This should also catch the case where std::thread::hardware_concurrency returns
        // bad values too
        LOG_WARN("Threads set to {}, resetting to 2", numThreads);
        numThreads = 2;
    }

    LOG_INFO("Starting thread pool with {} threads", numThreads);
    tPool = new ThreadPool(numThreads);

    // Start Window
    mainWindow window;
    return window.openWindow();

}
