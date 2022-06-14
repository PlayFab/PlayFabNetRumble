//--------------------------------------------------------------------------------------
// PlayFabLobby.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once
#include "pch.h"

using namespace NetRumble;

extern const char* GetPlayFabErrorMessage(HRESULT errorCode);
namespace
{
	static GameMessage CreateLobbyFailed(GameMessageType::CreateLobbyFailed, 0);
	static GameMessage LeaveGameComplete(GameMessageType::LeaveGameComplete, 0);

	std::string GetPFLobbyStateChangeTypeString(PFLobbyStateChangeType Type)
	{
		switch (Type)
		{
		case PFLobbyStateChangeType::CreateAndJoinLobbyCompleted:return "CreateAndJoinLobbyCompleted"; break;
		case PFLobbyStateChangeType::JoinLobbyCompleted:return "JoinLobbyCompleted"; break;
		case PFLobbyStateChangeType::MemberAdded:return "MemberAdded"; break;
		case PFLobbyStateChangeType::AddMemberCompleted:return "AddMemberCompleted"; break;
		case PFLobbyStateChangeType::MemberRemoved:return "MemberRemoved"; break;
		case PFLobbyStateChangeType::Updated:return "Updated"; break;
		case PFLobbyStateChangeType::Disconnected:return "Disconnected"; break;
		case PFLobbyStateChangeType::JoinArrangedLobbyCompleted:return "JoinArrangedLobbyCompleted"; break;
		case PFLobbyStateChangeType::FindLobbiesCompleted:return "FindLobbiesCompleted"; break;
		case PFLobbyStateChangeType::InviteReceived:return "InviteReceived"; break;
		case PFLobbyStateChangeType::InviteListenerStatusChanged:return "InviteListenerStatusChanged"; break;
		case PFLobbyStateChangeType::SendInviteCompleted:return "SendInviteCompleted"; break;
		}

		return "Unknown PFLobbyStateChangeType";
	}

	bool LogPFLobbyStateChangeType(const PFLobbyStateChange* change)
	{
		DEBUGLOG("PFLobbyStateChange: ");
		if (change)
		{
			DEBUGLOG("%s \n", GetPFLobbyStateChangeTypeString(change->stateChangeType).c_str());
			return true;
		}

		return false;
	}

	bool LogPFLobbyStateChangeFailResult(HRESULT result)
	{
		if (FAILED(result))
		{
			DEBUGLOG("[ERROR]: 0x%08X %s\n", static_cast<unsigned int>(result), GetPlayFabErrorMessage(result));
			return false;
		}
		return true;
	}
}

void PlayFabLobby::DoWork()
{
	TryProcessLobbyStateChanges();
}

bool PlayFabLobby::InviteSteamFriend(CSteamID steamID)
{
	std::string strParam = GetCmdLineFlag() + GetConnectString();
	return SteamFriends()->InviteUserToGame(steamID, strParam.c_str());
}

std::string PlayFabLobby::GetConnectString()
{
	std::string strResult;
	PFLobbyHandle pfHandle = GetLobbyHandle();
	if (pfHandle)
	{
		const char* pfConnectionString;
		if (SUCCEEDED(PFLobbyGetConnectionString(pfHandle, &pfConnectionString)))
		{
			strResult = pfConnectionString;
		}
	}
	return strResult;
}

void PlayFabLobby::RefreshLobbyOwner()
{
	const PFEntityKey* lobbyOwner;
	const HRESULT hr = PFLobbyGetOwner(m_lobbyHandle, &lobbyOwner);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to get lobby owner\n");
		return;
	}
	std::shared_ptr<PlayerState> player = g_game->GetLocalPlayerState();
	if (lobbyOwner->id == player->EntityId)
	{
		Managers::Get<OnlineManager>()->m_playfabParty.SetHost(true);
	}
}

