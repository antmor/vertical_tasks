#pragma once
#include "WindowProcess.h"
#include "pch.h"


namespace winrt::vertical_tasks::implementation
{
    enum class WindowSizeState
    {
        Normal,
        Minimized,
        Maximized,
        Unknown,
    };       /// Enum to simplify the state of the window

    enum class WindowCloakState
    {
        None,
        App,
        Shell,
        Inherited,
        OtherDesktop,
        Unknown,
    };          // Enum to simplify the cloak state of the window

        
    struct Window
    {
        WindowProcess processInfo;                      // An instance of <see cref="WindowProcess"/> that contains the process information for the window
        std::wstring getTitle();                                                     // Gets the title of the window (the string displayed at the top of the window)
        std::wstring getClassName();
        bool IsCloaked();
        bool IsToolWindow();
        bool IsAppWindow();
        bool TaskListDeleted();
        bool IsOwner();
        bool IsMinimized();
        Window(HWND hwnd);
        void SwitchToWindow();
        void CloseThisWindow(bool switchBeforeClose);
        void MinimizeThisWindow();
        WindowSizeState GetWindowSizeState();
        WindowCloakState GetWindowCloakState();

    private:
        HWND hwnd;        // The handle to the window
        std::map<HWND, WindowProcess> _handlesToWindowsCache;
        std::wstring GetWindowClassName(HWND hwnd);
        WindowProcess CreateWindowProcessInstance(HWND hWindow);
    };
}