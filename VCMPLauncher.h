#pragma once

void ShowSettings();
void CheckGameStatus(std::wstring commandLine, std::wstring vcmpDll, HANDLE hProcess, bool isSteam);

int MessageBoxPrintError(HWND hWnd, LPCWSTR lpText, ...)
{
	va_list argList;
	wchar_t buffer[256];
	va_start(argList, lpText);
	vswprintf_s(buffer, lpText, argList);
	va_end(argList);
	return TDMessageBox(hWnd, buffer, LoadStr(L"Error", IDS_ERROR), MB_ICONERROR);
}

void LaunchVCMP(wchar_t* commandLine, const wchar_t* gtaExe, const wchar_t* vcmpDll)
{
	// Get GTA directory.
	wchar_t GTADriectory[MAX_PATH];
	wcscpy_s(GTADriectory, MAX_PATH, gtaExe);
	wchar_t *pos = wcsrchr(GTADriectory, '\\');
	if (pos)
		pos[1] = 0;

	// Create GTA process.
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	if (CreateProcess(gtaExe, commandLine, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, GTADriectory, &si, &pi))
	{
		// Alloc memory in GTA process.
		size_t dllLength = (wcslen(vcmpDll) + 1) * sizeof(wchar_t);
		LPVOID lpMem = VirtualAllocEx(pi.hProcess, nullptr, dllLength, MEM_COMMIT, PAGE_READWRITE);
		if (lpMem)
		{
			// Wirte VCMP dll path to GTA process.
			if (WriteProcessMemory(pi.hProcess, lpMem, vcmpDll, dllLength, nullptr))
			{
				// Get kernel32.dll handle.
				HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
				if (hKernel)
				{
					// Get LoadLibraryW address.
					LPTHREAD_START_ROUTINE fnLoadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel, "LoadLibraryW");
					if (fnLoadLibraryW)
					{
						// Create remote thread in GTA process to inject VCMP dll.
						HANDLE hInjectThread = CreateRemoteThread(pi.hProcess, nullptr, 0, fnLoadLibraryW, lpMem, 0, nullptr);
						if (hInjectThread)
						{
							// Wiat for the inject thread.
							if (WaitForSingleObject(hInjectThread, 10000) == WAIT_OBJECT_0)
							{
								DWORD exitCode;
								if (GetExitCodeThread(hInjectThread, &exitCode))
								{
									if (exitCode != 0)
									{
										ResumeThread(pi.hThread);
										std::thread check(CheckGameStatus, std::wstring(commandLine), std::wstring(vcmpDll), pi.hProcess, false);
										check.detach();
									}
									else
									{
										TerminateProcess(pi.hProcess, 0);
										TDMessageBox(g_hMainWnd, LoadStr(L"Injected thread failed!", IDS_INJECTFAIL), LoadStr(L"Error", IDS_ERROR), MB_ICONERROR);
									}
								}
								else
									MessageBoxPrintError(g_hMainWnd, LoadStr(L"GetExitCodeThread failed! (%u)", IDS_GETEXITCODEFAIL), GetLastError());
							}
							else
								TDMessageBox(g_hMainWnd, LoadStr(L"Injected thread hung!", IDS_THREADHUNG), LoadStr(L"Error", IDS_ERROR), MB_ICONERROR);

							CloseHandle(hInjectThread);
						}
						else
							MessageBoxPrintError(g_hMainWnd, LoadStr(L"CreateRemoteThread failed! (%u)", IDS_CREATEREMOTETHREADFAIL), GetLastError());
					}
					else
						MessageBoxPrintError(g_hMainWnd, LoadStr(L"GetProcAddress failed! (%u)", IDS_GETPROCADDRESSFAIL), GetLastError());
				}
				else
					MessageBoxPrintError(g_hMainWnd, LoadStr(L"GetModuleHandle failed! (%u)", IDS_GETMODULEHANDLEFAIL), GetLastError());
			}
			else
				MessageBoxPrintError(g_hMainWnd, LoadStr(L"WriteProcessMemory failed! (%u)", IDS_WRITEMEMORYFAIL), GetLastError());

			VirtualFreeEx(pi.hProcess, lpMem, 0, MEM_RELEASE);
		}
		else
			MessageBoxPrintError(g_hMainWnd, LoadStr(L"VirtualAllocEx failed! (%u)", IDS_VIRTUALALLOCFAIL), GetLastError());

		CloseHandle(pi.hThread);
	}
	else
		MessageBoxPrintError(g_hMainWnd, LoadStr(L"CreateProcess failed! (%u)", IDS_CREATEPROCESSFAIL), GetLastError());
}