void PlayFabLobby::TryProcessLobbyStateChanges()
{
	uint32_t stateChangeCount;
	const PFLobbyStateChange* const* stateChanges;
	const OnlineManager* onlineManager = Managers::Get<OnlineManager>();
	if (!onlineManager)
	{
		DEBUGLOG("Failed to get OnlineManager");
		return;
	}

	HRESULT hr = PFMultiplayerStartProcessingLobbyStateChanges(onlineManager->m_pfMultiplayerHandle, &stateChangeCount, &stateChanges);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to start processing lobby state changes: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		return;
	}

	const PFLobbyStateChange* stateChange{ nullptr };
	for (uint32_t i = 0; i < stateChangeCount; ++i)
	{
		stateChange = stateChanges[i];
		if (!LogPFLobbyStateChangeType(stateChange))
		{
			continue;
		}

		switch (stateChange->stateChangeType)
		{
		case PFLobbyStateChangeType::CreateAndJoinLobbyCompleted:OnCreateAndJoinLobbyCompleted(stateChange); break;
		case PFLobbyStateChangeType::JoinLobbyCompleted:OnJoinLobbyCompleted(stateChange); break;
		case PFLobbyStateChangeType::MemberAdded:OnMemberAdded(stateChange); break;
		case PFLobbyStateChangeType::AddMemberCompleted:OnAddMemberCompleted(stateChange); break;
		case PFLobbyStateChangeType::MemberRemoved:OnMemberRemoved(stateChange); break;
		case PFLobbyStateChangeType::Updated:OnUpdated(stateChange); break;
		case PFLobbyStateChangeType::Disconnected:OnDisconnected(stateChange); break;
		case PFLobbyStateChangeType::LeaveLobbyCompleted:OnLeaveLobbyCompleted(stateChange); break;
		case PFLobbyStateChangeType::JoinArrangedLobbyCompleted:OnJoinArrangedLobbyCompleted(stateChange); break;
		case PFLobbyStateChangeType::FindLobbiesCompleted:OnFindLobbiesCompleted(stateChange); break;
		case PFLobbyStateChangeType::InviteReceived:OnInviteReceived(stateChange); break;
		case PFLobbyStateChangeType::InviteListenerStatusChanged:OnInviteListenerStatusChanged(stateChange); break;
		case PFLobbyStateChangeType::SendInviteCompleted:OnSendInviteCompleted(stateChange); break;
		default:
			break;
		}
	}

	hr = PFMultiplayerFinishProcessingLobbyStateChanges(onlineManager->m_pfMultiplayerHandle, stateChangeCount, stateChanges);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to finish processing lobby state changes: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		return;
	}
}

