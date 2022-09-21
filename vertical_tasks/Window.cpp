#include "pch.h"
#include "MainWindow.xaml.h"
#include "Helper.h"
#include "Window.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.


// Gets the title of the window (the string displayed at the top of the window)
std::wstring Window::getTitle()
{
    std::wstring title;
    const auto size = GetWindowTextLength(hwnd);
    if (size > 0)
    {
        std::vector<wchar_t> buffer(size + 1);
        GetWindowText(hwnd, buffer.data(), size + 1);

        return std::wstring(std::begin(buffer), std::end(buffer));
    }
    else
    {
        return string.Empty;
            
    }
}

std::wstring Window::getClassName()
{
    return GetWindowClassName(Hwnd);
            
}

bool Window::IsCloaked()
{
    return GetWindowCloakState() != WindowCloakState.None;
}

/// Gets a value indicating whether the window is a toolwindow
bool Window::IsToolWindow()
{
    return (GetWindowLong(Hwnd, Win32Constants.GWL_EXSTYLE) &
        (uint)ExtendedWindowStyles.WS_EX_TOOLWINDOW) ==
        (uint)ExtendedWindowStyles.WS_EX_TOOLWINDOW;
}

/// Gets a value indicating whether the window is an appwindow
bool Window::IsAppWindow()
{
    return (GetWindowLong(Hwnd, Win32Constants.GWL_EXSTYLE) &
        (uint)ExtendedWindowStyles.WS_EX_APPWINDOW) ==
        (uint)ExtendedWindowStyles.WS_EX_APPWINDOW;
}

bool Window::TaskListDeleted()
{
    return GetProp(hwnd, "ITaskList_Deleted") != NULL;
}

// Gets a value indicating whether the specified windows is the owner (i.e. doesn't have an owner)
bool Window::IsOwner()
{
    return GetWindow(hwnd, GetWindowCmd.GW_OWNER) == NULL;
}

// Gets a value indicating whether the window is minimized
bool Window::IsMinimized()
{
    return GetWindowSizeState() == WindowSizeState.Minimized;
}

/// Initializes a new instance of the <see cref="Window"/> class.
/// Initializes a new Window representation
/// <param name="hwnd">the handle to the window we are representing</param>
void Window::Window(HWND hwnd)
{
    if (IsWindow(hwnd))
    {
        this.hwnd = hwnd;
        processInfo = CreateWindowProcessInstance(hwnd);
    }
}

/// <summary>
/// Switches desktop focus to the window
/// </summary>
void Window::SwitchToWindow()
{
    // The following block is necessary because
    // 1) There is a weird flashing behavior when trying
    //    to use ShowWindow for switching tabs in IE
    // 2) SetForegroundWindow fails on minimized windows
    // Using Ordinal since this is internal

    std::wstring processName = processInfo.name;
    if (CSTR_EQUAL == CompareStringOrdinal(convert_case(processName, processName, false), -1, "IEXPLORE.EXE" -1, TRUE) || !IsMinimized())
    {
        SetForegroundWindow(Hwnd);
    }
    else
    {
        if (!ShowWindow(hwnd, SW_RESTORE))
        {
            // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
            SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE,0);
        }
    }

    FlashWindow(hwnd, true);
}

/// <summary>
/// Closes the window
/// </summary>
void Window::CloseThisWindow(bool switchBeforeClose)
{
    if (switchBeforeClose)
    {
        SwitchToWindow();
    }

    SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE,0);
}

