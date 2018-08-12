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
inline wchar_t* LoadStr(wchar_t* origString, UINT ID) { wchar_t* str; return (LoadString(g_hInst, ID, (LPWSTR)&str, 0) ? str : origString); }
#else // NO_I18N
#define LoadStr(origString, ID) origString
#endif // NO_I18N

bool ConvertCharset(const char *from, std::wstring &to)
{
	size_t size = MultiByteToWideChar(CP_ACP, 0, from, -1, nullptr, 0);

	if (size == 0)
		return false;

	wchar_t *buffer = new wchar_t[size];
	if (!buffer)
		return false;

	if (MultiByteToWideChar(CP_ACP, 0, from, -1, buffer, size) == 0)
		return false;

	to.clear();
	to.append(buffer);

	delete[] buffer;
	return true;
}
