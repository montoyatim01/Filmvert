#include <stdint.h>

#include "logger.h"
#include "metalGPU.h"
#include "window.h"

int main(void)
{
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", 3, 1, 0);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Initialize Metal/CUDA Subsystem
    metalGPU metalSubsystem;

    // Start Window
    mainWindow window;
    window.setGPU(&metalSubsystem);
    return window.openWindow();

}


// TODO/Wishlist:
// - Interface
// -- Allow pane resizing?
//
// - ImageIO
// -- Reincorporate OIIO for non-raw formats
// -- Single import image function, raw first?
// --- How to handle when image has preview file (dng)
// -- Import Pakon RAW files (Colorspace?)
//
// - Export Window:
// -- Single/Multi-export
// -- Roll export
// -- Verify metadata
//
// - Colorspaces
// -- OCIO Colorspaces in Export
// -- Regular spaces as well (ADX10)?
// -- Handling custom OCIO Configs?
//
// - Preferences/Settings
// -- Auto-save/frequency
// -- OCIO Config setting
// -- Default meta options (cameras, lenses)
//
// - Menu Bar
// -- Roll based settings
// --- Roll Analyze
// --- Roll Metadata
// --- Roll export
// -- Saving
// --- Save one
// --- Save all
// -- Metadata
// --- Edit Roll Meta
// --- Export Roll Meta
//
// - Hotkeys
// -- Copy/Paste meta **
// -- Copy/Paste params **
// -* Save state
// -* Save roll - Add Roll JSON
// -- Save all - Add Roll JSON
// -* Rot left
// -* Rot right
// -- Local Meta edit? (cmd + e?)
// -- Global Meta edit? (cmd + g)
// -- Printer lights? Other adjustments
//
// - Metadata
// -- Roll-based metadata
// -- Local image metadata
// -- Cache of common options (user-pref)
// -- Button in imParam for edit/export
