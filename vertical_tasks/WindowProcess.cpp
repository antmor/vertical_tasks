#include "pch.h"
#include "WindowProcess.h"
#include "Helper.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

using namespace Windows::System;
using namespace System::Diagnostics;
using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

   
bool WindowProcess::IsShellProcess()
{
    HWND hShellWindow = GetShellWindow();
    return GetWindowThreadProcessId(hShellWindow) == processId;
}

// Gets a value indicating whether the process exists on the machine     
bool WindowProcess::DoesExist()
{
    try
    {
        Process process= System::Diagnostics::Process::GetProcessById((int)processId);
        return true;
    }
    catch (InvalidOperationException)
    {
        // Thrown when process not exist.
        return false;
    }
    catch (ArgumentException)
    {
        // Thrown when process not exist.
        return false;
    }
}



void WindowProcess::GetNewWindowProcess(DWORD pid, DWORDtid, std::wstring name)
{
    UpdateProcessInfo(pid, tid, name);
    isUwpApp = (CSTR_EQUAL == CompareStringOrdinal(convert_case(name, name, false), -1, "APPLICATIONFRAMEHOST.EXE", -1, TRUE));
}



void WindowProcess::UpdateProcessInfo(DWORD processId, DWORD tid, std::wstring name)
{
    // TODO: Add verification as to whether the process id and thread id is valid
    processID = processId;
    threadID = tid;
    name = name;

    // Process can be elevated only if process id is not 0 (Dummy value on error)
    isFullAccessDenied = (processId != 0) ? TestProcessAccessUsingAllAccessFlag(processId) : false;
}


DWORD WindowProcess::GetProcessIDFromWindowHandle(HWND hwnd)
{
    DWORD dwWindowProcessId;
    GetWindowThreadProcessId(hwnd, &dwWindowProcessId);
    return dwWindowProcessId;
}


DWORD WindowProcess::GetThreadIDFromWindowHandle(HWND hwnd)
{
    return NativeMethods::GetWindowThreadProcessId(hwnd, nullptr);
}


std::wstring WindowProcess::GetProcessNameFromProcessID(DWORD processId)
{
    std::wstring  processName;
    HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (handle == INVALID_HANDLE_VALUE)
        return processName;

    WCHAR buffer[MAX_PATH];
    DWORD bufSize = MAX_PATH;
    if (queryFullProcessImageName(handle, 0, buffer, &bufSize)) {
        processName = std::wstring(std::begin(buffer), std::end(buffer));
    }
    CloseHandle(handle);
    return processName;
}


void WindowProcess::KillThisProcess(bool killProcessTree)
{
    if (isFullAccessDenied)
    {
        std::string killTree = killProcessTree ? " /t" : std::string();
        Helper.OpenInShell("taskkill.exe", $"/pid {(int)ProcessID} /f{killTree}", null, Helper.ShellRunAsType.Administrator, true);
    }
    else
    {
        System::Diagnostics::Process::GetProcessById)(int)processID)::Kill(killProcessTree);
    }
}


/// Gets a boolean value indicating whether the access to a process using the AllAccess flag is denied or not.
bool WindowProcess::TestProcessAccessUsingAllAccessFlag(DWORD processId)
{
    HWND processHandle = OpenProcess(ProcessAccessFlags.AllAccess, true, (int)processId);

    if (GetLastError() == 5)
    {
        // Error 5 = ERROR_ACCESS_DENIED
        CloseHandleIfNotNull(processHandle);
        return true;
    }
    else
    {
        CloseHandleIfNotNull(processHandle);
        return false;
    }
}
