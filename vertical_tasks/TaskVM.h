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
        
        Microsoft::UI::Xaml::Media::Imaging::SoftwareBitmapSource IconSource()
        {
            return m_iconSource;
        };

        void IconSource(Microsoft::UI::Xaml::Media::Imaging::SoftwareBitmapSource const& value)
        {
            m_iconSource = value;
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(winrt::hstring(L"IconSource")));
        }
        void Select();
        void Close();

        winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
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
        winrt::Microsoft::UI::Xaml::Media::Imaging::SoftwareBitmapSource m_iconSource{nullptr};
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;

    };
}
namespace winrt::vertical_tasks::factory_implementation
{
    struct TaskVM : TaskVMT<TaskVM, implementation::TaskVM>
    {
    };
}
