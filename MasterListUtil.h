#pragma once

CURLcode CurlRequset(const char *URL, std::string &data, const char *userAgent)
{
	CURLcode res = CURLE_FAILED_INIT;
	CURL *curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, URL);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)[](char *buffer, size_t size, size_t nitems, void *outstream) {
			if (outstream)
			{
				size_t realsize = size * nitems;
				((std::string *)outstream)->append(buffer, realsize);
				return realsize;
			}
			return (size_t)0;
		});

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	return res;
}

bool ParseJson(const char *json, serverMasterList &serversList)
{
	do {
		rapidjson::Document dom;
		if (dom.Parse(json).HasParseError() || !dom.IsObject())
			break;

		auto success = dom.FindMember("success");
		if (success == dom.MemberEnd() || !success->value.IsBool() || !success->value.GetBool())
			break;

		auto servers = dom.FindMember("servers");
		if (servers == dom.MemberEnd() || !servers->value.IsArray())
			break;

		for (auto it = servers->value.Begin(); it != servers->value.End(); ++it)
		{
			if (it->IsObject())
			{
				auto ip = it->FindMember("ip");
				auto port = it->FindMember("port");
				auto isOfficial = it->FindMember("is_official");
				if (ip != it->MemberEnd() && port != it->MemberEnd() && isOfficial != it->MemberEnd() && ip->value.IsString() && port->value.IsUint() && isOfficial->value.IsBool())
				{
					serverMasterListInfo server;
					server.address.ip = inet_addr(ip->value.GetString());
					server.address.port = (uint16_t)port->value.GetUint();
					server.isOfficial = isOfficial->value.GetBool();
					serversList.push_back(server);
				}
			}
		}
		return true;
	} while (0);
	return false;
}

bool GetServerInfo(char *data, int length, serverInfo &serverInfo)
{
	if (length < 11 + 12 + 1 + 2 + 2 + 4) // 12=Version name, 1=Password, 2=Players, 2=MaxPlayers, 4=strlen
		return false;

	char *_data = data + 11;

	memmove(serverInfo.versionName, _data, sizeof(serverInfo.versionName));
	_data += sizeof(serverInfo.versionName);

	serverInfo.isPassworded = *_data != false;
	_data += sizeof(serverInfo.isPassworded);

	serverInfo.players = *(uint16_t *)_data;
	_data += sizeof(serverInfo.players);

	serverInfo.maxPlayers = *(uint16_t *)_data;
	_data += sizeof(serverInfo.maxPlayers);

	int strLen = *(int *)_data;
	_data += sizeof(strLen);
	if (length < 11 + 12 + 1 + 2 + 2 + 4 + strLen + 4)
		return false;

	char *serverName = (char *)alloca(strLen + 1);
	serverInfo.serverName.clear();
	serverInfo.serverName.append(_data, strLen);
	serverInfo.serverName.append(1, '\0');
	_data += strLen;

	strLen = *(int *)_data;
	_data += sizeof(strLen);
	if (length < (_data - data) + strLen + 4)
		return false;

	serverInfo.gameMode.clear();
	serverInfo.gameMode.append(_data, strLen);
	serverInfo.gameMode.append(1, '\0');
	_data += strLen;

	strLen = *(int *)_data;
	_data += sizeof(strLen);
	if (length < (_data - data) + strLen)
		return false;

	serverInfo.mapName.clear();
	serverInfo.mapName.append(_data, strLen);
	serverInfo.mapName.append(1, '\0');

	return true;
}

bool GetServerPlayers(char *data, int length, serverPlayers &serverPlayers)
{
	serverPlayers.clear();

	if (length < 11 + 2)
		return false;

	char *_data = data + 11;

	uint16_t players = *(uint16_t *)_data;
	_data += sizeof(players);

	if (players != 0)
	{
		serverPlayers.reserve(players);

		for (uint16_t i = 0; i < players; ++i)
		{
			if (length < (_data - data) + 1)
				return false;

			uint8_t strLen = *_data;
			_data += sizeof(strLen);

			if (length < (_data - data) + strLen)
				return false;

			playerName playerName = {};
			strncpy(playerName.name, _data, strLen > 24 ? 24 : strLen);
			_data += strLen;

			serverPlayers.push_back(playerName);
		}
	}
	return true;
}
