#pragma once
#include <pch.h>

#include "TaskVM.g.h"
#include <wil\resource.h>
#include <dwmapi.h>

inline BOOL IsToolWindow(HWND m_hwnd)
{
    return (GetWindowLong(m_hwnd, GWL_EXSTYLE) &
        WS_EX_TOOLWINDOW) ==
        WS_EX_TOOLWINDOW;
}

/// Gets a value indicating whether the window is an appwindow
inline BOOL IsAppWindow(HWND m_hwnd)
{
    return (GetWindowLong(m_hwnd, GWL_EXSTYLE) &
        WS_EX_APPWINDOW) == WS_EX_APPWINDOW;
}

inline BOOL TaskListDeleted(HWND m_hwnd)
{
    return GetProp(m_hwnd, L"ITaskList_Deleted") != NULL;
}

namespace winrt::vertical_tasks::implementation
{
    struct TaskVM : TaskVMT<TaskVM>
    {
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
                case winrt::vertical_tasks::GroupId::GroupFour:
                {
                    m_title = L"Group Four";
                    break;
                }
                }
                OnPropertyChanged(L"IsGroupId");
            }
            else
            {
                RefreshTitleAndIcon();
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

        bool IsGroupId() const
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

        void GroupsAvailable(bool value)
        {
            m_groupsAvailable = value;
            OnPropertyChanged(L"GroupsAvailable");
        }

        bool GroupsAvailable() const
        {
            return m_groupsAvailable;
        }

        void IsGroupOneAvailable(bool value)
        {
            m_isGroupOneAvailable = value;
            OnPropertyChanged(L"IsGroupOneAvailable");
        }

        bool IsGroupOneAvailable() const
        {
            return m_isGroupOneAvailable;
        }

        void IsGroupTwoAvailable(bool value)
        {
            m_isGroupTwoAvailable = value;
            OnPropertyChanged(L"IsGroupTwoAvailable");
        }

        bool IsGroupTwoAvailable() const
        {
            return m_isGroupTwoAvailable;
        }

        void IsGroupThreeAvailable(bool value)
        {
            m_isGroupThreeAvailable = value;
            OnPropertyChanged(L"IsGroupThreeAvailable");
        }

        bool IsGroupThreeAvailable() const
        {
            return m_isGroupThreeAvailable;
        }

        void IsGroupFourAvailable(bool value)
        {
            m_isGroupFourAvailable = value;
            OnPropertyChanged(L"IsGroupFourAvailable");
        }

        bool IsGroupFourAvailable() const
        {
            return m_isGroupFourAvailable;
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

        winrt::fire_and_forget RefreshTitleAndIcon(bool update = true);

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

        static bool IsValidWindow(HWND hwnd)
        {
            DWORD cloakAttrib;
            DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloakAttrib, sizeof(cloakAttrib));
            bool isCloaked = cloakAttrib != 0;
            return IsWindow(hwnd) && IsWindowVisible(hwnd) && (0 == GetWindow(hwnd, GW_OWNER)) &&
                (!IsToolWindow(hwnd) || IsAppWindow(hwnd)) && !TaskListDeleted(hwnd) && !isCloaked;
        }
    private:

        bool m_isGroupHeader;
        bool m_isGroupedTask;
        bool m_groupsAvailable = false;

        bool m_isGroupOneAvailable = false;
        bool m_isGroupTwoAvailable = false;
        bool m_isGroupThreeAvailable = false;
        bool m_isGroupFourAvailable = false;

        winrt::vertical_tasks::GroupId m_groupId;

        HWND m_hwnd;
        std::wstring m_procName;
        winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiThread{ nullptr };

        winrt::hstring m_title;
        winrt::Microsoft::UI::Xaml::Media::Imaging::SoftwareBitmapSource m_iconSource{ nullptr };

        u_int m_groupIndex;

        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        void OnPropertyChanged(winrt::hstring propertyName)
        {
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
        }
    };
}
