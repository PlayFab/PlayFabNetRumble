//--------------------------------------------------------------------------------------
// GamePlayScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"
#include "GameStateManager.h"

namespace NetRumble
{

	class GamePlayScreen : public GameScreen
	{
	public:
		GamePlayScreen();
		virtual ~GamePlayScreen();

		virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
		virtual void HandleInput(float elapsedTime) override;
		virtual void Draw(float totalTime, float elapsedTime) override;

		virtual void ExitScreen(bool immediate = false) override;
		void HandleServerConnectionFailureEvent(GameEventMessage message);

	private:
		void DrawHud(float elapsedTime);

		bool m_isFailureEventReceived;
		float m_countdownTimer;
		float m_frameTime;

		std::string m_connectFailInGameMessage;

		std::shared_ptr<DirectX::SpriteFont> m_playerFont;
		std::shared_ptr<DirectX::SpriteFont> m_scoreFont;
	};

}
