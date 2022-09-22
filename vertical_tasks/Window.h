#pragma once
#include "WindowProcess.h"
#include "pch.h"
#include <mutex>


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
         static std::map<HWND, WindowProcess> s_handlesToWindowsCache;

        Window(HWND hwnd);
        Window() = default;
        std::unique_ptr <WindowProcess> processInfo;                      // An instance of <see cref="WindowProcess"/> that contains the process information for the window
        std::wstring getTitle();                                                     // Gets the title of the window (the string displayed at the top of the window)
        std::wstring getClassName();
        bool IsCloaked();
        bool IsToolWindow();
        bool IsAppWindow();
        bool TaskListDeleted();
        bool IsOwner();
        bool IsMinimized();
        void SwitchToWindow();
        void CloseThisWindow(bool switchBeforeClose);
        void MinimizeThisWindow();
        WindowSizeState GetWindowSizeState();
        WindowCloakState GetWindowCloakState();

        static BOOL CALLBACK EnumChildFunc(HWND hwnd, LPARAM lParam);
        bool EnumChildFunc(HWND hwnd);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&&) = default;
        Window& operator=(Window&&) = default;

        ~Window() {};

    private:
        HWND m_hwnd = nullptr;        // The handle to the window
        std::wstring GetWindowClassName(HWND hwnd);
        WindowProcess CreateWindowProcessInstance(HWND hWindow);

        static std::mutex m_cacheLock;
    };
}