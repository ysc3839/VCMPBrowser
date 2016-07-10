#pragma once

void SaveSettings()
{
	using rapidjson::UTF8;
	using rapidjson::UTF16;

	rapidjson::StringBuffer json;
	rapidjson::Writer<rapidjson::StringBuffer> writer(json);

	writer.StartObject();

	writer.Key("language");
	writer.Uint(g_browserSettings.language);

	writer.Key("codePage");
	writer.Uint(g_browserSettings.codePage);

	writer.Key("playerName");
	writer.String(g_browserSettings.playerName);

	writer.Key("gamePath");
	rapidjson::GenericStringStream<UTF16<>> source(g_browserSettings.gamePath.c_str());
	rapidjson::StringBuffer target;
	bool hasError = false;
	while (source.Peek() != '\0') if (!rapidjson::Transcoder<UTF16<>, UTF8<>>::Transcode(source, target)) { hasError = true; break; }
	writer.String(hasError ? "" : target.GetString());

	writer.Key("gameUpdateFreq");
	writer.Uint(g_browserSettings.gameUpdateFreq);

	writer.Key("gameUpdateURL");
	writer.String(g_browserSettings.gameUpdateURL);

	writer.Key("gameUpdatePassword");
	writer.String(g_browserSettings.gameUpdatePassword);

	writer.Key("masterlistURL");
	writer.String(g_browserSettings.masterlistURL);

	writer.Key("officialColor");
	writer.Uint(g_browserSettings.officialColor);

	writer.Key("custColors");
	writer.StartArray();
	for (size_t i = 0; i < 16; ++i)
		writer.Uint(g_browserSettings.custColors[i]);
	writer.EndArray();

	writer.EndObject();

	SetCurrentDirectory(g_exePath);

	FILE *file = _wfopen(L"settings.json", L"wb");
	if (file != nullptr)
	{
		fwrite(json.GetString(), sizeof(char), json.GetSize(), file);
		fclose(file);
	}
}

void DefaultSettings()
{
	memset(g_browserSettings.playerName, 0, sizeof(g_browserSettings.playerName));
	g_browserSettings.gamePath = L"";
	g_browserSettings.gameUpdateFreq = START;
	g_browserSettings.gameUpdateURL = "http://u04.maxorator.com";
	g_browserSettings.gameUpdatePassword = "";
	g_browserSettings.masterlistURL = "http://master.vc-mp.ovh";
	g_browserSettings.officialColor = RGB(0, 0, 255); // Blue
	std::fill_n(g_browserSettings.custColors, std::size(g_browserSettings.custColors), 0xFFFFFF); // White
}

void LoadSettings()
{
	DefaultSettings();

	SetCurrentDirectory(g_exePath);

	FILE *file = _wfopen(L"settings.json", L"rb");
	if (file != nullptr)
	{
		do {
			char readBuffer[512];
			rapidjson::FileReadStream is(file, readBuffer, sizeof(readBuffer));

			rapidjson::Document dom;

			if (dom.ParseStream(is).HasParseError() || !dom.IsObject())
				break;

			auto member = dom.FindMember("language");
			if (member != dom.MemberEnd() && member->value.IsUint())
				g_browserSettings.language = member->value.GetUint();

			member = dom.FindMember("codePage");
			if (member != dom.MemberEnd() && member->value.IsUint())
				g_browserSettings.codePage = member->value.GetUint();

			member = dom.FindMember("playerName");
			if (member != dom.MemberEnd() && member->value.IsString())
				strncpy(g_browserSettings.playerName, member->value.GetString(), sizeof(g_browserSettings.playerName));

			member = dom.FindMember("gamePath");
			if (member != dom.MemberEnd() && member->value.IsString())
			{
				using rapidjson::UTF8;
				using rapidjson::UTF16;

				rapidjson::GenericStringStream<UTF8<>> source(member->value.GetString());
				rapidjson::GenericStringBuffer<UTF16<>> target;
				bool hasError = false;
				while (source.Peek() != '\0') if (!rapidjson::Transcoder<UTF8<>, UTF16<>>::Transcode(source, target)) { hasError = true; break; }
				g_browserSettings.gamePath = std::wstring(target.GetString(), target.GetSize());
			}

			member = dom.FindMember("gameUpdateFreq");
			if (member != dom.MemberEnd() && member->value.IsUint())
				g_browserSettings.gameUpdateFreq = (updateFreq)member->value.GetUint();

			member = dom.FindMember("gameUpdateURL");
			if (member != dom.MemberEnd() && member->value.IsString())
				g_browserSettings.gameUpdateURL = std::string(member->value.GetString(), member->value.GetStringLength());

			member = dom.FindMember("gameUpdatePassword");
			if (member != dom.MemberEnd() && member->value.IsString())
				g_browserSettings.gameUpdatePassword = std::string(member->value.GetString(), member->value.GetStringLength());

			member = dom.FindMember("masterlistURL");
			if (member != dom.MemberEnd() && member->value.IsString())
				g_browserSettings.masterlistURL = std::string(member->value.GetString(), member->value.GetStringLength());

			member = dom.FindMember("officialColor");
			if (member != dom.MemberEnd() && member->value.IsUint())
				g_browserSettings.officialColor = member->value.GetUint();

			member = dom.FindMember("custColors");
			if (member != dom.MemberEnd() && member->value.IsArray())
			{
				size_t i = 0;
				for (auto it = member->value.Begin(); it != member->value.End() && i < 16; ++it, ++i)
				{
					if (it->IsUint())
						g_browserSettings.custColors[i] = it->GetUint();
				}
			}
		} while (0);
		fclose(file);
	}
}
