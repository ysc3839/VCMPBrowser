#include "VCMPBrowser.h"

HINSTANCE g_hInst;
HWND g_hMainWnd;
HWND g_hWndTab;
HWND g_hWndListViewServers;
HWND g_hWndListViewHistory;
HWND g_hWndGroupBox1;
HWND g_hWndListViewPlayers;
HWND g_hWndGroupBox2;
HWND g_hWndStatusBar;
serverMasterList *g_serversMasterList = nullptr;
SOCKET g_UDPSocket;
int g_currentTab = 0;// 0=Favorites, 1=Internet, 2=Official, 3=Lan, 4=History
serverList g_serversList;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
CURLcode CurlRequset(const char *URL, std::string &data, const char *userAgent);
bool ParseJson(const char *json, serverMasterList &serversList);
bool GetServerInfo(char *data, int length, serverInfo &serverInfo);
bool ConvertCharset(const char *from, std::wstring &to);
void SendQuery(serverAddress address, char opcode);
void DownloadVCMPGame(const char *version, const char *password = "");

inline wchar_t* LoadStr(wchar_t* origString, UINT ID) { wchar_t* str; return (LoadString(g_hInst, ID, (LPWSTR)&str, 0) ? str : origString); }

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	DownloadVCMPGame("04rel003");
	//SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));

	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
		return FALSE;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NOERROR)
		return FALSE;

	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
		return FALSE;

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();

	curl_global_cleanup();

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VCMPBROWSER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_VCMPBROWSER);
	wcex.lpszClassName = L"VCMPBrowser";
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInst = hInstance;

	g_hMainWnd = CreateWindowEx(0, L"VCMPBrowser", LoadStr(L"VCMPBrowser", IDS_APP_TITLE), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1000, 600, nullptr, nullptr, hInstance, nullptr);

	if (!g_hMainWnd)
		return FALSE;

	ShowWindow(g_hMainWnd, nCmdShow);
	UpdateWindow(g_hMainWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static uint32_t lanLastPing = 0;

	switch (message)
	{
	case WM_CREATE:
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);

		g_hWndListViewServers = CreateWindow(WC_LISTVIEW, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_AUTOARRANGE | LVS_OWNERDATA, 1, 21, rcClient.right - UI_PLAYERLIST_WIDTH - 4, rcClient.bottom - UI_SERVERINFO_HEIGHT - 21 - 2, hWnd, nullptr, g_hInst, nullptr);
		if (g_hWndListViewServers)
		{
			SetWindowTheme(g_hWndListViewServers, L"Explorer", nullptr);
			ListView_SetExtendedListViewStyle(g_hWndListViewServers, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

			LVCOLUMN lvc;
			lvc.mask = LVCF_WIDTH;
			lvc.cx = 30;
			ListView_InsertColumn(g_hWndListViewServers, 0, &lvc);

			lvc.mask = LVCF_WIDTH | LVCF_TEXT;
			lvc.cx = 240;
			lvc.pszText = LoadStr(L"Server Name", IDS_SERVERNAME);
			ListView_InsertColumn(g_hWndListViewServers, 1, &lvc);

			lvc.cx = 60;
			lvc.pszText = LoadStr(L"Ping", IDS_PING);
			ListView_InsertColumn(g_hWndListViewServers, 2, &lvc);

			lvc.cx = 80;
			lvc.pszText = LoadStr(L"Players", IDS_PLAYERS);
			ListView_InsertColumn(g_hWndListViewServers, 3, &lvc);

			lvc.cx = 70;
			lvc.pszText = LoadStr(L"Version", IDS_VERSION);
			ListView_InsertColumn(g_hWndListViewServers, 4, &lvc);

			lvc.cx = 120;
			lvc.pszText = LoadStr(L"Gamemode", IDS_GAMEMODE);
			ListView_InsertColumn(g_hWndListViewServers, 5, &lvc);

			lvc.cx = 100;
			lvc.pszText = LoadStr(L"Map Name", IDS_MAPNAME);
			ListView_InsertColumn(g_hWndListViewServers, 6, &lvc);
		}

		g_hWndListViewHistory = CreateWindow(WC_LISTVIEW, nullptr, WS_CHILD | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_AUTOARRANGE | LVS_OWNERDATA, 1, 21, rcClient.right - UI_PLAYERLIST_WIDTH - 4, rcClient.bottom - UI_SERVERINFO_HEIGHT - 21 - 2, hWnd, nullptr, g_hInst, nullptr);
		if (g_hWndListViewHistory)
		{
			SetWindowTheme(g_hWndListViewHistory, L"Explorer", nullptr);
			ListView_SetExtendedListViewStyle(g_hWndListViewHistory, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

			LVCOLUMN lvc;
			lvc.mask = LVCF_WIDTH;
			lvc.cx = 30;
			ListView_InsertColumn(g_hWndListViewHistory, 0, &lvc);

			lvc.mask = LVCF_WIDTH | LVCF_TEXT;
			lvc.cx = 220;
			lvc.pszText = LoadStr(L"Server Name", IDS_SERVERNAME);
			ListView_InsertColumn(g_hWndListViewHistory, 1, &lvc);

			lvc.cx = 60;
			lvc.pszText = LoadStr(L"Ping", IDS_PING);
			ListView_InsertColumn(g_hWndListViewHistory, 2, &lvc);

			lvc.cx = 80;
			lvc.pszText = LoadStr(L"Players", IDS_PLAYERS);
			ListView_InsertColumn(g_hWndListViewHistory, 3, &lvc);

			lvc.cx = 70;
			lvc.pszText = LoadStr(L"Version", IDS_VERSION);
			ListView_InsertColumn(g_hWndListViewHistory, 4, &lvc);

			lvc.cx = 100;
			lvc.pszText = LoadStr(L"Gamemode", IDS_GAMEMODE);
			ListView_InsertColumn(g_hWndListViewHistory, 5, &lvc);

			lvc.cx = 160;
			lvc.pszText = LoadStr(L"Last Played", IDS_LASTPLAYED);
			ListView_InsertColumn(g_hWndListViewHistory, 6, &lvc);
		}

		g_hWndTab = CreateWindow(WC_TABCONTROL, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, 0, 0, rcClient.right - UI_PLAYERLIST_WIDTH, rcClient.bottom - UI_SERVERINFO_HEIGHT, hWnd, nullptr, g_hInst, nullptr);
		if (g_hWndTab)
		{
			SetWindowFont(g_hWndTab, hFont, FALSE);

			HIMAGELIST hTabIml = ImageList_Create(16, 16, ILC_COLOR32, 0, 0);
			if (hTabIml)
			{
				for (int i = IDI_FAVORITE; i <= IDI_HISTORY; ++i)
					ImageList_AddIcon(hTabIml, LoadIcon(g_hInst, MAKEINTRESOURCE(i)));
				TabCtrl_SetImageList(g_hWndTab, hTabIml);
			}

			TCITEM tie;
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = 0;
			tie.pszText = LoadStr(L"Favorites", IDS_FAVORITES);
			TabCtrl_InsertItem(g_hWndTab, 0, &tie);

			tie.iImage = 1;
			tie.pszText = LoadStr(L"Internet", IDS_INTERNET);
			TabCtrl_InsertItem(g_hWndTab, 1, &tie);

			tie.iImage = 1;
			tie.pszText = LoadStr(L"Official", IDS_OFFICIAL);
			TabCtrl_InsertItem(g_hWndTab, 2, &tie);

			tie.iImage = 2;
			tie.pszText = LoadStr(L"Lan", IDS_LAN);
			TabCtrl_InsertItem(g_hWndTab, 3, &tie);

			tie.iImage = 3;
			tie.pszText = LoadStr(L"History", IDS_HISTORY);
			TabCtrl_InsertItem(g_hWndTab, 4, &tie);
		}

		g_hWndGroupBox1 = CreateWindow(WC_BUTTON, LoadStr(L"Players", IDS_PLAYERSLIST), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_GROUPBOX, rcClient.right - UI_PLAYERLIST_WIDTH, 0, UI_PLAYERLIST_WIDTH, rcClient.bottom - UI_SERVERINFO_HEIGHT, hWnd, nullptr, g_hInst, nullptr);
		if (g_hWndGroupBox1)
		{
			SetWindowFont(g_hWndGroupBox1, hFont, FALSE);

			g_hWndListViewPlayers = CreateWindowEx(0, WC_LISTVIEW, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER | LVS_SINGLESEL, 1, 12, UI_PLAYERLIST_WIDTH - 2, rcClient.bottom - UI_SERVERINFO_HEIGHT - 12 - 2, g_hWndGroupBox1, nullptr, g_hInst, nullptr);
			if (g_hWndListViewPlayers)
			{
				SetWindowTheme(g_hWndListViewPlayers, L"Explorer", nullptr);
				ListView_SetExtendedListViewStyle(g_hWndListViewPlayers, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

				LVCOLUMN lvc;
				lvc.mask = LVCF_WIDTH;
				lvc.cx = UI_PLAYERLIST_WIDTH - 2;
				ListView_InsertColumn(g_hWndListViewPlayers, 0, &lvc);
			}
		}

		g_hWndGroupBox2 = CreateWindow(WC_BUTTON, LoadStr(L"Server Info", IDS_SERVERINFO), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_GROUPBOX, 0, rcClient.bottom - UI_SERVERINFO_HEIGHT, rcClient.right, 118, hWnd, nullptr, g_hInst, nullptr);
		if (g_hWndGroupBox2)
		{
			SetWindowFont(g_hWndGroupBox2, hFont, FALSE);

			int y = 18;
#define LINE_GAP 20

			HWND hStatic = CreateWindow(WC_STATIC, LoadStr(L"Server Name:", IDS_SERVERNAME1), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 10, y, 100, 16, g_hWndGroupBox2, nullptr, g_hInst, nullptr);
			if (hStatic) SetWindowFont(hStatic, hFont, FALSE);

			HWND hEdit = CreateWindow(WC_EDIT, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | ES_READONLY, 112, y, 300, 16, g_hWndGroupBox2, (HMENU)1001, g_hInst, nullptr);
			if (hEdit) SetWindowFont(hEdit, hFont, FALSE);
			y += LINE_GAP;

			hStatic = CreateWindow(WC_STATIC, LoadStr(L"Server IP:", IDS_SERVERIP), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 10, y, 100, 16, g_hWndGroupBox2, nullptr, g_hInst, nullptr);
			if (hStatic) SetWindowFont(hStatic, hFont, FALSE);

			hEdit = CreateWindow(WC_EDIT, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | ES_READONLY, 112, y, 300, 16, g_hWndGroupBox2, (HMENU)1002, g_hInst, nullptr);
			if (hEdit) SetWindowFont(hEdit, hFont, FALSE);
			y += LINE_GAP;

			hStatic = CreateWindow(WC_STATIC, LoadStr(L"Server Players:", IDS_SERVERPLAYERS), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 10, y, 100, 16, g_hWndGroupBox2, nullptr, g_hInst, nullptr);
			if (hStatic) SetWindowFont(hStatic, hFont, FALSE);

			hEdit = CreateWindow(WC_EDIT, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | ES_READONLY, 112, y, 300, 16, g_hWndGroupBox2, (HMENU)1003, g_hInst, nullptr);
			if (hEdit) SetWindowFont(hEdit, hFont, FALSE);
			y += LINE_GAP;

			hStatic = CreateWindow(WC_STATIC, LoadStr(L"Server Ping:", IDS_SERVERPING), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 10, y, 100, 16, g_hWndGroupBox2, nullptr, g_hInst, nullptr);
			if (hStatic) SetWindowFont(hStatic, hFont, FALSE);

			hEdit = CreateWindow(WC_EDIT, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | ES_READONLY, 112, y, 300, 16, g_hWndGroupBox2, (HMENU)1004, g_hInst, nullptr);
			if (hEdit) SetWindowFont(hEdit, hFont, FALSE);
			y += LINE_GAP;

			hStatic = CreateWindow(WC_STATIC, LoadStr(L"Server Gamemode:", IDS_SERVERGAMEMODE), WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 10, y, 100, 16, g_hWndGroupBox2, nullptr, g_hInst, nullptr);
			if (hStatic) SetWindowFont(hStatic, hFont, FALSE);

			hEdit = CreateWindow(WC_EDIT, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | ES_READONLY, 112, y, 300, 16, g_hWndGroupBox2, (HMENU)1005, g_hInst, nullptr);
			if (hEdit) SetWindowFont(hEdit, hFont, FALSE);
		}

		g_hWndStatusBar = CreateWindow(STATUSCLASSNAME, nullptr, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, nullptr, g_hInst, nullptr);

		do {
			g_UDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (g_UDPSocket == INVALID_SOCKET)
				break;

			uint32_t timeout = 2000;
			setsockopt(g_UDPSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

			struct sockaddr_in bindaddr = { AF_INET };
			if (bind(g_UDPSocket, (sockaddr *)&bindaddr, 16) != NO_ERROR)
			{
				closesocket(g_UDPSocket);
				break;
			}

			if (WSAAsyncSelect(g_UDPSocket, hWnd, WM_SOCKET, FD_READ) == SOCKET_ERROR)
			{
				closesocket(g_UDPSocket);
				break;
			}

			return 0;
		} while (0);
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGE:
		{
			g_currentTab = TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom);
			switch (g_currentTab)
			{
			case 0: // Favorites
			case 1: // Internet
			case 2: // Official
			case 3: // Lan
				g_serversList.clear();
				ListView_DeleteAllItems(g_hWndListViewServers);
				ShowWindow(g_hWndListViewServers, SW_SHOW);
				ShowWindow(g_hWndListViewHistory, SW_HIDE);
				UpdateWindow(g_hWndListViewServers);
				if (g_currentTab == 1 || g_currentTab == 2)
				{
					HWND hDialog = CreateDialog(g_hInst, MAKEINTRESOURCEW(IDD_LOADING), hWnd, nullptr);
					SetWindowPos(hDialog, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
					UpdateWindow(hDialog);

					if (g_serversMasterList)
					{
						delete g_serversMasterList;
						g_serversMasterList = nullptr;
					}

					std::string data;
					data.reserve(2048);

					CURLcode curlRet = CurlRequset(g_currentTab == 1 ? "http://master.vc-mp.org/servers" : "http://master.vc-mp.org/official", data, "VCMP/0.4");
					if (curlRet == CURLE_OK)
					{
						serverMasterList serversList;
						if (ParseJson(data.data(), serversList))
						{
							for (auto it = serversList.begin(); it != serversList.end(); ++it)
							{
								SendQuery(it->address, 'i');
								it->lastPing = GetTickCount();
							}
							g_serversMasterList = new serverMasterList(serversList);
						}
						else
						{
							MessageBox(hWnd, LoadStr(L"Can't parse master list data.", IDS_MASTERLISTDATA), LoadStr(L"Error", IDS_ERROR), MB_ICONWARNING);
						}
					}
					else
					{
						wchar_t message[512];
						std::wstring strerror;
						ConvertCharset(curl_easy_strerror(curlRet), strerror);
						swprintf_s(message, LoadStr(L"Can't get information from master list.\n%s", IDS_MASTERLISTFAILED), strerror.c_str());
						MessageBox(hWnd, message, LoadStr(L"Error", IDS_ERROR), MB_ICONWARNING);
					}

					DestroyWindow(hDialog);
				}
				else if (g_currentTab == 3)
				{
					BOOL broadcast = TRUE;
					setsockopt(g_UDPSocket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast));

					for (uint16_t port = 8000; port <= 8200; ++port)
					{
						serverAddress address = { INADDR_BROADCAST, port };
						SendQuery(address, 'i');

					}

					broadcast = FALSE;
					setsockopt(g_UDPSocket, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast));

					lanLastPing = GetTickCount();
				}
				break;
			case 4: // History
				ShowWindow(g_hWndListViewHistory, SW_SHOW);
				ShowWindow(g_hWndListViewServers, SW_HIDE);
				break;
			}
		}
		break;
		case LVN_GETDISPINFO:
		{
			LPNMLVDISPINFOW di = (LPNMLVDISPINFOW)lParam;
			size_t i = di->item.iItem;
			if (g_serversList.size() > i)
			{
				if (di->item.iSubItem == 0 && di->item.mask & LVIF_IMAGE)
					di->item.iImage = 0;

				if (di->item.mask & LVIF_TEXT)
				{
					switch (di->item.iSubItem)
					{
					case 0: // Icon
						break;
					case 1: // Server Name
						if (di->item.cchTextMax > 0 && di->item.pszText)
						{
							MultiByteToWideChar(CP_ACP, 0, g_serversList[i].info.serverName.c_str(), -1, di->item.pszText, di->item.cchTextMax);
						}
						break;
					case 2: // Ping
					{
						uint32_t ping = g_serversList[i].lastRecv - g_serversList[i].lastPing[1];
						_itow_s(ping, di->item.pszText, di->item.cchTextMax, 10);
					}
					break;
					case 3: // Players
						swprintf_s(di->item.pszText, di->item.cchTextMax, L"%hu/%hu", g_serversList[i].info.players, g_serversList[i].info.maxPlayers);
						break;
					case 4: // Version
					{
						MultiByteToWideChar(CP_ACP, 0, g_serversList[i].info.versionName, -1, di->item.pszText, di->item.cchTextMax);
					}
					break;
					case 5: // Gamemode
					{
						MultiByteToWideChar(CP_ACP, 0, g_serversList[i].info.gameMode.c_str(), -1, di->item.pszText, di->item.cchTextMax);
					}
					break;
					case 6: // Map name
					{
						MultiByteToWideChar(CP_ACP, 0, g_serversList[i].info.mapName.c_str(), -1, di->item.pszText, di->item.cchTextMax);
					}
					break;
					}
				}
			}
		}
		break;
		case NM_CUSTOMDRAW:
		{
			LPNMLVCUSTOMDRAW nmcd = (LPNMLVCUSTOMDRAW)lParam;
			switch (nmcd->nmcd.dwDrawStage)
			{
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;
			case CDDS_ITEMPREPAINT:
			{
				COLORREF crText;
				size_t i = nmcd->nmcd.dwItemSpec;
				if (g_serversList.size() > i && g_serversList[i].isOfficial)
					crText = RGB(0, 0, 255);
				else
					crText = 0;

				nmcd->clrText = crText;
				return CDRF_DODEFAULT;
			}
			}
		}
		break;
		case NM_CLICK:
		{
			LPNMITEMACTIVATE nmitem = (LPNMITEMACTIVATE)lParam;
			size_t i = nmitem->iItem;
			if (i != -1 && g_serversList.size() > i)
			{
				std::wstring wstr;

				ConvertCharset(g_serversList[i].info.serverName.c_str(), wstr);
				SetDlgItemText(g_hWndGroupBox2, 1001, wstr.c_str()); // Server Name

				wchar_t ipstr[22];
				char *ip = (char *)&(g_serversList[i].address.ip);
				swprintf_s(ipstr, L"%hhu.%hhu.%hhu.%hhu:%hu", ip[0], ip[1], ip[2], ip[3], g_serversList[i].address.port);
				SetDlgItemText(g_hWndGroupBox2, 1002, ipstr); // Server IP

				wchar_t playersstr[12];
				swprintf_s(playersstr, L"%hu/%hu", g_serversList[i].info.players, g_serversList[i].info.maxPlayers);
				SetDlgItemText(g_hWndGroupBox2, 1003, playersstr); // Server Players

				wchar_t pingsstr[12];
				uint32_t ping = g_serversList[i].lastRecv - g_serversList[i].lastPing[1];
				_itow_s(ping, pingsstr, 10);
				SetDlgItemText(g_hWndGroupBox2, 1004, pingsstr); // Server Ping

				ConvertCharset(g_serversList[i].info.gameMode.c_str(), wstr);
				SetDlgItemText(g_hWndGroupBox2, 1005, wstr.c_str()); // Server Gamemode

				SendQuery(g_serversList[i].address, 'i');
				g_serversList[i].lastPing[0] = GetTickCount();
			}
			else
			{
				for (int i = 1001; i <= 1005; ++i)
					SetDlgItemText(g_hWndGroupBox2, i, nullptr);
			}
		}
		break;
		}
	}
	break;
	case WM_SIZE:
	{
		int clientWidth = GET_X_LPARAM(lParam), clientHeight = GET_Y_LPARAM(lParam);
		SetWindowPos(g_hWndTab, 0, 0, 0, clientWidth - UI_PLAYERLIST_WIDTH, clientHeight - UI_SERVERINFO_HEIGHT, SWP_NOZORDER);
		SetWindowPos(g_hWndListViewServers, 0, 1, 21, clientWidth - UI_PLAYERLIST_WIDTH - 4, clientHeight - UI_SERVERINFO_HEIGHT - 21 - 2, SWP_NOZORDER);
		SetWindowPos(g_hWndListViewHistory, 0, 1, 21, clientWidth - UI_PLAYERLIST_WIDTH - 4, clientHeight - UI_SERVERINFO_HEIGHT - 21 - 2, SWP_NOZORDER);
		SetWindowPos(g_hWndGroupBox1, 0, clientWidth - UI_PLAYERLIST_WIDTH, 0, UI_PLAYERLIST_WIDTH, clientHeight - UI_SERVERINFO_HEIGHT, SWP_NOZORDER);
		SetWindowPos(g_hWndListViewPlayers, 0, 1, 12, UI_PLAYERLIST_WIDTH - 2, clientHeight - UI_SERVERINFO_HEIGHT - 12 - 2, SWP_NOZORDER);
		SetWindowPos(g_hWndGroupBox2, 0, 0, clientHeight - UI_SERVERINFO_HEIGHT, clientWidth, 118, SWP_NOZORDER);
		SendMessage(g_hWndStatusBar, WM_SIZE, 0, 0);
	}
	break;
	case WM_GETMINMAXINFO:
		((LPMINMAXINFO)lParam)->ptMinTrackSize = { 750, 500 };
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SOCKET:
	{
		if (WSAGETSELECTEVENT(lParam) == FD_READ)
		{
			char *recvBuf = (char *)calloc(1024, sizeof(char));
			if (recvBuf)
			{
				struct sockaddr_in recvAddr;
				int addrLen = sizeof(recvAddr);
				int recvLen = recvfrom(g_UDPSocket, recvBuf, 1024, 0, (sockaddr *)&recvAddr, &addrLen);
				if (recvLen != -1 && recvLen >= 11)
				{
					if (recvLen > 1024)
						recvLen = 1024;

					if (*(int *)recvBuf == 0x3430504D) // MP04
					{
						uint32_t ip = recvAddr.sin_addr.s_addr;
						uint16_t port = ntohs(recvAddr.sin_port);

						bool found = false;
						serverMasterListInfo masterInfo;
						if (g_currentTab == 1 || g_currentTab == 2)
						{
							for (auto it = g_serversMasterList->begin(); it != g_serversMasterList->end(); ++it)
							{
								if (it->address.ip == ip && it->address.port == port)
								{
									found = true;
									masterInfo = *it;
									break;
								}
							}
						}
						else if (g_currentTab == 3) // Lan
						{
							found = true;
							masterInfo.address = { ip, port };
							masterInfo.isOfficial = false;
							masterInfo.lastPing = lanLastPing;
						}

						if (found)
						{
							serverInfo info;
							if (GetServerInfo(recvBuf, recvLen, info))
							{
								bool inList = false;
								for (auto it = g_serversList.begin(); it != g_serversList.end(); ++it)
								{
									if (it->address.ip == ip && it->address.port == port)
									{
										inList = true;
										it->lastRecv = GetTickCount();
										it->lastPing[1] = it->lastPing[0];
										it->info = info;
										auto i = it - g_serversList.begin();
										ListView_Update(g_hWndListViewServers, i);
										break;
									}
								}
								if (!inList)
								{
									serverAllInfo allInfo;
									allInfo.address = masterInfo.address;
									allInfo.info = info;
									allInfo.isOfficial = masterInfo.isOfficial;
									allInfo.lastPing[0] = masterInfo.lastPing;
									allInfo.lastPing[1] = masterInfo.lastPing;
									allInfo.lastRecv = GetTickCount();
									g_serversList.push_back(allInfo);

									LVITEM lvi = { 0 };
									ListView_InsertItem(g_hWndListViewServers, &lvi);
								}
							}
						}
					}
				}
				free(recvBuf);
			}
		}
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_CLICK:
		case NM_RETURN:
			ShellExecute(hDlg, L"open", ((PNMLINK)lParam)->item.szUrl, nullptr, nullptr, SW_SHOW);
		}
	}
	return (INT_PTR)FALSE;
}

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

