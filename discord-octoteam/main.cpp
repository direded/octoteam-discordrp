#include <iostream>
#include <stdlib.h>
#include <Windows.h>
#include <chrono>
#include <fstream>
#include <sstream>

#include "resource.h"

#define ID_TRAY_APP_ICON    1001
#define ID_TRAY_EXIT        1002
#define ID_TRAY_UPDATE		1003
#define WM_SYSICON          (WM_USER + 1)

#pragma comment(lib, "discord-rpc.lib")

#include "discord_register.h"
#include "discord_rpc.h"

using namespace std;
using namespace std::chrono;

// handle discord ready event
void handleDiscordReady(const DiscordUser *u) {
	printf("\nDisplaying Presence for %s#%s\n", u->username, u->discriminator);
}

// handle discord disconnected event
void handleDiscordDisconnected(int errcode, const char *message) {
	printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

// handle discord error event
void handleDiscordError(int errcode, const char *message) {
	printf("\nDiscord: error (%d: %s)\n", errcode, message);
}

TCHAR sAbsolutePathBuffer[2048];
char iniPath[2048] = ".\\config.ini";

const TCHAR szTIP[64] = TEXT("Octothorp Team");
const char szClassName[] = "Octothorp Team Rich Presence";
char state[128];
char details[128];
char largeImageKey[32];
char largeImageText[128];
char smallImageKey[32];
char smallImageText[128];

int enable_state = 1;
int enable_details = 1;
int enable_largeImageKey = 1;
int enable_largeImageText = 1;
int enable_smallImageKey = 1;
int enable_smallImageText = 1;
// "logo_white"
// "logo_dark"
// "logo_octopride"

// update discord rich presence

void updatePresence() {
	// set required variables
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	if (enable_state)
		discordPresence.state = state;
	if (enable_details)
		discordPresence.details = details;
	if (enable_largeImageKey)
		discordPresence.largeImageKey = largeImageKey;
	if (enable_largeImageText)
		discordPresence.largeImageText = largeImageText;
	if (enable_smallImageKey)
		discordPresence.smallImageKey = smallImageKey;
	if (enable_smallImageText)
		discordPresence.smallImageText = smallImageText;
	Discord_UpdatePresence(&discordPresence);
}

bool storeConfigString(string key, string value) {
	if (key == "first_row") {
		strcpy_s(details, value.c_str());
	}
	else if (key == "second_row") {
		strcpy_s(state, value.c_str());
	}
	else if (key == "small_id") {
		strcpy_s(smallImageKey, value.c_str());
	}
	else if (key == "small_text") {
		strcpy_s(smallImageText, value.c_str());
	}
	else if (key == "large_key") {
		strcpy_s(largeImageKey, value.c_str());
	}
	else if (key == "large_text") {
		strcpy_s(largeImageText, value.c_str());
	}
	else {
		return false;
	}
	return true;
}

bool storeConfigInt(string key, int value) {
	if (key == "enable_first_row") {
		enable_details = value;
	}
	else if (key == "enable_second_row") {
		enable_state = value;
	}
	else if (key == "enable_small_id") {
		enable_smallImageKey = value;
	}
	else if (key == "enable_small_text") {
		enable_smallImageText = value;
	}
	else if (key == "enable_large_key") {
		enable_largeImageKey = value;
	}
	else if (key == "enable_large_text") {
		enable_largeImageText = value;
	}
	else {
		return false;
	}
	return true;
}

void updateConfig() {

	std::ifstream ifile(iniPath);
	if (ifile) {
		std::string line;
		while (std::getline(ifile, line))
		{
			cout << line << endl;
			std::istringstream is_line(line);
			std::string key;
			if (std::getline(is_line, key, '='))
			{
				
				std::string value;
				cout << key << " " << value << endl;
				if (std::getline(is_line, value))
					if (!storeConfigString(key, value)) {
						storeConfigInt(key, stoi(value));
					}
			}
		}
	}
	/*
	int res = GetPrivateProfileString("values", "first_row", "Мы делаем игры лучше!", details, 128, iniPath);
	res = GetPrivateProfileString("values", "second_row", "www.octothorp.team", state, 128, iniPath);
	res = GetPrivateProfileString("values", "small_id", "logo_white", smallImageKey, 32, iniPath);
	res = GetPrivateProfileString("values", "small_text", "Octothorp team", smallImageText, 128, iniPath);
	res = GetPrivateProfileString("values", "large_key", "logo_white", largeImageKey, 32, iniPath);
	res = GetPrivateProfileString("values", "large_text", "Присоединяйся к нам!", largeImageText, 128, iniPath);

	enable_details = GetPrivateProfileInt("enable", "first_row", 1, iniPath);
	enable_state = GetPrivateProfileInt("enable", "second_row", 1, iniPath);
	enable_smallImageKey = GetPrivateProfileInt("enable", "small_id", 0, iniPath);
	enable_smallImageText = GetPrivateProfileInt("enable", "small_text", 0, iniPath);
	enable_largeImageKey = GetPrivateProfileInt("enable", "large_key", 1, iniPath);
	enable_largeImageText = GetPrivateProfileInt("enable", "large_text", 1, iniPath);
	*/
}

void discordInit() {
	DiscordEventHandlers handlers;
	
	memset(&handlers, 0, sizeof(handlers));
	handlers.ready = handleDiscordReady;
	handlers.disconnected = handleDiscordDisconnected;
	handlers.errored = handleDiscordError;
	Discord_Initialize("514095622358433845", &handlers, 1, NULL);
}

void discordRestart() {
	Discord_Shutdown();
	discordInit();
	updatePresence();
}

UINT WM_TASKBAR = 0;
HWND Hwnd;
HMENU Hmenu;
NOTIFYICONDATA notifyIconData;

/*procedures  */
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
void minimize();
void restore();
void InitNotifyIconData();
HANDLE hMutex;

steady_clock::time_point lastUpdate;

int WINAPI WinMain(HINSTANCE hThisInstance,

	HINSTANCE hPrevInstance,
	LPSTR lpszArgument,
	int nCmdShow)
{

	try {
		// Try to open the mutex.
		hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, "octothorp.team");

		if (!hMutex)
			// Mutex doesn’t exist. This is
			// the first instance so create
			// the mutex.
			hMutex = CreateMutex(0, 0, "octothorp.team");
		else
			// The mutex exists so this is the
			// the second instance so return.
			return 0;

		// The app is closing so release
		// the mutex.
	}
	catch (exception &e) {
		return 0;
	}

	/* This is the handle for our window */
	MSG messages;            /* Here messages to the application are saved */
	WNDCLASSEX wincl;        /* Data structure for the windowclass */
	WM_TASKBAR = RegisterWindowMessageA("TaskbarCreated");
	/* The Window structure */
	wincl.hInstance = hThisInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
	wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
	wincl.cbSize = sizeof(WNDCLASSEX);

	/* Use default icon and mouse-pointer */
	wincl.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICON_LOGO_DARK));
	wincl.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICON_LOGO_DARK));
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;                 /* No menu */
	wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
	wincl.cbWndExtra = 0;                      /* structure or the window instance */
	wincl.hbrBackground = (HBRUSH)(CreateSolidBrush(RGB(255, 255, 255)));
	/* Register the window class, and if it fails quit the */
	if (!RegisterClassEx (&wincl))
	return 0;

	/* The class is registered, let's create the program*/
	Hwnd = CreateWindowEx(
		0,                   /* Extended possibilites for variation */
		szClassName,         /* Classname */
		szClassName,       /* Title Text */
		WS_OVERLAPPEDWINDOW, /* default window */
		CW_USEDEFAULT,       /* Windows decides the position */
		CW_USEDEFAULT,       /* where the window ends up on the screen */
		544,                 /* The programs width */
		375,                 /* and height in pixels */
		HWND_DESKTOP,        /* The window is a child-window to desktop */
		NULL,                /* No menu */
		hThisInstance,       /* Program Instance handler */
		NULL                 /* No Window Creation data */
	);
	GetFullPathName(iniPath, sizeof(sAbsolutePathBuffer) / sizeof(TCHAR), sAbsolutePathBuffer, NULL);
	strcpy_s(iniPath, sAbsolutePathBuffer);

	/*Initialize the NOTIFYICONDATA structure only once*/
	InitNotifyIconData();
	updateConfig();
	/* Make the window visible on the screen */
	//ShowWindow(Hwnd, nCmdShow);
	Shell_NotifyIcon(NIM_ADD, &notifyIconData); // add
	discordInit();
	updatePresence();
	lastUpdate = steady_clock::now();
	/* Run the message loop. It will run until GetMessage() returns 0 */
	while (GetMessage(&messages, NULL, 0, 0))
	{
		/* Translate virtual-key messages into character messages */
		TranslateMessage(&messages);
		steady_clock::time_point curUpdate = steady_clock::now();
		if (duration_cast<std::chrono::seconds>(curUpdate - lastUpdate).count() > 10) {
			//MessageBoxW(NULL, (wchar_t *)"Update!", (LPCWSTR)"Message", MB_OK | MB_ICONINFORMATION);
			lastUpdate = curUpdate;
			discordRestart();
		}
		/* Send message to WindowProcedure */
		DispatchMessage(&messages);
	}

	return messages.wParam;
}

