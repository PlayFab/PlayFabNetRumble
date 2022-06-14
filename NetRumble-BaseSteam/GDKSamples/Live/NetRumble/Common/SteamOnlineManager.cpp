#include "pch.h"

using namespace NetRumble;

NetRumble::SteamOnlineManager::SteamOnlineManager()
{
	// Initialize the peer to peer connection process
	SteamNetworkingUtils()->InitRelayNetworkAccess();
}

NetRumble::SteamOnlineManager::~SteamOnlineManager()
{
}

void NetRumble::SteamOnlineManager::CreateLobby()
{
	m_lobby.CreateLobby();
}

void NetRumble::SteamOnlineManager::FindLobbies()
{
	m_lobby.FindLobbies();
}

void NetRumble::SteamOnlineManager::JoinLobby(CSteamID steamLobbyID)
{
	m_lobby.JoinLobby(steamLobbyID);
}

void NetRumble::SteamOnlineManager::LeaveLobby()
{
	m_lobby.LeaveLobby();
}

void NetRumble::SteamOnlineManager::SetReadyStateInLobby(bool readyState)
{
	m_lobby.SetReadyStateInLobby(readyState);
}

void NetRumble::SteamOnlineManager::SetShipColorInLobby(byte shipColor)
{
	m_lobby.SetShipColorInLobby(shipColor);
}

void NetRumble::SteamOnlineManager::SetShipVariationInLobby(byte shipVariation)
{
	m_lobby.SetShipVariationInLobby(shipVariation);
}

void NetRumble::SteamOnlineManager::SetIsServer(bool isServer)
{
	m_lobby.SetIsServer(isServer);
}

bool NetRumble::SteamOnlineManager::IsServer() const
{
	return m_lobby.IsServer();
}

void NetRumble::SteamOnlineManager::SetFindLobbyCallback(FindLobbyCallback completionCallback)
{
	m_lobby.SetFindLobbyCallback(completionCallback);
}

const CSteamID NetRumble::SteamOnlineManager::GetSteamIDLobby()
{
	return m_lobby.GetSteamIDLobby();
}

void NetRumble::SteamOnlineManager::PreparingGameServer()
{
	m_lobby.PreparingGameServer();
}

void NetRumble::SteamOnlineManager::AcceptInvitationWhenGameNotStart(const char* pLobbyID)
{
	m_lobby.AcceptInvitationWhenGameNotStart(pLobbyID);
}

void SteamOnlineManager::InviteFriendsIntoLobby()
{
	m_lobby.InviteFriendsIntoLobby();
}

void SteamOnlineManager::CheckForItemDrops()
{
	m_inventory.CheckForItemDrops();
}

void SteamOnlineManager::GetAllItems()
{
	m_inventory.GetAllItems();
}

void NetRumble::SteamOnlineManager::Tick(float elapsedTime)
{
	UNREFERENCED_PARAMETER(elapsedTime);

	SteamAPI_RunCallbacks();

	ClientProcessNetworkMessage();
}

// Steam will not let the game start when network is not available
bool SteamOnlineManager::IsNetworkAvailable() const
{
	return true;
}

void SteamOnlineManager::StartMatchmaking()
{
	m_matchmaking.StartMatchmaking();
}

bool NetRumble::SteamOnlineManager::IsMatchmaking()
{
	return m_matchmaking.IsMatchmaking();
}

void SteamOnlineManager::CancelMatchmaking()
{
	m_matchmaking.CancelMatchmaking();
}

void SteamOnlineManager::LeaveMultiplayerGame()
{
	DEBUGLOG("Leave Multipalyer Game\n");

	g_game->DeleteOtherPlayerInPeersAndExitLobby();
	Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
	DisconnectFromServer();
	m_lobby.SetIsServer(false);
}

void SteamOnlineManager::UpdateLeaderboards(LeaderboardsTypes typeOfLeaderboard)
{
	m_leaderboard.UpdateLeaderboards(typeOfLeaderboard);
}

void SteamOnlineManager::GetLeaderboards(LeaderboardsTypes typeOfLeaderboard)
{
	m_leaderboard.GetLeaderboards(typeOfLeaderboard);
}

void SteamOnlineManager::SetFindLeaderboardCallback(FindLeaderboardCallback completionCallback)
{
	m_leaderboard.SetFindLeaderboardCallback(completionCallback);
}

int SteamOnlineManager::GetStartGameCount()
{
	return m_statsAndAchievements.GetStartGameCount();
}

int NetRumble::SteamOnlineManager::GetDeathCount()
{
	return m_statsAndAchievements.GetDeathCount();
}

int NetRumble::SteamOnlineManager::GetVictoryCount()
{
	return m_statsAndAchievements.GetVictoryCount();
}

void SteamOnlineManager::SetVictoryCount()
{
	m_statsAndAchievements.SetVictoryCount();
}

void SteamOnlineManager::SetStartGameCount()
{
	m_statsAndAchievements.SetStartGameCount();
}

void SteamOnlineManager::SetDeathCount()
{
	m_statsAndAchievements.SetDeathCount();
}

void  SteamOnlineManager::InitializeStatsAndAchievements()
{
	m_statsAndAchievements.InitializeStatsAndAchievements();
}
