#include "pch.h"
#include "Helper.h"
#include "Window.h"
#include "WindowProcess.h"
#include "dwmapi.h"
#include "winuser.h"
#include <map>
#include <future>


using namespace winrt; 
using namespace winrt::vertical_tasks::implementation;


// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

std::mutex winrt::vertical_tasks::implementation::Window::m_cacheLock{};
std::map<HWND, WindowProcess> winrt::vertical_tasks::implementation::Window::s_handlesToWindowsCache{};

/// Initializes a new instance of the <see cref="Window"/> class.
/// Initializes a new Window representation
/// <param name="m_hwnd">the handle to the window we are representing</param>
winrt::vertical_tasks::implementation::Window::Window(HWND hwnd)
{
    if (IsWindow(hwnd))
    {
        this->m_hwnd = hwnd;
        this->processInfo = std::make_unique<WindowProcess>(CreateWindowProcessInstance(m_hwnd));
    }
}

// Gets the title of the window (the string displayed at the top of the window)
std::wstring Window::getTitle()
{
    std::wstring title;
    const auto size = GetWindowTextLength(m_hwnd);
    if (size > 0)
    {
        std::vector<wchar_t> buffer(size + 1);
        GetWindowText(m_hwnd, buffer.data(), size + 1);

        return std::wstring(std::begin(buffer), std::end(buffer));
    }
    else
    {
        return std::wstring();
            
    }
}

std::wstring Window::getClassName()
{
    return GetWindowClassName(m_hwnd);
            
}

bool Window::IsCloaked()
{
    return GetWindowCloakState() != WindowCloakState::None;
}

/// Gets a value indicating whether the window is a toolwindow
bool Window::IsToolWindow()
{
    return (GetWindowLong(m_hwnd, GWL_EXSTYLE) &
        WS_EX_TOOLWINDOW) ==
        WS_EX_TOOLWINDOW;
}

/// Gets a value indicating whether the window is an appwindow
bool Window::IsAppWindow()
{
    return (GetWindowLong(m_hwnd, GWL_EXSTYLE) &
        WS_EX_APPWINDOW) == WS_EX_APPWINDOW;
}

bool Window::TaskListDeleted()
{
    return GetProp(m_hwnd, L"ITaskList_Deleted") != NULL;
}

// Gets a value indicating whether the specified windows is the owner (i.e. doesn't have an owner)
bool Window::IsOwner()
{
    return GetWindow(m_hwnd, GW_OWNER) == NULL;
}

// Gets a value indicating whether the window is minimized
bool Window::IsMinimized()
{
    return GetWindowSizeState() == WindowSizeState::Minimized;
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

    std::wstring processName = processInfo->name;
    if (CaseInsensitiveEqual(processName, L"IEXPLORE.EXE") || !IsMinimized())
    {
        SetForegroundWindow(m_hwnd);
    }
    else
    {
        if (!ShowWindow(m_hwnd, SW_RESTORE))
        {
            // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
            SendMessage(m_hwnd, WM_SYSCOMMAND, SC_RESTORE,0);
        }
    }

    FlashWindow(m_hwnd, true);
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

    SendMessage(m_hwnd, WM_SYSCOMMAND, SC_CLOSE,0);
}