/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	if (message == WM_TASKBAR && !IsWindowVisible(Hwnd))
	{
		minimize();
		return 0;
	}

	switch (message)                  /* handle the messages */
	{
	case WM_ACTIVATE:
		//Shell_NotifyIcon(NIM_ADD, &notifyIconData);
		break;
	case WM_CREATE:

		//ShowWindow(Hwnd, SW_HIDE);
		Hmenu = CreatePopupMenu();
		AppendMenu(Hmenu, MF_STRING, ID_TRAY_UPDATE, TEXT("Update"));
		AppendMenu(Hmenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));

		break;

	case WM_SYSCOMMAND:
		/*In WM_SYSCOMMAND messages, the four low-order bits of the wParam parameter
		are used internally by the system. To obtain the correct result when testing the value of wParam,
		an application must combine the value 0xFFF0 with the wParam value by using the bitwise AND operator.*/

		switch (wParam & 0xFFF0)
		{
		case SC_MINIMIZE:
		case SC_CLOSE:
			minimize();
			return 0;
			break;
		}
		break;


		// Our user defined WM_SYSICON message.
	case WM_SYSICON:
	{

		switch (wParam)
		{
		case ID_TRAY_APP_ICON:
			SetForegroundWindow(Hwnd);

			break;
		}

		if (lParam == WM_LBUTTONUP)
		{

			restore();
		}
		else if (lParam == WM_RBUTTONDOWN)
		{
			// Get current mouse position.
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(Hwnd);

			// TrackPopupMenu blocks the app until TrackPopupMenu returns

			UINT clicked = TrackPopupMenu(Hmenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);



			SendMessage(hwnd, WM_NULL, 0, 0); // send benign message to window to make sure the menu goes away.
			switch (clicked) {
				case ID_TRAY_EXIT:
					// quit the application.
					Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
					ReleaseMutex(hMutex);
					PostQuitMessage(0);
					break;
				case ID_TRAY_UPDATE:
					updateConfig();
					discordRestart();
					break;
			}
		}
	}
	break;

	// intercept the hittest message..
	case WM_NCHITTEST:
	{
		UINT uHitTest = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
		if (uHitTest == HTCLIENT)
			return HTCAPTION;
		else
			return uHitTest;
	}

	case WM_CLOSE:

		minimize();
		return 0;
		break;

	case WM_DESTROY:
		ReleaseMutex(hMutex);
		PostQuitMessage(0);
		break;

	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}


void minimize()
{
	// hide the main window
	//ShowWindow(Hwnd, SW_HIDE);
}


void restore()
{
	//ShowWindow(Hwnd, SW_SHOW);
}

void InitNotifyIconData()
{
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));

	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = Hwnd;
	notifyIconData.uID = ID_TRAY_APP_ICON;
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = WM_SYSICON; //Set up our invented Windows Message
	notifyIconData.hIcon = (HICON)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICON_LOGO_WHITE));
	strncpy_s(notifyIconData.szTip, szTIP, sizeof(szTIP));
}
