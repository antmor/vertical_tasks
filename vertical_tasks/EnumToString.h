#pragma once
#include <windows.h>
#include <winuser.h>

#include <algorithm>
#include <string>

#include <wil\resource.h>

template <typename T>
struct EnumStringMapping
{
	T enumVal{};
	std::wstring_view stringVal{};
};

template <typename T, typename TArr>
inline std::wstring_view SingleEnumToString(T enumVal, TArr arr)
{
	auto found = std::find_if(arr.begin(), arr.end(), [&enumVal](auto val) {
		return val.enumVal == enumVal;
	});
	if (found == arr.end())
	{
		return L"NOT FOUND!!!";
	}
	return (*found).stringVal;
}

#define BEGIN_ENUM_VAL(type, name) \
constexpr static std::array name##ToStringArray = {

#define ENUM_VAL_TO_STRING(type, x) EnumStringMapping<type>{ x, std::wstring_view{ L#x } },

#define END_ENUM_VAL(type, name)		\
};										\
inline auto name##ToString(type enumVal)		\
{										\
	return SingleEnumToString<type>(enumVal,	\
		name##ToStringArray);			\
};

template <typename T, typename TArr>
inline auto EnumFlagToString(T enumFlags, TArr arr)
{
	bool isFirst{ true };
	std::wstringstream stringBuilder{};
	for (const auto& currVal : arr)
	{
		//if (WI_IsFlagSet(enumFlags, currVal.enumVal))
		if (enumFlags & currVal.enumVal)
		{
			if (isFirst)
			{
				stringBuilder << currVal.stringVal;
				isFirst = false;
			}
			else
			{

				stringBuilder << L", " << currVal.stringVal ;
			}
		}
	}

	return stringBuilder.str();
}

#define BEGIN_ENUMFLAG_VAL(type, name) \
constexpr static std::array name##ToStringArray = {

#define ENUM_VAL_TO_STRING(type, x) EnumStringMapping<type>{ x, std::wstring_view{ L#x } },

#define END_ENUMFLAG_VAL(type, name)		\
};										\
inline auto name##ToString(type enumVal)		\
{										\
	return EnumFlagToString<type>(enumVal,	\
		name##ToStringArray);			\
};			

BEGIN_ENUMFLAG_VAL(DWORD, WindowStyles)
ENUM_VAL_TO_STRING(DWORD, WS_POPUP)
ENUM_VAL_TO_STRING(DWORD, WS_CHILD)
ENUM_VAL_TO_STRING(DWORD, WS_MINIMIZE)
ENUM_VAL_TO_STRING(DWORD, WS_VISIBLE)
ENUM_VAL_TO_STRING(DWORD, WS_DISABLED)
ENUM_VAL_TO_STRING(DWORD, WS_CLIPSIBLINGS)
ENUM_VAL_TO_STRING(DWORD, WS_CLIPCHILDREN)
ENUM_VAL_TO_STRING(DWORD, WS_MAXIMIZE)
//ENUM_VAL_TO_STRING(DWORD, WS_CAPTION       )
ENUM_VAL_TO_STRING(DWORD, WS_BORDER)
ENUM_VAL_TO_STRING(DWORD, WS_DLGFRAME)
ENUM_VAL_TO_STRING(DWORD, WS_VSCROLL)
ENUM_VAL_TO_STRING(DWORD, WS_HSCROLL)
ENUM_VAL_TO_STRING(DWORD, WS_SYSMENU)
ENUM_VAL_TO_STRING(DWORD, WS_THICKFRAME)
ENUM_VAL_TO_STRING(DWORD, WS_GROUP)
ENUM_VAL_TO_STRING(DWORD, WS_TABSTOP)
END_ENUMFLAG_VAL(DWORD, WindowStyles)

BEGIN_ENUMFLAG_VAL(DWORD, ExWindowStyles)
ENUM_VAL_TO_STRING(DWORD, WS_EX_DLGMODALFRAME)
ENUM_VAL_TO_STRING(DWORD, WS_EX_NOPARENTNOTIFY)
ENUM_VAL_TO_STRING(DWORD, WS_EX_TOPMOST)
ENUM_VAL_TO_STRING(DWORD, WS_EX_ACCEPTFILES)
ENUM_VAL_TO_STRING(DWORD, WS_EX_TRANSPARENT)
ENUM_VAL_TO_STRING(DWORD, WS_EX_MDICHILD)
ENUM_VAL_TO_STRING(DWORD, WS_EX_TOOLWINDOW)
ENUM_VAL_TO_STRING(DWORD, WS_EX_WINDOWEDGE)
ENUM_VAL_TO_STRING(DWORD, WS_EX_CLIENTEDGE)
ENUM_VAL_TO_STRING(DWORD, WS_EX_CONTEXTHELP)
ENUM_VAL_TO_STRING(DWORD, WS_EX_RIGHT)
ENUM_VAL_TO_STRING(DWORD, WS_EX_RTLREADING)
ENUM_VAL_TO_STRING(DWORD, WS_EX_LEFTSCROLLBAR)
ENUM_VAL_TO_STRING(DWORD, WS_EX_CONTROLPARENT)
ENUM_VAL_TO_STRING(DWORD, WS_EX_STATICEDGE)
ENUM_VAL_TO_STRING(DWORD, WS_EX_APPWINDOW)
ENUM_VAL_TO_STRING(DWORD, WS_EX_LAYERED)
ENUM_VAL_TO_STRING(DWORD, WS_EX_NOINHERITLAYOUT)
ENUM_VAL_TO_STRING(DWORD, WS_EX_NOREDIRECTIONBITMAP)
ENUM_VAL_TO_STRING(DWORD, WS_EX_LAYOUTRTL)
ENUM_VAL_TO_STRING(DWORD, WS_EX_COMPOSITED)
ENUM_VAL_TO_STRING(DWORD, WS_EX_NOACTIVATE)
END_ENUMFLAG_VAL(DWORD, ExWindowStyles)
