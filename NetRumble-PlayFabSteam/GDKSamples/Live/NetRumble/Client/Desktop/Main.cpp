
#include "pch.h"
#include <appnotify.h>

using namespace NetRumble;
using namespace DirectX;

constexpr int width = 1280;
constexpr int height = 720;
const AppId_t k_uMyAppId = k_uAppIdInvalid;

std::unique_ptr<Game> g_game;

LPCWSTR g_szAppName = L"NetRumble";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool ParseCommandLine(const char* pchCmdLine, const char** ppchServerAddress, const char** ppchLobbyID);

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	// Replace with your App ID
	if (SteamAPI_RestartAppIfNecessary(k_uMyAppId))
	{
		// If Steam is not running or the game wasn't started through Steam, SteamAPI_RestartAppIfNecessary starts the 
		// local Steam client and also launches this game again.

		// Once you get a public Steam AppID assigned for this game, you need to replace k_uAppIdInvalid with it and
		// removed steam_appid.txt from the game depot.

		return EXIT_FAILURE;
	}

	if (!SteamAPI_Init())
	{
#ifdef _DEBUG
		DEBUGLOG("SteamAPI_Init() failed\n");
#endif
		MessageBoxA(nullptr, "Fatal Error", "Steam must be running to play this game (SteamAPI_Init() failed).\n", MB_ICONERROR);
		return EXIT_FAILURE;
	}

	if (!XMVerifyCPUSupport())
	{
#ifdef _DEBUG
		DEBUGLOG("ERROR: This hardware does not support the required instruction set.\n");
#endif
		return EXIT_FAILURE;
	}

	// NOTE: When running the app from the Start Menu (required for
	// Store API's to work) the Current Working Directory will be
	// returned as C:\Windows\system32 unless you overwrite it.
	// The sample relies on the font and image files in the .exe's
	// directory and so we do the following to set the working
	// directory to what we want.
	char dir[1024];
	GetModuleFileNameA(nullptr, dir, 1024);
	std::string exe = dir;
	exe = exe.substr(0, exe.find_last_of("\\"));
	SetCurrentDirectoryA(exe.c_str());

	::g_game = std::make_unique<Game>();

	// Register class and create window
	{
		// Register class
		WNDCLASSEX wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.hInstance = hInstance;
		wcex.lpszClassName = L"NetRumbleWindowClass";
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.hIcon = LoadIcon(nullptr, MAKEINTRESOURCE(IDI_ICON1));
		wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
		if (!RegisterClassEx(&wcex))
		{
			return EXIT_FAILURE;
		}
		// Create window
		int w = width, h = height;

		RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
		AdjustWindowRect(&rc, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);

		HWND hwnd = CreateWindowExW(0, L"NetRumbleWindowClass", g_szAppName, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
			nullptr);
		if (!hwnd)
		{
			return EXIT_FAILURE;
		}

		ShowWindow(hwnd, nCmdShow);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

		GetClientRect(hwnd, &rc);

		g_game->Initialize(hwnd);
		if (wcslen(lpCmdLine) > 0)
		{
			std::string strParam = DX::WideToUtf8(lpCmdLine);
			PlayFabOnlineManager* pOnlineManager = Managers::Get<OnlineManager>();
			if (pOnlineManager->ParseInviteParam(strParam))
			{
				DEBUGLOG("Accepted the invitation successfully");
				pOnlineManager->SetHasPendingInvite(true);
			}
		}
	}

	// CommandLine
	const char* pServerAddress, * pLobbyID;
	char szCommandLine[1024] = {};
	WideCharToMultiByte(CP_ACP, 0, lpCmdLine, -1, szCommandLine, sizeof(szCommandLine) / sizeof(szCommandLine[0]), nullptr, nullptr);
	if (!ParseCommandLine(szCommandLine, &pServerAddress, &pLobbyID))
	{
		if (SteamApps()->GetLaunchCommandLine(szCommandLine, sizeof(szCommandLine)) > 0)
		{
			ParseCommandLine(szCommandLine, &pServerAddress, &pLobbyID);
		}
	}
	else
	{
		DEBUGLOG("Failed to parse the command line, get invalid parameter from command line\n");
	}

	// Main message loop
	MSG msg = {};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			g_game->Tick();
		}
	}
	g_game->ExitGame();
	g_game.reset();

	return (int)msg.wParam;
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = false;
	static bool s_fullscreen = false;

	Game* game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if (game)
	{
		Managers::Get<InputManager>()->ProcessMessage(message, wParam, lParam);
	}

	switch (message)
	{
	case WM_CREATE:
		break;

	case WM_ACTIVATEAPP:
		break;

	case WM_PAINT:
		if (s_in_sizemove && game)
		{
			game->Tick();
		}
		else
		{
			PAINTSTRUCT ps;
			(void)BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_MOVE:
		if (game)
		{
			game->OnWindowMoved();
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			if (!s_minimized)
			{
				s_minimized = true;
				if (!s_in_suspend && game)
					game->OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (s_minimized)
		{
			s_minimized = false;
			if (s_in_suspend && game)
				game->OnResuming();
			s_in_suspend = false;
		}
		else if (!s_in_sizemove && game)
		{
			game->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
		}
		break;

	case WM_ENTERSIZEMOVE:
		s_in_sizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		if (game)
		{
			RECT rc;
			GetClientRect(hWnd, &rc);

			game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
		}
		break;

	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
		info->ptMinTrackSize.x = 320;
		info->ptMinTrackSize.y = 200;
	}
	break;

	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMQUERYSUSPEND:
			if (!s_in_suspend && game)
				game->OnSuspending();
			s_in_suspend = true;
			return TRUE;

		case PBT_APMRESUMESUSPEND:
			if (!s_minimized)
			{
				if (s_in_suspend && game)
					game->OnResuming();
				s_in_suspend = false;
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		// TODO: Add mouse
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		{
			// Implements the classic ALT+ENTER fullscreen toggle
			if (s_fullscreen)
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

				constexpr int sampleWidth = 800;
				constexpr int sampleHeight = 600;

				ShowWindow(hWnd, SW_SHOWNORMAL);

				SetWindowPos(hWnd, HWND_TOP, 0, 0, sampleWidth, sampleHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
			else
			{
				SetWindowLongPtr(hWnd, GWL_STYLE, 0);
				SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

				ShowWindow(hWnd, SW_SHOWMAXIMIZED);
			}

			s_fullscreen = !s_fullscreen;
		}
		break;

	case WM_MENUCHAR:
		// A menu is active and the user presses a key that does not correspond
		// to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
		return MAKELRESULT(0, MNC_CLOSE);

	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitGame()
{
	PostQuitMessage(0);
}

// Extracts some feature from the command line
bool ParseCommandLine(const char* pchCmdLine, const char** ppchServerAddress, const char** ppchLobbyID)
{
	// Look for the +connect ipaddress:port parameter in the command line,
	// Steam will pass this when a user has used the Steam Server browser to find
	// a server for our game and is trying to join it. 
	const char* pchConnectParam = "+connect ";
	const char* pchConnect = strstr(pchCmdLine, pchConnectParam);
	*ppchServerAddress = nullptr;
	if (pchConnect && strlen(pchCmdLine) > (pchConnect - pchCmdLine) + strlen(pchConnectParam))
	{
		// Address should be right after the +connect
		*ppchServerAddress = pchCmdLine + (pchConnect - pchCmdLine) + strlen(pchConnectParam);
	}

	// Look for +connect_lobby lobbyid paramter on the command line
	// Steam will pass this in if a user taken up an invite to a lobby
	const char* pchConnectLobbyParam = "+connect_lobby ";
	const char* pchConnectLobby = strstr(pchCmdLine, pchConnectLobbyParam);
	*ppchLobbyID = nullptr;
	if (pchConnectLobby && strlen(pchCmdLine) > (pchConnectLobby - pchCmdLine) + strlen(pchConnectLobbyParam))
	{
		// Lobby ID should be right after the +connect_lobby
		*ppchLobbyID = pchCmdLine + (pchConnectLobby - pchCmdLine) + strlen(pchConnectLobbyParam);
	}

	return *ppchServerAddress || *ppchLobbyID;
}
