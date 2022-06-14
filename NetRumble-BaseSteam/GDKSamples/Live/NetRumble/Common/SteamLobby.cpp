#include "pch.h"

using namespace NetRumble;

namespace {
	constexpr int LobbyMaxMembers = 4;
	const char* KeyReady = "ready";
	const char* KeyShipColor = "shipColor";
	const char* KeyShipVariation = "shipVariation";
	const char* KeyGameStarting = "game_starting";
	const char* CharTrue = "1";
	const char* CharFalse = "0";
}

void NetRumble::Lobby::CreateLobby()
{
	SteamAPICall_t hSteamAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, MAX_PLAYERS_PER_SERVER);
	m_SteamCallResultLobbyCreated.Set(hSteamAPICall, this, &Lobby::OnLobbyCreated);
}

void NetRumble::Lobby::FindLobbies()
{
	SteamAPICall_t hSteamAPICall = SteamMatchmaking()->RequestLobbyList();
	m_SteamCallResultLobbyMatchList.Set(hSteamAPICall, this, &Lobby::OnLobbyMatchListCallback);
}

void NetRumble::Lobby::JoinLobby(CSteamID steamLobbyID)
{
	SteamAPICall_t hSteamAPICall = SteamMatchmaking()->JoinLobby(steamLobbyID);
	m_SteamCallResultLobbyEntered.Set(hSteamAPICall, this, &Lobby::OnLobbyEntered);
	Managers::Get<GameStateManager>()->SwitchToState(GameState::MPJoinLobby);
}

void NetRumble::Lobby::LeaveLobby()
{
	if (m_steamIDLobby.IsValid())
	{
		SteamMatchmaking()->LeaveLobby(m_steamIDLobby);
	}
}

void NetRumble::Lobby::SetReadyStateInLobby(bool readyState)
{
	SteamMatchmaking()->SetLobbyMemberData(m_steamIDLobby, KeyReady, readyState ? CharTrue : CharFalse);
}

void NetRumble::Lobby::SetShipColorInLobby(byte shipColor)
{
	std::string s = std::to_string(shipColor);
	SteamMatchmaking()->SetLobbyMemberData(m_steamIDLobby, KeyShipColor, s.c_str());
}

void NetRumble::Lobby::SetShipVariationInLobby(byte ShipVariation)
{
	std::string s = std::to_string(ShipVariation);
	SteamMatchmaking()->SetLobbyMemberData(m_steamIDLobby, KeyShipVariation, s.c_str());
}

void NetRumble::Lobby::OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure)
{
	UNREFERENCED_PARAMETER(bIOFailure);

	if (pCallback->m_eResult == EResult::k_EResultOK)
	{
		DEBUGLOG("success and lobby ID == %llu \n", pCallback->m_ulSteamIDLobby);
		m_steamIDLobby = pCallback->m_ulSteamIDLobby;

		char rgchLobbyName[256];
		sprintf_safe(rgchLobbyName, "%s's lobby", SteamFriends()->GetPersonaName());
		SteamMatchmaking()->SetLobbyData(m_steamIDLobby, "name", rgchLobbyName);
		Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);

		m_isServer = true;

		AddPlayerStateToPeers(SteamUser()->GetSteamID().ConvertToUint64(), true);
	}
	else
	{
		DEBUGLOG("Failed to create lobby(lost connection to Steam back - end servers).");
	}
}

void NetRumble::Lobby::OnLobbyMatchListCallback(LobbyMatchList_t* pLobbyMatchList, bool bIOFailure)
{
	UNREFERENCED_PARAMETER(bIOFailure);

	if (m_FindLobbyCallback) 
	{
		std::vector<std::shared_ptr<OnlineUser>> users;

		for (uint32 iLobby = 0; iLobby < pLobbyMatchList->m_nLobbiesMatching; iLobby++)
		{
			CSteamID steamIDLobby = SteamMatchmaking()->GetLobbyByIndex(static_cast<int>(iLobby));
			const char* pchLobbyName = SteamMatchmaking()->GetLobbyData(steamIDLobby, "name");
			if (pchLobbyName && pchLobbyName[0]) {
				DEBUGLOG("success and lobby ID == %s \n", pchLobbyName);

				std::shared_ptr<OnlineUser> lobby = std::make_shared<OnlineUser>();
				lobby->Id = steamIDLobby.ConvertToUint64();
				lobby->Name = pchLobbyName;
				
				users.push_back(lobby);
			}
		}

		m_FindLobbyCallback(users);
	}
}

