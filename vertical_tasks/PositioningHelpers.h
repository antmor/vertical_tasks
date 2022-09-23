#pragma once
#include "pch.h"
#include "shellscalingapi.h"


template <typename T>
inline T ConvertDPI(HMONITOR mon, T value)
{
    UINT dpi_x, dpi_y;
    if (GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y) == S_OK)
    {
        return (value * dpi_x) / 96;
    }
    return value;
}

inline std::vector<RECT> GetSplits(RECT monitor, size_t count)
{
    const auto monWidth = monitor.right - monitor.left;
    const auto monHeight = monitor.bottom - monitor.top;

    const bool horizontal = (monWidth >= monHeight);

    if (count == 1)
    {
        return { monitor };
    }
    else if (count % 2 == 0)
    {
        const auto horizontalHalf1 = horizontal
            ? monWidth / 2
            : monWidth;
        const auto verticalHalf1 = horizontal
            ? 0
            : monHeight / 2;
        const auto horizontalHalf2 = horizontal
            ? monWidth / 2
            : 0;
        const auto verticalHalf2 = horizontal
            ? monHeight
            : monHeight / 2;


        auto firstSplit = GetSplits({ monitor.left, monitor.top, monitor.left + horizontalHalf1, monitor.top + verticalHalf2 }, count / 2);
        auto secondSplit = GetSplits({ monitor.left + horizontalHalf2, monitor.top + verticalHalf1, monitor.right, monitor.bottom }, count / 2);

        std::move(secondSplit.begin(), secondSplit.end(), std::back_inserter(firstSplit));
        return firstSplit;
    }
    else if ( (count == 3) && ((monWidth > monHeight * 2) || (monHeight > monWidth * 2)))
    {
        // ultrawide, special case
        // +--+--+--+
        // |  |  |  |
        // +--+--+--+
        const auto horizontalThird = horizontal
            ? monWidth / 3
            : 0;
        const auto verticalThird = horizontal
            ? 0
            : monHeight / 3;
        const auto horizontalThird2 = horizontal
            ? monWidth / 3
            : monWidth;
        const auto verticalThird2 = horizontal
            ? monHeight
            : monHeight / 3;

        const auto secondHorThird = horizontal
            ? 2 * horizontalThird
            : 0;
        const auto secondVerticalThird = horizontal
            ? 0
            : 2 * verticalThird;

        const auto secondHorThird2 = horizontal
            ? 2 * horizontalThird
            : monWidth;
        const auto secondVerticalThird2 = horizontal
            ? monHeight
            : 2 * verticalThird;

        return { { monitor.left, monitor.top, monitor.left + horizontalThird, monitor.top + verticalThird2 },
                { monitor.left + horizontalThird2, monitor.top + verticalThird, monitor.left + secondHorThird, monitor.top + secondVerticalThird2 },
                { monitor.left + secondHorThird2, monitor.top + secondVerticalThird, monitor.right, monitor.bottom } };
    }
    else
    {
        // do a split, but the second half will be split again.
        // i.e 
        // +--+--+
        // |  |--|
        // +--+--+
        auto splitIn2 = GetSplits({ monitor.left, monitor.top, monitor.right, monitor.bottom }, 2);
        auto firstSplit = GetSplits(splitIn2[0], (count / 2));
        auto secondSplit = GetSplits(splitIn2[1], (count / 2)+1);

        std::move(secondSplit.begin(), secondSplit.end(), std::back_inserter(firstSplit));
        return firstSplit;
    }
}
// FancyZonesWindowUtils::SizeWindowToRect
inline void SizeWindowToRect(HWND window, RECT rect) noexcept
{
    WINDOWPLACEMENT placement{};
    ::GetWindowPlacement(window, &placement);

    // Wait if SW_SHOWMINIMIZED would be removed from window (Issue #1685)
    for (int i = 0; i < 5 && (placement.showCmd == SW_SHOWMINIMIZED); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ::GetWindowPlacement(window, &placement);
    }

    // Do not restore minimized windows. We change their placement though so they restore to the correct zone.
    if ((placement.showCmd != SW_SHOWMINIMIZED) &&
        (placement.showCmd != SW_MINIMIZE))
    {
        placement.showCmd = SW_RESTORE;
    }

    // Remove maximized show command to make sure window is moved to the correct zone.
    if (placement.showCmd == SW_SHOWMAXIMIZED)
    {
        placement.showCmd = SW_RESTORE;
        placement.flags &= ~WPF_RESTORETOMAXIMIZED;
    }

    //ScreenToWorkAreaCoords(window, rect);

    placement.rcNormalPosition = rect;
    placement.flags |= WPF_ASYNCWINDOWPLACEMENT;

    LOG_IF_WIN32_BOOL_FALSE(::SetWindowPlacement(window, &placement));

    // Do it again, allowing Windows to resize the window and set correct scaling
    // This fixes Issue #365
    LOG_IF_WIN32_BOOL_FALSE(::SetWindowPlacement(window, &placement));
}

