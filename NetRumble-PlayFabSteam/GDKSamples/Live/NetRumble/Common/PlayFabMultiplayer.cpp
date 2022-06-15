//--------------------------------------------------------------------------------------
// PlayFabMultiplayer.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

// It also interfaces with the PlayFab Party manager to coordinate creation and usage of networking and VOIP.
//
// There are three ways to enter a multiplayer game:
//
//    - Hosting a game          - HostMultiplayerGame()
//    - Joining a hosted game   - JoinMultiplayerGame()
//    - Matchmaking             - StartMatchmaking()
//
// Each of these will result in the user joining or creating a 'lobby' session, a 'game' session and the Party.

using namespace NetRumble;

namespace
{
	static GameMessage OnlineDisconnect(GameMessageType::OnlineDisconnect, 0);
	static GameMessage MatchmakingFailed(GameMessageType::MatchmakingFailed, 0);
	static GameMessage MatchmakingCanceled(GameMessageType::MatchmakingCanceled, 0);
	static GameMessage JoinGameFailed(GameMessageType::JoinGameFailed, 0);
	static GameMessage PlayerLeftGame(GameMessageType::PlayerLeftGame, 0);
	static GameMessage JoinGameCompleted(GameMessageType::JoinedGameComplete, 0);
	static GameMessage LeaveGameComplete(GameMessageType::LeaveGameComplete, 0);
	static GameMessage JoiningGame(GameMessageType::JoiningGame, 0);
}

extern const char* GetPlayFabErrorMessage(HRESULT errorCode);

void PlayFabOnlineManager::MultiplayerSDKInitialize()
{
	// Initialize PlayFab Multiplayer
	if (m_pfMultiplayerHandle == nullptr)
	{
		// PFMultiplayerInitialize() cannot be called again without a subsequent PFMultiplayerUninitialize() call.
		// Every call to PFMultiplayerInitialize() should have a corresponding PFMultiplayerUninitialize() call.
		HRESULT hr = PFMultiplayerInitialize(NETRUMBLE_PLAYFAB_TITLE_ID.data(), &m_pfMultiplayerHandle);
		if (FAILED(hr))
		{
			DEBUGLOG("Failed to initialize multiplayer: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
			return;
		}
	}

	// Set an entity token for a local user. The token will be used for operations on behalf of this user. If using
	// the PF Core SDK, this should be called each time the PF Core SDK provides a refreshed token.
	PlayFabLogin& playFabLogin = Managers::Get<OnlineManager>()->m_playfabLogin;
	const PFEntityKey localUserEntityKey{ playFabLogin.GetEntityKey().Id.c_str(),  playFabLogin.GetEntityKey().Type.c_str() };
	HRESULT hr = PFMultiplayerSetEntityToken(m_pfMultiplayerHandle, &localUserEntityKey, playFabLogin.GetEntityToken().c_str());
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to set multiplayer entity token: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		return;
	}
}

