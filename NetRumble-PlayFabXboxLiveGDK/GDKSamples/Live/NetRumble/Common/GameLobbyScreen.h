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
		virtual ~GameLobbyScreen() = default;

		virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
		virtual void HandleInput(float elapsedTime) override;
		virtual void Draw(float totalTime, float elapsedTime) override;

	protected:
		virtual void OnCancel() override;

	private:
		const byte IncrementShipColor(byte shipColor);
		void DoGameReadyState(std::shared_ptr<PlayerState> playerState, bool gameReadyState);

		bool m_ready;
		std::string m_strMsg;
		bool m_showMsg;
		bool m_exiting;
		float m_countdownTimer;

		TextureHandle m_inGameTexture;
		TextureHandle m_readyTexture;

		std::string m_connectFailMessage;
	};

}
