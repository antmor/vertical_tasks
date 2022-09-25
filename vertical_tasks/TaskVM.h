#pragma once
#include <pch.h>

#include "TaskVM.g.h"
#include <wil\resource.h>

namespace winrt::vertical_tasks::implementation
{
    struct TaskVM : TaskVMT<TaskVM>
    {
        //static Windows::UI::Xaml::DependencyProperty IsGroupedTaskProperty() { return s_isGroupedTaskProperty; }

        TaskVM(HWND hwnd,
            winrt::Microsoft::UI::Dispatching::DispatcherQueue uiThread,
            const winrt::vertical_tasks::GroupId groupId,
            const bool isGroup,
            const u_int groupIndex) :
            m_hwnd(reinterpret_cast<HWND>(hwnd)),
            m_uiThread(uiThread),
            m_isGroupHeader(isGroup),
            m_isGroupedTask(!isGroup && groupId != winrt::vertical_tasks::GroupId::Ungrouped),
            m_groupId(groupId),
            m_groupIndex(groupIndex)
        {
            if (m_isGroupHeader)
            {
                switch (groupId)
                {
                case winrt::vertical_tasks::GroupId::GroupOne:
                {
                    m_title = L"Group One";
                    break;
                }
                case winrt::vertical_tasks::GroupId::GroupTwo:
                {
                    m_title = L"Group Two";
                    break;
                }
                case winrt::vertical_tasks::GroupId::GroupThree:
                {
                    m_title = L"Group Three";
                    break;
                }
                }

            }
            else
            {
                DWORD dwWindowProcessId;
                GetWindowThreadProcessId(hwnd, &dwWindowProcessId);
                wil::unique_handle handle(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwWindowProcessId));
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
        }

        winrt::vertical_tasks::GroupId Group()
        {
            return m_groupId;
        }

        void Group(winrt::vertical_tasks::GroupId id)
        {
            m_groupId = id;
        }

        bool IsGroupId()
        {
            return m_isGroupHeader;
        }

        bool IsTask()
        {
            return !m_isGroupHeader;
        }

        void IsGroupedTask(bool value)
        {
            m_isGroupedTask = value;
            OnPropertyChanged(L"IsGroupedTask");
        }

        bool IsGroupedTask() const
        {
            return m_isGroupedTask;
        }

        hstring Title() const
        {
            return m_title;
        };

        hstring Spacing()
        {
            return L"       ";
        }
        

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

        bool isGroupHeader()
        {
            return m_isGroupHeader;
        }

        std::wstring_view ProcessName() const
        {
            return m_procName;
        }

        u_int GroupIndex() const
        {
            return m_groupIndex;
        }

        void GroupIndex(u_int index)
        {
            m_groupIndex = index;
        }

    private:

        bool m_isGroupHeader;
        bool m_isGroupedTask;
        winrt::vertical_tasks::GroupId m_groupId;

        HWND m_hwnd;
        std::wstring m_procName;
        winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiThread{ nullptr };

        winrt::hstring m_title;
        wil::unique_hicon m_icon;

        u_int m_groupIndex;

        winrt::Microsoft::UI::Xaml::Media::Imaging::SoftwareBitmapSource m_iconSource{nullptr};
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        void OnPropertyChanged(winrt::hstring propertyName)
        {
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
        }
    };
}
