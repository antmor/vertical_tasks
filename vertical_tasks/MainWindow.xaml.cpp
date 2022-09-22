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

#include <TaskVM.h>
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

    UINT g_shellHookMsgId{ UINT_MAX };
    bool g_initialized{ false };

    // returns true if window already existed
    winrt::vertical_tasks::TaskVM MainWindow::AddOrUpdateWindow(HWND hwnd, bool shouldUpdate)
    {
        if (IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)))
        {
            auto found= m_tasks->find(hwnd);

            if (found != m_tasks->end())
            {
                // todo call recalculate
                return found->as<winrt::vertical_tasks::TaskVM>();
            }
            else
            {
                winrt::vertical_tasks::TaskVM newItem(reinterpret_cast<uint64_t>(hwnd));
                if (shouldUpdate)
                {
                    m_tasks->Append(newItem);
                }
                else
                {
                    // don't update, add to the internal
                    m_tasks->get_container().push_back(newItem);
                }
                return newItem;
            }
        }
        return nullptr;
    }

    BOOL CALLBACK WindowEnumerationCallBack(HWND hwnd, LPARAM lParam)
    {
        auto thisRef = reinterpret_cast<MainWindow*>(lParam);

        thisRef->AddOrUpdateWindow(hwnd);
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
        auto found = m_tasks->find(hwnd);

        if (found != m_tasks->end())
        {
            //found->second.icon = std::move(iconCopy);
        }
        // TODO update icon in ViewModel
    }

    //int i = 0;
    void MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        if (!g_initialized)
        {
            myButton().Content(box_value(L"Clicked"));
            EnumWindows(&WindowEnumerationCallBack, reinterpret_cast<LPARAM>(this));
            
            m_tasks->do_call_changed(winrt::Windows::Foundation::Collections::CollectionChange::Reset, 0u);


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

    winrt::fire_and_forget MainWindow::OnItemClick(Windows::Foundation::IInspectable const& /*sender*/, Microsoft::UI::Xaml::Controls::ItemClickEventArgs const& /*args*/)
    {
        
        co_return;
    }

    winrt::fire_and_forget MainWindow::OnSelectionChanged(Windows::Foundation::IInspectable const& /*sender*/, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& /*args*/)
    {
        if (selectionFromShell)
        {
            // seleciton change from shell
            co_return;
        }

        std::vector<HWND> windowsToShow;
        {
            auto scope = selectionFromClick.onInScope();

            auto selection = myList().SelectedItems();
            for (auto&& item : selection)
            {
                auto& taskVM = item.as<vertical_tasks::implementation::TaskVM>();
                // select window
                windowsToShow.emplace_back(taskVM->Hwnd());
            }
        }
        co_await winrt::resume_background();
        for (auto&& hwnd : windowsToShow)
        {
            SetForegroundWindow(hwnd);
        
            if (!ShowWindow(hwnd, SW_RESTORE))
            {
                // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
                SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
        }
    }


    void MainWindow::SelectItem(HWND hwnd)
    {
        if (selectionFromClick)
        {
            // we caused the selection, so ignore it. 
            return;
        }
        auto scope = selectionFromShell.onInScope();
        auto found = m_tasks->find(hwnd);

        if (found != m_tasks->end())
        {
            myList().SelectedItem(*found);
        }
        else
        {
            LOG_HR(E_ACCESSDENIED);
        }
    }

    void  MainWindow::DeleteItem(HWND hwnd)
    {
        auto found = m_tasks->find(hwnd);

        if (found != m_tasks->end())
        {
            const auto indexToErase{ std::distance(m_tasks->begin(), found) };
            m_tasks->get_container().erase(found);
            m_tasks->do_call_changed(Windows::Foundation::Collections::CollectionChange::ItemRemoved, indexToErase);

        }
        else
        {
            LOG_HR(E_ACCESSDENIED);
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
            auto&& added = AddOrUpdateWindow(reinterpret_cast<HWND>(lParam), true /*send change update*/);
            if (added)
            {
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
        }
        myString << std::endl;
        OutputDebugString(myString.str().c_str());

    }

}
