#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <vector>
#include <string>

std::vector<std::string> ShowFileOpenDialog(bool allowMultiple = true, bool canChooseDirectories = false) {
    std::vector<std::string> result;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog *pFileOpen;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                              IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr)) {
            DWORD dwFlags;
            pFileOpen->GetOptions(&dwFlags);

            if (allowMultiple) {
                dwFlags |= FOS_ALLOWMULTISELECT;
            }
            if (canChooseDirectories) {
                dwFlags |= FOS_PICKFOLDERS;
            }

            pFileOpen->SetOptions(dwFlags);
            pFileOpen->SetTitle(L"Select File(s)");

            hr = pFileOpen->Show(NULL);
            if (SUCCEEDED(hr)) {
                if (allowMultiple) {
                    IShellItemArray *pItems;
                    hr = pFileOpen->GetResults(&pItems);
                    if (SUCCEEDED(hr)) {
                        DWORD dwNumItems;
                        pItems->GetCount(&dwNumItems);

                        for (DWORD i = 0; i < dwNumItems; i++) {
                            IShellItem *pItem;
                            hr = pItems->GetItemAt(i, &pItem);
                            if (SUCCEEDED(hr)) {
                                PWSTR pszFilePath;
                                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                if (SUCCEEDED(hr)) {
                                    // Convert wide string to narrow string
                                    int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                                    std::string path(size - 1, 0);
                                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &path[0], size, NULL, NULL);
                                    result.push_back(path);
                                    CoTaskMemFree(pszFilePath);
                                }
                                pItem->Release();
                            }
                        }
                        pItems->Release();
                    }
                } else {
                    IShellItem *pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr)) {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                        if (SUCCEEDED(hr)) {
                            int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                            std::string path(size - 1, 0);
                            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &path[0], size, NULL, NULL);
                            result.push_back(path);
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }

    return result;
}

std::vector<std::string> ShowFolderSelectionDialog(bool allowMultiple = true) {
    std::vector<std::string> result;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog *pFileOpen;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                              IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr)) {
            DWORD dwFlags;
            pFileOpen->GetOptions(&dwFlags);
            dwFlags |= FOS_PICKFOLDERS;

            if (allowMultiple) {
                dwFlags |= FOS_ALLOWMULTISELECT;
            }

            pFileOpen->SetOptions(dwFlags);
            pFileOpen->SetTitle(L"Select Folder(s)");

            hr = pFileOpen->Show(NULL);
            if (SUCCEEDED(hr)) {
                if (allowMultiple) {
                    IShellItemArray *pItems;
                    hr = pFileOpen->GetResults(&pItems);
                    if (SUCCEEDED(hr)) {
                        DWORD dwNumItems;
                        pItems->GetCount(&dwNumItems);

                        for (DWORD i = 0; i < dwNumItems; i++) {
                            IShellItem *pItem;
                            hr = pItems->GetItemAt(i, &pItem);
                            if (SUCCEEDED(hr)) {
                                PWSTR pszFilePath;
                                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                if (SUCCEEDED(hr)) {
                                    int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                                    std::string path(size - 1, 0);
                                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &path[0], size, NULL, NULL);
                                    result.push_back(path);
                                    CoTaskMemFree(pszFilePath);
                                }
                                pItem->Release();
                            }
                        }
                        pItems->Release();
                    }
                } else {
                    IShellItem *pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr)) {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                        if (SUCCEEDED(hr)) {
                            int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                            std::string path(size - 1, 0);
                            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &path[0], size, NULL, NULL);
                            result.push_back(path);
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }

    return result;
}

#elif defined(__linux__)
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

// Execute a command and return its output
std::string exec_command(const char* cmd) {
    char buffer[128];
    std::string result = "";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        return result;
    }
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
        result += buffer;
    }
    return result;
}

// Split a string by delimiter and return vector of strings
std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        // Remove trailing newline if present
        if (!token.empty() && token.back() == '\n') {
            token.pop_back();
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::vector<std::string> ShowFileOpenDialog(bool allowMultiple = true, bool canChooseDirectories = false) {
    std::vector<std::string> result;

    std::string command = "zenity --file-selection";
    command += " --title=\"Select File(s)\"";

    if (allowMultiple) {
        command += " --multiple";
    }

    if (canChooseDirectories) {
        command += " --directory";
    }

    // Redirect stderr to /dev/null to suppress zenity warnings
    command += " 2>/dev/null";

    std::string output = exec_command(command.c_str());

    if (!output.empty()) {
        if (allowMultiple) {
            // Multiple files are separated by '|' in zenity output
            result = split_string(output, '|');
        } else {
            // Single file, just remove trailing newline
            if (output.back() == '\n') {
                output.pop_back();
            }
            result.push_back(output);
        }
    }

    return result;
}

std::vector<std::string> ShowFolderSelectionDialog(bool allowMultiple = true) {
    std::vector<std::string> result;

    std::string command = "zenity --file-selection --directory";
    command += " --title=\"Select Folder(s)\"";

    if (allowMultiple) {
        command += " --multiple";
    }

    // Redirect stderr to /dev/null to suppress zenity warnings
    command += " 2>/dev/null";

    std::string output = exec_command(command.c_str());

    if (!output.empty()) {
        if (allowMultiple) {
            // Multiple folders are separated by '|' in zenity output
            result = split_string(output, '|');
        } else {
            // Single folder, just remove trailing newline
            if (output.back() == '\n') {
                output.pop_back();
            }
            result.push_back(output);
        }
    }

    return result;
}

#endif
