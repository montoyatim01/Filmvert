#include <stdint.h>

#include "logger.h"
#include "metalGPU.h"
#include "window.h"

int main(void)
{
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", 1, 0, 0);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Initialize Metal/CUDA Subsystem
    metalGPU metalSubsystem;

    // Start Window
    mainWindow window;
    window.setGPU(&metalSubsystem);
    return window.openWindow();

}


// TODO/Wishlist:
//
// - Preferences/Settings
// -- Undo levels? Default 100?
//
// - Hotkeys
// -* Copy/Paste meta **
// -* Copy/Paste params **
// -* Save state
// -* Save roll - Add Roll JSON
// -* Save all - Add Roll JSON
// -* Rot left
// -* Rot right
// -* Local Meta edit? (cmd + e?)
// -* Global Meta edit? (cmd + g)
// -- Printer lights? Other adjustments
// -* Make a popup with key shortcuts displayed
//
// - Bugs
// -- Add in import from image? (import the metadata or inversion settings)
// --Analysis doesn't work very well (turned up blur)
// --- Cap limits on white point?
// --Histogram to follow crop points?
// --Which images are unsaved? Have a colored dot?

// -- Undo/Redo
// -- How does an undo operation on many images affect thumbnails?
// -- Render current image and then call a state-specific
// ---render function?
// -- Use the differences between states to determine if render needed.
// -- Deal with copy + paste and updating structs (paramchange)
// -- Fix off by one errors for both ends of the state
// -- Add an initial state
//
// -- Bug when closing multiple rolls (maybe roll is loading?)
// -- Get arrow keys to always switch images?
// -- Hotkeys for BP/WP RGB
// -- Sort by name/ripple frame number?
//
// -- Priority in the render queue
// -- User-initiated operations go in the front
// -- Finish the auto-sort options (nix it?)

/*
Updates

- Grade happens in JPLog2 now
- White balance/tint are more intense
- Render queue supports rough priority (front or back)
- Analysis blur default is 10.0 now (produces somewhat better results)
- Organized filmRoll code
- Ability to sort roll based on frame number/filename
- Modified the color controls for finer tune (with shift) control
- Added the ability to ripple frame changes
- Image changes now appear as a purple dot next to the filename
- Histogram now only calculates based on the crop box
- Added the ability to undo/redo changes
-- Roll-based storage allowing multi-image operations to be undone/redone
- Added app icon

- Fixed base color coordinate calculation
- Fixed save roll/save all hotkeys
- Fixed metadata editing not flagging image state change


*/
