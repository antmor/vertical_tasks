#include "pch.h"
#include "TaskVM.h"
#include "TaskVM.g.cpp"

#include "imagehelper.h"
#include <winrt/microsoft.ui.xaml.media.imaging.h>
#include <appmodel.h>

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Graphics::Imaging;
}

namespace winui
{
    using namespace winrt::Microsoft::UI::Xaml;
    using namespace winrt::Microsoft::UI::Xaml::Media;
    using namespace winrt::Microsoft::UI::Xaml::Media::Imaging;
}
const static std::wstring_view s_app_frame_host{ L"ApplicationFrameHost.exe" };

// Get the executable path or module name for modern apps
inline std::wstring get_process_path(DWORD pid) noexcept
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
        if (name.length() >= s_app_frame_host.length() &&
            // ends_with
            name.compare(name.length() - s_app_frame_host.length(), s_app_frame_host.length(), s_app_frame_host) == 0)
        {
            UINT32 aumidL{};
            LOG_IF_WIN32_ERROR(GetApplicationUserModelId(process.get(), &aumidL, nullptr));
            if (aumidL > 0)
            {
                std::wstring aumid = L"no process handle";
                aumid.resize(aumidL + 1);
                LOG_IF_WIN32_ERROR(GetApplicationUserModelId(process.get(), &aumidL, aumid.data()));
                aumid.resize(aumidL);
            }
        }

    }
    return name;
}

// Get the executable path or module name for modern apps
inline std::wstring get_process_path(HWND window) noexcept
{

    DWORD pid{};
    GetWindowThreadProcessId(window, &pid);
    auto name = get_process_path(pid);

    if (name.length() >= s_app_frame_host.length() &&
        // ends_with
        name.compare(name.length() - s_app_frame_host.length(), s_app_frame_host.length(), s_app_frame_host) == 0)
    {
        // It is a UWP app. We will enumerate the windows and look for one created
        // by something with a different PID
        DWORD new_pid = pid;

        EnumChildWindows(
            window, [](HWND hwnd, LPARAM param) -> BOOL {
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
            return get_process_path(new_pid);
        }
    }

    return name;
}


namespace winrt::vertical_tasks::implementation
{
    winrt::fire_and_forget TaskVM::RefreshTitleAndIcon(bool update)
    {
        co_await winrt::resume_background();
        // recalc m_procName if we are applicationframehost
        m_procName = get_process_path(m_hwnd);

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
            LOG_HR_MSG(E_INVALIDARG, "%ws has no window name", m_procName.c_str());
        }

        HICON icon;
        SendMessageTimeout(m_hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG,
            500/*ms*/, reinterpret_cast<PDWORD_PTR>(&icon));
        if (!icon)
        {
            SendMessageTimeout(m_hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG,
                500/*ms*/, reinterpret_cast<PDWORD_PTR>(&icon));
        }

        if (!icon)
        {
            icon = reinterpret_cast<HICON>(GetClassLongPtr(m_hwnd, GCLP_HICONSM));
        }

        if (!icon)
        {
            LOG_HR_MSG(E_INVALIDARG, "%ws has no icon", m_procName.c_str());
        }

        winrt::SoftwareBitmap bitmap{nullptr};
        if (icon)
        {
            wil::unique_hicon iconCopy(CopyIcon(icon));
            bitmap = co_await GetBitmapFromIconFileAsync(std::move(iconCopy));
            if ((bitmap.BitmapPixelFormat() != BitmapPixelFormat::Bgra8) ||
                (bitmap.BitmapAlphaMode() != BitmapAlphaMode::Premultiplied))
            {
                bitmap = SoftwareBitmap::Convert(bitmap, BitmapPixelFormat::Bgra8, BitmapAlphaMode::Premultiplied);
            }
        }

        if (update
            && (bitmap || (m_title != newTitle)))
        {
            if (auto queue = m_uiThread.get())
            {
                co_await wil::resume_foreground(queue);

                if (bitmap)
                {
                    winui::SoftwareBitmapSource source{};
                    co_await source.SetBitmapAsync(bitmap);
                    IconSource(source);
                }

                // send change notification
                if (m_title != newTitle) 
                {
                    m_title = newTitle;
                    OnPropertyChanged(L"Title");
                }
            }
        }
    }


    void TaskVM::Select()
    {
        WINDOWPLACEMENT wPos;
        GetWindowPlacement(m_hwnd, &wPos);

        if ((wPos.showCmd == SW_MINIMIZE) || (wPos.showCmd == SW_SHOWMINIMIZED))
        {
            SetForegroundWindow(m_hwnd);
            if (!ShowWindow(m_hwnd, SW_RESTORE))
            {
                // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
                SendMessage(m_hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
        }
        else
        {
            Minimize();
        }
    }

    void TaskVM::Close()
    {
        SendMessage(m_hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    void TaskVM::Kill()
    {
        DWORD processId;
        GetWindowThreadProcessId(m_hwnd, &processId);
        wil::unique_handle processHandle(OpenProcess(PROCESS_TERMINATE, FALSE, processId));
        TerminateProcess(processHandle.get(), 0);
    }


    void TaskVM::Minimize()
    {
        SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }
}
