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

void LaunchVCMP(const char* IP, uint16_t port, const char* playerName, const char* password, const wchar_t* gtaExe, const char* vcmpDll)
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
	if (CreateProcess(gtaExe, commandLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, GTADriectory, &si, &pi))
	{
		// Alloc memory in GTA process.
		size_t dllLength = strlen(vcmpDll) + 1;
		LPVOID lpMem = VirtualAllocEx(pi.hProcess, NULL, dllLength, MEM_COMMIT, PAGE_READWRITE);
		if (lpMem)
		{
			// Wirte VCMP dll path to GTA process.
			if (WriteProcessMemory(pi.hProcess, lpMem, vcmpDll, dllLength, NULL))
			{
				// Get kernel32.dll handle.
				HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
				if (hKernel)
				{
					// Get LoadLibraryA address.
					LPTHREAD_START_ROUTINE fnLoadLibraryA = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel, "LoadLibraryA");
					if (fnLoadLibraryA)
					{
						// Create remote thread in GTA process to inject VCMP dll.
						HANDLE hInjectThread = CreateRemoteThread(pi.hProcess, NULL, 0, fnLoadLibraryA, lpMem, 0, NULL);
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
