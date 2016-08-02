#pragma once
#include <direct.h>

struct progInfo {
	HWND hWnd;
	uint32_t time;
	curl_off_t size;
};

HANDLE hCancelEvent;

int curlProgress(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	if (WaitForSingleObject(hCancelEvent, 0) != WAIT_TIMEOUT) // Cancel
		return 1;
	if (dltotal != 0 && ultotal != 0)
	{
		progInfo *info = (progInfo *)p;
		uint32_t now = GetTickCount(), span_ms = now - info->time;
		if (span_ms >= 500) // Update every 0.5s
		{
			info->time = now;
			curl_off_t dlsize = dlnow > ulnow ? dlnow : ulnow;
			curl_off_t current_speed = (dlsize - info->size) / span_ms * CURL_OFF_T_C(1000);
			info->size = dlsize;

			PostMessage(info->hWnd, WM_PROGRESS, (long)(dlnow * CURL_OFF_T_C(100) / dltotal), (long)(current_speed));
		}
	}
	return 0;
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		DLGPROC dlgProc = (DLGPROC)lParam;
		return dlgProc(hDlg, message, wParam, lParam);
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_DESTROY:
		SetEvent(hCancelEvent);
		CloseHandle(hCancelEvent);
		hCancelEvent = nullptr;
		return (INT_PTR)TRUE;
	case WM_PROGRESS:
		if (wParam != -1)
		{
			SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, wParam, 0);
			const wchar_t *unit = L"B/s";
			float speed = (float)lParam;
			if (speed > 1024)
			{
				speed = speed / 1024; // KB
				unit = L"KB/s";
				if (speed > 1024)
				{
					speed = speed / 1024; // MB
					unit = L"MB/s";
				}
			}
			wchar_t speedText[32];
			swprintf_s(speedText, L"%.1f %s", speed, unit);
			SetDlgItemText(hDlg, IDC_TIPSTATIC, speedText);
		}
		else
		{
			SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_ERROR, 0);
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool Extract7z(const char *fileName)
{
	CFileInStream archiveStream;
	if (InFile_Open(&archiveStream.file, fileName) == 0)
	{
		CLookToRead lookStream;
		FileInStream_CreateVTable(&archiveStream);
		LookToRead_CreateVTable(&lookStream, False);

		lookStream.realStream = &archiveStream.s;
		LookToRead_Init(&lookStream);

		CSzArEx db;
		SzArEx_Init(&db);

		static ISzAlloc szAlloc = { SzAlloc, SzFree };

		SRes res = SzArEx_Open(&db, &lookStream.s, &szAlloc, &szAlloc);
		if (res == SZ_OK)
		{
			uint32_t blockIndex = 0xFFFFFFFF;
			uint8_t *outBuffer = 0;
			size_t outBufferSize = 0;

			wchar_t *nameBuf = nullptr;
			size_t buflen = 0;

			for (size_t i = 0; i < db.NumFiles; i++)
			{
				size_t namelen = SzArEx_GetFileNameUtf16(&db, i, NULL);
				if (namelen > buflen)
				{
					free(nameBuf);
					buflen = namelen;
					nameBuf = (wchar_t *)malloc(buflen * sizeof(wchar_t));
					if (nameBuf == nullptr)
					{
						res = SZ_ERROR_MEM;
						break;
					}
				}

				SzArEx_GetFileNameUtf16(&db, i, (UInt16 *)nameBuf);

				if (wcsstr(nameBuf, L"../") != nullptr)
					continue;

				for (size_t j = 0; nameBuf[j] != 0; j++)
					if (nameBuf[j] == L'/')
					{
						nameBuf[j] = 0;
						_wmkdir(nameBuf);
						nameBuf[j] = L'\\';
					}

				FILE *outFile;
				bool isDir = SzArEx_IsDir(&db, i);
				if (isDir)
				{
					_wmkdir(nameBuf);
					continue;
				}
				else
				{
					size_t offset = 0;
					size_t outSizeProcessed = 0;
					res = SzArEx_Extract(&db, &lookStream.s, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &szAlloc, &szAlloc);
					if (res != SZ_OK)
						break;

					outFile = _wfopen(nameBuf, L"wb+");
					if (outFile == nullptr)
					{
						res = SZ_ERROR_WRITE;
						break;
					}
					if (fwrite(outBuffer + offset, 1, outSizeProcessed, outFile) != outSizeProcessed)
					{
						res = SZ_ERROR_WRITE;
						break;
					}
					fclose(outFile);
				}
			}

			free(nameBuf);
			SzArEx_Free(&db, &szAlloc);
		}
		File_Close(&archiveStream.file);
		return res == SZ_OK;
	}
	return false;
}