void PlayFabOnlineManager::MultiplayerSDKUninitialize()
{
	// Uninitialize PlayFab Multiplayer
	if (m_pfMultiplayerHandle != nullptr)
	{
		const HRESULT hr = PFMultiplayerUninitialize(m_pfMultiplayerHandle);
		if (FAILED(hr))
		{
			DEBUGLOG("Failed to uninitialize multiplayer: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
			return;
		}
		m_pfMultiplayerHandle = nullptr;
	}
}

// StartMatchmaking is the entry point for SmartMatch based matchmaking.
void PlayFabOnlineManager::StartMatchmaking()
{
	DEBUGLOG("Start matchmaking\n");

	if (m_onlineState != OnlineState::Ready)
	{
		DEBUGLOG("Failed to start matchmaking. Multiplayer state is %d, expected %d.\n", m_onlineState, OnlineState::Ready);
		m_messageHandler(GetLocalEntityId(), &MatchmakingFailed);
		return;
	}

	if (m_pfMatchmaking.StartMatchmaking())
	{
		m_onlineState = OnlineState::Matchmaking;
	}
}

void PlayFabOnlineManager::CancelMatchmaking()
{
	DEBUGLOG("Cancel matchmaking\n");

	if (m_onlineState != OnlineState::Matchmaking)
	{
		DEBUGLOG("Multiplayer state is %d, expected %d.\n", m_onlineState, OnlineState::Ready);
		return;
	}

	m_pfMatchmaking.CancelMatchmaking();

	m_messageHandler(GetLocalEntityId(), &MatchmakingCanceled);
	m_onlineState = OnlineState::Ready;
}

// HostMultiplayerGame is the entry point for hosting a joinable game lobby.
void PlayFabOnlineManager::HostMultiplayerGame(PFLobbyAccessPolicy accessPolicy)
{
	std::string accessPolicyString{ "Public" };
	if (accessPolicy == PFLobbyAccessPolicy::Private)
	{
		accessPolicyString = "Private";
	}
	else if (accessPolicy == PFLobbyAccessPolicy::Friends)
	{
		accessPolicyString = "Friends";
	}

	DEBUGLOG("Host multiplayer game: %s\n", accessPolicyString.c_str());

	if (m_onlineState != OnlineState::Ready)
	{
		DEBUGLOG("Failed to host multiplayer game, online state is %d, expected %d.\n", m_onlineState, OnlineState::Ready);
		m_messageHandler(GetLocalEntityId(), &JoinGameFailed);
		return;
	}

	if (!g_game->m_isLoggedIn)
	{
		DEBUGLOG("Failed to host multiplayer game, you must login playfab first before you can start hosting game.\n");
		return;
	}

	m_onlineState = OnlineState::Hosting;
	CreatePlayFabParty();

	Managers::Get<OnlineManager>()->m_playfabParty.SetHost(true);
}

bool PlayFabOnlineManager::JoinPendingInviteSession()
{
	if (!m_connectionString.empty())
	{
		DEBUGLOG("Join pending invite session\n");
		SwitchToJoiningFromInvite();
		JoinGameFromInvite(GetConnectionString());
		return true;
	}
	return false;
}

// Receive the PlayFab ConnectionString
bool PlayFabOnlineManager::ParseInviteParam(std::string& strParam)
{
	size_t nFlagLength = strlen(PlayFabLobby::GetCmdLineFlag());
	if (strParam.length() <= nFlagLength)
	{
		DEBUGLOG("Failed to get invite parameter: Incorrect parameter length\n");
		return false;
	}
	std::string strFlag = strParam.substr(0, nFlagLength);
	if (strFlag != PlayFabLobby::GetCmdLineFlag())
	{
		DEBUGLOG("Failed to get invite parameter: Invalid data parameter\n");
		return false;
	}
	strParam = strParam.substr(nFlagLength, strParam.length() - nFlagLength);

	SetConnectionString(strParam);
	return true;
}

void PlayFabOnlineManager::JoinGameFromInvite(const std::string& connectionString)
{
	DEBUGLOG("Join game from invite '%hs'\n", connectionString.c_str());

	if (m_onlineState == OnlineState::Ready)
	{
		Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Joining);
		JoinMultiplayerGame(connectionString);
	}
	else
	{
		m_connectionString = connectionString;
		LeaveMultiplayerGame();
	}
}

// JoinMultiplayerGame is the entry point for joining an existing game session.
void PlayFabOnlineManager::JoinMultiplayerGame(const std::string& connectionString)
{
	DEBUGLOG("Join multiplayer game '%hs'\n", connectionString.c_str());

	if (m_onlineState != OnlineState::Joining)
	{
		LeaveMultiplayerGame();
		m_messageHandler(GetLocalEntityId(), &JoinGameFailed);
		return;
	}

	Managers::Get<OnlineManager>()->SetHost(false);
	m_pfLobby.JoinLobby(connectionString);

	Managers::Get<GameStateManager>()->SwitchToState(GameState::MPJoinLobby);
}

void PlayFabOnlineManager::LeaveMultiplayerGame()
{
	DEBUGLOG("Leave multipalyer game\n");

	if (m_onlineState != OnlineState::Ready)
	{
		LeaveLobby();
		if (!IsHost())
		{
			Managers::Get<OnlineManager>()->m_playfabParty.LeaveNetwork(nullptr);
		}
	}
	else
	{
		DEBUGLOG("Trying to leave while OnlineState is: %d\n", m_onlineState);
	}
}

void PlayFabOnlineManager::MultiplayerTick()
{
	const OnlineState currentOnlineState = GetOnlineState();
	switch (currentOnlineState)
	{
	case OnlineState::Matchmaking:
	{
		m_pfMatchmaking.DoWork();
		break;
	}
	case OnlineState::Hosting:
	case OnlineState::Joining:
	{
		m_pfLobby.DoWork();
		break;
	}
	default:
		break;
	}
}

void PlayFabOnlineManager::CreatePlayFabParty()
{
	DEBUGLOG("Create playFab party\n");

	m_networkId = GuidUtil::NewGuid();

	Managers::Get<OnlineManager>()->m_playfabParty.CreateAndConnectToNetwork(
		m_networkId.c_str(),
		[this](std::string descriptor)
		{
			DEBUGLOG("Create playFab party complete\n");
			m_networkDescriptor = descriptor;
			Managers::Get<OnlineManager>()->CreateLobby();
		});
}

