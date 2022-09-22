#pragma once
#include "pch.h"
#include "Window.h"
#include "Helper.h"

namespace winrt::vertical_tasks::implementation
{
    

    struct OpenWindows
    {
        std::vector<Window>& getAllOpenWindows();
        void UpdateOpenWindowsList();
        static OpenWindows& GetInstance();


        // OpenWindows(OpenWindows const&) = delete;
        // OpenWindows(OpenWindows&&) = delete; 

        static BOOL CALLBACK EnumWindow(HWND hwnd, LPARAM lParam);
        bool EnumWindow(HWND hwnd);

    private:
        OpenWindows()=default;
        std::vector<Window> windows{};

    };
}