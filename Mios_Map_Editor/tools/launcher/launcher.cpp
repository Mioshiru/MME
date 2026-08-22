#include <windows.h>
#include <string>
#include <vector>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        return 1;
    }

    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';
    }

    std::wstring rootDir = exePath;
    std::wstring binaryDir = rootDir + L"\\Binaries\\Win64";
    std::wstring targetExe = binaryDir + L"\\MME-Win64.exe";

    // Set DLL directory so the core binary's runtime dependencies are resolved from Binaries\Win64
    SetDllDirectoryW(binaryDir.c_str());

    // Prepend Binaries\Win64 directory to PATH
    wchar_t oldPath[32768];
    DWORD pathLen = GetEnvironmentVariableW(L"PATH", oldPath, 32768);
    if (pathLen > 0 && pathLen < 32768) {
        std::wstring newPath = binaryDir + L";" + std::wstring(oldPath);
        SetEnvironmentVariableW(L"PATH", newPath.c_str());
    } else {
        SetEnvironmentVariableW(L"PATH", binaryDir.c_str());
    }

    // Prepare command line
    std::wstring fullCmdLine = L"\"" + targetExe + L"\"";
    if (pCmdLine && wcslen(pCmdLine) > 0) {
        fullCmdLine += L" ";
        fullCmdLine += pCmdLine;
    }

    std::vector<wchar_t> cmdBuffer(fullCmdLine.begin(), fullCmdLine.end());
    cmdBuffer.push_back(L'\0');

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = nCmdShow;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(
        targetExe.c_str(),
        cmdBuffer.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        rootDir.c_str(), // Working directory set to the root Mios_Map_Editor directory
        &si,
        &pi
    )) {
        std::wstring msg = L"Could not launch core binary:\n" + targetExe;
        MessageBoxW(NULL, msg.c_str(), L"Mios Map Editor - Startup Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Wait for the main process to exit, then return its exit code
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
}
