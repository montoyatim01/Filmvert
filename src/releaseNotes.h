#ifndef _releasenotes_h
#define _releasenotes_h

#include <string.h>

const std::string relNotes_1_1_2(R"V0G0N(
New Features:
- Added the ability to export greyscale images
- Added a CPU render mode utilizing a Mitchell sampling algorithm
- Saving roll metadata now merges changes instead of overwriting

UI Enhancements:
- Image viewer scale is image agnostic. Different resolution/orientation images now display at the same scale
- (macOS) The dock icon now bounces when trying to close with unsaved changes
- (macOS) The export window now allows folder creation
- The export dialog will be disabled until a path is selected

Bug Fixes:
- Addressed an issue where the histogram wouldn't update with an image visible, but not selected
- Addressed an issue where the copied saturation value wouldn't paste to other images
- (UNIX) Hidden files are now ignored while importing a roll
- Addressed an issue where some images would import with the wrong library resulting in incorrect colors
- Image exports will fail-over to CPU rendering if GPU rendering fails
- Simultaneous image loading follows max simultaneous exports preference number
- Addressed an issue where images may still export cropped with the bake crop option de-selected
- Addressed an issue where in some cases rotating an image left or right would perform the opposite of intended operation

)V0G0N");

const std::string relNotes_1_1_1(R"V0G0N(
UI Enhancements:
- Added release notes popups for new versions
- Added high-dpi window rendering for supported operating systems

Bug Fixes:
- Addressed an issue where exporting would lock up the system
- Addressed an issue where single-channel RAW files would import blank
- Addressed an issue where images would import with the incorrect IDT set
- Addressed an issue where some images would export with incorrect metadata tags
)V0G0N");

const std::string relNotes_1_1_0(R"V0G0N(
New Features:
- Saturation slider
- Flip horizontal/vertical. All image orientations are now accessible
- Image crop, and rotation
- Bake crop/rotation in exports
- Add a border to exported images
- Roll contact sheets (which can be used to save/import full-roll metadata)
- Added ACES 1.3 configuration (for troublesome images)

UI Enhancements:
- Analysis and grade controls are color coded
- Image input/output colorspace settings are hidden by default

Bug Fixes:
- Addressed an issue where images wouldn't import to the selected roll
- Addressed an issue with exporting non-raw images
- Addressed an issue that would allow for setting non-legal preferences

Under The Hood:
- Revamped OCIO backend
- Images store which configuration was used, attempt to re-use the same config/settings on import
- Updated CMake compatibility
- 'Hidden' preferences for various uncommon settings
)V0G0N");

#endif