void NetRumble::Lobby::OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure)
{
	UNREFERENCED_PARAMETER(bIOFailure);

	if (pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess)
	{
		switch (pCallback->m_EChatRoomEnterResponse)
		{
		case k_EChatRoomEnterResponseFull:
		{
			std::string message = "Failed to enter lobby, lobby has reached its maximum size";
			DEBUGLOG("%s\n", message.c_str());
			Managers::Get<GameEventManager>()->DispatchEvent(GameEventMessage{ GameEventType::LobbyFull, const_cast<char*>(message.c_str()) });
			break;
		}
		default:
		{
			DEBUGLOG("Failed to enter lobby");
			break;
		}
		}
		return;
	}

	m_steamIDLobby = pCallback->m_ulSteamIDLobby;

	uint64 localPlayerID = SteamUser()->GetSteamID().ConvertToUint64();
	int lobbyMemberCount = SteamMatchmaking()->GetNumLobbyMembers(m_steamIDLobby);
	for (int i = 0; i < lobbyMemberCount; i++)
	{
		CSteamID player = SteamMatchmaking()->GetLobbyMemberByIndex(m_steamIDLobby, i);
		if (player == k_steamIDNil)
		{
			continue;
		}

		uint64 playerID = player.ConvertToUint64();
		bool isLocalPlayer = playerID == localPlayerID;
		AddPlayerStateToPeers(player.ConvertToUint64(), isLocalPlayer);
	}

	Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);
}

bool NetRumble::Lobby::CheckAllPlayerReady()
{
	bool allIsReady = true;

	int lobbyMembersCount = SteamMatchmaking()->GetNumLobbyMembers(m_steamIDLobby);

	if (lobbyMembersCount > 0) 
	{
		for (int i = 0; i < lobbyMembersCount; i++)
		{
			CSteamID steamIDLobbyMember = SteamMatchmaking()->GetLobbyMemberByIndex(m_steamIDLobby, i);

			const char* pchName = SteamFriends()->GetFriendPersonaName(steamIDLobbyMember);

			if (pchName && *pchName)
			{
				const char* pchReady = SteamMatchmaking()->GetLobbyMemberData(m_steamIDLobby, steamIDLobbyMember, KeyReady);
				bool bReady = (pchReady && atoi(pchReady) == 1);

				if (bReady == false) 
				{
					allIsReady = false;
					break;
				}
			}
		}
	}
	else
	{
		allIsReady = false;
	}

	return allIsReady;
}

void NetRumble::Lobby::AddPlayerStateToPeers(uint64_t steamUserID, bool isLocalPlayer)
{
	std::shared_ptr<PlayerState> localPlayerState;

	if (steamUserID == SteamUser()->GetSteamID().ConvertToUint64()) {
		localPlayerState = g_game->GetLocalPlayerState();
	}
	else
	{
		localPlayerState = std::make_shared<PlayerState>();
	}

	localPlayerState->IsLocalPlayer = isLocalPlayer;
	localPlayerState->InLobby = true;
	localPlayerState->IsReturnedToMainMenu = false;
	localPlayerState->ShipColor(0);
	localPlayerState->ShipVariation(0);
	localPlayerState->PeerId = steamUserID;

	const char* pchReady = SteamMatchmaking()->GetLobbyMemberData(m_steamIDLobby, steamUserID, KeyReady);
	bool bReady = (pchReady && atoi(pchReady) == 1);
	localPlayerState->LobbyReady = bReady;

	const char* pchShipColor = SteamMatchmaking()->GetLobbyMemberData(m_steamIDLobby, steamUserID, KeyShipColor);
	localPlayerState->ShipColor(static_cast<byte>(atoi(pchShipColor)));

	const char* pchShipVariation = SteamMatchmaking()->GetLobbyMemberData(m_steamIDLobby, steamUserID, KeyShipVariation);
	localPlayerState->ShipVariation(static_cast<byte>(atoi(pchShipVariation)));

	const char* pchName = SteamFriends()->GetFriendPersonaName(steamUserID);
	if (pchName && *pchName)
	{
		localPlayerState->DisplayName = pchName;
	}

	g_game->SetPlayerState(localPlayerState->PeerId, localPlayerState);
}

void NetRumble::Lobby::OnPersonaStateChange(PersonaStateChange_t* pCallback)
{
	if (!SteamFriends()->IsUserInSource(pCallback->m_ulSteamID, m_steamIDLobby))
	{
		return;
	}
}

