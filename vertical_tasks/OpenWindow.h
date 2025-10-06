#include "pch.h"

#include <wil\resource.h>
#include <dwmapi.h>
#include <appmodel.h>
#include <string>
#include <format>

#include "EnumToString.h"


const static std::wstring_view s_app_frame_host{ L"ApplicationFrameHost.exe" };

inline BOOL TaskListDeleted(HWND m_hwnd)
{
    return GetProp(m_hwnd, L"ITaskList_Deleted") != NULL;
}

struct ProcessId
{
    DWORD pid{};
    std::wstring processPath{};
    std::wstring aumid{};

    const std::wstring& processId() const
    {
        return aumid.empty()
            ? processPath
            : aumid;
    }
    std::wstring_view for_display() const
    {
        return processId();
    }
};

struct ProcessInfo
{
    std::wstring name{};
    wil::unique_handle process{};
};

inline ProcessInfo GetProcessNameFromProcessId(DWORD pid)
{
    ProcessInfo info{};
    info.process.reset(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, TRUE, pid));
    if (info.process)
    {
        THROW_IF_NULL_ALLOC(info.process);
        std::wstring name = L"no process handle";
        name.resize(MAX_PATH);
        DWORD name_length = static_cast<DWORD>(name.length());
        if (QueryFullProcessImageNameW(info.process.get(), 0, (PWSTR)name.data(), &name_length) == 0)
        {
            name_length = 0;
        }
        name.resize(name_length);
        info.name = std::move(name);
    }
    return info;
}

// Get the executable path or module name for modern apps
inline ProcessId get_process_path(DWORD pid, bool uwpRequery = false) noexcept
{
    auto [name, process] = GetProcessNameFromProcessId(pid);

    if (process && uwpRequery)
    {
        UINT32 aumidL{};
        std::ignore = (GetApplicationUserModelId(process.get(), &aumidL, nullptr));
        if (aumidL > 0)
        {
            std::wstring aumid = L"no process handle";
            aumid.resize(aumidL + 1);
            LOG_IF_WIN32_ERROR(GetApplicationUserModelId(process.get(), &aumidL, aumid.data()));
            aumid.resize(aumidL);
            return { pid, name, aumid };
        }
    }

    return { pid, name };
}

