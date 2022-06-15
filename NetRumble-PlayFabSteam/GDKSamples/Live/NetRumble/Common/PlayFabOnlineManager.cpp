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

void PlayFabOnlineManager::LoginWithSteam(std::function<void(bool, const std::string&)> callback)
{
	m_playfabLogin.LoginWithSteam(callback);
}

void PlayFabOnlineManager::OnInviteUserToGame(GameRichPresenceJoinRequested_t* response)
{
	GameStateManager* pStateManager = Managers::Get<GameStateManager>();
	if (pStateManager->GetState() == GameState::StartMenu || pStateManager->GetState() == GameState::Gaming)
	{
		return;
	}

	if (response)
	{
		std::string strParam = response->m_rgchConnect;

		if (ParseInviteParam(strParam))
		{
			SwitchToJoiningFromInvite();
			JoinGameFromInvite(strParam);
		}
	}
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
