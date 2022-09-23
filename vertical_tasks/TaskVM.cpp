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
