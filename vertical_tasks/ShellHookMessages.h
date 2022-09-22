#include <pch.h>
#include <wil\resource.h>
#include <winuser.h>
#include <processthreadsapi.h>
#include <functional>

const wchar_t* c_wzShellHookMessage = L"SHELLHOOK";

// retrieve the HINSTANCE for the current DLL or EXE using this symbol that
// the linker provides for every module, avoids the need for a global HINSTANCE variable
// and provides access to this value for static libraries
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
__inline HINSTANCE GetModuleHINSTANCE() { return (HINSTANCE)&__ImageBase; }

struct ShellHookMessages : std::enable_shared_from_this<ShellHookMessages>
{
    wil::unique_hwnd m_hwnd{};
    DWORD threadId{};
    UINT shellHookMsgId{};
    std::function<void(WPARAM wParam, LPARAM lParam)> callback;

    static LRESULT WINAPI StaticWndProc(HWND hwnd, UINT msgId, WPARAM wParam, LPARAM lParam)
    {
        ShellHookMessages* thisRef{ nullptr };
        if (msgId == WM_NCCREATE)
        {
            auto lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            thisRef = static_cast<ShellHookMessages*>(lpcs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thisRef));
        }
        else
        {
            thisRef = reinterpret_cast<ShellHookMessages*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        if (thisRef)
        {
            return thisRef->WndProc(hwnd, msgId, wParam, lParam);
        }
        return DefWindowProc(hwnd, msgId, wParam, lParam);
    }

    ShellHookMessages()
    {
        WNDCLASSEX wc = { 0 };
        wc.lpfnWndProc = ShellHookMessages::StaticWndProc;
        wc.cbSize = sizeof(wc);
        wc.style = CS_DBLCLKS | CS_NOCLOSE;
        wc.cbWndExtra = sizeof(ShellHookMessages*);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"ShellHookMessages";
        wc.hInstance = GetModuleHINSTANCE();
        THROW_LAST_ERROR_IF(RegisterClassEx(&wc) == 0);

        // Create the window.
        // Pass "this" param so we can use derived class implementation later.
        m_hwnd.reset(CreateWindowEx(
            0, // dwExStyle
            L"ShellHookMessages", // lpClassName
            L"ShellHookMessages", // lpClassName
            0,  // dwStyle
            0, 0, 0, 0,  // x, y, width, height
            HWND_MESSAGE, // parent
            nullptr, // hMenu
            GetModuleHINSTANCE(), //hInstance
            this // lParam
        ));
        THROW_LAST_ERROR_IF_NULL(m_hwnd);
    }

    LRESULT WndProc(HWND hwnd, UINT msgId, WPARAM wParam, LPARAM lParam)
    {
        switch (msgId)
        {
        case WM_CREATE:
        {
            shellHookMsgId = RegisterWindowMessage(c_wzShellHookMessage);
            RegisterShellHookWindow(hwnd);
            return 0;
            break;
        }
        case WM_DESTROY:
        {
            DeregisterShellHookWindow(hwnd);

            return 0;
            break;
        }
        default:
            if (msgId == shellHookMsgId)
            {
                callback(wParam, lParam);
            }
            break;
        }

        return -1;
    }
    
    void Register(std::function<void(WPARAM wParam, LPARAM lParam)>&& toRegister)
    {
        callback = toRegister;
    }
};