void PlayFabLobby::CreateLobby(PFLobbyAccessPolicy accessPolicy)
{
	m_arrangedLobbyNetworkReady = false;
	const std::string strHostName = g_game->GetLocalPlayerName().data() + std::string("'s lobby ");
	const std::string strNetwork_Id = Managers::Get<OnlineManager>()->GetNetworkId();
	const std::string strDescriptor = Managers::Get<OnlineManager>()->GetDescriptor();
	const char* LobbyPropertyKey[LOBBY_PROPERTY_COUNT]{ LOBBY_PROPERTY_HOSTNAME, LOBBY_PROPERTY_NETWORKID, LOBBY_PROPERTY_DESCRIPTOR };
	const char* LobbyPropertyValue[LOBBY_PROPERTY_COUNT]{ strHostName.c_str(), strNetwork_Id.c_str(), strDescriptor.c_str() };
	const char* SearchPropertyKey[SEARCH_PROPERTY_COUNT]{ LOBBY_PROPERTY_HOSTNAME };
	const char* SearchPropertyValue[SEARCH_PROPERTY_COUNT]{ strHostName.c_str() };

	PFLobbyCreateConfiguration lobbyConfiguration{};
	lobbyConfiguration.maxMemberCount = MAX_MEMBER_COUNT_PER_LOBBY;
	lobbyConfiguration.ownerMigrationPolicy = PFLobbyOwnerMigrationPolicy::Automatic;
	lobbyConfiguration.accessPolicy = accessPolicy;
	lobbyConfiguration.searchPropertyCount = SEARCH_PROPERTY_COUNT;
	lobbyConfiguration.searchPropertyKeys = SearchPropertyKey;
	lobbyConfiguration.searchPropertyValues = SearchPropertyValue;
	lobbyConfiguration.lobbyPropertyCount = LOBBY_PROPERTY_COUNT;
	lobbyConfiguration.lobbyPropertyKeys = LobbyPropertyKey;
	lobbyConfiguration.lobbyPropertyValues = LobbyPropertyValue;

	PFLobbyJoinConfiguration creatorMemberConfiguration{};
	std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();

	creatorMemberConfiguration.memberPropertyCount = 0;

	// Create a new lobby and add the creating PlayFab entity to it as the creator
	// This is an asynchronous operation.
	// Upon successful completion, the title will be provided a
	// PFLobbyMemberAddedStateChange followed by a PFLobbyCreateAndJoinLobbyCompletedStateChange with the
	// PFLobbyCreateAndJoinLobbyCompletedStateChange::result field set to S_OK. 
	// Upon a failed completion, the title will be provided a PFLobbyCreateAndJoinLobbyCompletedStateChange with the
	// PFLobbyCreateAndJoinLobbyCompletedStateChange::result field set to a failure.
	const auto& onlineManager = Managers::Get<OnlineManager>();
	HRESULT hr = PFMultiplayerCreateAndJoinLobby(
		onlineManager->GetMultiplayerHandle(), // The handle of the PFMultiplayer API instance
		&Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey(),    // The local PlayFab entity creating the lobby
		&lobbyConfiguration,                                    // The initial configuration data used when creating the lobby
		&creatorMemberConfiguration,                            // The initial configuration data for the member creating and joining the lobby
		nullptr,                                                // An optional, app-defined, pointer-sized context value
		nullptr);                                               // The optional, output lobby object which can be used to queue operations for immediate execution if this operation completes

	if (FAILED(hr))
	{
		g_game->WriteDebugLogMessage("Failed to create and join lobby: 0x%08X, %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		onlineManager->GetOnlineMessageHandler()(onlineManager->GetLocalEntityId(), &CreateLobbyFailed);
		return;
	}
}

void PlayFabLobby::JoinLobby(const std::string& connectionString) {
	if (connectionString.empty())
	{
		DEBUGLOG("Failed to join lobby, empty connectionString\n");
		return;
	}

	m_arrangedLobbyNetworkReady = false;

	// Configuration can pass some key-value pairs as the initial member properties
	// See PFLobbyJoinConfiguration.memberPropertyKeys, PFLobbyJoinConfiguration.memberPropertyValues
	PFLobbyJoinConfiguration joinConfig;
	joinConfig.memberPropertyCount = 0;

	// Join the lobby using the connection string
	const auto& onlineManager = Managers::Get<OnlineManager>();
	HRESULT hr = PFMultiplayerJoinLobby(
		onlineManager->m_pfMultiplayerHandle, // The handle of the PFMultiplayer API instance
		&Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey(),   // The local entity joining the lobby
		connectionString.data(),                               // The connection string used by the entity to join the lobby
		&joinConfig,                                           // The initial configuration data used when joining the lobby
		nullptr,
		nullptr);

	if (FAILED(hr))
	{
		g_game->WriteDebugLogMessage("Failed to join lobby: connectionString(%s),\n ErrorCode(0x%08X) %s\n", static_cast<unsigned int>(hr), connectionString.c_str(), GetPlayFabErrorMessage(hr));
		onlineManager->GetOnlineMessageHandler()(onlineManager->GetLocalEntityId(), &CreateLobbyFailed);
		return;
	}
}

void PlayFabLobby::SetFindLobbyCallback(FindLobbyCallback completionCallback)
{
	m_FindLobbyCallback = completionCallback;
}

void PlayFabLobby::UpdateLobbyState(const PFLobbyAccessPolicy accessPolicy)
{
	if (Managers::Get<OnlineManager>()->IsHost())
	{
		PFLobbyDataUpdate lobbyUpdate;
		lobbyUpdate.newOwner = nullptr;
		lobbyUpdate.maxMemberCount = nullptr;
		lobbyUpdate.accessPolicy = &accessPolicy;
		lobbyUpdate.membershipLock = nullptr;
		lobbyUpdate.searchPropertyCount = 0;
		lobbyUpdate.lobbyPropertyCount = 0;
		HRESULT hr = PFLobbyPostUpdate(
			GetLobbyHandle(),
			&Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey(),
			&lobbyUpdate,
			nullptr,
			nullptr
		);

		if (FAILED(hr))
		{
			g_game->WriteDebugLogMessage("Failed to update lobby state: ErrorCode(0x%08X) %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
			return;
		}
	}
}

void PlayFabLobby::JoinArrangedLobby(const char* lobbyArrangementString)
{
	m_arrangedLobbyNetworkReady = false;
	const std::string strHostName = g_game->GetLocalPlayerName().data();
	const std::string strNetwork_Id = ""; // Don't set default network for arranged Matchmaking lobbies: lobby owner will create one
	const std::string strDescriptor = ""; // Don't set default network for arranged Matchmaking lobbies: lobby owner will create one

	const char* gameHostPropertyKey[LOBBY_PROPERTY_COUNT]{ LOBBY_PROPERTY_HOSTNAME, LOBBY_PROPERTY_NETWORKID, LOBBY_PROPERTY_DESCRIPTOR };
	const char* gameHostPropertyValue[LOBBY_PROPERTY_COUNT]{ strHostName.c_str(), strNetwork_Id.c_str(), strDescriptor.c_str() };
	PFLobbyArrangedJoinConfiguration lobbyConfiguration{};
	lobbyConfiguration.maxMemberCount = MAX_MEMBER_COUNT_PER_LOBBY;
	lobbyConfiguration.ownerMigrationPolicy = PFLobbyOwnerMigrationPolicy::Automatic;
	lobbyConfiguration.accessPolicy = PFLobbyAccessPolicy::Private;
	lobbyConfiguration.memberPropertyCount = LOBBY_PROPERTY_COUNT;
	lobbyConfiguration.memberPropertyKeys = gameHostPropertyKey;
	lobbyConfiguration.memberPropertyValues = gameHostPropertyValue;

	HRESULT hr = PFMultiplayerJoinArrangedLobby(
		Managers::Get<OnlineManager>()->m_pfMultiplayerHandle,
		&Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey(),
		lobbyArrangementString,
		&lobbyConfiguration,
		nullptr,
		nullptr
	);

	if (FAILED(hr))
	{
		g_game->WriteDebugLogMessage("Failed to join arranged lobby\n");
		return;
	}
	Managers::Get<OnlineManager>()->m_isJoiningArrangedLobby = true;
}

void PlayFabLobby::UpdateLobbyNetwork(std::string networkId, std::string descriptor)
{
	if (Managers::Get<OnlineManager>()->IsHost())
	{
		const std::string strHostName = g_game->GetLocalPlayerName().data();
		const std::string strNetwork_Id = networkId;
		const std::string strDescriptor = descriptor;
		const char* gameHostPropertyKey[LOBBY_PROPERTY_COUNT]{ LOBBY_PROPERTY_HOSTNAME, LOBBY_PROPERTY_NETWORKID, LOBBY_PROPERTY_DESCRIPTOR };
		const char* gameHostPropertyValue[LOBBY_PROPERTY_COUNT]{ strHostName.c_str(), strNetwork_Id.c_str(), strDescriptor.c_str() };

		const PFLobbyAccessPolicy accessPolicy = PFLobbyAccessPolicy::Public;
		PFLobbyHandle lobby = GetLobbyHandle();
		PFEntityKey localUser = Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey();
		PFLobbyDataUpdate lobbyUpdate;
		lobbyUpdate.newOwner = nullptr;
		lobbyUpdate.maxMemberCount = nullptr;
		lobbyUpdate.accessPolicy = &accessPolicy;
		lobbyUpdate.membershipLock = nullptr;
		lobbyUpdate.searchPropertyCount = 0;
		lobbyUpdate.lobbyPropertyCount = LOBBY_PROPERTY_COUNT;
		lobbyUpdate.lobbyPropertyKeys = gameHostPropertyKey;
		lobbyUpdate.lobbyPropertyValues = gameHostPropertyValue;
		HRESULT hr = PFLobbyPostUpdate(
			lobby,
			&localUser,
			&lobbyUpdate,
			nullptr,
			nullptr
		);

		if (FAILED(hr))
		{
			g_game->WriteDebugLogMessage("Failed to update lobby state: ErrorCode(0x%08X) %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
			return;
		}
	}
}

void PlayFabLobby::UpdateArrangedLobbyNetwork()
{
	UpdateLobbyNetwork(m_arrangedLobbyNetworkId, m_arrangedLobbyNetworkDescriptor);
}

void PlayFabLobby::Cleanup()
{
	m_lobbyId.clear();
	m_lobbyHandle = nullptr;
	m_CurrentMemberCount = 0;
	m_MatchmakingMemberCount = 0;
	m_pfLobbySearchResults.clear();
	m_FindLobbyCallback = nullptr;
	m_arrangedLobbyNetworkReady = false;
	m_arrangedLobbyNetworkId = "";
	m_arrangedLobbyNetworkDescriptor = "";
}

void PlayFabLobby::LeaveLobby()
{
	DEBUGLOG("Leave lobby");
	m_arrangedLobbyNetworkReady = false;
	if (!m_lobbyHandle)
	{
		DEBUGLOG("Failed to get lobby handle, the lobby does not exist.");
		m_lobbyId.clear();
		Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
		return;
	}
	HRESULT hr = PFLobbyLeave(
		m_lobbyHandle,
		&Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey(),
		nullptr);

	if (FAILED(hr))
	{
		g_game->WriteDebugLogMessage("Failed to leave lobby: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		const auto& onlineManager = Managers::Get<OnlineManager>();
		onlineManager->GetOnlineMessageHandler()(onlineManager->GetLocalEntityId(), &CreateLobbyFailed);
		return;
	}
	m_lobbyHandle = nullptr;
	m_lobbyId.clear();
}

void PlayFabLobby::OnCreateAndJoinLobbyCompleted(const PFLobbyStateChange* change)
{
	const auto stateChangeDetail = static_cast<const PFLobbyCreateAndJoinLobbyCompletedStateChange*>(change);
	if (!LogPFLobbyStateChangeFailResult(stateChangeDetail->result))
	{
		g_game->WriteDebugLogMessage(PFMultiplayerGetErrorMessage(stateChangeDetail->result));
		return;
	}

	m_lobbyHandle = stateChangeDetail->lobby;
	auto localPlayer = g_game->GetLocalPlayerState();
	localPlayer->InLobby = true;
	Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);
}

void PlayFabLobby::OnJoinLobbyCompleted(const PFLobbyStateChange* change)
{
	PlayFabOnlineManager* pManager = Managers::Get<OnlineManager>();
	const auto stateChangeDetail = static_cast<const PFLobbyJoinLobbyCompletedStateChange*>(change);
	if (!LogPFLobbyStateChangeFailResult(stateChangeDetail->result))
	{
		Managers::Get<ScreenManager>()->ShowError(GetPlayFabErrorMessage(stateChangeDetail->result), []() {
			Managers::Get<OnlineManager>()->InviteFinished();
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
			Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Ready);
			});
		return;
	}

	const PFEntityKey& localPlayerPFLoginEntityKey = Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey();
	if (strcmp(stateChangeDetail->newMember.id, localPlayerPFLoginEntityKey.id) != 0)
	{
		pManager->InviteFinished();
		Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
		Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Ready);
		DEBUGLOG("Received join lobby"
			"completed state change PFEntityKey.id = %s, local player login PFEntityKey.id = %s\n", stateChangeDetail->newMember.id, localPlayerPFLoginEntityKey.id);
		return;
	}
	m_lobbyHandle = stateChangeDetail->lobby;
	const char* pLobbyId;

	HRESULT GetLobbyidResult = PFLobbyGetLobbyId(m_lobbyHandle, &pLobbyId);
	if (!LogPFLobbyStateChangeFailResult(GetLobbyidResult))
	{
		Managers::Get<ScreenManager>()->ShowError(GetPlayFabErrorMessage(GetLobbyidResult), []() {
			auto localPlayer = g_game->GetLocalPlayerState();
			localPlayer->InLobby = true;
			Managers::Get<OnlineManager>()->InviteFinished();
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
			Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Ready);
			});

		return;
	}

	m_lobbyId = pLobbyId;

	if (pManager->ResetNetwork())
	{
		pManager->FindAndConnectToNetwork(pManager->GetNetworkId(), pManager->GetDescriptor());
	}
	else
	{
		DEBUGLOG("Network is not reset");
	}
}

