#pragma once

inline wchar_t* LoadStr(wchar_t* origString, UINT ID) { wchar_t* str; return (LoadString(g_hInst, ID, (LPWSTR)&str, 0) ? str : origString); }

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
	to.append(buffer, size);

	delete[] buffer;
	return true;
}
