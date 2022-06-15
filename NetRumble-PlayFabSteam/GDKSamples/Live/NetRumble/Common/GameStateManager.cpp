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

		ScreenManager* screenManager = Managers::Get<ScreenManager>();
		PlayFabOnlineManager* onlineManager = Managers::Get<OnlineManager>();
		switch (_state)
		{
		case GameState::StartMenu:
		{
			screenManager->ExitAllScreens();
			screenManager->AddGameScreen<UserStartupScreen>();
			screenManager->SetBackgroundsVisible(true);
			break;
		}
		case GameState::MainMenu:
		{
			screenManager->ExitAllScreens();
			screenManager->AddGameScreen<MainMenuScreen>();
			screenManager->SetBackgroundsVisible(true);
			Managers::Get<OnlineManager>()->InitializeStatsAndAchievements();
			onlineManager->SwitchToOnlineState(OnlineState::Ready);

			// If invite info exists, join now
			if (onlineManager->HasPendingInviteSession() || onlineManager->IsJoiningFromInvite())
			{
				if (onlineManager->JoinPendingInviteSession())
				{
					onlineManager->SetHasPendingInvite(false);
				}
			}
			break;
		}
		case GameState::MPHostGame:
		{
			screenManager->ExitAllScreens();
			screenManager->AddGameScreen<GameLobbyScreen>();
			onlineManager->HostMultiplayerGame();
			break;
		}
		case GameState::MPJoinLobby:
		{
			screenManager->ExitAllScreens();
			screenManager->AddGameScreen<GameLobbyScreen>();
			break;
		}
		case GameState::MPMatchmaking:
		{
			screenManager->ExitAllScreens();
			screenManager->AddGameScreen<GameLobbyScreen>();
			onlineManager->StartMatchmaking();
			break;
		}
		case GameState::LeavingToMainMenu:
		{
			SwitchToState(GameState::MainMenu);
			break;
		}
		case GameState::JoinGameFromLobby:
		{
			if (g_game->GetLocalPlayerState()->InGame == false && g_game->GetLocalPlayerState()->InLobby == true)
			{
				g_game->GetLocalPlayerState()->InGame = true;
				g_game->GetLocalPlayerState()->InLobby = false;
				onlineManager->SendGameMessage(
					GameMessage(
						GameMessageType::PlayerState,
						g_game->GetLocalPlayerState()->SerializePlayerStateData()
					)
				);
				Managers::Get<OnlineManager>()->SetStartGameCount();
				Managers::Get<GameStateManager>()->SwitchToState(GameState::Gaming);
			}
			break;
		}
		case GameState::Gaming:
		{
			screenManager->ExitAllScreens();
			screenManager->AddGameScreen<GamePlayScreen>();
			screenManager->SetBackgroundsVisible(false);
			g_game->StartGame();
			break;
		}
		default:
			break;
		}
	}
}

void GameStateManager::SwitchToState(GameState state)
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
