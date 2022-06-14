//--------------------------------------------------------------------------------------
// GameStateManager.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace NetRumble
{
	class PlayerState;
	class World;


	enum class GameState : int
	{
		Initialize,
		StartMenu,
		Login,
		MainMenu,
		MPHostGame,
		MPJoinLobby,
		MPMatchmaking,
		Lobby,
		MatchingGame,
		GameConnecting,
		JoinGameFromLobby,
		EvaluatingNetwork,
		Gaming,
		GameConnectFail,
		LeavingToMainMenu,
		MigratingNetwork,
		InternetConnectivityError,
		NetworkNoLongerExists,
		Invalid
	};

	class GameStateManager : public Manager
	{
	public:
		GameStateManager();

		void Update();

		void SwitchToState(GameState state);
		inline GameState GetState() const { return _state; }

	private:
		GameState _state;
		GameState _nextState;
		bool _immutableState;
	};
}
