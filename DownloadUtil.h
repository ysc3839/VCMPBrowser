#pragma once
#include <direct.h>

struct progInfo {
	HWND hWnd;
	uint32_t time;
	curl_off_t size;
};

LONG canceled = FALSE;

int curlProgress(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	// canceled_old = canceled; if (canceled == TRUE) canceled = FALSE; return canceled_old;
	if (InterlockedCompareExchange(&canceled, FALSE, TRUE) == TRUE) // Cancel
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

bool Extract7z(const char *fileName)
{
#define kInputBufSize ((size_t)1 << 18)
	static const ISzAlloc szAlloc = { SzAlloc, SzFree };

	struct mycompare { bool operator()(const wchar_t *a, const wchar_t *b) const { return _wcsicmp(a, b) < 0; } };
	static const std::set<const wchar_t*, mycompare> excludeFilenames = { L"Thumbs.db" };

	CFileInStream archiveStream;
	if (InFile_Open(&archiveStream.file, fileName) == 0)
	{
		CLookToRead2 lookStream;
		FileInStream_CreateVTable(&archiveStream);
		LookToRead2_CreateVTable(&lookStream, False);
		lookStream.buf = NULL;

		SRes res = SZ_OK;

		{
			lookStream.buf = (Byte *)ISzAlloc_Alloc(&szAlloc, kInputBufSize);
			if (!lookStream.buf)
				res = SZ_ERROR_MEM;
			else
			{
				lookStream.bufSize = kInputBufSize;
				lookStream.realStream = &archiveStream.vt;
				LookToRead2_Init(&lookStream);
			}
		}

		static bool crcTableGenerated = false;
		if (!crcTableGenerated)
		{
			CrcGenerateTable();
			crcTableGenerated = true;
		}

		CSzArEx db;
		SzArEx_Init(&db);

		if (res == SZ_OK)
		{
			res = SzArEx_Open(&db, &lookStream.vt, &szAlloc, &szAlloc);
			if (res == SZ_OK)
			{
				uint32_t blockIndex = 0xFFFFFFFF;
				Byte *outBuffer = 0;
				size_t outBufferSize = 0;

				size_t buflen = 32;
				wchar_t *nameBuf = (wchar_t *)malloc(buflen * sizeof(wchar_t));

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

					if (!PathIsRelativeW(nameBuf) || wcsstr(nameBuf, L"../") != nullptr)
						continue;

					/*for (size_t j = 0; nameBuf[j] != 0; j++)
						if (nameBuf[j] == L'/')
						{
							nameBuf[j] = 0;
							_wmkdir(nameBuf);
							nameBuf[j] = L'\\';
						}*/

					bool isDir = SzArEx_IsDir(&db, i);
					if (isDir)
					{
						CreateDirectoryW(nameBuf, nullptr);
						continue;
					}
					else
					{
						wchar_t *filename = wcsrchr(nameBuf, L'/');
						if (filename)
							filename++;
						else
							filename = nameBuf;

						if (excludeFilenames.find(filename) != excludeFilenames.cend())
							continue;

						size_t offset = 0;
						size_t outSizeProcessed = 0;
						res = SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &szAlloc, &szAlloc);
						if (res != SZ_OK)
							break;

						FILE *outFile = _wfopen(nameBuf, L"wb+");
						if (outFile == nullptr)
						{
							res = SZ_ERROR_WRITE;
							break;
						}

						size_t outSize = fwrite(outBuffer + offset, 1, outSizeProcessed, outFile);
						fclose(outFile);

						if (outSize != outSizeProcessed)
						{
							res = SZ_ERROR_WRITE;
							break;
						}
					}
				}
				ISzAlloc_Free(&szAlloc, outBuffer);

				free(nameBuf);
				SzArEx_Free(&db, &szAlloc);
			}
			ISzAlloc_Free(&szAlloc, lookStream.buf);
		}
		File_Close(&archiveStream.file);
		return res == SZ_OK;
	}
	return false;
}

using DownloadFunc = void(*)(HWND hTDWnd, HWND hMsgWnd);

HRESULT CALLBACK TaskDialogCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData)
{
	switch (msg)
	{
	case TDN_CREATED:
		PostMessageW(hWnd, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 0);

		HWND hMsgWnd = CreateWindowW(WC_STATICW, nullptr, WS_CHILD, 0, 0, 0, 0, hWnd, nullptr, g_hInst, nullptr);
		SetWindowLongPtrW(hMsgWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hWnd));

		static bool progBarStatus;
		progBarStatus = false;
		static LONG_PTR prevWndProc;
		prevWndProc = SetWindowLongPtrW(hMsgWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(static_cast<WNDPROC>(
		[](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
			if (message == WM_PROGRESS)
			{
				HWND hTDWnd = reinterpret_cast<HWND>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
				if (wParam != -1)
				{
					if (!progBarStatus)
					{
						SendMessageW(hTDWnd, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
						progBarStatus = true;
					}
					SendMessageW(hTDWnd, TDM_SET_PROGRESS_BAR_POS, wParam, 0);
					const wchar_t *unit = L"B/s";
					float speed = static_cast<float>(lParam);
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
					SendMessageW(hTDWnd, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, reinterpret_cast<LPARAM>(speedText));
				}
				else
				{
					SendMessageW(hTDWnd, TDM_SET_PROGRESS_BAR_STATE, PBST_ERROR, 0);
					SendMessageW(hTDWnd, TDM_UPDATE_ICON, TDIE_ICON_MAIN, reinterpret_cast<LPARAM>(TD_ERROR_ICON));
					SendMessageW(hTDWnd, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, reinterpret_cast<LPARAM>(L"Download failed."));
				}
				return 0;
			}
			return CallWindowProcW(reinterpret_cast<WNDPROC>(prevWndProc), hWnd, message, wParam, lParam);
		})));

		canceled = FALSE;
		reinterpret_cast<DownloadFunc>(lpRefData)(hWnd, hMsgWnd);
		break;
	}
	return S_OK;
}

