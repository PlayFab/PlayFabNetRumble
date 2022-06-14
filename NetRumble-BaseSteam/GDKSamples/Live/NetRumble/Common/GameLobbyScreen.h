//--------------------------------------------------------------------------------------
// GameLobbyScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace NetRumble
{

	class GameLobbyScreen : public MenuScreen
	{
	public:
		GameLobbyScreen() noexcept;
		virtual ~GameLobbyScreen();

		virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
		virtual void HandleInput(float elapsedTime) override;
		virtual void Draw(float totalTime, float elapsedTime) override;
		const byte IncrementShipColor(byte shipColor);

		void SetServerConnectionEvent(GameEventMessage message);
		void SetSteamConnectionEvent(GameEventMessage message);
	protected:
		virtual void OnCancel() override;

	private:
		bool m_ready;
		bool m_exiting;
		float m_countdownTimer;

		GameState m_lastState;
		TextureHandle m_inGameTexture;
		TextureHandle m_readyTexture;

		std::string m_connectFailMessage;
	};

}