/// <summary>
/// Closes the window
/// </summary>
void Window::MinimizeThisWindow()
{
    SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

// Returns what the window size is
/// <returns>The state (minimized, maximized, etc..) of the window</returns>
WindowSizeState Window::GetWindowSizeState()
{
    WINDOWPLACEMENT wPos; 
    GetWindowPlacement(hwnd, &wPos);

    switch (wPos.ShowCmd)
    {
        case SW_SHOWNORMAL:
            return WindowSizeState.Normal;
        case SW_SHOWMINIMIZED:
        case SW_MINIMIZE:
            return WindowSizeState.Minimized;
        case SW_SHOWMAXIMIZED: // No need for ShowMaximized here since its also of value 3
            return WindowSizeState.Maximized;
        default:
            // throw new Exception("Don't know how to handle window state = " + placement.ShowCmd);
            return WindowSizeState.Unknown;
    }
}

// Returns the window cloak state from DWM
/// (A cloaked window is not visible to the user. But the window is still composed by DWM.)
/// <returns>The state (none, app, ...) of the window</returns>          ---- ASK
WindowCloakState Window::GetWindowCloakState()
{
    DWORD dwCloaked;
    if (SUCCEEDED(DwmGetWindowAttribute(Hwnd, DWMWA_CLOAKED, &dwCloaked, sizeof(dwCloaked))) && (dwCloaked == 0))
    {
        switch (dwCloaked)
        {
            case (int)DwmWindowCloakStates.None:
                return WindowCloakState.None;
            case (int)DwmWindowCloakStates.CloakedApp:
                return WindowCloakState.App;
            case (int)DwmWindowCloakStates.CloakedShell:
                return Main.VirtualDesktopHelperInstance.IsWindowCloakedByVirtualDesktopManager(hwnd, Desktop.Id) ? WindowCloakState.OtherDesktop : WindowCloakState.Shell;
            case (int)DwmWindowCloakStates.CloakedInherited:
                return WindowCloakState.Inherited;
            default:
                return WindowCloakState.Unknown;
        }
    }
}
                                           
/// Returns the class name of a window.
/// <param name="hwnd">Handle to the window.</param>
/// <returns>Class name</returns>
std::wstring Window::GetWindowClassName(HWND hwnd)
{
    std::vector<wchar_t> buffer(256);
    var numCharactersWritten = GetClassName(hwnd, buffer.data(), 256);

    if (numCharactersWritten == 0)
    {
        return std:wstring();
    }

    return std::wstring(std::begin(buffer), std::end(buffer));
}

/// Gets an instance of <see cref="WindowProcess"/> form process cache or creates a new one. A new one will be added to the cache.
/// <param name="hWindow">The handle to the window</param>
/// <returns>A new Instance of type <see cref="WindowProcess"/></returns>
WindowProcess Window::CreateWindowProcessInstance(HWND hWindow)
{
    std::lock_guard<std::mutex> guard(m_cacheLock);

    if (_handlesToWindowsCache.size > 7000)
    {
        _handlesToWindowsCache.clear();
    }

    // Add window to cache if missing
    if (!_handlesToWindowsCache.contains(hWindow)
    {

        // Get process ID and name
        var processId = GetProcessIDFromWindowHandle(hWindow);
            var threadId = GetThreadIDFromWindowHandle(hWindow);
            var processName = GetProcessNameFromProcessID(processId);

            if (processName.Length != 0)
            {
                _handlesToWindowsCache.insert(hWindow, WinData{ hWindow, std::wstring(std::begin(buffer), std::end(buffer)) });
            }
            else
            {
                // For the dwm process we can not receive the name. This is no problem because the window isn't part of result list.
                //  Log.Debug($"Invalid process {processId} ({processName}) for window handle {hWindow}.", typeof(Window));
                _handlesToWindowsCache.insert(hWindow, WinData{ hwnd, std::wstring(0, 0) });
            }
    }
    // Correct the process data if the window belongs to a uwp app hosted by 'ApplicationFrameHost.exe'
    // (This only works if the window isn't minimized. For minimized windows the required child window isn't assigned.)  ---ASK
    std::wstring processName = _handlesToWindowsCache.find(hWindow).name;
    if(CSTR_EQUAL == CompareStringOrdinal(convert_case(processName, processName, false), -1, "APPLICATIONFRAMEHOST.EXE", -1, TRUE))
    {
        new Task(() = >
        {
            EnumWindowsProc callbackptr = new EnumWindowsProc((HWND hwnd, IntPtr lParam) = >
            {
                std::wstring className = GetWindowClassName(hwnd);

                // Every uwp app main window has at least three child windows. Only the one we are interested in has a class starting with "Windows.UI.Core." and is assigned to the real app process.
                // (The other ones have a class name that begins with the string "ApplicationFrame".)
                if (StartsWithCaseInsensitive(className,L"Windows.UI.Core."))
                {
                    var childProcessId = GetProcessIDFromWindowHandle(hwnd);
                    var childThreadId = GetThreadIDFromWindowHandle(hwnd);
                    var childProcessName = GetProcessNameFromProcessID(childProcessId);

                    // Update process info in cache
                    _handlesToWindowsCache.find(hWindow).UpdateProcessInfo(childProcessId, childThreadId, childProcessName);
                    return false;
                }
                else
                {
                    return true;
                }
            });
            EnumChildWindows(hWindow, callbackptr, 0);
        }).Start();

    }
    
    return _handlesToWindowsCache.find(hWindow);
}
