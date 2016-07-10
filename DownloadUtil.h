#pragma once
#include <direct.h>

bool DownloadVCMPGame(const char *version, const char *password)
{
	struct downloadInfo {
		const char *version;
		const char *password;
	};
	downloadInfo dlInfo = { version, password };
	static bool success = false;

	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_UPDATE), g_hMainWnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
		static HANDLE hEvent;
		switch (message)
		{
		case WM_INITDIALOG:
		{
			downloadInfo *dlInfo = (downloadInfo *)lParam;

			hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			if (!hEvent)
				return (INT_PTR)FALSE;

			std::thread thread([](HWND hWnd, downloadInfo *dlInfo) {
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

					rapidjson::StringBuffer json;
					rapidjson::Writer<rapidjson::StringBuffer> writer(json);

					writer.StartObject();
					writer.Key("password");
					writer.String(dlInfo->password);
					writer.Key("version");
					writer.String(dlInfo->version);
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

								_mkdir(dlInfo->version);
								SetCurrentDirectoryA(dlInfo->version);

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

								SetCurrentDirectoryA("..");

								free(nameBuf);
								SzArEx_Free(&db, &szAlloc);
							}
							File_Close(&archiveStream.file);
							if (res == SZ_OK)
								success = true;
						}
					}
					remove(fileName);
				}
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				return 0;
			}, hDlg, dlInfo);
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
#define PBM_SETSTATE (WM_USER+16)
#define PBST_NORMAL 1
#define PBST_ERROR 2
#define PBST_PAUSED 3
				SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_ERROR, 0);
			}
			break;
		}
		return (INT_PTR)FALSE;
	}, (LPARAM)&dlInfo);
	return success;
}
