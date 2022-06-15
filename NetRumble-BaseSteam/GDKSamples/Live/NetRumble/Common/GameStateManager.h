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
		Invalid
	};

	class GameStateManager : public Manager
	{
	public:
		GameStateManager();

		void Update();

		void SwitchToState(GameState state);
		inline GameState GetState() const { return _state; }

		inline bool IsStateImmutable() { return _immutableState; }
		inline void SetImmutableState() { _immutableState = true; }
		inline void CancelImmutableState() { _immutableState = false; }

	private:
		GameState _state;
		GameState _nextState;
		bool _immutableState;
	};
}
