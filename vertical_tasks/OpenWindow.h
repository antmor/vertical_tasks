#include "pch.h"

#include <wil\resource.h>
#include <dwmapi.h>
#include <appmodel.h>

const static std::wstring_view s_app_frame_host{ L"ApplicationFrameHost.exe" };

inline BOOL TaskListDeleted(HWND m_hwnd)
{
    return GetProp(m_hwnd, L"ITaskList_Deleted") != NULL;
}

struct ProcessId
{
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

// Get the executable path or module name for modern apps
inline ProcessId get_process_path(DWORD pid, bool uwpRequery = false) noexcept
{
    wil::unique_handle process{ OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, TRUE, pid) };
    std::wstring name = L"no process handle";
    if (process)
    {
        name.resize(MAX_PATH);
        DWORD name_length = static_cast<DWORD>(name.length());
        if (QueryFullProcessImageNameW(process.get(), 0, (LPWSTR)name.data(), &name_length) == 0)
        {
            name_length = 0;
        }
        name.resize(name_length);
        if (uwpRequery)
        {
            UINT32 aumidL{};
            std::ignore = (GetApplicationUserModelId(process.get(), &aumidL, nullptr));
            if (aumidL > 0)
            {
                std::wstring aumid = L"no process handle";
                aumid.resize(aumidL + 1);
                LOG_IF_WIN32_ERROR(GetApplicationUserModelId(process.get(), &aumidL, aumid.data()));
                aumid.resize(aumidL);
                return { name, aumid };
            }
        }

    }
    return { name };
}

// Get the executable path or module name for modern apps
inline ProcessId get_process_path(HWND window) noexcept
{
    DWORD pid{};
    GetWindowThreadProcessId(window, &pid);
    auto&& [name, aumid] = get_process_path(pid);

    if (name.length() >= s_app_frame_host.length() &&
        // ends_with
        name.compare(name.length() - s_app_frame_host.length(), s_app_frame_host.length(), s_app_frame_host) == 0)
    {
        // It is a UWP app. We will enumerate the windows and look for one created
        // by something with a different PID
        DWORD new_pid = pid;

        EnumChildWindows(
            window, [](HWND hwnd, LPARAM param) -> BOOL
        {
            auto new_pid_ptr = reinterpret_cast<DWORD*>(param);
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid != *new_pid_ptr)
            {
                *new_pid_ptr = pid;
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

    return { name };
}


struct OpenWindow
{
    OpenWindow(HWND hwndIn) : m_hwnd(hwndIn)
    {
        m_process = get_process_path(hwndIn);
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

    bool IsCloaked()
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
            LOG_HR_MSG(E_INVALIDARG, "%ws has no window title", m_process.for_display().data());
        }
        return newTitle;
    }

    wil::unique_hicon TryGetIcon()
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

private:

    const ::HWND m_hwnd;
    ProcessId m_process;
};
