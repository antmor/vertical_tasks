#include "pch.h"
#include "OpenWindows.h"
#include "Window.h"

using namespace winrt;
using namespace winrt::vertical_tasks::implementation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.


/// <summary>
/// Gets the list of all open windows
/// </summary>
std::vector<Window>& OpenWindows::getAllOpenWindows()
{
    return windows;
}

/// <summary>
/// Gets an instance property of this class that makes sure that
/// the first instance gets created and that all the requests
/// end up at that one instance
/// </summary>
OpenWindows& OpenWindows::GetInstance()
{
    static OpenWindows instance;
    return instance;
}

/// <summary>
/// Updates the list of open windows
/// </summary>
void OpenWindows::UpdateOpenWindowsList()
{
    windows.clear();
    EnumWindows(&OpenWindows::EnumWindow, reinterpret_cast<LPARAM>(this));
}


BOOL CALLBACK OpenWindows::EnumWindow(HWND hwnd, LPARAM lParam)
{
    auto thisPtr = reinterpret_cast<OpenWindows*>(lParam);
    return thisPtr->EnumWindow(hwnd);
}

bool OpenWindows::EnumWindow(HWND hwnd)
{
    Window newWindow(hwnd);
    if (IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)) &&
        (!newWindow.IsToolWindow() || newWindow.IsAppWindow()) && !newWindow.TaskListDeleted() &&
        (!CaseInsensitiveEqual(newWindow.getClassName(), L"Windows.UI.Core.CoreWindow")))
        //newWindow->getClassName() != "Windows.UI.Core.CoreWindow" ) //&& newWindow.Process.name != _powerLauncherExe)
    {
        // To hide (not add) preloaded uwp app windows that are invisible to the user and other cloaked windows, we check the cloak state. (Issue #13637.)
        // (If user asking to see cloaked uwp app windows again we can add an optional plugin setting in the future.)
        if (!newWindow.IsCloaked())
        {
            windows.emplace_back(std::move(newWindow));
        }
    }

    return TRUE;
}