void NetRumble::Lobby::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback)
{
	if (m_steamIDLobby != pCallback->m_ulSteamIDLobby)
	{
		return;
	}

	if (pCallback->m_ulSteamIDMember == pCallback->m_ulSteamIDLobby)
	{
		const char* gameStarting = SteamMatchmaking()->GetLobbyData(m_steamIDLobby, KeyGameStarting);
		bool bGameStarting = (gameStarting && atoi(gameStarting) == 1);
		if (bGameStarting)
		{
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MatchingGame);
		}
	}
	else
	{
		if (pCallback->m_ulSteamIDLobby != pCallback->m_ulSteamIDMember)
		{
			std::shared_ptr<PlayerState> playerState = g_game->GetPlayerState(pCallback->m_ulSteamIDMember);

			const char* pchReady = SteamMatchmaking()->GetLobbyMemberData(m_steamIDLobby, pCallback->m_ulSteamIDMember, KeyReady);
			bool bReady = (pchReady && atoi(pchReady) == 1);
			playerState->LobbyReady = bReady;

			const char* pchShipColor = SteamMatchmaking()->GetLobbyMemberData(m_steamIDLobby, pCallback->m_ulSteamIDMember, KeyShipColor);
			playerState->ShipColor(static_cast<byte>(atoi(pchShipColor)));

			const char* pchShipVariation = SteamMatchmaking()->GetLobbyMemberData(m_steamIDLobby, pCallback->m_ulSteamIDMember, KeyShipVariation);
			playerState->ShipVariation(static_cast<byte>(atoi(pchShipVariation)));
		}

		bool lobbyReady = CheckAllPlayerReady();
		if (lobbyReady && m_isServer)
		{
			PreparingGameServer();
		}
	}
}

void NetRumble::Lobby::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback)
{
	if (m_steamIDLobby != pCallback->m_ulSteamIDLobby)
	{
		return;
	}

	if (pCallback->m_ulSteamIDUserChanged == SteamUser()->GetSteamID().ConvertToUint64() &&
		(pCallback->m_rgfChatMemberStateChange &
			(k_EChatMemberStateChangeLeft |
				k_EChatMemberStateChangeDisconnected |
				k_EChatMemberStateChangeKicked |
				k_EChatMemberStateChangeBanned)))
	{
		m_steamIDLobby = CSteamID();
	}

	if (pCallback->m_rgfChatMemberStateChange == k_EChatMemberStateChangeEntered)
	{
		AddPlayerStateToPeers(pCallback->m_ulSteamIDUserChanged);
	}
	else
	{
		// Player left
		bool removePlayerFromLobbyPeersResult = g_game->RemovePlayerFromLobbyPeers(pCallback->m_ulSteamIDUserChanged);
		if (!removePlayerFromLobbyPeersResult) {
			DEBUGLOG("Remove player from lobby peers failed.");
			return;
		}
	}
}

void NetRumble::Lobby::OnLobbyGameCreated(LobbyGameCreated_t* pCallback)
{
	if (IsServer())
	{
		return;
	}

	if (CSteamID(pCallback->m_ulSteamIDGameServer).IsValid())
	{
		Managers::Get<OnlineManager>()->InitiateServerConnection(CSteamID(pCallback->m_ulSteamIDGameServer));
	}
}

void NetRumble::Lobby::OnGameJoinRequested(GameRichPresenceJoinRequested_t* pCallback)
{
	UNREFERENCED_PARAMETER(pCallback);
}

void NetRumble::Lobby::PreparingGameServer()
{
	SteamMatchmaking()->SetLobbyData(m_steamIDLobby, KeyGameStarting, CharTrue);

	Managers::Get<OnlineManager>()->CreateGameServer();
}

void NetRumble::Lobby::SetFindLobbyCallback(FindLobbyCallback completionCallback)
{
	m_FindLobbyCallback = completionCallback;
}

void NetRumble::Lobby::InviteFriendsIntoLobby()
{
	CSteamID steamIDLobby = Managers::Get<OnlineManager>()->GetSteamIDLobby();
	if (steamIDLobby.IsValid())
	{
		SteamFriends()->ActivateGameOverlayInviteDialog(steamIDLobby);
	}
}

void NetRumble::Lobby::AcceptFriendInvitationsInMenuOrLobby(GameLobbyJoinRequested_t* pJoin)
{
	GameState currentState = Managers::Get<GameStateManager>()->GetState();

	if (currentState == GameState::MainMenu)
	{
		if (pJoin->m_steamIDLobby.IsValid())
		{
			UserBeInvitedIntoLobby(pJoin->m_steamIDLobby);
		}
	}
	else if (currentState == GameState::Lobby)
	{
		CSteamID steamIDLobby = Managers::Get<OnlineManager>()->GetSteamIDLobby();
		if (steamIDLobby == pJoin->m_steamIDLobby)
		{
			return;
		}
		else 
		{
			if (pJoin->m_steamIDLobby.IsValid())
			{
				LeaveLobby();
				Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
				UserBeInvitedIntoLobby(pJoin->m_steamIDLobby);
			}	
		}
	}
}

void NetRumble::Lobby::UserBeInvitedIntoLobby(CSteamID invitedLobbyID)
{
	m_steamIDLobby = invitedLobbyID;
	JoinLobby(m_steamIDLobby);
}

void NetRumble::Lobby::AcceptInvitationWhenGameNotStart(const char* pLobbyID)
{
	if (pLobbyID)
	{
		CSteamID steamIDLobby((uint64)atoll(pLobbyID));
		if (steamIDLobby.IsValid())
		{
			UserBeInvitedIntoLobby(steamIDLobby);
		}
	}
}