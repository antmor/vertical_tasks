#pragma once
#include "MainWindow.g.h"
#include <winrt\microsoft.ui.xaml.h>
#include "TaskVM.h"
#include <functional>
#include "ShellHookMessages.h"

#include <winrt\windows.system.h>
#include <winrt\microsoft.ui.composition.h>
#include <winrt\Microsoft.UI.Composition.SystemBackdrops.h>

namespace winrt
{
    namespace MUC = Microsoft::UI::Composition;
    namespace MUCSB = Microsoft::UI::Composition::SystemBackdrops;
    namespace MUX = Microsoft::UI::Xaml;
    namespace WS = Windows::System;
};

namespace winrt::vertical_tasks::implementation
{
    using namespace Windows::Foundation::Collections;
    struct MyTasks : public implements<MyTasks, IObservableVector<IInspectable>, IVector<IInspectable>, IVectorView<IInspectable>, IIterable<IInspectable>>,
        winrt::observable_vector_base<MyTasks, IInspectable>
    {
        auto& get_container() const noexcept
        {
            return m_values;
        }

        auto& get_container() noexcept
        {
            return m_values;
        }

        auto begin()
        {
            return get_container().begin();

        }
        auto end()
        {
            return get_container().end();
        }

        auto find(HWND hwnd)
        {
            return std::find_if(begin(), end(), [hwnd](IInspectable& other)
                {
                    return hwnd == other.as<vertical_tasks::implementation::TaskVM>()->Hwnd();
                });
        }

        auto find(winrt::hstring title)
        {
            return std::find_if(begin(), end(), [title](IInspectable& other)
                {
                    return title == other.as<vertical_tasks::implementation::TaskVM>()->Title();
                });
        }

        void sort()
        {
            // resort
            std::sort(begin(), end(),
                [](const auto& l, const auto& r)
                {
                    auto lt = l.as<winrt::vertical_tasks::implementation::TaskVM>();
                    auto rt = r.as<winrt::vertical_tasks::implementation::TaskVM>();

                    int lGroup = static_cast<int>(lt->Group());
                    int rGroup = static_cast<int>(rt->Group());
                    if (lGroup != rGroup)
                    {
                        return lGroup < rGroup;
                    }
                    // same group, return the header first
                    if (lt->IsGroupId() || rt->IsGroupId())
                    { 
                        return lt->IsGroupId();
                    }
                    // TODO index in group
                    auto ls = l.as<winrt::vertical_tasks::implementation::TaskVM>()->ProcessName();
                    auto rs = r.as<winrt::vertical_tasks::implementation::TaskVM>()->ProcessName();
                    return CSTR_LESS_THAN == CompareStringOrdinal(ls.data(), static_cast<int>(ls.size()), rs.data(), static_cast<int>(rs.size()), TRUE);
                });
            call_changed(Windows::Foundation::Collections::CollectionChange::Reset, 0u);
        }

        void do_call_changed(Windows::Foundation::Collections::CollectionChange const change, uint32_t const index)
        {
            call_changed(change, index);
        }

        u_int m_taskCount = 0;
    private:
        std::vector<IInspectable> m_values{ };
    };

    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow();

        void myButton_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        winrt::fire_and_forget OnItemClick(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Controls::ItemClickEventArgs const& args);
        winrt::fire_and_forget OnSelectionChanged(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);

        IObservableVector<IInspectable> Tasks() 
        { 
            return m_tasks.as< winrt::Windows::Foundation::Collections::IObservableVector<IInspectable>>(); 
        };
        std::map <winrt::vertical_tasks::TaskVM, winrt::Windows::Foundation::Collections::IObservableVector<winrt::vertical_tasks::TaskVM>> TasksByGroup()
        {
            return m_tasksByGroup;
        };

        // This must always exist in the m_tasksByGroup map but will not be apart of m_tasks, as it does not show up in the taskbar - it's just to help
        // organize the ungrouped tasks
        winrt::vertical_tasks::TaskVM UngroupedTasksHeader()
        {
            return m_ungroupedTaskHeader;
        };

        winrt::vertical_tasks::TaskVM AddOrUpdateWindow(HWND hwnd, bool shouldUpdate = false);
        void SelectItem(HWND hwnd);
        void DeleteItem(HWND hwnd);

        void TaskClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AddGroup(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        void RenameItem(HWND hwnd);

    private:
        Windows::Foundation::IAsyncOperation<Windows::Graphics::Imaging::SoftwareBitmap> GetBitmapFromIconFileAsync(wil::unique_hicon hicon);
        winrt::fire_and_forget OnShellMessage(WPARAM wParam, LPARAM lParam);
        winrt::fire_and_forget FetchIcon(HWND hwnd);

        winrt::com_ptr<MyTasks> m_tasks{ winrt::make_self<MyTasks>() };
        winrt::vertical_tasks::TaskVM m_ungroupedTaskHeader = (winrt::make<winrt::vertical_tasks::implementation::TaskVM>(nullptr,
            DispatcherQueue(), winrt::vertical_tasks::GroupId::Ungrouped, true, 0)).as<winrt::vertical_tasks::TaskVM>();
        std::map<winrt::vertical_tasks::TaskVM, winrt::Windows::Foundation::Collections::IObservableVector<winrt::vertical_tasks::TaskVM>> m_tasksByGroup;

        struct scope_toggle
        {
            operator bool()
            {
                return toggle;
            }
            auto onInScope()
            {
                toggle = true;
                return wil::scope_exit([this]() {
                    toggle = false; });
            }
        private: 
            bool toggle{ false };

        };
        
        HWND m_hwnd;
        HMONITOR m_mon;
        long m_left;
        std::unique_ptr<ShellHookMessages> m_shellHook;
        scope_toggle selectionFromShell;
        scope_toggle selectionFromClick;

        winrt::MUCSB::SystemBackdropConfiguration m_configuration{ nullptr };
        winrt::MUCSB::MicaController m_backdropController{ nullptr };
        winrt::MUX::Window::Activated_revoker m_activatedRevoker;
        winrt::MUX::Window::Closed_revoker m_closedRevoker;
        winrt::MUX::FrameworkElement::ActualThemeChanged_revoker m_themeChangedRevoker;
        winrt::MUX::FrameworkElement m_rootElement{ nullptr };
        winrt::WS::DispatcherQueueController m_dispatcherQueueController{ nullptr };
        void SetupSystemBackdropConfiguration();
    };
}

namespace winrt::vertical_tasks::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
