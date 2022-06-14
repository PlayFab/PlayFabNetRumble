#include "pch.h"

void NetRumble::Matchmaking::StartMatchmaking()
{
	DEBUGLOG("Start Matchmaking\n");
	Managers::Get<OnlineManager>()->SetFindLobbyCallback([](std::vector<std::shared_ptr<OnlineUser>> lobbies) {
		GameState gameState = Managers::Get<GameStateManager>()->GetState();
		if (gameState == GameState::MPMatchmaking) {
			if (!lobbies.empty()) {
				Managers::Get<OnlineManager>()->JoinLobby(lobbies.begin()->get()->Id);
			}
			else
			{
				DEBUGLOG("Vector users is empty, creating a lobby ourselves");
				Managers::Get<OnlineManager>()->CreateLobby();
			}
		}});
	Managers::Get<OnlineManager>()->FindLobbies();
}

void NetRumble::Matchmaking::CancelMatchmaking()
{
	DEBUGLOG("Cancel Matchmaking\n");

	if (m_onlineState != OnlineState::Matchmaking)
	{
		DEBUGLOG("Multiplayer state is %d, expected %d.\n", m_onlineState, OnlineState::Ready);
		return;
	}
}
