#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "ImageHelper.h"

#include <microsoft.ui.xaml.window.h>
#include <dispatcherqueue.h>

#include <winrt/microsoft.ui.xaml.media.imaging.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <winrt\microsoft.ui.xaml.h>
#include <winrt\windows.ui.core.h>
#include <winrt\microsoft.ui.h>
#include <winrt\microsoft.ui.windowing.h>
#include <winrt\microsoft.ui.interop.h>
#include <wil\cppwinrt_helpers.h>

#include <sstream>
#include "winuser.h"

#include <TaskVM.h>

#include "PositioningHelpers.h"

#include <iostream>
#include <iterator>
#include <map>


namespace winrt
{
    using namespace Microsoft::UI::Xaml;
    using namespace Microsoft::UI::Xaml::Controls;
    using namespace Windows::Foundation;
    using namespace Windows::UI::Xaml::Media::Imaging;
    using namespace Windows::Storage::Streams;
    using namespace Windows::Graphics::Imaging;
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
            EnableMenuItem(GetSystemMenu(m_hwnd, FALSE), SC_CLOSE,
                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

                SetWindowLong(m_hwnd, GWL_STYLE,
                    GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
      
            
                SetWindowLong(m_hwnd, GWL_STYLE,
                    GetWindowLong(m_hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);

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
    bool g_ungroupedTasksHeaderAdded{ false };

    // returns true if window already existed
    winrt::vertical_tasks::TaskVM MainWindow::AddOrUpdateWindow(HWND hwnd, bool shouldUpdate)
    {
        if (!g_ungroupedTasksHeaderAdded)
        {
            m_tasksByGroup.insert({ m_ungroupedTaskHeader, winrt::single_threaded_observable_vector<winrt::vertical_tasks::TaskVM>()});
            g_ungroupedTasksHeaderAdded = true;
        }

        if ((hwnd != m_hwnd) && IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)) &&
            (!IsToolWindow(hwnd) || IsAppWindow(hwnd)) && !TaskListDeleted(hwnd))
        {
            auto found = m_tasks->find(hwnd);

            if (found != m_tasks->end())
            {
                // todo call recalculate
                return found->as<winrt::vertical_tasks::TaskVM>();
            }
            else
            {
                winrt::vertical_tasks::GroupId groupId = winrt::vertical_tasks::GroupId::Ungrouped;
                const int ungroupedTasks = m_tasksByGroup.at(m_ungroupedTaskHeader).Size();

                auto newTask = winrt::make<winrt::vertical_tasks::implementation::TaskVM>(hwnd, 
                    DispatcherQueue(), groupId, false, ungroupedTasks + 1);
                auto newItem = newTask.as<winrt::vertical_tasks::TaskVM>();

                m_tasksByGroup.at(m_ungroupedTaskHeader).Append(newItem);

                FetchIcon(hwnd);
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
        if (!icon)
        {
            co_return;
        }

        wil::unique_hicon iconCopy(CopyIcon(icon));
        auto bitmap = co_await GetBitmapFromIconFileAsync(std::move(iconCopy));
        if ((bitmap.BitmapPixelFormat() != BitmapPixelFormat::Bgra8) ||
            (bitmap.BitmapAlphaMode() != BitmapAlphaMode::Premultiplied))
        {
            bitmap = SoftwareBitmap::Convert(bitmap, BitmapPixelFormat::Bgra8, BitmapAlphaMode::Premultiplied);
        }
        co_await wil::resume_foreground(DispatcherQueue());

        Microsoft::UI::Xaml::Media::Imaging::SoftwareBitmapSource source{};
        co_await source.SetBitmapAsync(bitmap);
        co_await wil::resume_foreground(DispatcherQueue());

        auto found = m_tasks->find(hwnd);

        if (found != m_tasks->end())
        {
            found->as<winrt::vertical_tasks::TaskVM>().IconSource(source);
        }
    }

    //int i = 0;
    void MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        
        myButton().Content(box_value(L"Clicked"));
        m_tasks->Clear();
        EnumWindows(&WindowEnumerationCallBack, reinterpret_cast<LPARAM>(this));
        //m_tasks->do_call_changed(Windows::Foundation::Collections::CollectionChange::Reset, 0u);

        m_tasks->sort();
        if (!m_shellHook)
        { 
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
    }

    void MainWindow::AddGroup(winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        int groups = m_tasksByGroup.size();
        winrt::vertical_tasks::GroupId newGroupId = winrt::vertical_tasks::GroupId::GroupOne;

        switch (groups)
        {
        case 1:
        {
            newGroupId = winrt::vertical_tasks::GroupId::GroupOne;
            break;
        }
        case 2:
        {
            newGroupId = winrt::vertical_tasks::GroupId::GroupTwo;
            break;
        }
        case 3:
        {
            newGroupId = winrt::vertical_tasks::GroupId::GroupThree;
            break;
        }
        case 4:
        {
            newGroupId = winrt::vertical_tasks::GroupId::GroupFour;
            break;
        }
        }

        // Create new group and update tasks list and create list entry in tasks by group map
        auto newGroup = winrt::make<winrt::vertical_tasks::implementation::TaskVM>(nullptr,
            DispatcherQueue(), newGroupId, true, 0);
        newGroup.IsGroupedTask(false);

        m_tasks->get_container().insert(m_tasks->get_container().begin(), newGroup);
        m_tasksByGroup.insert({ newGroup, winrt::single_threaded_observable_vector<winrt::vertical_tasks::TaskVM>() });

        // Update menu item being added to group
        // TODO - should check to see if this task was in another group & remove it from the map if so 
        auto menuItem = sender.as< Microsoft::UI::Xaml::Controls::MenuFlyoutItem>();
        auto dataContext = menuItem.DataContext().as<winrt::vertical_tasks::TaskVM>();
        dataContext.Group(newGroupId);
        dataContext.GroupIndex(1);
        dataContext.IsGroupedTask(true);

        // Update new group's tasks in map
        m_tasksByGroup.at(newGroup).Append(dataContext);

        m_tasks->sort();
    }

    void MainWindow::TaskClick(winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        Microsoft::UI::Xaml::Data::ItemIndexRange deselectRange{ 0, Tasks().Size() };
        myList().DeselectRange(deselectRange);

        Microsoft::UI::Xaml::Controls::Button wrappingButton = sender.as<Microsoft::UI::Xaml::Controls::Button>();
        auto clickedTaskVM = wrappingButton.DataContext().as<vertical_tasks::implementation::TaskVM>();

        if (clickedTaskVM->IsGroupId())
        {
            auto groupedTasks = m_tasksByGroup.at(*clickedTaskVM);
            uint32_t groupedTasksSize = groupedTasks.Size() + 1;

            uint32_t index;
            if (Tasks().IndexOf(*clickedTaskVM, index))
            {
                Microsoft::UI::Xaml::Data::ItemIndexRange selectRange{ static_cast<int32_t>(index), groupedTasksSize };
                myList().SelectRange(selectRange);
            }
        }
        else
        {
            WINDOWPLACEMENT wPos;
            GetWindowPlacement(clickedTaskVM->Hwnd(), &wPos);

            if ((wPos.showCmd == SW_MINIMIZE) || (wPos.showCmd == SW_SHOWMINIMIZED))
            {
                SetForegroundWindow(clickedTaskVM->Hwnd());
                if (!ShowWindow(clickedTaskVM->Hwnd(), SW_RESTORE))
                {
                    // ShowWindow doesn't work if the process is running elevated: fallback to SendMessage
                    SendMessage(clickedTaskVM->Hwnd(), WM_SYSCOMMAND, SC_RESTORE, 0);
                }
            }
            else
            {
                clickedTaskVM->Minimize();
            }
        }
    }

    void MainWindow::MoveToGroup(winrt::Windows::Foundation::IInspectable const& sender,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        if (auto menuFlyoutItem = sender.as<Microsoft::UI::Xaml::Controls::MenuFlyoutItem>())
        {
            auto dataContext = menuFlyoutItem.DataContext().as<winrt::vertical_tasks::TaskVM>();

            winrt::vertical_tasks::GroupId currentGroup = dataContext.Group();
            winrt::vertical_tasks::GroupId newGroup = currentGroup;

            if (menuFlyoutItem.Text() == L"Group One")
            {
                newGroup = winrt::vertical_tasks::GroupId::GroupOne;
            }
            else if (menuFlyoutItem.Text() == L"Group Two")
            {
                newGroup = winrt::vertical_tasks::GroupId::GroupTwo;
            }
            else if (menuFlyoutItem.Text() == L"Group Three")
            {
                newGroup = winrt::vertical_tasks::GroupId::GroupThree;
            }
            else if (menuFlyoutItem.Text() == L"Group Four")
            {
                newGroup = winrt::vertical_tasks::GroupId::GroupFour;
            }

            if (newGroup != currentGroup)
            {
                dataContext.IsGroupedTask(true);
                dataContext.Group(newGroup);

                for (auto groupTask : m_tasksByGroup)
                {
                    uint32_t groupIndex;
                    if (groupTask.second.IndexOf(dataContext, groupIndex))
                    {
                        groupTask.second.RemoveAt(groupIndex);
                    }
                    if (groupTask.first.Group() == newGroup)
                    {
                        groupTask.second.Append(dataContext);
                    }
                }

                m_tasks->sort();
            }
        }
    }

    winrt::fire_and_forget MainWindow::OnSelectionChanged(Windows::Foundation::IInspectable const& /*sender*/, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& /*args*/)
    {
        if (selectionFromShell)
        {
            // seleciton change from shell
            co_return;
        }
		auto scope = selectionFromClick.onInScope();

        std::vector<HWND> windowsToShow;

        // auto scope = selectionFromClick.onInScope();

        auto selection = myList().SelectedItems();
        for (auto&& item : selection)
        {
            auto taskVM = item.as<vertical_tasks::implementation::TaskVM>();
            // select window
            if (taskVM->Hwnd())
            {
                windowsToShow.emplace_back(taskVM->Hwnd());
            }
        }
        
        co_await winrt::resume_background();

        if (windowsToShow.size() == 0)
        {
        }
        else if (windowsToShow.size() == 1)
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
			auto taskVM = found->as<vertical_tasks::implementation::TaskVM>();
            if (taskVM->ProcessName().find(L"devenv") != std::wstring::npos)
            {
                // skip visual studio. 
            }
            else
            {
                myList().SelectedItem(*found);
            }
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
        myString << L"shell message: " << wParam << L", " << std::hex << lParam;
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
    
    //Windows::Foundation::
    winrt::IAsyncOperation<Windows::Graphics::Imaging::SoftwareBitmap> MainWindow::GetBitmapFromIconFileAsync(wil::unique_hicon hicon)
    {
        wil::com_ptr<IWICImagingFactory> wicImagingFactory;
        THROW_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicImagingFactory)));

        wil::com_ptr<IWICBitmap> wicBitmap;
        THROW_IF_FAILED(wicImagingFactory->CreateBitmapFromHICON(hicon.get(), &wicBitmap));

        wil::com_ptr<IWICBitmapSource> wicBitmapSource;
        wicBitmap.query_to(&wicBitmapSource);
        THROW_HR_IF_NULL(E_UNEXPECTED, wicBitmapSource);

        wil::com_ptr<IStream> stream;
        THROW_IF_FAILED(GetStreamOfWICBitmapSource(wicImagingFactory.get(), wicBitmapSource.get(), GUID_ContainerFormatPng, &stream));
        
        winrt::IRandomAccessStream randomAccessStream{ nullptr };
        THROW_IF_FAILED(CreateRandomAccessStreamOverStream(stream.get(), BSOS_DEFAULT, winrt::guid_of<winrt::IRandomAccessStream>(), winrt::put_abi(randomAccessStream)));

        winrt::BitmapDecoder decoder = co_await winrt::BitmapDecoder::CreateAsync(randomAccessStream);
        co_return co_await decoder.GetSoftwareBitmapAsync();
    }
}
