#include "pch.h"
#include "TaskVM.h"
#include "TaskVM.g.cpp"


namespace winrt::vertical_tasks::implementation
{

    void TaskVM::Select()
    {
        throw hresult_not_implemented();
    }
    void TaskVM::Close()
    {
        SwitchToWindow();

        SendMessage(m_hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    void TaskVM::Minimize()
    {
        SendMessage(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }

    void TaskVM::SwitchToWindow()
    {
        if (!ShowWindow(m_hwnd, SW_RESTORE))
        {
            // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
            SendMessage(m_hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        }

        FlashWindow(m_hwnd, true);
    }
}
