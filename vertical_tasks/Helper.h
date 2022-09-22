#include "pch.h"
#include "winuser.h"
#include <errhandlingapi.h>
#include <handleapi.h>


// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

inline BOOL CloseHandleIfNotNull(HANDLE handle)
{
    if (handle ==nullptr)
    {
        // Return true if there is nothing to close.
        return true;
    }
    return CloseHandle(handle);
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

    return std::equal(string.begin(), string.begin() + prefix.size(), prefix.begin(), prefix.end(), [](wchar_t a, wchar_t b) { return tolower(a) == tolower(b); });
}

inline bool CaseInsensitiveEqual(std::wstring string1, std::wstring string2)
{
    if (string1.size() != string2.size())
    {
        return false;
    }

    return std::equal(string1.begin(), string1.end(), string2.begin(), string2.end(), [](wchar_t a, wchar_t b) { return tolower(a) == tolower(b); });
}