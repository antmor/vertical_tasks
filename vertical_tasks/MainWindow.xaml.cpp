#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <microsoft.ui.xaml.window.h>

#include <winrt\microsoft.ui.xaml.h>
#include <winrt\windows.ui.core.h>
#include <wil\cppwinrt_helpers.h>

#include "ShellHookMessages.h"

#include <sstream>

namespace winrt
{
    using namespace Microsoft::UI::Xaml;
}
// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::vertical_tasks::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
    struct WinData
    {
        HWND hwnd;
        std::wstring title;

    };

    std::vector<WinData> g_windows;
    UINT g_shellHookMsgId{ UINT_MAX };
    bool g_initialized{ false };

    BOOL CALLBACK WindowEnumerationCallBack(HWND hwnd, LPARAM /*lParam*/)
    {
        if (IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)))
        {
            std::wstring title;
            const auto size = GetWindowTextLength(hwnd);
            if (size > 0)
            {
                std::vector<wchar_t> buffer(size + 1);
                GetWindowText(hwnd, buffer.data(), size + 1);

                bool found{ false };
                for (auto&& winData : g_windows)
                {
                    if (winData.hwnd == hwnd)
                    {
                        auto oldTitle = std::move(winData.title);
                        winData.title = std::wstring(std::begin(buffer), std::end(buffer));
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    g_windows.emplace_back(WinData{ hwnd, std::wstring(std::begin(buffer), std::end(buffer)) });
                }
            }
        }
        // keep on looping
        return true;
    }

    void MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (!g_initialized)
        {
            myButton().Content(box_value(L"Clicked"));
            g_windows.clear();
            EnumWindows(&WindowEnumerationCallBack, 0);
            std::vector<IInspectable> newTitles;
            newTitles.reserve(g_windows.size());

            for (auto&& winData : g_windows)
            {
                if (!winData.title.empty())
                {
                    newTitles.emplace_back(winrt::box_value(winData.title));
                }
            }
            m_windowTitles.ReplaceAll(newTitles);
            static ShellHookMessages s_myMessages;
            s_myMessages.Register([weak_this = get_weak()](WPARAM wParam, LPARAM lParam)
                {
                    auto strong_this = weak_this.get();
                    {
                        strong_this->OnShellMessage(wParam, lParam);
                    }
                });
            g_initialized = true;
        }
    }

    void MainWindow::SelectItem(HWND hwnd)
    {
        std::wstring_view titleToSelect;
        for (auto&& winData : g_windows)
        {
            if (winData.hwnd == hwnd)
            {
                titleToSelect = winData.title;
                break;
            }
        }

        if (!titleToSelect.empty())
        {
            for (auto&& item : m_windowTitles)
            {
                auto&& title = winrt::unbox_value<hstring>(item);
                if (title == titleToSelect)
                {
                    myList().SelectedItem(item);
                    return;
                }
            }
        }
    }

    void  MainWindow::DeleteItem(HWND hwnd)
    {
        std::wstring_view titleToDelete;
        for (auto&& winData : g_windows)
        {
            if (winData.hwnd == hwnd)
            {
                titleToDelete = winData.title;
                break;
            }
        }

        if (!titleToDelete.empty())
        {
            for (uint32_t i = 0; i < m_windowTitles.Size(); i++)
            {
                auto&& title = winrt::unbox_value<hstring>(m_windowTitles.GetAt(i));
                if (title == titleToDelete)
                {
                    m_windowTitles.RemoveAt(i);
                    break;
                }
            }
        }
    }

    winrt::fire_and_forget MainWindow::OnShellMessage(WPARAM wParam, LPARAM lParam)
    {
        co_await wil::resume_foreground(DispatcherQueue());
        OutputDebugString(L"shell message");
        std::wstringstream myString;
        myString << std::hex << wParam << L", " << lParam << std::endl;
        OutputDebugString(myString.str().c_str());
        switch (wParam)
        {
        case HSHELL_WINDOWACTIVATED:
        case HSHELL_RUDEAPPACTIVATED:
            SelectItem(reinterpret_cast<HWND>(lParam));
            break;
        case HSHELL_WINDOWCREATED:
            // add the window
            WindowEnumerationCallBack(reinterpret_cast<HWND>(lParam), 0);
            SelectItem(reinterpret_cast<HWND>(lParam));
            break;
        case HSHELL_WINDOWDESTROYED:
            DeleteItem(reinterpret_cast<HWND>(lParam));
            break;
        }
    }

    Windows::Foundation::Collections::IObservableVector<IInspectable> MainWindow::WindowTitles()
    {
        return m_windowTitles;
    }

}
