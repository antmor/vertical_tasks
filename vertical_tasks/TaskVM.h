#pragma once
#include <pch.h>

#include "TaskVM.g.h"
#include <wil\resource.h>


namespace winrt::vertical_tasks::implementation
{
    struct TaskVM : TaskVMT<TaskVM>
    {
        TaskVM(HWND hwnd, winrt::Microsoft::UI::Dispatching::DispatcherQueue uiThread): m_hwnd(reinterpret_cast<HWND>(hwnd)), m_uiThread(uiThread)
        {
            DWORD dwWindowProcessId;
            GetWindowThreadProcessId(hwnd, &dwWindowProcessId);
            wil::unique_handle handle ( OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwWindowProcessId));
            if (handle)
            {
                WCHAR buffer[MAX_PATH] = {};
                DWORD bufSize = MAX_PATH;
                if (QueryFullProcessImageName(handle.get(), 0, buffer, &bufSize))
                {
                    m_procName = std::wstring(std::begin(buffer), std::end(buffer));
                }
                RefreshTitle(false);
            }
        }

        hstring Title() const
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
            OnPropertyChanged(L"IconSource");
        }

        void Select();
        void Close();
        void Kill();
        void Minimize();

        winrt::fire_and_forget RefreshTitle(bool update = true);

        winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
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

        std::wstring_view ProcessName() const
        {
            return m_procName;
        }
    private:
        HWND m_hwnd;
        std::wstring m_procName;
        winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiThread{ nullptr };

        winrt::hstring m_title;
        wil::unique_hicon m_icon;

        winrt::Microsoft::UI::Xaml::Media::Imaging::SoftwareBitmapSource m_iconSource{nullptr};
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        void OnPropertyChanged(winrt::hstring propertyName)
        {
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
        }
    };
}
