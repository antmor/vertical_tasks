#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <microsoft.ui.xaml.window.h>
#include <dispatcherqueue.h>

#include <winrt\microsoft.ui.xaml.h>
#include <winrt\windows.ui.core.h>
#include <winrt\microsoft.ui.h>
#include <winrt\microsoft.ui.windowing.h>
#include <winrt\microsoft.ui.interop.h>
#include <wil\cppwinrt_helpers.h>

#include <sstream>

#include <TaskVM.h>

#include "PositioningHelpers.h"


namespace winrt
{
    using namespace Microsoft::UI::Xaml;
}
// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info

inline BOOL IsToolWindow(HWND m_hwnd)
{
    return (GetWindowLong(m_hwnd, GWL_EXSTYLE) &
        WS_EX_TOOLWINDOW) ==
        WS_EX_TOOLWINDOW;
}

/// Gets a value indicating whether the window is an appwindow
inline BOOL IsAppWindow(HWND m_hwnd)
{
    return (GetWindowLong(m_hwnd, GWL_EXSTYLE) &
        WS_EX_APPWINDOW) == WS_EX_APPWINDOW;
}

inline BOOL TaskListDeleted(HWND m_hwnd)
{
    return GetProp(m_hwnd, L"ITaskList_Deleted") != NULL;
}

namespace winrt::vertical_tasks::implementation
{
    winrt::MUCSB::SystemBackdropTheme ConvertToSystemBackdropTheme(
        winrt::MUX::ElementTheme const& theme)
    {
        switch (theme)
        {
        case winrt::MUX::ElementTheme::Dark:
            return winrt::MUCSB::SystemBackdropTheme::Dark;
        case winrt::MUX::ElementTheme::Light:
            return winrt::MUCSB::SystemBackdropTheme::Light;
        default:
            return winrt::MUCSB::SystemBackdropTheme::Default;
        }
    }

    winrt::WS::DispatcherQueueController CreateSystemDispatcherQueueController()
    {
        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_NONE
        };

