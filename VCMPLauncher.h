#pragma once

int MessageBoxPrintError(HWND hWnd, LPCWSTR lpText, ...)
{
	va_list argList;
	wchar_t buffer[256];
	va_start(argList, lpText);
	vswprintf_s(buffer, lpText, argList);
	va_end(argList);
	return MessageBox(hWnd, buffer, LoadStr(L"Error", IDS_ERROR), MB_ICONERROR);
}

void LaunchVCMP(const char* IP, uint16_t port, const char* playerName, const char* password, const wchar_t* gtaExe, const wchar_t* vcmpDll)
{
	wchar_t commandLine[128];
	if (password != nullptr)
		swprintf_s(commandLine, std::size(commandLine), L"-c -h %hs -c -p %hu -n %hs -z %hs", IP, port, playerName, password);
	else
		swprintf_s(commandLine, std::size(commandLine), L"-c -h %hs -c -p %hu -n %hs", IP, port, playerName);

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
					LPTHREAD_START_ROUTINE fnLoadLibraryA = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel, "LoadLibraryW");
					if (fnLoadLibraryA)
					{
						// Create remote thread in GTA process to inject VCMP dll.
						HANDLE hInjectThread = CreateRemoteThread(pi.hProcess, nullptr, 0, fnLoadLibraryA, lpMem, 0, nullptr);
						if (hInjectThread)
						{
							// Wiat for the inject thread.
							if (WaitForSingleObject(hInjectThread, 10000) == WAIT_OBJECT_0)
							{
								DWORD exitCode;
								if (GetExitCodeThread(hInjectThread, &exitCode))
								{
									if (exitCode != 0)
										ResumeThread(pi.hThread);
									else
									{
										TerminateProcess(pi.hProcess, 0);
										MessageBox(g_hMainWnd, LoadStr(L"Injected thread failed!", IDS_INJECTFAIL), LoadStr(L"Error", IDS_ERROR), MB_ICONERROR);
									}
								}
								else
									MessageBoxPrintError(g_hMainWnd, LoadStr(L"GetExitCodeThread failed! (%u)", IDS_GETEXITCODEFAIL), GetLastError());
							}
							else
								MessageBox(g_hMainWnd, LoadStr(L"Injected thread hung!", IDS_THREADHUNG), LoadStr(L"Error", IDS_ERROR), MB_ICONERROR);

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

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else
		MessageBoxPrintError(g_hMainWnd, LoadStr(L"CreateProcess failed! (%u)", IDS_CREATEPROCESSFAIL), GetLastError());
}

void LaunchSteamVCMP(const char* IP, uint16_t port, const char* playerName, const char* password, const wchar_t* gtaExe, const wchar_t* vcmpDll)
{
	wchar_t commandLine[128];
	if (password != nullptr)
		swprintf_s(commandLine, std::size(commandLine), L"-c -h %hs -c -p %hu -n %hs -z %hs", IP, port, playerName, password);
	else
		swprintf_s(commandLine, std::size(commandLine), L"-c -h %hs -c -p %hu -n %hs", IP, port, playerName);

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
				FARPROC fnLoadLibraryA = GetProcAddress(hKernel, "LoadLibraryW");
				if (fnLoadLibraryA)
				{
					uint8_t code[19];
					code[0] = 0x68; *(int*)&code[1] = (int)lpMem + sizeof(code);		// push lpMem + 19
					code[5] = 0xE8; *(int*)&code[6] = (int)fnLoadLibraryA - (int)lpMem - 10;	// call kernel32.LoadLibraryW
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

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else
		MessageBoxPrintError(g_hMainWnd, LoadStr(L"CreateProcess failed! (%u)", IDS_CREATEPROCESSFAIL), GetLastError());
}