bool DownloadVCMPGame(const char *version, const char *password)
{
	struct downloadInfo {
		const char *version;
		const char *password;
	};
	downloadInfo dlInfo = { version, password };
	static downloadInfo *pdlInfo;
	pdlInfo = &dlInfo;
	static bool success;
	success = false;

	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_UPDATE), g_hMainWnd, DialogProc, (LPARAM)(DLGPROC)[](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
		hCancelEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (!hCancelEvent)
			return (INT_PTR)FALSE;

		std::thread thread([](HWND hWnd) {
			CURL *curl = curl_easy_init();
			if (curl)
			{
				std::string url = g_browserSettings.gameUpdateURL + "/download";
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

				SetCurrentDirectory(g_exePath);

				char fileName[16];
				srand(GetTickCount());
				snprintf(fileName, sizeof(fileName), "tmp%.4X.7z", rand());

				FILE *file = fopen(fileName, "wb");
				if (file == nullptr)
				{
					curl_easy_cleanup(curl);
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					return 0;
				}
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

				progInfo progress = { hWnd, GetTickCount(), 0 };
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
				curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
				curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curlProgress);

				rapidjson::StringBuffer json;
				rapidjson::Writer<rapidjson::StringBuffer> writer(json);

				writer.StartObject();
				writer.Key("password");
				writer.String(pdlInfo->password);
				writer.Key("version");
				writer.String(pdlInfo->version);
				writer.EndObject();

				struct curl_httppost* post = nullptr;
				struct curl_httppost* last = nullptr;
				curl_formadd(&post, &last, CURLFORM_COPYNAME, "json", CURLFORM_PTRCONTENTS, json.GetString(), CURLFORM_END);
				curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

				CURLcode ret = curl_easy_perform(curl);
				curl_easy_cleanup(curl);
				fclose(file);

				if (ret == CURLE_OK)
				{
					_mkdir(pdlInfo->version);
					SetCurrentDirectoryA(pdlInfo->version);
					char fileName_[16];
					strcpy(fileName_, "../");
					strcat(fileName_, fileName);
					success = Extract7z(fileName_);
					SetCurrentDirectoryA("..");
				}
				remove(fileName);
			}
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			return 0;
		}, hDlg);
		thread.detach();
		return 0;
	});

	pdlInfo = nullptr;
	return success;
}

struct updateInfo
{
	std::string version;
	std::string date;
	std::string downloadLink;
};

updateInfo g_updateInfo;

void CheckBrowserUpdate()
{
	bool result = false;
	std::string data;
	data.reserve(256);
	CURLcode curlRet = CurlRequset("https://ysc3839.github.io/VCMPBrowser/update/version.json", data, "VCMP/0.4");
	if (curlRet == CURLE_OK)
	{
		do {
			rapidjson::Document dom;
			if (dom.Parse(data).HasParseError() || !dom.IsObject())
				break;

			auto version = dom.FindMember("version");
			if (version == dom.MemberEnd() || !version->value.IsString())
				break;
			g_updateInfo.version = std::string(version->value.GetString(), version->value.GetStringLength());

			auto date = dom.FindMember("date");
			if (date == dom.MemberEnd() || !date->value.IsString())
				break;
			g_updateInfo.date = std::string(date->value.GetString(), date->value.GetStringLength());

			auto downloadLink = dom.FindMember("downloadLink");
			if (downloadLink == dom.MemberEnd() || !downloadLink->value.IsString())
				break;
			g_updateInfo.downloadLink = std::string(downloadLink->value.GetString(), downloadLink->value.GetStringLength());

			result = true;
		} while (0);
	}
	PostMessage(g_hMainWnd, WM_UPDATE, result, 0);
}

void UpdateBrowser()
{
	if (g_updateInfo.version != VERSION) // Version not equal
	{
		SendMessage(g_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)LoadStr(L"Found new browser version!", IDS_FOUNDUPDATE));

		wchar_t message[512];
		swprintf_s(message, L"A new version of VCMPBrowser is available!\nVersion: %hs\nRelease Date: %hs\nDo you want to update?", g_updateInfo.version.c_str(), g_updateInfo.date.c_str());

		if (MessageBox(g_hMainWnd, message, LoadStr(L"Information", IDS_INFORMATION), MB_YESNO | MB_ICONINFORMATION) != IDYES)
			return;

		DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_UPDATE), g_hMainWnd, DialogProc, (LPARAM)(DLGPROC)[](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
			hCancelEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			if (!hCancelEvent)
				return (INT_PTR)FALSE;

			std::thread thread([](HWND hWnd) {
				CURL *curl = curl_easy_init();
				if (curl)
				{
					curl_easy_setopt(curl, CURLOPT_URL, g_updateInfo.downloadLink.c_str());

					SetCurrentDirectory(g_exePath);

					char fileName[16];
					srand(GetTickCount());
					snprintf(fileName, sizeof(fileName), "tmp%.4X.7z", rand());

					FILE *file = fopen(fileName, "wb");
					if (file == nullptr)
					{
						curl_easy_cleanup(curl);
						PostMessage(hWnd, WM_CLOSE, 0, 0);
						return 0;
					}
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

					progInfo progress = { hWnd, GetTickCount(), 0 };
					curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
					curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
					curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curlProgress);

					CURLcode ret = curl_easy_perform(curl);
					curl_easy_cleanup(curl);
					fclose(file);

					if (ret == CURLE_OK)
					{
						wchar_t newFile[MAX_PATH];
						wcscpy_s(newFile, _wpgmptr);
						wcscat_s(newFile, L".old");
						MoveFileEx(_wpgmptr, newFile, MOVEFILE_REPLACE_EXISTING);

						Extract7z(fileName);
						remove(fileName);

						STARTUPINFO si = { sizeof(si) };
						GetStartupInfo(&si);
						PROCESS_INFORMATION pi;
						if (CreateProcess(_wpgmptr, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
						{
							CloseHandle(pi.hThread);
							CloseHandle(pi.hProcess);
							EndDialog(hWnd, 0);
							PostMessage(g_hMainWnd, WM_CLOSE, 0, 0);
							return 0;
						}
					}

					remove(fileName);
				}
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return 0;
			}, hDlg);
			thread.detach();
			return 0;
		});
	}
	else
		SendMessage(g_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)LoadStr(L"No update found.", IDS_NOUPDATE));
}
