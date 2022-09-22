#pragma once
#include "pch.h"

namespace winrt::vertical_tasks::implementation
{
    struct WindowProcess
    {
        DWORD processId;            // Gets the id of the process
        DWORD threadID;             /// Gets the id of the thread
        std::wstring name;           /// Gets the name of the process
        bool IsUwpApp;          // Gets a value indicating whether the window belongs to an 'Universal Windows Platform (UWP)' process
        bool isFullAccessDenied;           // Gets a value indicating whether full access to the process is denied or not

        WindowProcess(DWORD pid, DWORD tid, std::wstring name);         // Initializes a new instance of the <see cref="WindowProcess"/> class
        WindowProcess();
                                                                                   /// Gets a value indicating whether this is the shell process or not
        /// The shell process (like explorer.exe) hosts parts of the user interface (like taskbar, start menu, ...)     
        bool IsShellProcess();
        bool DoesExist();           // Gets a value indicating whether the process exists on the machine   --ASK  
        void UpdateProcessInfo(DWORD processId, DWORD tid, std::wstring name);    /// Updates the process information of the <see cref="WindowProcess"/> instance.
        static DWORD GetProcessIDFromWindowHandle(HWND hwnd);                            /// Gets the process ID for the window handle
        static DWORD GetThreadIDFromWindowHandle(HWND hwnd);                             /// Gets the thread ID for the window handle
        static std::wstring GetProcessNameFromProcessID(DWORD processId);                /// Gets the process name for the process ID
        void KillThisProcess(bool killProcessTree);            /// Kills the process by it's id. If permissions are required, they will be requested.

    private:
        //Maximum size of a file name  
        unsigned int const maximumFileNameLength = 256;
        bool isUwpApp;        //An indicator if the window belongs to an 'Universal Windows Platform (UWP)' process


        bool TestProcessAccessUsingAllAccessFlag(DWORD processId);           /// Gets a boolean value indicating whether the access to a process using the AllAccess flag is denied or not.
    };
}