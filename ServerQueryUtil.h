#pragma once

SOCKET g_UDPSocket;

void SendQuery(serverAddress address, char opcode)
{
	struct sockaddr_in sendaddr = { AF_INET };
	sendaddr.sin_addr.s_addr = address.ip;
	sendaddr.sin_port = htons(address.port);

	char *cip = (char *)&(address.ip);
	char *cport = (char *)&(address.port);
	char buffer[] = { 'V', 'C', 'M', 'P', cip[0], cip[1], cip[2], cip[3], cport[0], cport[1], opcode };

	sendto(g_UDPSocket, buffer, sizeof(buffer), 0, (sockaddr *)&sendaddr, sizeof(sendaddr));
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
	serverInfo.serverName = std::string(_data, strLen);
	_data += strLen;

	strLen = *(int *)_data;
	_data += sizeof(strLen);
	if (length < (_data - data) + strLen + 4)
		return false;

	serverInfo.gameMode = std::string(_data, strLen);
	_data += strLen;

	strLen = *(int *)_data;
	_data += sizeof(strLen);
	if (length < (_data - data) + strLen)
		return false;

	serverInfo.mapName = std::string(_data, strLen);

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
