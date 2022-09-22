#pragma once
#include "pch.h"
#include "Window.h"
#include "Helper.h"

namespace winrt::vertical_tasks::implementation
{
    

    struct OpenWindows
    {
        std::vector<Window> getAllOpenWindows();
        void UpdateOpenWindowsList();
        static OpenWindows& GetInstance();
        std::vector<Window> windows;


        // OpenWindows(OpenWindows const&) = delete;
        // OpenWindows(OpenWindows&&) = delete;

        /// Call back method for window enumeration       
        BOOL CALLBACK WindowEnumerationCallBack(HWND hwnd, LPARAM /*lParam*/)
        {
            Window* newWindow = new Window(hwnd);

            if (IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)) &&
                (!newWindow->IsToolWindow() || newWindow->IsAppWindow()) && !newWindow->TaskListDeleted() &&
                (CaseInsensitiveEqual(newWindow->getClassName(), L"Windows.UI.Core.CoreWindow")))
                //newWindow->getClassName() != "Windows.UI.Core.CoreWindow" ) //&& newWindow.Process.name != _powerLauncherExe)
            {
                // To hide (not add) preloaded uwp app windows that are invisible to the user and other cloaked windows, we check the cloak state. (Issue #13637.)
                // (If user asking to see cloaked uwp app windows again we can add an optional plugin setting in the future.)
                if (!newWindow->IsCloaked())
                {
                    windows.emplace_back(newWindow);
                }
            }

            return TRUE;
        }

    private:
        OpenWindows();

    };
}