#pragma once
#include <vector>
#include <utility>
#include <codecvt>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commctrl.h>
#include <Shellapi.h>
#include <WindowsX.h>
#include <Uxtheme.h>
#include <Winsock2.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"

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

struct serverInfoi
{
	char versionName[12];
	bool isPassworded;
	uint16_t players;
	uint16_t maxPlayers;
	std::string serverName;
	std::string gameMode;
	std::string mapName;
};

struct serverInfo
{
	serverMasterListInfo listInfo;
	serverInfoi info;
};

typedef std::vector<serverMasterListInfo> serverMasterList;
typedef std::vector<serverInfo> serverList;

#define WM_SOCKET WM_USER+1