bool DownloadVCMPGame(const char *version, const char *password)
{
	struct downloadInfo {
		const char *version;
		const char *password;
	};
	downloadInfo dlInfo = { version, password };
	static downloadInfo* pdlInfo;
	pdlInfo = &dlInfo;
	static bool success;
	success = false;

	TASKDIALOGCONFIG tdConfig = { sizeof(tdConfig) };
	tdConfig.hwndParent = g_hMainWnd;
	tdConfig.hInstance = g_hInst;
	tdConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_POSITION_RELATIVE_TO_WINDOW;
	tdConfig.dwCommonButtons = TDCBF_CANCEL_BUTTON;
	tdConfig.pszWindowTitle = L"Downloading";
	tdConfig.pfCallback = TaskDialogCallback;
	tdConfig.lpCallbackData = reinterpret_cast<LONG_PTR>(static_cast<DownloadFunc>([](HWND hTDWnd, HWND hMsgWnd) {
		std::thread([](HWND hTDWnd, HWND hMsgWnd) {
			CURL *curl = curl_easy_init();
			if (curl)
			{
				std::string url = g_browserSettings.gameUpdateURL + "/download";
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_PROXY, g_browserSettings.proxy.empty() ? nullptr : g_browserSettings.proxy.c_str());

				SetCurrentDirectoryW(g_exePath.c_str());

				char fileName[16];
				srand(GetTickCount());
				snprintf(fileName, sizeof(fileName), "tmp%.4X.7z", rand());

				FILE *file = fopen(fileName, "wb");
				if (file == nullptr)
				{
					curl_easy_cleanup(curl);
					PostMessageW(hMsgWnd, WM_PROGRESS, -1, 0);
					return 0;
				}
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

				progInfo progress = { hMsgWnd, GetTickCount(), 0 };
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
			if (success)
				PostMessageW(hTDWnd, WM_CLOSE, 0, 0);
			else
				PostMessageW(hMsgWnd, WM_PROGRESS, -1, 0);
			return 0;
		}, hTDWnd, hMsgWnd).detach();
	}));
	TaskDialogIndirect(&tdConfig, nullptr, nullptr, nullptr);
	InterlockedExchange(&canceled, TRUE); // canceled = TRUE;
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

		if (TDMessageBox(g_hMainWnd, message, LoadStr(L"Information", IDS_INFORMATION), MB_YESNO | MB_ICONINFORMATION) != IDYES)
			return;

		TASKDIALOGCONFIG tdConfig = { sizeof(tdConfig) };
		tdConfig.hwndParent = g_hMainWnd;
		tdConfig.hInstance = g_hInst;
		tdConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_POSITION_RELATIVE_TO_WINDOW;
		tdConfig.dwCommonButtons = TDCBF_CANCEL_BUTTON;
		tdConfig.pszWindowTitle = L"Downloading";
		tdConfig.pfCallback = TaskDialogCallback;
		tdConfig.lpCallbackData = reinterpret_cast<LONG_PTR>(static_cast<DownloadFunc>([](HWND hTDWnd, HWND hMsgWnd) {
			std::thread([](HWND hTDWnd, HWND hMsgWnd) {
				CURL *curl = curl_easy_init();
				if (curl)
				{
					curl_easy_setopt(curl, CURLOPT_URL, g_updateInfo.downloadLink.c_str());
					curl_easy_setopt(curl, CURLOPT_PROXY, g_browserSettings.proxy.empty() ? nullptr : g_browserSettings.proxy.c_str());

					SetCurrentDirectoryW(g_exePath.c_str());

					char fileName[16];
					srand(GetTickCount());
					snprintf(fileName, sizeof(fileName), "tmp%.4X.7z", rand());

					FILE *file = fopen(fileName, "wb");
					if (file == nullptr)
					{
						curl_easy_cleanup(curl);
						PostMessageW(hMsgWnd, WM_PROGRESS, -1, 0);
						return 0;
					}
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

					progInfo progress = { hMsgWnd, GetTickCount(), 0 };
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
							PostMessageW(hTDWnd, WM_CLOSE, 0, 0);
							PostMessageW(g_hMainWnd, WM_CLOSE, 0, 0);
							return 0;
						}
					}

					remove(fileName);
				}
				PostMessageW(hMsgWnd, WM_PROGRESS, -1, 0);
				return 0;
			}, hTDWnd, hMsgWnd).detach();
		}));
		TaskDialogIndirect(&tdConfig, nullptr, nullptr, nullptr);
		InterlockedExchange(&canceled, TRUE); // canceled = TRUE;
	}
	else
		SendMessage(g_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)LoadStr(L"No update found.", IDS_NOUPDATE));
}
