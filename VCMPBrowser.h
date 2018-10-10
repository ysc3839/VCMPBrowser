#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <utility>
#include <string>
#include <thread>
#include <array>
#include <io.h>
#include <algorithm>
#include <map>
#include <fstream>
#include <list>
#include <set>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Windows.h>
#include <Commctrl.h>
#include <Shellapi.h>
#include <WindowsX.h>
#include <Uxtheme.h>
#include <Winsock2.h>
#include <Commdlg.h>
#include <Wincred.h>
#include <Combaseapi.h>
#include <Vssym32.h>
#include <Dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodings.h"
#include "rapidjson/filereadstream.h"

#define CURL_STATICLIB
#include "curl/curl.h"

#include "7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zFile.h"

#include "resource.h"

#include "ServerInfoUtil.h"

enum updateFreq
{
	START = 0,
	DAY,
	TWODAY,
	WEEK,
	NEVER
};

struct settings
{
	LANGID language;
	char playerName[24];
	std::wstring gamePath;
	updateFreq gameUpdateFreq;
	std::string gameUpdateURL;
	std::string gameUpdatePassword;
	std::string masterlistURL;
	std::string proxy;
	COLORREF officialColor;
	COLORREF custColors[16];
	uint32_t codePage;
};

#define WM_SOCKET WM_USER+1
#define WM_PROGRESS WM_USER+1 // wParam=progress, lParam=speed
#define WM_UPDATE WM_USER+2 // wParam=result(bool)

// UI sizes
#define UI_PLAYERLIST_WIDTH 200
#define UI_SERVERINFO_HEIGHT 140

#define VERSION "1.0-preview5"