void PlayFabLobby::OnMemberAdded(const PFLobbyStateChange* change)
{
	const auto stateChangeDetail = static_cast<const PFLobbyMemberAddedStateChange*>(change);
	const PFEntityKey newAddedMember = stateChangeDetail->member;

	m_lobbyHandle = stateChangeDetail->lobby;
	DEBUGLOG("New member added, member:%s\n", newAddedMember.id);
	m_CurrentMemberCount++;

	if (m_arrangedLobbyNetworkReady)
	{
		UpdateArrangedLobbyNetwork();
	}
}

// This state change will be generated by all operations which add members to lobbies,
// such as : "PFMultiplayerCreateAndJoinLobby", "PFMultiplayerJoinLobby", "PFLobbyAddMember"
void PlayFabLobby::OnAddMemberCompleted(const PFLobbyStateChange* change)
{
	const auto stateChangeDetail = static_cast<const PFLobbyAddMemberCompletedStateChange*>(change);
	HRESULT result = stateChangeDetail->result;
	if (!LogPFLobbyStateChangeFailResult(result))
	{
		DEBUGLOG("Failed to add member to lobby\n");
		return;
	}
}

void PlayFabLobby::OnMemberRemoved(const PFLobbyStateChange* change)
{
	DEBUGLOG("Member removed\n");
	// If a player was removed because of internet connectivity error, display the error message on the screen instead of returning to the main menu
	if (Managers::Get<GameStateManager>()->GetState() == GameState::InternetConnectivityError)
	{
		return;
	}
	m_CurrentMemberCount--;
	if (m_CurrentMemberCount <= 0)
	{
		UpdateLobbyState(PFLobbyAccessPolicy::Private);
	}
	const auto stateChangeDetail = static_cast<const PFLobbyMemberRemovedStateChange*>(change);

	const PFEntityKey removedMember = stateChangeDetail->member;
	const PFLobbyMemberRemovedReason removedReason = stateChangeDetail->reason;

	switch (removedReason)
	{
	case PFLobbyMemberRemovedReason::LocalUserLeftLobby:
	{
		m_CurrentMemberCount = 0;
		m_MatchmakingMemberCount = 0;
		Managers::Get<OnlineManager>()->m_playfabParty.LeaveNetwork(
			[this]()
			{
				auto onlineManager = Managers::Get<OnlineManager>();
				onlineManager->m_messageHandler(onlineManager->GetLocalEntityId(), &LeaveGameComplete);
			});
		break;
	}
	case PFLobbyMemberRemovedReason::RemoteUserLeftLobby:
	{
		g_game->RemovePlayerFromLobbyPeers(removedMember.id);
		if (Managers::Get<OnlineManager>()->m_isJoiningArrangedLobby)
		{
			Managers::Get<ScreenManager>()->ShowError("Match failed, some player leave game.", []() {
				Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
				Managers::Get<OnlineManager>()->m_isJoiningArrangedLobby = false;
				});
		}
		RefreshLobbyOwner();
		break;
	}
	default:
		break;
	}
}

