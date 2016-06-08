#pragma once

void DownloadVCMPGame(const char *version, const char *password)
{
	rapidjson::StringBuffer json;
	rapidjson::Writer<rapidjson::StringBuffer> writer(json);

	writer.StartObject();
	writer.Key("password");
	writer.String(password);
	writer.Key("version");
	writer.String(version);
	writer.EndObject();

	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_UPDATE), g_hMainWnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
		static HANDLE hEvent;
		switch (message)
		{
		case WM_INITDIALOG:
		{
			char *json = (char *)lParam;
			hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			if (!hEvent)
				return (INT_PTR)FALSE;
			std::thread thread([](HWND hWnd, char *json) {
				CURL *curl = curl_easy_init();
				if (curl)
				{
					curl_easy_setopt(curl, CURLOPT_URL, "http://u04.maxorator.com/download");

					SetCurrentDirectory(g_exePath);

					char fileName[16];
					srand(GetTickCount());
					snprintf(fileName, 16, "tmp%.4X.7z", rand());
					FILE *file = fopen(fileName, "wb");
					if (file == nullptr)
					{
						curl_easy_cleanup(curl);
						return 0;
					}
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

					curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
					struct progInfo {
						HWND hWnd;
						uint32_t time;
						curl_off_t size;
					};
					progInfo progress = { hWnd, GetTickCount(), 0 };
					curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
					curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, (curl_xferinfo_callback)[](void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) -> int {
						if (WaitForSingleObject(hEvent, 0) != WAIT_TIMEOUT) // Cancel
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
					});

					struct curl_httppost* post = nullptr;
					struct curl_httppost* last = nullptr;
					curl_formadd(&post, &last, CURLFORM_COPYNAME, "json", CURLFORM_PTRCONTENTS, json, CURLFORM_END);
					curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

					CURLcode ret = curl_easy_perform(curl);

					fclose(file);

					if (ret == CURLE_ABORTED_BY_CALLBACK) // User cancel
					{
						remove(fileName);
					}

					curl_easy_cleanup(curl);
				}
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return 0;
			}, hDlg, json);
			thread.detach();
		}
		return (INT_PTR)TRUE;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
		case WM_DESTROY:
			SetEvent(hEvent);
			CloseHandle(hEvent);
			return (INT_PTR)TRUE;
		case WM_PROGRESS:
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
			break;
		}
		return (INT_PTR)FALSE;
	}, (LPARAM)json.GetString());
}
