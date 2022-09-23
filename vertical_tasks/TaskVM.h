#pragma once
#include <pch.h>

#include "TaskVM.g.h"


namespace winrt::vertical_tasks::implementation
{
    struct TaskVM : TaskVMT<TaskVM>
    {
        TaskVM(HWND hwnd, winrt::Microsoft::UI::Dispatching::DispatcherQueue uiThread): m_hwnd(reinterpret_cast<HWND>(hwnd)), m_uiThread(uiThread)
        {
            RefreshTitle(false);
        }

        hstring Title() const
        {
            return m_title;
        };
        
        winrt::Microsoft::UI::Xaml::Controls::IconSource IconSource() const
        {
            return m_iconSource;
        };

        void Select();
        void Close();

        winrt::fire_and_forget RefreshTitle(bool update = true);


        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
        {
            return m_propertyChanged.add(handler);
        };
        void PropertyChanged(winrt::event_token const& token) noexcept
        {
            m_propertyChanged.remove(token);
        };

        HWND Hwnd() const
        {
            return m_hwnd;
        }
    private:
        HWND m_hwnd;
        winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiThread{ nullptr };

        winrt::hstring m_title;
        wil::unique_hicon m_icon;
        winrt::Microsoft::UI::Xaml::Controls::IconSource m_iconSource{nullptr};


        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        void OnPropertyChanged(winrt::hstring propertyName)
        {
            m_propertyChanged(*this, winrt::Windows::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
        }
    };
}