        ::ABI::Windows::System::IDispatcherQueueController* ptr{ nullptr };
        winrt::check_hresult(CreateDispatcherQueueController(options, &ptr));
        return { ptr, take_ownership_from_abi };
    }

    MainWindow::MainWindow()
    {
        InitializeComponent();
        auto windowNative{ this->try_as<::IWindowNative>() };
        winrt::check_bool(windowNative);
        windowNative->get_WindowHandle(&m_hwnd);

        if (winrt::Microsoft::UI::Windowing::AppWindowTitleBar::IsCustomizationSupported())
        {
            winrt::Microsoft::UI::WindowId windowId =
                winrt::Microsoft::UI::GetWindowIdFromWindow(m_hwnd);

            // Lastly, retrieve the AppWindow for the current (XAML) WinUI 3 window.
            winrt::Microsoft::UI::Windowing::AppWindow appWindow =
                winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);


            if (appWindow)
            {
                // You now have an AppWindow object, and you can call its methods to manipulate the window.
                // As an example, let's change the title text of the window.
                appWindow.TitleBar().ExtendsContentIntoTitleBar(true);
            }
        }

        m_mon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONULL);
        MONITORINFOEXW monitorInfo{ sizeof(MONITORINFOEXW) };
        THROW_IF_WIN32_BOOL_FALSE(GetMonitorInfoW(m_mon, &monitorInfo));
        auto sideBarRect = monitorInfo.rcWork;
        m_left = ConvertDPI(m_mon, 300);
        sideBarRect.right = sideBarRect.left + m_left;
        SizeWindowToRect(m_hwnd, sideBarRect);

        if (winrt::MUCSB::MicaController::IsSupported())
        {
            // We ensure that there is a Windows.System.DispatcherQueue on the current thread.
                     // Always check if one already exists before attempting to create a new one.
            if (nullptr == winrt::WS::DispatcherQueue::GetForCurrentThread() &&
                nullptr == m_dispatcherQueueController)
            {
                m_dispatcherQueueController = CreateSystemDispatcherQueueController();
            }

            // Setup the SystemBackdropConfiguration object.
            SetupSystemBackdropConfiguration();

            // Setup Mica on the current Window.
            m_backdropController = winrt::MUCSB::MicaController();
            m_backdropController.SetSystemBackdropConfiguration(m_configuration);
            m_backdropController.AddSystemBackdropTarget(
                this->try_as<winrt::MUC::ICompositionSupportsSystemBackdrop>());

            m_closedRevoker = this->Closed(winrt::auto_revoke, [&](auto&&, auto&&)
                {
                    if (nullptr != m_backdropController)
                    {
                        m_backdropController.Close();
                        m_backdropController = nullptr;
                    }

                    if (nullptr != m_dispatcherQueueController)
                    {
                        m_dispatcherQueueController.ShutdownQueueAsync();
                        m_dispatcherQueueController = nullptr;
                    }
                });
        }
    }

    void MainWindow::SetupSystemBackdropConfiguration()
    {
        m_configuration = winrt::MUCSB::SystemBackdropConfiguration();

        // Activation state.
        m_activatedRevoker = this->Activated(winrt::auto_revoke,
            [&](auto&&, MUX::WindowActivatedEventArgs const& args)
            {
                m_configuration.IsInputActive(
                    winrt::MUX::WindowActivationState::Deactivated != args.WindowActivationState());
            });

        // Initial state.
        m_configuration.IsInputActive(true);

        // Application theme.
        m_rootElement = this->Content().try_as<winrt::MUX::FrameworkElement>();
        if (nullptr != m_rootElement)
        {
            m_themeChangedRevoker = m_rootElement.ActualThemeChanged(winrt::auto_revoke,
                [&](auto&&, auto&&)
                {
                    m_configuration.Theme(
                        ConvertToSystemBackdropTheme(m_rootElement.ActualTheme()));
                });

            // Initial state.
            m_configuration.Theme(
                ConvertToSystemBackdropTheme(m_rootElement.ActualTheme()));
        }
    }

    UINT g_shellHookMsgId{ UINT_MAX };
    bool g_initialized{ false };

    // returns true if window already existed
    winrt::vertical_tasks::TaskVM MainWindow::AddOrUpdateWindow(HWND hwnd, bool shouldUpdate)
    {
        if ((hwnd != m_hwnd) && IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)) &&
            (!IsToolWindow(hwnd) || IsAppWindow(hwnd)) && !TaskListDeleted(hwnd))
        {
            auto found= m_tasks->find(hwnd);

            if (found != m_tasks->end())
            {
                // todo call recalculate
                return found->as<winrt::vertical_tasks::TaskVM>();
            }
            else
            {
                auto newTask = winrt::make<winrt::vertical_tasks::implementation::TaskVM>(hwnd, DispatcherQueue());
                auto newItem = newTask.as<winrt::vertical_tasks::TaskVM>();
                if (shouldUpdate)
                {
                    m_tasks->get_container().push_back(newItem);
                    m_tasks->sort();


                }
                else
                {
                    // don't update, add to the internal list
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
            
            m_tasks->sort();

            m_shellHook = std::make_unique<ShellHookMessages>();
            m_shellHook->Register([weak_this = get_weak()](WPARAM wParam, LPARAM lParam)
                {
                    if (auto strong = weak_this.get())
                    {
                        strong->OnShellMessage(wParam, lParam);
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
                auto taskVM = item.as<vertical_tasks::implementation::TaskVM>();
                // select window
                windowsToShow.emplace_back(taskVM->Hwnd());
            }
        }
        co_await winrt::resume_background();

        if (windowsToShow.size() == 0)
        {
        }
        else if(windowsToShow.size() == 1)
        {
            SetForegroundWindow(windowsToShow[0]);

            if (!ShowWindow(windowsToShow[0], SW_RESTORE))
            {
                // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
                SendMessage(windowsToShow[0], WM_SYSCOMMAND, SC_RESTORE, 0);
            }
        }
        else
        {
            auto mon = MonitorFromWindow(windowsToShow[0], MONITOR_DEFAULTTONULL);
            MONITORINFOEXW monitorInfo{ sizeof(MONITORINFOEXW) };
            THROW_IF_WIN32_BOOL_FALSE(GetMonitorInfoW(mon, &monitorInfo));

            if (mon == m_mon)
            {
                monitorInfo.rcWork.left += m_left;
            }
            const auto rects = GetSplits(monitorInfo.rcWork, windowsToShow.size());
            size_t i = 0;
            for (auto&& hwnd : windowsToShow)
            {
                SetForegroundWindow(hwnd);

                if (!ShowWindow(hwnd, SW_RESTORE))
                {
                    // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
                    SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                }
                SizeWindowToRect(hwnd, rects[i++]);
            }
        }
        co_return;
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

    void MainWindow::DeleteItem(HWND hwnd)
    {
        auto found = m_tasks->find(hwnd);

        if (found != m_tasks->end())
        {
            const auto indexToErase{ std::distance(m_tasks->begin(), found) };
            m_tasks->get_container().erase(found);
            m_tasks->do_call_changed(Windows::Foundation::Collections::CollectionChange::ItemRemoved, 
                static_cast<uint32_t>(indexToErase));

        }
        else
        {
            LOG_HR(E_ACCESSDENIED);
        }
    }

    void MainWindow::RenameItem(HWND hwnd)
    {
        auto found = m_tasks->find(hwnd);

        if (found != m_tasks->end())
        {
            auto taskVM = found->as<vertical_tasks::implementation::TaskVM>();
            taskVM->RefreshTitle();
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
        myString << L"shell message: " << wParam << L", " << std::hex <<  lParam;
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
        case HSHELL_REDRAW: 
        {
            // todo recalculate text and icon
            RenameItem(reinterpret_cast<HWND>(lParam));
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
