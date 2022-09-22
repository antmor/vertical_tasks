#include "pch.h"
#include "MainWindow.xaml.h"
#include "OpenWindows.h"
#include "Window.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace winrt::vertical_tasks::implementation;


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

    void MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));

        OpenWindows::GetInstance().UpdateOpenWindowsList();
        std::vector<Window>& g_windows = OpenWindows::GetInstance().getAllOpenWindows();

        std::vector<IInspectable> newTitles;
        newTitles.reserve(g_windows.size());

        for (auto&& window : g_windows)
        {
            std::wstring title = window.processInfo->name;
            if (!title.empty())
            {
                newTitles.emplace_back(winrt::box_value(title));
            }
        }
        m_windowTitles.ReplaceAll(newTitles);
    }

    Windows::Foundation::Collections::IObservableVector<IInspectable> MainWindow::WindowTitles()
    {
        return m_windowTitles;
    }

}