void PlayFabOnlineManager::OnPFPartyNetworkCreated(const std::string& descriptor)
{
	m_networkDescriptor = descriptor;

	// Set the values in the session so all the other clients can find and join the Party session
	SetSessionProperty("invite", m_networkId.c_str());
	SetSessionProperty("descriptor", m_networkDescriptor.c_str());

	// We're now ready to be in the game lobby
	Managers::Get<OnlineManager>()->m_playfabParty.SetHost(true);
	m_onlineState = OnlineState::InGame;
	m_messageHandler(GetLocalEntityId(), &JoinGameCompleted);
}

void PlayFabOnlineManager::MigrateToNewNetwork()
{
	DEBUGLOG("Migrate to new network\n");

	// Ask the PFP manager to swap networks
	Managers::Get<OnlineManager>()->m_playfabParty.MigrateToNetwork(
		m_connectionString.c_str(),
		[](bool succeeded)
		{
			DEBUGLOG("Migrate to network completed %s\n", succeeded ? "successfully" : "unsuccessfully");

			Managers::Get<OnlineManager>()->SendGameMessage(
				GameMessage(
					GameMessageType::PlayerState,
					g_game->GetLocalPlayerState()->SerializePlayerStateData()
				)
			);
		});
}

void PlayFabOnlineManager::FindAndConnectToNetwork(std::string_view networkId, std::string_view descriptor)
{
	DEBUGLOG("Find and connect to network\n");

	m_networkId = networkId;
	m_networkDescriptor = descriptor;

	// If the info is there, connect now otherwise wait for the properties to be set
	if (!m_networkId.empty() && !m_networkDescriptor.empty())
	{
		Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Joining);
		Managers::Get<OnlineManager>()->m_playfabParty.ConnectToNetwork(
			m_networkId.c_str(),
			m_networkDescriptor.c_str(),
			[this]()
			{
				DEBUGLOG("Connect to network complete\n");
				Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);
				g_game->GetLocalPlayerState()->InLobby = true;
				Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(
					GameMessageType::PlayerJoined,
					g_game->GetLocalPlayerState()->SerializePlayerStateData()
				));
				m_isJoiningArrangedLobby = false;
			});
	}
}

void PlayFabOnlineManager::FindAndAddRemoteUsers()
{
	DEBUGLOG("Find and add remote users\n");
}

void PlayFabOnlineManager::FindAndRemoveRemoteUsers()
{
	DEBUGLOG("Find and remove remote users\n");
}

void PlayFabOnlineManager::SetSessionProperty(const char* name, const char* value)
{
	// Wrap the value in quotes so it's a valid JSON string value
	std::string quotedValue = "\"";
	quotedValue += value;
	quotedValue += "\"";

	DEBUGLOG("SetSessionProperty: %s = %s\n", name, value);
}

bool PlayFabOnlineManager::ResetNetwork()
{
	const char* pValue = nullptr;
	std::string networkId;
	std::string networkDescriptor;

	HRESULT hr = PFLobbyGetLobbyProperty(m_pfLobby.GetLobbyHandle(), LOBBY_PROPERTY_NETWORKID, &pValue);
	auto OutputError = [this](HRESULT hrLocal)
	{
		const char* pErrorMsg = PFMultiplayerGetErrorMessage(hrLocal);
		if (pErrorMsg)
		{
			DEBUGLOG("PFLobbyGetLobbyProperty FAILED[%S]", pErrorMsg);
		}
	};
	if (FAILED(hr))
	{
		OutputError(hr);
		return false;
	}
	if (pValue)
	{
		networkId = pValue;
	}
	else
	{
		DEBUGLOG("Network Id is null");
		return false;
	}

	hr = PFLobbyGetLobbyProperty(m_pfLobby.GetLobbyHandle(), LOBBY_PROPERTY_DESCRIPTOR, &pValue);
	if (FAILED(hr))
	{
		OutputError(hr);
		return false;
	}
	if (pValue)
	{
		networkDescriptor = pValue;
	}
	else
	{
		DEBUGLOG("Network descriptor is null");
		return false;
	}

	// Do not reset network if it was already created
	if (networkId == m_networkId && networkDescriptor == m_networkDescriptor)
	{
		DEBUGLOG("Online manager is already in the same network");
		return false;
	}

	m_networkId = networkId;
	m_networkDescriptor = networkDescriptor;
	return true;
}
