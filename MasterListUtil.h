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
