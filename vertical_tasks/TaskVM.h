#pragma once
#include <pch.h>

#include "TaskVM.g.h"


namespace winrt::vertical_tasks::implementation
{
    struct TaskVM : TaskVMT<TaskVM>
    {
        TaskVM() = default;

        TaskVM(uint64_t hwnd): m_hwnd(reinterpret_cast<HWND>(hwnd))
        {
            const auto size = GetWindowTextLength(m_hwnd);
            if (size > 0)
            {
                std::vector<wchar_t> buffer(size + 1);
                GetWindowText(m_hwnd, buffer.data(), size + 1);

                m_title = std::wstring_view(buffer.data(), buffer.size());
            }
            else
            {
                LOG_HR(E_INVALIDARG);
            }
        }

        hstring Title()
        {
            return m_title;
        };
        
        winrt::Microsoft::UI::Xaml::Controls::IconSource IconSource()
        {
            return m_iconSource;
        };

        void Select();
        void Close();
        void Minimize();

        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
        {
            return m_propertyChanged.add(handler);
        };
        void PropertyChanged(winrt::event_token const& token) noexcept
        {
            m_propertyChanged.remove(token);
        };

        HWND Hwnd()
        {
            return m_hwnd;
        }
    private:
        HWND m_hwnd;
        winrt::hstring m_title;
        wil::unique_hicon m_icon;
        winrt::Microsoft::UI::Xaml::Controls::IconSource m_iconSource{nullptr};
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        void SwitchToWindow();

    };
}
namespace winrt::vertical_tasks::factory_implementation
{
    struct TaskVM : TaskVMT<TaskVM, implementation::TaskVM>
    {
    };
}