// Get the executable path or module name for modern apps
inline ProcessId get_process_path(HWND window) noexcept
{
    DWORD pid{};
    GetWindowThreadProcessId(window, &pid);
    auto&& [_, name, aumid] = get_process_path(pid);

    if (name.length() >= s_app_frame_host.length() &&
        // ends_with
        name.compare(name.length() - s_app_frame_host.length(), s_app_frame_host.length(), s_app_frame_host) == 0)
    {
        // It is a UWP app. We will enumerate the windows and look for one created
        // by something with a different PID
        DWORD new_pid = pid;

        EnumChildWindows(
            window, [](HWND currHwnd, LPARAM param) -> BOOL
        { 
            // Every uwp app main window has at least three child windows. Only the one we are interested in has a class starting with "Windows.UI.Core." and is assigned to the real app process.
            // (The other ones have a class name that begins with the string "ApplicationFrame".)
                std::wstring_view uiCore{ L"Windows.UI.Core." };
            std::wstring className;
            className.resize(255);
            auto written = GetClassNameW(currHwnd, className.data(), static_cast<int>(className.capacity()));
            className.resize(written);
            if (className.compare(0, uiCore.length(), uiCore))
            //    if (GetWindowClassName(currHwnd).StartsWith("Windows.UI.Core.", StringComparison.OrdinalIgnoreCase))
            {
                DWORD childProcessId{};
                std::ignore = GetWindowThreadProcessId(currHwnd , &childProcessId);
                auto [childProcessName, handle] = GetProcessNameFromProcessId(childProcessId);
                auto new_pid_ptr = reinterpret_cast<DWORD*>(param);

                if (childProcessId != *new_pid_ptr)
                {
                    *new_pid_ptr = childProcessId;
                    return FALSE;
                }
                // Update process info in cache
                //_handlesToProcessCache[hWindow].UpdateProcessInfo(childProcessId, childThreadId, childProcessName);
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        },
            reinterpret_cast<LPARAM>(&new_pid));

        // If we have a new pid, get the new name.
        if (new_pid != pid)
        {
            return get_process_path(new_pid, true);
        }
    }

    return { pid, name };
}


struct OpenWindow
{
    OpenWindow(HWND hwndIn) : m_hwnd(hwndIn)
    {
        if (hwndIn == nullptr)
        {
            // placehodler 
            return;
        }

        m_process = get_process_path(hwndIn);
        m_className.resize(255);
        auto writen = GetClassNameW(hwndIn, m_className.data(), static_cast<int>(m_className.capacity()));
        m_className.resize(writen);

        auto currStyle = GetWindowLong(hwndIn, GWL_STYLE);
        auto currExStyle = GetWindowLong(hwndIn, GWL_EXSTYLE);

        auto formatted = std::format(L"OpenWindow \n\t HWND = {:8x}; Cloaked = {}; PID = {}; \n\tProcess = {};\n\t AUMID = {}\n\t classname = {}\n\t {:8x} {} \n\t {:8x} {}\n",
            reinterpret_cast<ULONG_PTR>(m_hwnd), IsCloaked(), m_process.pid, m_process.processPath, m_process.aumid, m_className,
            currStyle, WindowStylesToString(currStyle),
            currExStyle, ExWindowStylesToString(currExStyle));
        OutputDebugString(formatted.c_str());
    }

    HWND HWND() const
    {
        return m_hwnd;
    }

    const std::wstring& ProcessPath() const
    {
        return m_process.processPath;
    }

    const std::wstring& Aumid() const
    {
        return m_process.aumid;
    }

    const std::wstring_view ProcessName() const
    {
        return m_process.for_display();
    }
    
    const std::wstring_view CachedTitle() const
    {
        return m_cachedTitle;
    }

    bool IsCloaked() const
    {
        DWORD cloakAttrib;
        DwmGetWindowAttribute(m_hwnd, DWMWA_CLOAKED, &cloakAttrib, sizeof(cloakAttrib));
        return cloakAttrib != 0;
    }

    winrt::hstring GetTitle()
    {
        winrt::hstring newTitle;
        const auto size = GetWindowTextLength(m_hwnd);
        if (size > 0)
        {
            std::vector<wchar_t> buffer(size + 1);
            GetWindowText(m_hwnd, buffer.data(), size + 1);

            newTitle = std::wstring_view(buffer.data(), buffer.size());
        }
        else
        {
            //LOG_HR_MSG(E_INVALIDARG, "%ws has no window title", m_process.for_display().data());
        }
        m_cachedTitle = newTitle;
        return newTitle;
    }

    wil::unique_hicon TryGetIconFromWindow()
    {
        wil::unique_hicon icon;
        SendMessageTimeout(m_hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG,
            500/*ms*/, reinterpret_cast<PDWORD_PTR>(&icon));
        if (!icon)
        {
            SendMessageTimeout(m_hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG,
                500/*ms*/, reinterpret_cast<PDWORD_PTR>(&icon));
        }

        if (!icon)
        {
            icon.reset(reinterpret_cast<HICON>(GetClassLongPtr(m_hwnd, GCLP_HICONSM)));
        }
        return icon;
    }

    void Select()
    {
        WINDOWPLACEMENT wPos{};
        GetWindowPlacement(m_hwnd, &wPos);

        const bool isMinimixed = (wPos.showCmd == SW_MINIMIZE) || (wPos.showCmd == SW_SHOWMINIMIZED);
        const bool isForeground = m_hwnd == GetForegroundWindow();
        if (isMinimixed || !isForeground)
        {
            // bring window to the foreground
            if (!ShowWindow(m_hwnd, SW_RESTORE))
            {
                // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
                SendMessage(m_hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
            SetForegroundWindow(m_hwnd);

        }
        else
        {
            Minimize();
        }
    }

    void Close() { SendMessage(m_hwnd, WM_SYSCOMMAND, SC_CLOSE, 0); }
    void Minimize() { SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0); }
    void Kill()
    {
        DWORD processId;
        GetWindowThreadProcessId(m_hwnd, &processId);
        wil::unique_handle processHandle(OpenProcess(PROCESS_TERMINATE, FALSE, processId));
        TerminateProcess(processHandle.get(), 0);
    }

    bool IsValidWindow() const
    {
        return IsValidWindow(m_hwnd);
    }

    static bool IsValidWindow(::HWND hwnd)
    {
        std::wstring className;
        className.resize(255);
        auto written = GetClassNameW(hwnd, className.data(), static_cast<int>(className.capacity()));
        className.resize(written);

        const auto wsf = GetWindowLong(hwnd, GWL_EXSTYLE);
        return IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)) &&
            !TaskListDeleted(hwnd) &&
            (WI_IsFlagClear(wsf, WS_EX_TOOLWINDOW) || WI_IsFlagSet(wsf, WS_EX_APPWINDOW)) &&
            className != L"Windows.UI.Core.CoreWindow";
    }
private:

    const ::HWND m_hwnd;
    ProcessId m_process;
    std::wstring m_cachedTitle;
    std::wstring m_className;
};
