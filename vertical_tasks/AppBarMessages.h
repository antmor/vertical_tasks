#pragma once
#include <WinUser.h>

DWORD APPBAR_CALLBACK = (WM_USER + 0x01);

inline bool RegisterAccessBar(HWND hwndAccessBar, bool shouldRegister)
{
    APPBARDATA abd;

    // Specify the structure size and handle to the appbar. 
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hwndAccessBar;
    abd.uEdge = ABE_LEFT;

    if (shouldRegister)
    {
        // Provide an identifier for notification messages. 
        abd.uCallbackMessage = APPBAR_CALLBACK;

        // Register the appbar. 
        if (!SHAppBarMessage(ABM_NEW, &abd))
        {
            return false;
        }
    }
    else
    {
        // Unregister the appbar. 
        SHAppBarMessage(ABM_REMOVE, &abd);
    }

    return true;
}


struct AppBarLifetime
{
    AppBarLifetime(HWND hwnd) : m_hwnd(hwnd)
    {
        m_registerSuccess = RegisterAccessBar(hwnd, true);
    }

    // AppBarQuerySetPos - sets the size and position of an appbar. 
    // uEdge - screen edge to which the appbar is to be anchored 
    // lprc - current bounding rectangle of the appbar 
    // pabd - address of the APPBARDATA structure with the hWnd and cbSize members filled

    void PASCAL AppBarQuerySetPos(UINT uEdge, LPRECT lprc, PAPPBARDATA pabd)
    {
        int iHeight = 0;
        int iWidth = 0;

        pabd->rc = *lprc;
        pabd->uEdge = uEdge;

        // Copy the screen coordinates of the appbar's bounding 
        // rectangle into the APPBARDATA structure. 
        if ((uEdge == ABE_LEFT) || (uEdge == ABE_RIGHT))
        {
            iWidth = pabd->rc.right - pabd->rc.left;
            pabd->rc.top = 0;
            pabd->rc.bottom = GetSystemMetrics(SM_CYSCREEN);
        }
        else // unsupported... don't allow
        {
            iHeight = pabd->rc.bottom - pabd->rc.top;
            pabd->rc.left = 0;
            pabd->rc.right = GetSystemMetrics(SM_CXSCREEN);
        }

        // Query the system for an approved size and position. 
        SHAppBarMessage(ABM_QUERYPOS, pabd);

        // Adjust the rectangle, depending on the edge to which the appbar is anchored.
        switch (uEdge)
        {
        case ABE_LEFT:
            pabd->rc.right = pabd->rc.left + iWidth;
            break;

        case ABE_RIGHT:
            pabd->rc.left = pabd->rc.right - iWidth;
            break;

        case ABE_TOP:
            pabd->rc.bottom = pabd->rc.top + iHeight;
            break;

        case ABE_BOTTOM:
            pabd->rc.top = pabd->rc.bottom - iHeight;
            break;
        }

        // Pass the final bounding rectangle to the system. 
        SHAppBarMessage(ABM_SETPOS, pabd);

        // Move and size the appbar so that it conforms to the 
        // bounding rectangle passed to the system. 
        MoveWindow(pabd->hWnd,
            pabd->rc.left,
            pabd->rc.top,
            pabd->rc.right - pabd->rc.left,
            pabd->rc.bottom - pabd->rc.top,
            TRUE);

    }

    // AppBarPosChanged - adjusts the appbar's size and position. 

    // pabd - address of an APPBARDATA structure that contains information 
    //        used to adjust the size and position. 

    void PASCAL AppBarPosChanged(PAPPBARDATA pabd)
    {
        RECT rc;
        RECT rcWindow;
        int iHeight;
        int iWidth;

        rc.top = 0;
        rc.left = 0;
        rc.right = GetSystemMetrics(SM_CXSCREEN);
        rc.bottom = GetSystemMetrics(SM_CYSCREEN);

        GetWindowRect(pabd->hWnd, &rcWindow);

        iHeight = rcWindow.bottom - rcWindow.top;
        iWidth = rcWindow.right - rcWindow.left;

        switch (m_side)
        {
        case ABE_TOP:
            rc.bottom = rc.top + iHeight;
            break;

        case ABE_BOTTOM:
            rc.top = rc.bottom - iHeight;
            break;

        case ABE_LEFT:
            rc.right = rc.left + iWidth;
            break;

        case ABE_RIGHT:
            rc.left = rc.right - iWidth;
            break;
        }

        AppBarQuerySetPos(m_side, &rc, pabd);
    }

    void AppBarCallback(HWND hwndAccessBar, UINT uNotifyMsg,
        LPARAM lParam)
    {
        APPBARDATA abd;
        UINT uState;

        abd.cbSize = sizeof(abd);
        abd.hWnd = hwndAccessBar;

        switch (uNotifyMsg)
        {
        case ABN_STATECHANGE:

            // Check to see if the taskbar's always-on-top state has changed
            // and, if it has, change the appbar's state accordingly.
            uState = SHAppBarMessage(ABM_GETSTATE, &abd);

            SetWindowPos(hwndAccessBar,
                (ABS_ALWAYSONTOP & uState) ? HWND_TOPMOST : HWND_BOTTOM,
                0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            break;

        case ABN_FULLSCREENAPP:

            // A full-screen application has started, or the last full-screen
            // application has closed. Set the appbar's z-order appropriately.
            if (lParam)
            {
                SetWindowPos(hwndAccessBar,
                    (ABS_ALWAYSONTOP & uState) ? HWND_TOPMOST : HWND_BOTTOM,
                    0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            else
            {
                uState = SHAppBarMessage(ABM_GETSTATE, &abd);

                if (uState & ABS_ALWAYSONTOP)
                    SetWindowPos(hwndAccessBar,
                        HWND_TOPMOST,
                        0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }

        case ABN_POSCHANGED:

            // The taskbar or another appbar has changed its size or position.
            AppBarPosChanged(&abd);
            break;
        }
    }

    ~AppBarLifetime()
    {
        if (m_registerSuccess)
        {
            RegisterAccessBar(m_hwnd, false);
        }
    }
private:
    HWND m_hwnd{ nullptr };
    bool m_registerSuccess{ false };
    DWORD m_side{ ABE_LEFT };
};