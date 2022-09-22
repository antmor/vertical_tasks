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
    EnumWindows(&WindowEnumerationCallBack, 0);
}


