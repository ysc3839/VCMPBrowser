#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <utility>
#include <string>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commctrl.h>
#include <Shellapi.h>
#include <WindowsX.h>
#include <Uxtheme.h>
#include <Winsock2.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

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

struct serverAllInfo
{
	serverAddress address;
	serverInfo info;
	bool isOfficial;
	uint32_t lastPing[2];
	uint32_t lastRecv;
};

typedef std::vector<serverMasterListInfo> serverMasterList;
typedef std::vector<serverAllInfo> serverList;

#define WM_SOCKET WM_USER+1
