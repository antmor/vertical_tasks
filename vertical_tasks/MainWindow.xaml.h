#pragma once
#include "MainWindow.g.h"

namespace winrt::vertical_tasks::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void myButton_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        Windows::Foundation::Collections::IObservableVector<IInspectable> WindowTitles();

    private:
        winrt::fire_and_forget OnShellMessage(WPARAM wParam, LPARAM lParam);
        void SelectItem(HWND hwnd);
        void DeleteItem(HWND hwnd);
        Windows::Foundation::Collections::IObservableVector<IInspectable> m_windowTitles{winrt::single_threaded_observable_vector<IInspectable>()};
    };
}

namespace winrt::vertical_tasks::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