void PlayFabLobby::OnUpdated(const PFLobbyStateChange* change)
{
	// This state change signifies that the lobby has updated and provides hints as to which values have changed. 
	// Multiple updates may be provided by a single call to "PFMultiplayerStartProcessingLobbyStateChanges()". 
	// All states reflected by these updates will become available simultaneously when StartProcessingLobbyStateChanges() is
	// called, so the updates can be reconciled either individually or as a batch.
	const auto stateChangeDetail = static_cast<const PFLobbyUpdatedStateChange*>(change);
	const bool ownerUpdated = stateChangeDetail->ownerUpdated;
	const bool maxMembersUpdated = stateChangeDetail->maxMembersUpdated;
	const bool accessPolicyUpdated = stateChangeDetail->accessPolicyUpdated;
	const bool membershipLockUpdated = stateChangeDetail->membershipLockUpdated;
	const uint32_t updatedSearchPropertyCount = stateChangeDetail->updatedSearchPropertyCount;
	const uint32_t updatedLobbyPropertyCount = stateChangeDetail->updatedLobbyPropertyCount;
	const char* const* updatedLobbyPropertyKeys = stateChangeDetail->updatedLobbyPropertyKeys;
	const uint32_t memberUpdateCount = stateChangeDetail->memberUpdateCount;

	PlayFabOnlineManager* pManager = Managers::Get<OnlineManager>();
	if (!pManager->m_playfabParty.IsHost()
		&& Managers::Get<GameStateManager>()->GetState() == GameState::Lobby
		&& updatedLobbyPropertyCount != 0)
	{
		if (stateChangeDetail->lobby != m_lobbyHandle)
		{
			DEBUGLOG("Received an update notification from a different lobby");
			return;
		}

		if (pManager->ResetNetwork())
		{
			pManager->FindAndConnectToNetwork(pManager->GetNetworkId(), pManager->GetDescriptor());
		}
		else
		{
			DEBUGLOG("Network is not reset");
		}
	}
}

