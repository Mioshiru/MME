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
    std::wstring dllDir = rootDir + L"\\DLLs";
    std::wstring targetExe = dllDir + L"\\MME_core.exe";

    // Set DLL directory so the core binary's runtime dependencies are resolved
    SetDllDirectoryW(dllDir.c_str());

    // Prepend DLLs directory to PATH
    wchar_t oldPath[32768];
    DWORD pathLen = GetEnvironmentVariableW(L"PATH", oldPath, 32768);
    if (pathLen > 0 && pathLen < 32768) {
        std::wstring newPath = dllDir + L";" + std::wstring(oldPath);
        SetEnvironmentVariableW(L"PATH", newPath.c_str());
    } else {
        SetEnvironmentVariableW(L"PATH", dllDir.c_str());
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
        rootDir.c_str(), // Working directory set to the root dist directory
        &si,
        &pi
    )) {
        std::wstring errMsg = L"Failed to start Mios Map Editor core executable:\n" + targetExe;
        MessageBoxW(NULL, errMsg.c_str(), L"Mios Map Editor", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Wait for the main editor process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exitCode;
}
