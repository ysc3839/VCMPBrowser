#pragma once

int TDMessageBox(HWND hWnd, const wchar_t *text, const wchar_t *title, uint32_t style)
{
	typedef HRESULT (WINAPI *fnTaskDialog)(HWND hwndOwner, HINSTANCE hInstance, PCWSTR pszWindowTitle, PCWSTR pszMainInstruction, PCWSTR pszContent, TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons, PCWSTR pszIcon, int *pnButton);
	static fnTaskDialog pTaskDialog = nullptr;
	static int taskDialogStatus = 0; // 0-Unchecked 1-Available 2-Unavailable
	if (taskDialogStatus == 0)
	{
		taskDialogStatus = 2;
		HMODULE hComctl = GetModuleHandle(L"comctl32.dll");
		if (hComctl)
		{
			pTaskDialog = (fnTaskDialog)GetProcAddress(hComctl, "TaskDialog");
			if (pTaskDialog)
				taskDialogStatus = 1;
		}
	}
	if (taskDialogStatus == 1)
	{
		TASKDIALOG_COMMON_BUTTON_FLAGS btnflags = 0;
		switch (style & MB_TYPEMASK)
		{
		case MB_OK:			 btnflags = TDCBF_OK_BUTTON; break;
		case MB_OKCANCEL:	 btnflags = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON; break;
		//case MB_ABORTRETRYIGNORE:	// No abort.
		case MB_YESNOCANCEL: btnflags = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON; break;
		case MB_YESNO:		 btnflags = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON; break;
		case MB_RETRYCANCEL: btnflags = TDCBF_CANCEL_BUTTON | TDCBF_RETRY_BUTTON; break;
		//case MB_CANCELTRYCONTINUE: // No continue.
		}

		PCWSTR icon = nullptr;
		switch (style & MB_ICONMASK)
		{
		case MB_ICONHAND: icon = TD_ERROR_ICON; break;
		case MB_ICONQUESTION: icon = IDI_QUESTION;  break;
		case MB_ICONEXCLAMATION: icon = TD_WARNING_ICON; break;
		case MB_ICONASTERISK: icon = TD_INFORMATION_ICON; break;
		}

		int retval = 0;
		if (pTaskDialog(hWnd, nullptr, title, nullptr, text, btnflags, icon, &retval) != S_OK)
			retval = 0;

		return retval;
	}
	return MessageBoxW(hWnd, text, title, style);
}
