//
// pch.h
// Header for standard system include files.
//

#pragma once

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

#define _ENABLE_EXTENDED_ALIGNED_STORAGE
// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP
#include <WinSock2.h>

#include <Windows.h>

#include <wrl.h>
#include <wrl/client.h>

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

// xbox live
#include <XGameRuntime.h>
#include <xsapi-c/services_c.h>

// PlayFab SDK
#include "Party.h"
#include "playfab/PlayFabError.h"
#include "playfab/PlayFabClientApi.h"
#include "playfab/PlayFabSettings.h"
#include "playfab/PlayFabApiSettings.h"
#include "PFEntityKey.h"
#include "PFMultiplayer.h"
#include "PFLobby.h"
#include "PFMatchmaking.h"
#include "PlayFabLogin.h"
#include "PlayFabLobby.h"
#include "PlayFabMatchmaking.h"
#include "PlayFabParty.h"

#define _XM_NO_XMVECTOR_OVERLOADS_

#include <DirectXMath.h>
#include <DirectXColors.h>

#include <algorithm>
#include <atomic>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>

#include "StepTimer.h"
#include "Texture.h"

#include "Audio.h"
#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "GamePad.h"
#include "Keyboard.h"
#include "SimpleMath.h"
#include "SpriteFont.h"

#include "Managers.h"
#include "StringUtil.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "Debug.h"

#include "ArrayView.h"
#include "AsyncHelper.h"
#include "GuidUtil.h"

#include "GameEventManager.h"
#include "ServerConfig.h"
#include "UserStartupScreen.h"
#include "MainMenuScreen.h"
#include "MenuScreen.h"
#include "GameLobbyScreen.h"
#include "GamePlayScreen.h"
#include "PowerUp.h"
#include "Asteroid.h"
#include "Starfield.h"
#include "Ship.h"
#include "DataBuffer.h"
#include "Weapon.h"
#include "LaserWeapon.h"
#include "MineWeapon.h"
#include "DoubleLaserWeapon.h"
#include "TripleLaserWeapon.h"
#include "RocketWeapon.h"
#include "DoubleLaserPowerUp.h"
#include "TripleLaserPowerUp.h"
#include "RocketPowerUp.h"
#include "RandomMath.h"
#include "OnlineManager.h"
#include "World.h"
#include "Game.h"
#include "DebugOverlayScreen.h"
#include "StarfieldScreen.h"
#include "PlayFabOnlineManager.h"
#include "JoinFriendsMenu.h"
#include "OptionsPopUpScreen.h"
#include "GameScreen.h"
#include "PlayerState.h"
#include "NetworkMessages.h"
#include "GameStateManager.h"
#include "CollisionManager.h"

namespace DX
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) : result(hr) {}

		const char* what() const override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
#ifdef _DEBUG
			char str[64] = {};
			sprintf_s(str, "**ERROR** Fatal Error with HRESULT of %08X\n", static_cast<unsigned int>(hr));
			DEBUGLOG(str);
			__debugbreak();
#endif
			throw com_exception(hr);
		}
	}
}

// OUT_Z_ARRAY indicates an output array that will be null-terminated.
#if _MSC_VER >= 1600
	   // Include the annotation header file.
#include <sal.h>
#if _MSC_VER >= 1700
	   // VS 2012+
#define OUT_Z_ARRAY _Post_z_
#else
	   // VS 2010
#define OUT_Z_ARRAY _Deref_post_z_
#endif
#else
	   // gcc, clang, old versions of VS
#define OUT_Z_ARRAY
#endif

template <size_t maxLenInChars> void sprintf_safe(OUT_Z_ARRAY char(&pDest)[maxLenInChars], const char* pFormat, ...)
{
	va_list params;
	va_start(params, pFormat);
#ifdef POSIX
	vsnprintf(pDest, maxLenInChars, pFormat, params);
#else
	_vsnprintf_s(pDest, maxLenInChars, maxLenInChars, pFormat, params);
#endif
	pDest[maxLenInChars - 1] = '\0';
	va_end(params);
}

inline void strncpy_safe(char* pDest, char const* pSrc, size_t maxLen)
{
	size_t nCount = maxLen;
	char* pstrDest = pDest;
	const char* pstrSource = pSrc;

	while (0 < nCount && 0 != (*pstrDest++ = *pstrSource++))
		nCount--;

	if (maxLen > 0)
		pstrDest[-1] = 0;
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch"
#endif


// Enable off by default warnings to improve code conformance
#pragma warning(default : 4191 4242 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
