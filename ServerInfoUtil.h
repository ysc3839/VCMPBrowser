#pragma once

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

inline int GetIPString(uint32_t ip, uint16_t port, wchar_t *ipStr, size_t bufCount = 22)
{
	char *_ip = (char *)&(ip);
	return swprintf_s(ipStr, bufCount, L"%hhu.%hhu.%hhu.%hhu:%hu", _ip[0], _ip[1], _ip[2], _ip[3], port);
}

inline int GetIPString(serverAddress address, wchar_t *ipstr)
{
	return GetIPString(address.ip, address.port, ipstr);
}

inline int GetPlayersString(uint16_t players, uint16_t maxPlayers, wchar_t *playersStr, size_t bufCount = 12)
{
	return swprintf_s(playersStr, 12, L"%hu/%hu", players, maxPlayers);
}

inline void GetPingString(uint32_t lastRecv, uint32_t lastPing, wchar_t *pingStr, size_t bufCount = 12)
{
	if (lastRecv != 0)
	{
		uint32_t ping = lastRecv - lastPing;
		_itow_s(ping, pingStr, bufCount, 10);
	}
	else
	{
		pingStr[0] = L'-';
		pingStr[1] = 0;
	}
}
