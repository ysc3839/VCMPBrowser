#pragma once

uint32_t dpiScale;

// https://stackoverflow.com/a/25065519
#define muldiv(number, numerator, denominator) \
		(int)(((long)number * numerator + (denominator >> 1)) / denominator)

void SetDPIAware()
{
	HMODULE hUser = GetModuleHandle(L"user32.dll");
	if (hUser)
	{
		BOOL (WINAPI *pSetProcessDPIAware)();
		pSetProcessDPIAware = GetProcAddress(hUser, "SetProcessDPIAware");
		if (pSetProcessDPIAware)
			pSetProcessDPIAware();
	}
}

void InitDPIScale()
{
	HDC hdc = GetDC(0);
	if (hdc)
	{
		dpiScale = GetDeviceCaps(hdc, LOGPIXELSX);
		if (dpiScale == 96)
			dpiScale = 0;
		else
			dpiScale = muldiv(dpiScale, 100, 96);
		ReleaseDC(0, hdc);
	}
}

int Scale(int i)
{
	if (dpiScale == 0)
		return i;
	else
		return muldiv(i, dpiScale, 100);
}
