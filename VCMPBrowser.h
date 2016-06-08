#pragma once
#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0501 // XP
#include <SDKDDKVer.h>

#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <utility>
#include <string>
#include <thread>
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commctrl.h>
#include <Shellapi.h>
#include <WindowsX.h>
#include <Uxtheme.h>
#include <Winsock2.h>
#include <Commdlg.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodings.h"
#include "rapidjson/filereadstream.h"

#define CURL_STATICLIB
#include "curl/curl.h"

#include "resource.h"

struct serverAddress
{
	uint32_t ip;
	uint16_t port;
};

struct serverMasterListInfo
{
	serverAddress address;
	bool isOfficial;
	uint32_t lastPing;
};

struct serverInfo
{
	char versionName[12];
	bool isPassworded;
	uint16_t players;
	uint16_t maxPlayers;
	std::string serverName;
	std::string gameMode;
	std::string mapName;
};

struct playerName
{
	char name[25];
};
typedef std::vector<playerName> serverPlayers;

struct serverAllInfo
{
	serverAddress address;
	serverInfo info;
	serverPlayers players;
	bool isOfficial;
	uint32_t lastPing[2];
	uint32_t lastRecv;
};

typedef std::vector<serverMasterListInfo> serverMasterList;
typedef std::vector<serverAllInfo> serverList;

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
	wchar_t gamePath[MAX_PATH];
	updateFreq gameUpdateFreq;
	std::string gameUpdateURL;
	std::string gameUpdatePassword;
	std::string masterlistURL;
	COLORREF officialColor;
	COLORREF custColors[16];
};

#define WM_SOCKET WM_USER+1
#define WM_PROGRESS WM_USER+1 // wParam=progress, lParam=speed

// UI sizes
#define UI_PLAYERLIST_WIDTH 200
#define UI_SERVERINFO_HEIGHT 140
