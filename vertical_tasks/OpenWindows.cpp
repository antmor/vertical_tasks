#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#include "OpenWindows.h"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.


/// <summary>
/// Gets the list of all open windows
/// </summary>
std::vector<Window> OpenWindows::getAllOpenWindows()
{
    std::vector<Window> openWindows(windows);
    return openWindows;
}

/// <summary>
/// Gets an instance property of this class that makes sure that
/// the first instance gets created and that all the requests
/// end up at that one instance
/// </summary>
OpenWindows& winrt::vertical_tasks::implementation::OpenWindows::GetInstance()
{
    static winrt::vertical_tasks::implementation::OpenWindows instance;
    return instance;
}


/// <summary>
/// Updates the list of open windows
/// </summary>
void OpenWindows::UpdateOpenWindowsList()
{
    windows.clear();
    EnumWindows(&WindowEnumerationCallBack, 0);
}


/// <summary>
/// Call back method for window enumeration
/// </summary>
/// <param name="hwnd">The handle to the current window being enumerated</param>
/// <param name="lParam">Value being passed from the caller (we don't use this but might come in handy
/// in the future</param>
/// <returns>true to make sure to continue enumeration</returns>
BOOL CALLBACK WindowEnumerationCallBack(HWND hWnd, LPARAM /*lParam*/)
{
    Window newWindow = new Window(hwnd);

    if (IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)) &&
        (!newWindow.IsToolWindow() || newWindow.IsAppWindow()) && !newWindow.TaskListDeleted() &&
        newWindow.getClassName != "Windows.UI.Core.CoreWindow" && newWindow.Process.name != _powerLauncherExe)
    {
        // To hide (not add) preloaded uwp app windows that are invisible to the user and other cloaked windows, we check the cloak state. (Issue #13637.)
        // (If user asking to see cloaked uwp app windows again we can add an optional plugin setting in the future.)
        if (!newWindow.IsCloaked() || newWindow.GetWindowCloakState() == Window.WindowCloakState.OtherDesktop)
        {
            windows.Add(newWindow);
        }
    }

    return true;
}
