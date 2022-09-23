#pragma once
#include "MainWindow.g.h"
#include <winrt\microsoft.ui.xaml.h>
#include "TaskVM.h"

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

        IObservableVector<IInspectable> Tasks() { return m_tasks.as< winrt::Windows::Foundation::Collections::IObservableVector<IInspectable>>(); };

        winrt::vertical_tasks::TaskVM AddOrUpdateWindow(HWND hwnd, bool shouldUpdate = false);
        void SelectItem(HWND hwnd);
        void DeleteItem(HWND hwnd);
        void TaskClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);


    private:
        winrt::fire_and_forget OnShellMessage(WPARAM wParam, LPARAM lParam);
        winrt::fire_and_forget FetchIcon(HWND hwnd);

        winrt::com_ptr<MyTasks> m_tasks{ winrt::make_self<MyTasks>() };
        
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

        scope_toggle selectionFromShell;
        scope_toggle selectionFromClick;

    };
}

namespace winrt::vertical_tasks::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
