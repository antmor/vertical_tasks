#include "TaskVM.h"
#include "TaskVM.g.cpp"

namespace winrt::vertical_tasks::implementation
{
    winrt::fire_and_forget TaskVM::RefreshTitle(bool update)
    {
        co_await winrt::resume_background();
        auto oldTitle = m_title;
        const auto size = GetWindowTextLength(m_hwnd);
        if (size > 0)
        {
            std::vector<wchar_t> buffer(size + 1);
            GetWindowText(m_hwnd, buffer.data(), size + 1);

            m_title = std::wstring_view(buffer.data(), buffer.size());
        }
        else
        {
            LOG_HR(E_INVALIDARG);
        }

        if (update)
        {
            if (auto queue = m_uiThread.get()) 
            {
                co_await wil::resume_foreground(queue);

                // send change notification
                if (oldTitle != m_title)
                {
                    OnPropertyChanged(L"Title");
                }
            }
        }
    }

    void TaskVM::Select()
    {
        throw hresult_not_implemented();
    }
    void TaskVM::Close()
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
