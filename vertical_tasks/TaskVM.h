#pragma once
#include <pch.h>

#include "TaskVM.g.h"


namespace winrt::vertical_tasks::implementation
{
    struct TaskVM : TaskVMT<TaskVM>
    {
        TaskVM() = default;

        TaskVM(uint64_t hwnd, const winrt::vertical_tasks::GroupId groupId, const bool isGroup): 
            m_hwnd(reinterpret_cast<HWND>(hwnd)),
            m_isGroupHeader(isGroup),
            m_groupId(groupId)
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

        bool IsGroupedTask()
        {
            return !m_isGroupHeader && m_groupId != winrt::vertical_tasks::GroupId::Ungrouped;
        }

        hstring Title()
        {
            return m_title;
        };

        hstring Spacing()
        {
            return L"       ";
        }
        
        winrt::Microsoft::UI::Xaml::Controls::IconSource IconSource()
        {
            return m_iconSource;
        };

        void Select();
        void Close();

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

        bool isGroupHeader()
        {
            return m_isGroupHeader;
        }
    private:
        HWND m_hwnd;
        bool m_isGroupHeader;
        winrt::vertical_tasks::GroupId m_groupId;
        winrt::hstring m_title;
        wil::unique_hicon m_icon;
        winrt::Microsoft::UI::Xaml::Controls::IconSource m_iconSource{nullptr};
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;

    };
}
namespace winrt::vertical_tasks::factory_implementation
{
    struct TaskVM : TaskVMT<TaskVM, implementation::TaskVM>
    {
    };
}
