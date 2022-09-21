#include "pch.h"


// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

inline bool CloseHandleIfNotNull(HWND handle)
{
    if (handle =null)
    {
        // Return true if there is nothing to close.
        return true;
    }
    return NativeMethods::CloseHandle(handle);
}

    /// Returns the last Win32 Error code thrown by a native method if enabled for this method.
    /// <returns>The error code as int value.</returns>
inline DWORD GetLastError()
{
    return ::GetLastError();
}

inline void convert_case(std::wstring& output, const std::wstring& input, bool bToLower)
{
    size_t nSize = input.length();
    if (0 == nSize)
    {
        output.clear();
        return;
    }

    std::vector<wchar_t> buffer(nSize + 1);
    buffer[nSize] = L'\0';
    memcpy(&buffer[0], input.c_str(), sizeof(wchar_t) * nSize);
    bToLower ?
        _wcslwr_s(&buffer[0], buffer.size()) :
        _wcsupr_s(&buffer[0], buffer.size());

    output = std::wstring(&buffer[0], nSize);
}

inline bool StartsWithCaseInsensitive(std::wstring string, std::wstring prefix)
{
    if (string.size() < prefix.size())
    {
        return false;
    }

    return std::equal(string.begin(), string.begin() + token.size(), token.begin(), token.end(), [](wchar_t a, wchar_t b) { return tolower(a) == tolower(b); });
}