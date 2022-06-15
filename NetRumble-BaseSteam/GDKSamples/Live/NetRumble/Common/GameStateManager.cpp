//--------------------------------------------------------------------------------------
// GameStateManager.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

GameStateManager::GameStateManager() :
	_state{ GameState::Initialize },
	_nextState{ GameState::Initialize },
	_immutableState{ false }
{
}

void GameStateManager::Update()
{
	if (_state != _nextState)
	{
		_state = _nextState;

		switch (_state)
		{
		case GameState::MainMenu:
		{
			ScreenManager* screenMgr = Managers::Get<ScreenManager>();
			screenMgr->ExitAllScreens();
			screenMgr->AddGameScreen<MainMenuScreen>();
			screenMgr->SetBackgroundsVisible(true);
			Managers::Get<OnlineManager>()->InitializeStatsAndAchievements();

			// If invite info exists, join now
			break;
		}
		case GameState::MPHostGame:
		{
			ScreenManager* screenMgr = Managers::Get<ScreenManager>();
			screenMgr->ExitAllScreens();
			screenMgr->AddGameScreen<GameLobbyScreen>();

			// Request create lobby by steam
			Managers::Get<OnlineManager>()->CreateLobby();

			break;
		}
		case GameState::MPJoinLobby:
		{
			ScreenManager* screenMgr = Managers::Get<ScreenManager>();
			screenMgr->ExitAllScreens();
			screenMgr->AddGameScreen<GameLobbyScreen>();
			break;
		}
		case GameState::MPMatchmaking:
		{
			ScreenManager* screenMgr = Managers::Get<ScreenManager>();
			screenMgr->ExitAllScreens();
			screenMgr->AddGameScreen<GameLobbyScreen>();

			Managers::Get<OnlineManager>()->StartMatchmaking();
			break;
		}
		case GameState::JoinGameFromLobby:
		{
			if (g_game->GetLocalPlayerState()->InGame == false && g_game->GetLocalPlayerState()->InLobby == true)
			{
				g_game->GetLocalPlayerState()->InGame = true;
				g_game->GetLocalPlayerState()->InLobby = false;
				g_game->GetLocalPlayerState()->IsReturnedToMainMenu = false;
				Managers::Get<OnlineManager>()->SendGameMessageWithSourceID(
					GameMessage(
						GameMessageType::PlayerJoined,
						g_game->GetLocalPlayerState()->SerializePlayerStateData()
					)
				);

				Managers::Get<OnlineManager>()->LeaveLobby();
				Managers::Get<OnlineManager>()->SetStartGameCount();
				Managers::Get<GameStateManager>()->SwitchToState(GameState::Gaming);

				// Upload start count to leaderboard
				Managers::Get<OnlineManager>()->UpdateLeaderboards(LeaderboardsTypes::EnteryGameTimesLeaderboard);
			}
			break;
		}
		case GameState::EvaluatingNetwork:
		{
			// Only server can jump here
			if (Managers::Get<OnlineManager>()->IsServer())
			{
				g_game->ServerPrepareGameEnviroment();
				g_game->StartGame();
			}
			break;
		}
		case GameState::Gaming:
		{
			if (!Managers::Get<OnlineManager>()->IsServer())
			{
				g_game->StartGame();
			}
			break;
		}
		default:
			break;
		}
	}
}

void NetRumble::GameStateManager::SwitchToState(GameState state)
{
	if (_immutableState)
	{
		DEBUGLOG("_immutableState is true, the _nextState cannot be chagned \n");
	}
	else 
	{
		_nextState = state;
	}
}
