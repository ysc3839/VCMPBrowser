#pragma once

const LANGID languages[] = {
	LANG_NEUTRAL,
	MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
#ifndef NO_I18N
	MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT),
	MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN),
	MAKELANGID(LANG_DUTCH, SUBLANG_DEFAULT),
	MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT),
	MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN),
	MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT),
	MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
	MAKELANGID(LANG_BENGALI, SUBLANG_BENGALI_BANGLADESH)
#endif // NO_I18N
};

const wchar_t *languageNames[] = {
	L"Default",
	L"English",
#ifndef NO_I18N
	L"Arabic",
	L"Spanish",
	L"Dutch",
	L"Polish",
	L"Portuguese (Brazil)",
	L"Turkish",
	L"Chinese (Simplified)",
	L"Bengali"
#endif // NO_I18N
};

#ifndef NO_I18N
inline const wchar_t* LoadStr(const wchar_t* origString, UINT ID) { wchar_t* str; return (LoadStringW(g_hInst, ID, reinterpret_cast<LPWSTR>(&str), 0) ? str : origString); }
#else // NO_I18N
#define LoadStr(origString, ID) origString
#endif // NO_I18N

bool ConvertCharset(const char *from, std::wstring &to)
{
	constexpr DWORD kFlags = MB_ERR_INVALID_CHARS;
	do
	{
		const int size = MultiByteToWideChar(CP_ACP, kFlags, from, -1, nullptr, 0);

		if (size == 0)
			break;

		to.resize(size);

		int result = MultiByteToWideChar(CP_ACP, kFlags, from, -1, &to[0], size);

		if (result == 0)
			break;

		return true;
	} while (0);

	to.clear();
	return false;
}
