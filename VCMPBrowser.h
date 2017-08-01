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

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodings.h"
#include "rapidjson/filereadstream.h"

#define CURL_STATICLIB
#include "curl/curl.h"

#include "7z/7z.h"
#include "7z/7zAlloc.h"
#include "7z/7zCrc.h"
#include "7z/7zFile.h"

#include "resource.h"

struct serverAddress
{
	bool operator==(const serverAddress &rhs)
	{
		return (this->ip == rhs.ip && this->port == rhs.port);
	}

	bool operator!=(const serverAddress &rhs)
	{
		return !(*this == rhs);
	}

	bool operator<(const serverAddress &rhs) const
	{
		if (this->ip < rhs.ip)
		{
			return true;
		}
		else if (this->ip == rhs.ip)
		{
			return this->port < rhs.port;
		}
		return false;
	}

	uint32_t ip;
	uint16_t port;
};

struct serverHost
{
	std::string hostname;
	uint16_t port;
};

struct serverMasterListInfo
{
	serverAddress address;
	bool isOfficial;
	uint32_t lastPing;
};

struct serverMasterListInfoValue
{
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
	uint32_t lastRecv; // == 0 offline
	time_t lastPlayed;
};

typedef std::map<serverAddress, serverMasterListInfoValue> serverMasterList;
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

#define VERSION "1.0-preview4"
