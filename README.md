<div align="center">

![logo](assets/logo.png)


# Filmvert

A roll-based film inversion software with a simple repeatable workflow.
</div>
Features:

- Lossless floating point operation

- Highly customizeable, repeatable inversion parameters

- Wide support for input image formats (Camera RAW, Pakon, TIFF/DNG)

- Fully color managed with [OpenColorIO](https://github.com/AcademySoftwareFoundation/OpenColorIO) integration

- Metadata tagging, import and export

![mainApp](assets/MainApp.png)

## ⚡️ Quick start
Download the latest release from the [Releases Page](https://github.com/montoyatim01/Filmvert/releases).

### Loading Images/Rolls
Load in images by selecting a few individual images, or an entire directory's worth at once. Filmvert operates per-roll, so directories will import as rolls. Individual images will need to be added to a roll.

### Analysis
![basicWorkflow](assets/BasicWorkflow.gif)

Set the four corners of the analysis region to encompass the image. Exclude any sprocket holes or any scanning equipment as this will throw off the inversion. The analysis only looks at pixels within the set region, so portions of the frame can be included/excluded to influence the analysis.

Set the base color by holding Cmd + Shift, and clicking and dragging a clear space in the image where the film base is visible.

Click "Analyze"

The engine will process the image, setting the white point and black point based on the lightest and darkest areas of the image. These points will be displayed on screen.

The "Analysis Bias" slider can be adjusted, and the image re-analyzed to tweak the results of the analysis.

### Image Adjustments
Depending upon the results of the analysis, the **Analyzed White Point** and **Analyzed Black Point** can be adjusted to taste.

Further, the **Grade** options can be used to set a quick grade on an image.


## 📖 Project Wiki

For full documentation on all features and workflows, take a look at the [project wiki](https://github.com/montoyatim01/Filmvert/wiki)

## 🚚 Building from source
**Requirements**

- Conan 1.66.0

- Cmake 3.18 or greater

- Python 3

- (Windows) [Git Bash](https://gitforwindows.org/)

Simply execute the build script based on your platform, using either 'Debug' or 'Release' to specify the build type.
```./build.sh Release```

If you get a warning about missing conan profiles, run the command:
```conan profile new default --detect```
to generate default profiles

## ⭐️ Contributing
If there are any features you'd like to see, submit a PR for inclusion into the codebase.

If you like the program, consider buying me a cup of coffee:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/G2G51BYM3L)

## ‼️ Troubleshooting
Feel free to submit a GitHub issue for any recurring problem or bug.

When submitting an issue, please include the following:
- The Filmvert log file.
By default, Filmvert creates a log file at the following location:
- macOS: ```/Users/Shared/Filmvert/Filmvert.log```
- Windows: ```C:/ProgramData/Filmvert/Filmvert.log```
- Linux: ```~/.local/Filmvert/Filmvert.log```

- System specifications (os version, graphics card, driver version, etc).

- Steps to reproduce the issue

- Any applicable crash log (for macOS, click 'Send to Apple' to get the print out of the crash log, copy & paste into a text file).
