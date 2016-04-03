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

struct serverInfo
{
	uint32_t ip;
	uint16_t port;
	bool isOfficial;
};

typedef std::vector<serverInfo> serverList;
