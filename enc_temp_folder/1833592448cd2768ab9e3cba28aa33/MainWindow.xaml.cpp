#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

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

                g_windows.emplace_back(WinData{ hwnd, std::wstring(std::begin(buffer), std::end(buffer)) });
            }
        }
        // keep on looping
        return true;
    }

    void MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));
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
    }

    Windows::Foundation::Collections::IObservableVector<IInspectable> MainWindow::WindowTitles()
    {
        return m_windowTitles;
    }

}
