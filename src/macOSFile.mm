#include <vector>
#include <string>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>

std::vector<std::string> ShowFileOpenDialog(bool allowMultiple = true, bool canChooseDirectories = false) {
    std::vector<std::string> result;

    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setAllowsMultipleSelection:allowMultiple];
        [panel setCanChooseDirectories:canChooseDirectories];
        [panel setCanCreateDirectories:NO];
        [panel setResolvesAliases:YES];
        [panel setTitle:@"Select File(s)"];
        [panel setPrompt:@"Choose"];

        NSInteger response = [panel runModal];
        if (response == NSModalResponseOK) {
            for (NSURL *url in [panel URLs]) {
                result.emplace_back([[url path] UTF8String]);
            }
        }
    }

    return result;
}

std::vector<std::string> ShowFolderSelectionDialog(bool allowMultiple = true) {
    std::vector<std::string> result;

    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:NO];
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:allowMultiple];
        [panel setCanCreateDirectories:NO];
        [panel setResolvesAliases:YES];
        [panel setTitle:@"Select Folder(s)"];
        [panel setPrompt:@"Choose"];

        NSInteger response = [panel runModal];
        if (response == NSModalResponseOK) {
            for (NSURL *url in [panel URLs]) {
                result.emplace_back([[url path] UTF8String]);
            }
        }
    }

    return result;
}

void setMacOSWindowModified(GLFWwindow* window, bool modified) {
    NSWindow* nswindow = glfwGetCocoaWindow(window);
    if (nswindow) {
        [nswindow setDocumentEdited:modified ? YES : NO];
    }
}
