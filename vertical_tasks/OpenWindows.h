#pragma once
#include "pch.h"
#include "Window.h"

namespace winrt::vertical_tasks::implementation
{
    struct OpenWindows
    {
        std::vector<Window> getAllOpenWindows();
        void UpdateOpenWindowsList();
        static OpenWindows& GetInstance();
        std::vector<Window> windows;          /// List of all the open windows

        // OpenWindows(OpenWindows const&) = delete;
        // OpenWindows(OpenWindows&&) = delete;

    private:
        OpenWindows();

       // std::wstring _powerLauncherExe = Path.GetFileName(Environment.ProcessPath);          // PowerLauncher main executable -- ASK
        static OpenWindows& instance;
    };
}