void PlayFabLobby::OnDisconnected(const PFLobbyStateChange* change)
{
	m_arrangedLobbyNetworkReady = false;

	// This is the last state change that will be provided for this lobby object. 
	// Once this state change is returned to "PFMulitplayerFinishProcessingLobbyStateChanges()",
	// the lobby object will become invalid.
	m_lobbyHandle = nullptr;
	m_lobbyId.clear();
}

void PlayFabLobby::OnLeaveLobbyCompleted(const PFLobbyStateChange* change)
{
	m_arrangedLobbyNetworkReady = false;
	const auto stateChangeDetail = static_cast<const PFLobbyLeaveLobbyCompletedStateChange*>(change);
	if (!Managers::Get<OnlineManager>()->IsHost())
	{
		Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
	}
}

void PlayFabLobby::OnJoinArrangedLobbyCompleted(const PFLobbyStateChange* change)
{
	m_arrangedLobbyNetworkReady = false;
	const auto stateChangeDetail = static_cast<const PFLobbyJoinArrangedLobbyCompletedStateChange*>(change);
	const HRESULT result = stateChangeDetail->result;
	if (!LogPFLobbyStateChangeFailResult(stateChangeDetail->result))
	{
		g_game->WriteDebugLogMessage(PFMultiplayerGetErrorMessage(stateChangeDetail->result));
		return;
	}

	const PFEntityKey newMember = stateChangeDetail->newMember;
	const PFEntityKey* lobbyOwnerMember;
	HRESULT hr = PFLobbyGetOwner(m_lobbyHandle, &lobbyOwnerMember);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to get lobby owner\n");
		return;
	}
	std::shared_ptr<PlayerState> player = g_game->GetLocalPlayerState();
	if (lobbyOwnerMember->id == player->EntityId)
	{
		Managers::Get<OnlineManager>()->m_playfabParty.SetHost(true);
		std::string networkId = GuidUtil::NewGuid();
		Managers::Get<OnlineManager>()->m_playfabParty.CreateAndConnectToNetwork(
			networkId.c_str(),
			[this, networkId](std::string descriptor)
				{
					g_game->GetLocalPlayerState()->InLobby = true;
					m_arrangedLobbyNetworkId = networkId;
					m_arrangedLobbyNetworkDescriptor = descriptor;
					m_arrangedLobbyNetworkReady = true;
					UpdateArrangedLobbyNetwork();
				});
	}
	Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);
}