void LaunchSteamVCMP(wchar_t* commandLine, const wchar_t* gtaExe, const wchar_t* vcmpDll)
{
	// Get GTA directory.
	wchar_t GTADriectory[MAX_PATH];
	wcscpy_s(GTADriectory, MAX_PATH, gtaExe);
	wchar_t *pos = wcsrchr(GTADriectory, '\\');
	if (pos)
		pos[1] = 0;

	// Create GTA process.
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	if (CreateProcess(gtaExe, commandLine, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, GTADriectory, &si, &pi))
	{
		// Alloc memory in GTA process.
		size_t dllLength = (wcslen(vcmpDll) + 1) * sizeof(wchar_t);
		size_t dataLength = dllLength + 19; // 19 = sizeof(code)
		LPVOID lpMem = VirtualAllocEx(pi.hProcess, nullptr, dataLength, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (lpMem)
		{
			// Get kernel32.dll handle.
			HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
			if (hKernel)
			{
				// Get LoadLibraryW address.
				FARPROC fnLoadLibraryW = GetProcAddress(hKernel, "LoadLibraryW");
				if (fnLoadLibraryW)
				{
					uint8_t code[19];
					code[0] = 0x68; *(int*)&code[1] = (int)lpMem + sizeof(code);		// push lpMem + 19
					code[5] = 0xE8; *(int*)&code[6] = (int)fnLoadLibraryW - (int)lpMem - 10;	// call kernel32.LoadLibraryW
					code[10] = 0x58; // pop eax ; get the OEP
					code[11] = 0x5D; // pop ebp
					code[12] = 0x5F; // pop edi
					code[13] = 0x5E; // pop esi
					code[14] = 0x5A; // pop edx
					code[15] = 0x59; // pop ecx
					code[16] = 0x5B; // pop ebx
					code[17] = 0xFF; code[18] = 0xE0; // jmp eax ; jump to OEP

					// Wirte mechine code to GTA process.
					if (WriteProcessMemory(pi.hProcess, lpMem, code, sizeof(code), nullptr))
					{
						// Wirte VCMP dll path to GTA process.
						if (WriteProcessMemory(pi.hProcess, (LPVOID)((size_t)lpMem + sizeof(code)), vcmpDll, dllLength, nullptr))
						{
							// CRC Check 00A405A5 74 07 je short testapp.00A405AE
							// je->jmp 74->EB
							DWORD oldProtect;
							if (VirtualProtectEx(pi.hProcess, (LPVOID)0xA405A5, 1, PAGE_EXECUTE_READWRITE, &oldProtect))
							{
								static const uint8_t opcode = 0xEB;

								BOOL success = WriteProcessMemory(pi.hProcess, (LPVOID)0xA405A5, &opcode, 1, nullptr);
								VirtualProtectEx(pi.hProcess, (LPVOID)0xA405A5, 1, oldProtect, &oldProtect);

								if (success)
								{
									if (VirtualProtectEx(pi.hProcess, (LPVOID)0xA41298, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
									{
										uint8_t code2[6];
										code2[0] = 0x50; // push eax ; save the OEP
										code2[1] = 0xB8; *(int*)&code2[2] = (int)lpMem; // mov eax,lpMem
										// The next code is "jmp eax", our code will be executed first.

										success = WriteProcessMemory(pi.hProcess, (LPVOID)0xA41298, code2, sizeof(code2), nullptr);
										VirtualProtectEx(pi.hProcess, (LPVOID)0xA41298, 6, oldProtect, &oldProtect);

										if (success)
										{
											ResumeThread(pi.hThread);
											std::thread check(CheckGameStatus, std::wstring(commandLine), std::wstring(vcmpDll), pi.hProcess, true);
											check.detach();
										}
										else
											MessageBoxPrintError(g_hMainWnd, LoadStr(L"WriteProcessMemory failed! (%u)", IDS_WRITEMEMORYFAIL), GetLastError());

									}
									else
										MessageBoxPrintError(g_hMainWnd, LoadStr(L"VirtualProtectEx failed! (%u)", IDS_VIRTUALPROTECTFAIL), GetLastError());
								}
								else
									MessageBoxPrintError(g_hMainWnd, LoadStr(L"WriteProcessMemory failed! (%u)", IDS_WRITEMEMORYFAIL), GetLastError());
							}
							else
								MessageBoxPrintError(g_hMainWnd, LoadStr(L"VirtualProtectEx failed! (%u)", IDS_VIRTUALPROTECTFAIL), GetLastError());

						}
						else
							MessageBoxPrintError(g_hMainWnd, LoadStr(L"WriteProcessMemory failed! (%u)", IDS_WRITEMEMORYFAIL), GetLastError());
					}
					else
						MessageBoxPrintError(g_hMainWnd, LoadStr(L"WriteProcessMemory failed! (%u)", IDS_WRITEMEMORYFAIL), GetLastError());
				}
				else
					MessageBoxPrintError(g_hMainWnd, LoadStr(L"GetProcAddress failed! (%u)", IDS_GETPROCADDRESSFAIL), GetLastError());
			}
			else
				MessageBoxPrintError(g_hMainWnd, LoadStr(L"GetModuleHandle failed! (%u)", IDS_GETMODULEHANDLEFAIL), GetLastError());
		}
		else
			MessageBoxPrintError(g_hMainWnd, LoadStr(L"VirtualAllocEx failed! (%u)", IDS_VIRTUALALLOCFAIL), GetLastError());

		CloseHandle(pi.hThread);
	}
	else
		MessageBoxPrintError(g_hMainWnd, LoadStr(L"CreateProcess failed! (%u)", IDS_CREATEPROCESSFAIL), GetLastError());
}

void CheckGameStatus(std::wstring commandLine, std::wstring vcmpDll, HANDLE hProcess, bool isSteam)
{
	DWORD t = GetTickCount();
	while (1)
	{
		DWORD retval = WaitForSingleObject(hProcess, 500);
		if (retval == WAIT_OBJECT_0)
		{
			if (GetTickCount() - t < 3000) // Less than 3s
			{
				if (TDMessageBox(g_hMainWnd, L"Game process was alive less than 3 sec, do you want to launch again?", nullptr, MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					wchar_t *_commandLine = _wcsdup(commandLine.c_str());
					if (isSteam)
						LaunchSteamVCMP(_commandLine, g_browserSettings.gamePath.c_str(), vcmpDll.c_str());
					else
						LaunchVCMP(_commandLine, g_browserSettings.gamePath.c_str(), vcmpDll.c_str());
					free(_commandLine);
				}
			}
			break;
		}
		else if (retval == WAIT_FAILED)
			break;
		else if (GetTickCount() - t > 3000)
			break;
	}
	CloseHandle(hProcess);
	SendMessage(g_hWndStatusBar, SB_SETTEXT, 0, 0);
}

const char* AskForPassword(const serverAddress &address)
{
	const char *password = nullptr;
	DWORD password_len = 1;
	auto password_it = g_passwordList.find(address);
	if (password_it != g_passwordList.end())
	{
		password = password_it->second.c_str();
		password_len = password_it->second.length() + 1;
	}

	wchar_t *password_w = (wchar_t *)malloc(password_len * sizeof(wchar_t));
	if (password)
	{
		for (size_t i = 0; i < password_len; i++)
			password_w[i] = password[i];
	}
	else
		password_w[0] = 0;

	wchar_t username[24];
	for (size_t i = 0; i < ARRAYSIZE(username); i++)
		username[i] = g_browserSettings.playerName[i];

	DWORD in_auth_buf_size = 0;
	if (!CredPackAuthenticationBufferW(0, username, password_w, nullptr, &in_auth_buf_size) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		void *in_auth_buf = CoTaskMemAlloc(in_auth_buf_size);
		if (CredPackAuthenticationBufferW(0, username, password_w, (PBYTE)in_auth_buf, &in_auth_buf_size))
		{
			CREDUI_INFOW ui = { sizeof(ui) };
			ui.hwndParent = g_hMainWnd;
			ui.pszCaptionText = L"Test1";
			ui.pszMessageText = L"Test2";

			ULONG auth_package = 0;
			void *out_auth_buf;
			ULONG out_auth_buf_size;
			BOOL save = password ? TRUE : FALSE;
			if (CredUIPromptForWindowsCredentialsW(&ui, 0, &auth_package, in_auth_buf, in_auth_buf_size, &out_auth_buf, &out_auth_buf_size, &save, CREDUIWIN_GENERIC | CREDUIWIN_CHECKBOX | CREDUIWIN_IN_CRED_ONLY) == ERROR_SUCCESS)
			{
				if (password && save == FALSE)
				{
					g_passwordList.erase(password_it);
					password = nullptr;
				}

				DWORD username_size = ARRAYSIZE(username);
				BOOL result = CredUnPackAuthenticationBufferW(0, out_auth_buf, out_auth_buf_size, username, &username_size, nullptr, nullptr, password_w, &password_len);
				if (!result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					free(password_w);
					password_w = (wchar_t *)malloc(password_len * sizeof(wchar_t));
					result = CredUnPackAuthenticationBufferW(0, out_auth_buf, out_auth_buf_size, username, &username_size, nullptr, nullptr, password_w, &password_len);
				}
				if (result)
				{
					if (password_len == 1)
						password = nullptr;
					else
					{
						static std::string _password;
						_password.clear();
						_password.reserve(password_len);
						for (size_t i = 0; i < password_len; i++)
							_password.push_back((char)password_w[i]);
						password = _password.c_str();

						if (save)
						{
							if (password_it != g_passwordList.end())
								password_it->second = _password;
							else
								g_passwordList[address] = _password;
						}
					}
				}
			}
			else
				password = nullptr;

			SecureZeroMemory(in_auth_buf, in_auth_buf_size);
			CoTaskMemFree(in_auth_buf);
		}
	}

	free(password_w);
	return password;
}

wchar_t* ServerInfoToCommandLine(const serverAllInfo &serverInfo)
{
	char ipstr[16];
	char *ip = (char *)&(serverInfo.address.ip);
	snprintf(ipstr, sizeof(ipstr), "%hhu.%hhu.%hhu.%hhu", ip[0], ip[1], ip[2], ip[3]);

	static wchar_t commandLine[128];
	if (serverInfo.info.isPassworded)
	{
		const char *password = AskForPassword(serverInfo.address);
		if (password)
			swprintf_s(commandLine, std::size(commandLine), L"-c -h %hs -c -p %hu -n %hs -z %hs", ipstr, serverInfo.address.port, g_browserSettings.playerName, password);
		else
			return nullptr;
	}
	else
		swprintf_s(commandLine, std::size(commandLine), L"-c -h %hs -c -p %hu -n %hs", ipstr, serverInfo.address.port, g_browserSettings.playerName);

	return commandLine;
}

void LaunchGame(wchar_t *commandLine, const char *versionName)
{
	SendMessage(g_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)L"Launching game...");

	bool isSteam = false;

	const wchar_t *name = wcsrchr(g_browserSettings.gamePath.c_str(), L'\\');
	if (wcsncmp(++name, L"testapp.exe", 11) == 0)
		isSteam = true;

	wchar_t vcmpDll[MAX_PATH];
	swprintf(vcmpDll, MAX_PATH, L"%s%hs\\%s", g_exePath, versionName, isSteam ? L"vcmp-steam.dll" : L"vcmp-game.dll");

	if (_waccess(vcmpDll, 0) == -1)
		if (!DownloadVCMPGame(versionName, g_browserSettings.gameUpdatePassword.c_str()))
			return;

	if (isSteam)
		LaunchSteamVCMP(commandLine, g_browserSettings.gamePath.c_str(), vcmpDll);
	else
		LaunchVCMP(commandLine, g_browserSettings.gamePath.c_str(), vcmpDll);
}

void LaunchGame(const serverAllInfo &serverInfo)
{
	wchar_t *commandLine = ServerInfoToCommandLine(serverInfo);
	if (commandLine)
		LaunchGame(commandLine, serverInfo.info.versionName);
}
