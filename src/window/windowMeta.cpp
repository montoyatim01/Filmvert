#include "window.h"

void mainWindow::checkMeta() {
    if (!metaRefresh)
        return;
    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastChange);
    if (dur.count() > 5) {
        for (int r = 0; r < activeRolls.size(); r++) {
            for (int i = 0; i < activeRolls[r].images.size(); i++) {
                image* img = getImage(r, i);
                if (!img) {
                   LOG_WARN("Cannot update meta on nullptr");
                }

                if (img->needMetaWrite) {
                    img->writeXMPFile();
                }
            }
        }
        metaRefresh = false;
    }
    return;
}