/// <summary>
/// Closes the window
/// </summary>
void Window::MinimizeThisWindow()
{
    SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

// Returns what the window size is
/// <returns>The state (minimized, maximized, etc..) of the window</returns>
WindowSizeState Window::GetWindowSizeState()
{
    WINDOWPLACEMENT wPos; 
    GetWindowPlacement(m_hwnd, &wPos);

    switch (wPos.showCmd)
    {
        case SW_SHOWNORMAL:
            return WindowSizeState::Normal;
        case SW_SHOWMINIMIZED:
        case SW_MINIMIZE:
            return WindowSizeState::Minimized;
        case SW_SHOWMAXIMIZED: // No need for ShowMaximized here since its also of value 3
            return WindowSizeState::Maximized;
        default:
            // throw new Exception("Don't know how to handle window state = " + placement.ShowCmd);
            return WindowSizeState::Unknown;
    }
}

// Returns the window cloak state from DWM
/// (A cloaked window is not visible to the user. But the window is still composed by DWM.)
/// <returns>The state (none, app, ...) of the window</returns>          ---- ASK  OherDesktop
WindowCloakState Window::GetWindowCloakState()
{
    DWORD dwCloaked;
    if (SUCCEEDED(DwmGetWindowAttribute(m_hwnd, DWMWA_CLOAKED, &dwCloaked, sizeof(dwCloaked))))
    {
        switch (dwCloaked)
        {     
        case 0:
            return WindowCloakState::None;

            case DWM_CLOAKED_APP:
                return WindowCloakState::App;
            case DWM_CLOAKED_SHELL:
                return WindowCloakState::Shell;
            case DWM_CLOAKED_INHERITED:
                return WindowCloakState::Inherited;
            default:
                return WindowCloakState::Unknown;
        }
    }
    return WindowCloakState::Unknown;

}
                                           
/// Returns the class name of a window.
/// <param name="m_hwnd">Handle to the window.</param>
/// <returns>Class name</returns>
std::wstring Window::GetWindowClassName(HWND hwnd)
{
    std::vector<wchar_t> buffer(256);
    int numCharactersWritten = GetClassName(hwnd, buffer.data(), 256);

    if (numCharactersWritten == 0)
    {
        return std::wstring();
    }

    return std::wstring(std::begin(buffer), std::end(buffer));
}

/// Gets an instance of <see cref="WindowProcess"/> form process cache or creates a new one. A new one will be added to the cache.
/// <param name="hWindow">The handle to the window</param>
/// <returns>A new Instance of type <see cref="WindowProcess"/></returns>
WindowProcess Window::CreateWindowProcessInstance(HWND hWindow)
{
    std::lock_guard<std::mutex> guard(m_cacheLock);

    if (s_handlesToWindowsCache.size() > 7000)
    {
        s_handlesToWindowsCache.clear();
    }

    // Add window to cache if missing
    if (s_handlesToWindowsCache.find(hWindow)== s_handlesToWindowsCache.end())
    {
        // Get process ID and name
        DWORD processId = WindowProcess::GetProcessIDFromWindowHandle(hWindow);
        DWORD threadId = WindowProcess::GetThreadIDFromWindowHandle(hWindow);
        std::wstring processName = WindowProcess::GetProcessNameFromProcessID(processId);

        if (processName.size() != 0)
        {
            s_handlesToWindowsCache.emplace(hWindow, WindowProcess{processId, threadId, processName});
        }
        else
        {
            // For the dwm process we can not receive the name. This is no problem because the window isn't part of result list.
            //  Log.Debug($"Invalid process {processId} ({processName}) for window handle {hWindow}.", typeof(Window));
            s_handlesToWindowsCache.emplace(hWindow, WindowProcess{ 0, 0, std::wstring()});
        }
    }
    // Correct the process data if the window belongs to a uwp app hosted by 'ApplicationFrameHost.exe'
    // (This only works if the window isn't minimized. For minimized windows the required child window isn't assigned.)  ---ASK
    if(CaseInsensitiveEqual(s_handlesToWindowsCache[hWindow].name, L"APPLICATIONFRAMEHOST.EXE"))
    {
        auto task = std::async(std::launch::async,
            [&]()
            {                
                EnumChildWindows(hWindow, &Window::EnumChildFunc,
                    0);
        });
        task.get();
    }    
    return s_handlesToWindowsCache[hWindow];
}

BOOL CALLBACK Window::EnumChildFunc(HWND hwnd, LPARAM lParam)
{
    auto thisPtr = reinterpret_cast<Window*>(lParam);
    return thisPtr->EnumChildFunc(hwnd);
}

bool Window::EnumChildFunc(HWND hwnd)
{
    std::wstring className = GetWindowClassName(hwnd);

    // Every uwp app main window has at least three child windows. Only the one we are interested in has a class starting with "Windows.UI.Core." and is assigned to the real app process.
    // (The other ones have a class name that begins with the string "ApplicationFrame".)
    if (StartsWithCaseInsensitive(className, L"Windows.UI.Core."))
    {
        DWORD childProcessId = WindowProcess::GetProcessIDFromWindowHandle(hwnd);
        DWORD childThreadId = WindowProcess::GetThreadIDFromWindowHandle(hwnd);
        std::wstring childProcessName = WindowProcess::GetProcessNameFromProcessID(childProcessId);

        // Update process info in cache
        s_handlesToWindowsCache[hwnd].UpdateProcessInfo(childProcessId, childThreadId, childProcessName);
        return false;
    }
    return true;
}