void PlayFabLobby::FindLobbies()
{
	Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Joining);
	PFLobbySearchConfiguration  searchConfiguration;
	searchConfiguration.filterString = "";
	searchConfiguration.sortString = "";
	searchConfiguration.friendsFilter = nullptr;
	const uint32_t* temp = new uint32_t(MAX_LOBBY_COUNTS);
	searchConfiguration.clientSearchResultCount = temp;
	const auto& onlineManager = Managers::Get<OnlineManager>();
	HRESULT hr = PFMultiplayerFindLobbies(
		onlineManager->GetMultiplayerHandle(), // The handle of the PFMultiplayer API instance
		&Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey(),    // The local PlayFab entity creating the lobby
		&searchConfiguration,                                    // The initial configuration data used when creating the lobby
		nullptr);                                               // The optional, output lobby object which can be used to queue operations for immediate execution if this operation completes

	if (FAILED(hr))
	{
		g_game->WriteDebugLogMessage("Failed to create and join lobby: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		onlineManager->GetOnlineMessageHandler()(onlineManager->GetLocalEntityId(), &CreateLobbyFailed);
		return;
	}
}

void PlayFabLobby::OnFindLobbiesCompleted(const PFLobbyStateChange* change)
{
	const auto stateChangeDetail = static_cast<const PFLobbyFindLobbiesCompletedStateChange*>(change);
	const HRESULT result = stateChangeDetail->result;
	if (!LogPFLobbyStateChangeFailResult(stateChangeDetail->result))
	{
		g_game->WriteDebugLogMessage(PFMultiplayerGetErrorMessage(stateChangeDetail->result));
		return;
	}

	m_pfLobbySearchResults.clear();

	const PFEntityKey searchingEntity = stateChangeDetail->searchingEntity;
	const uint32_t searchResultCount = stateChangeDetail->searchResultCount;
	if (searchResultCount != 0)
	{
		const auto& searchResults{ stateChangeDetail->searchResults };
		for (size_t i = 0; i < searchResultCount; ++i)
		{
			if (searchResults[i].lobbyId != nullptr)
			{
				auto resultI = searchResults[i];
				std::shared_ptr<LobbySearchResult> searchResult = std::make_shared<LobbySearchResult>();
				searchResult->lobbyId = resultI.lobbyId;
				searchResult->connectionString = resultI.connectionString;
				searchResult->ownerEntityKeyId = resultI.ownerEntity->id;
				searchResult->ownerEntityKeyType = resultI.ownerEntity->type;
				searchResult->maxMemberCount = resultI.maxMemberCount;
				searchResult->currentMemberCount = resultI.currentMemberCount;
				searchResult->searchPropertyCount = resultI.searchPropertyCount;
				for (size_t j = 0; j < searchResult->searchPropertyCount; ++j)
				{
					searchResult->searchProperties.insert({ resultI.searchPropertyKeys[j], resultI.searchPropertyValues[j] });
				}
				searchResult->friendCount = resultI.friendCount;
				for (size_t k = 0; k < searchResult->friendCount; ++k)
				{
					searchResult->friends.insert({ resultI.friends[k].id, resultI.friends[k].type });
				}
				m_pfLobbySearchResults.push_back(searchResult);
			}
		}
	}
	m_FindLobbyCallback(m_pfLobbySearchResults);
}