bool GetServerInfo(char *data, int length, serverInfo &serverInfo)
{
	char *_data = data + 10;

	char opcode = *_data;
	_data += 1;
	switch (opcode)
	{
	case 'i':
	{
		if (length < 11 + 12 + 1 + 2 + 2 + 4) // 12=Version name, 1=Password, 2=Players, 2=MaxPlayers, 4=strlen
			return false;

		memmove(serverInfo.versionName, _data, sizeof(serverInfo.versionName));
		_data += 12;

		serverInfo.isPassworded = *_data != false;
		_data += 1;

		serverInfo.players = *(uint16_t *)_data;
		_data += 2;

		serverInfo.maxPlayers = *(uint16_t *)_data;
		_data += 2;

		int strLen = *(int *)_data;
		_data += 4;
		if (length < 11 + 12 + 1 + 2 + 2 + 4 + strLen + 4)
			return false;

		char *serverName = (char *)alloca(strLen + 1);
		serverInfo.serverName.clear();
		serverInfo.serverName.append(_data, strLen);
		serverInfo.serverName.append(1, '\0');
		_data += strLen;

		strLen = *(int *)_data;
		_data += 4;
		if (length < (_data - data) + strLen + 4)
			return false;

		serverInfo.gameMode.clear();
		serverInfo.gameMode.append(_data, strLen);
		serverInfo.gameMode.append(1, '\0');
		_data += strLen;

		strLen = *(int *)_data;
		_data += 4;
		if (length < (_data - data) + strLen)
			return false;

		serverInfo.mapName.clear();
		serverInfo.mapName.append(_data, strLen);
		serverInfo.mapName.append(1, '\0');
	}
	break;
	case 'c':
		break;
	}
	return true;
}

