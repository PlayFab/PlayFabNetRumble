//--------------------------------------------------------------------------------------
// PlayFabOnlineManager.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

extern const char* GetPlayFabErrorMessage(HRESULT errorCode)
{
	const char* errorMsg = PFMultiplayerGetErrorMessage(errorCode);
	if (errorMsg == nullptr)
	{
		DEBUGLOG("Failed to get error message %lu.\n", errorCode);
		return "[ERROR] Get unmapped error code.";
	}

	return errorMsg;
}

void PlayFabOnlineManager::InitializeMultiplayer()
{
	MultiplayerSDKInitialize();
}

void PlayFabOnlineManager::UninitializeMultiplayer()
{
	MultiplayerSDKUninitialize();
}

void PlayFabOnlineManager::Tick(float)
{
	MultiplayerTick();
}

bool PlayFabOnlineManager::IsMatchmaking()
{
	return m_pfMatchmaking.IsMatchmaking();
}

void PlayFabOnlineManager::CreateLobby()
{
	m_pfLobby.CreateLobby(PFLobbyAccessPolicy::Public);
}

bool PlayFabOnlineManager::IsNetworkAvailable() const
{
	return true;
}

void PlayFabOnlineManager::LoginWithCustomID(std::function<void(bool, const std::string&)> callback)
{
	m_playfabLogin.LoginWithCustomId(callback);
}

void PlayFabOnlineManager::LoginWithXboxLive(std::function<void(bool, const std::string&)> callback)
{
	m_playfabLogin.LoginWithXboxLive(callback);
}

void PlayFabOnlineManager::CleanupOnlineServices()
{
	m_playfabParty.LeaveNetwork();
	m_playfabParty.Shutdown();
}

void PlayFabOnlineManager::Cleanup()
{
	m_playfabLogin.Cleanup();
	m_pfLobby.Cleanup();
}