void PlayFabLobby::OnInviteReceived(const PFLobbyStateChange* change)
{
	const auto stateChangeDetail = static_cast<const PFLobbyInviteReceivedStateChange*>(change);

	// The entity which is listening for invites and which has been invited.
	const PFEntityKey listeningEntity = stateChangeDetail->listeningEntity;
	// The entity which has invited the "listeningEntity" to a lobby.
	const PFEntityKey invitingEntity = stateChangeDetail->invitingEntity;
	const std::string_view connectionString = stateChangeDetail->connectionString;
	if (connectionString.empty())
	{
		DEBUGLOG("Failed to get connectionString, the connectionString is empty\n");
		return;
	}
}

void PlayFabLobby::OnInviteListenerStatusChanged(const PFLobbyStateChange* change)
{
	const auto stateChangeDetail = static_cast<const PFLobbyInviteListenerStatusChangedStateChange*>(change);
	const PFEntityKey listeningEntity = stateChangeDetail->listeningEntity;
}

void PlayFabLobby::OnSendInviteCompleted(const PFLobbyStateChange* change)
{
	const auto stateChangeDetail = static_cast<const PFLobbySendInviteCompletedStateChange*>(change);

	// The result of the send invite operation.
	const HRESULT result = stateChangeDetail->result;
	if (!LogPFLobbyStateChangeFailResult(stateChangeDetail->result))
	{
		DEBUGLOG("Failed to send invite");
		return;
	}
}