bool ConvertCharset(const char *from, std::wstring &to)
{
	size_t size = MultiByteToWideChar(CP_ACP, 0, from, -1, nullptr, 0);

	if (size == 0)
		return false;

	wchar_t *buffer = new wchar_t[size];
	if (!buffer)
		return false;

	if (MultiByteToWideChar(CP_ACP, 0, from, -1, buffer, size) == 0)
		return false;

	to.clear();
	to.append(buffer, size);

	delete[] buffer;
	return true;
}

void SendQuery(serverAddress address, char opcode)
{
	struct sockaddr_in sendaddr = { AF_INET };
	sendaddr.sin_addr.s_addr = address.ip;
	sendaddr.sin_port = htons(address.port);

	char *cip = (char *)&(address.ip);
	char *cport = (char *)&(address.port);
	char buffer[] = { 'V', 'C', 'M', 'P', cip[0], cip[1], cip[2], cip[3], cport[0], cport[1], opcode };

	sendto(g_UDPSocket, buffer, sizeof(buffer), 0, (sockaddr *)&sendaddr, sizeof(sendaddr));
}

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

	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DOWNLOAD), g_hMainWnd, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
		static long running = false;
		switch (message)
		{
		case WM_INITDIALOG:
		{
			char *json = (char *)lParam;
			std::thread thread([](HWND hWnd, char *json) {
				CURL *curl = curl_easy_init();
				if (curl)
				{
					curl_easy_setopt(curl, CURLOPT_URL, "http://u04.maxorator.com/download");

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
						if (!_InterlockedCompareExchange(&running, 0, 0)) // Cancel
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

					struct curl_httppost* post = NULL;
					struct curl_httppost* last = NULL;
					curl_formadd(&post, &last, CURLFORM_COPYNAME, "json", CURLFORM_PTRCONTENTS, json, CURLFORM_END);
					curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

					running = true;
					CURLcode ret = curl_easy_perform(curl);
					running = false;

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
			running = false;
			return (INT_PTR)TRUE;
		case WM_PROGRESS:
			SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETPOS, wParam, 0);
			wchar_t *unit = L"B/s";
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
			SetDlgItemText(hDlg, IDC_STATIC1, speedText);
			break;
		}
		return (INT_PTR)FALSE;
	}, (LPARAM)json.GetString());
}
