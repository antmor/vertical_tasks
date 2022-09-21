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
        std::wstring title;
        wil::unique_hicon icon;
        bool inView{ false };
    };

    std::unordered_map<HWND, WinData> g_windows;
    UINT g_shellHookMsgId{ UINT_MAX };
    bool g_initialized{ false };

    // returns true if window already existed
    decltype(g_windows)::iterator AddOrUpdateWindow(HWND hwnd)
    {
        if (IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)))
        {
            std::wstring title;
            const auto size = GetWindowTextLength(hwnd);
            if (size > 0)
            {
                std::vector<wchar_t> buffer(size + 1);
                GetWindowText(hwnd, buffer.data(), size + 1);

                auto found{ g_windows.find(hwnd) };

                if (found != g_windows.end())
                {
                    auto&& [_, winData] = *found;
                    auto oldTitle = std::move(winData.title);
                    winData.title = std::wstring(std::begin(buffer), std::end(buffer));
                    winData.inView = true;
                    return found;
                }
                else
                {
                    return g_windows.emplace(hwnd, WinData{ std::wstring(std::begin(buffer), std::end(buffer)) }).first;
                }
            }
        }
        return g_windows.end();
    }

    BOOL CALLBACK WindowEnumerationCallBack(HWND hwnd, LPARAM /*lParam*/)
    {
        AddOrUpdateWindow(hwnd);
        // keep on looping
        return true;
    }

    winrt::fire_and_forget MainWindow::FetchIcon(HWND hwnd)
    {
        co_await winrt::resume_background();

        HICON icon;
        SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG,
            500/*ms*/, reinterpret_cast<PDWORD_PTR>(&icon));
        if (!icon)
        {
            SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG,
                500/*ms*/, reinterpret_cast<PDWORD_PTR>(&icon));
        }
        wil::unique_hicon iconCopy(CopyIcon(icon));

        co_await wil::resume_foreground(DispatcherQueue());
        auto found{ g_windows.find(hwnd) };

        if (found != g_windows.end())
        {
            found->second.icon = std::move(iconCopy);
        }
        // TODO update icon in ViewModel
    }

    //int i = 0;
    void MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (!g_initialized)
        {
            myButton().Content(box_value(L"Clicked"));
            g_windows.clear();
            EnumWindows(&WindowEnumerationCallBack, 0);
            std::vector<IInspectable> newTitles;
            newTitles.reserve(g_windows.size());
            size_t index = 0;
            for (auto&& [hwnd, winData] : g_windows)
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
        else
        {
            /*g_windows[i].ShowWindow();
            i++;*/
        }
    }

    void MainWindow::SelectItem(HWND hwnd)
    {
        std::wstring_view titleToSelect;
        auto& found{ g_windows.find(hwnd) };

        if (found != g_windows.end())
        {
            titleToSelect = found->second.title;
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
        
        auto& found{ g_windows.find(hwnd) };

        if (found != g_windows.end())
        {
            titleToDelete = found->second.title;
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
        std::wstringstream myString;
        myString << L"shell message: " << std::hex << wParam << L", " << lParam;
        switch (wParam)
        {
        case HSHELL_WINDOWACTIVATED:
        case HSHELL_RUDEAPPACTIVATED:
        {
            SelectItem(reinterpret_cast<HWND>(lParam));
        }
            break;
        case HSHELL_WINDOWCREATED:
        {
            // add the window
            auto&& added = AddOrUpdateWindow(reinterpret_cast<HWND>(lParam));
            if (added != g_windows.end())
            {
                auto&& [_, data] = *added;
                if (!data.inView)
                {
                    m_windowTitles.Append(winrt::box_value(data.title));
                }
                SelectItem(reinterpret_cast<HWND>(lParam));

            }
        }
            break;
        case HSHELL_WINDOWDESTROYED:
        {
            DeleteItem(reinterpret_cast<HWND>(lParam));
        }
            break;
        default:
        {
            myString << L" ! UNKNOWN";
        }
            break;
        }noti
        myString << std::endl;
        OutputDebugString(myString.str().c_str());

    }

    Windows::Foundation::Collections::IObservableVector<IInspectable> MainWindow::WindowTitles()
    {
        return m_windowTitles;
    }

}
