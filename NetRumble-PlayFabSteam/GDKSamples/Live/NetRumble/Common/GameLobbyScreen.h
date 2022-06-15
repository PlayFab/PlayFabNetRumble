//--------------------------------------------------------------------------------------
// GameLobbyScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace NetRumble
{
	static const int TIME_ERROR_SHOW = 2000; // A duration for error message in milliseconds
	static const int INDEX_MENU_INVITE_USER = 1; // The default value for inviting players
	static const int INDEX_MENU_DEFAULT = 0; // Default index value
	static const int MAXCOUNT_SHOW_ONLINE_USER = 5; // Maximum number of players can be displayed
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
		struct SteamFriend
		{
			CSteamID id;
			std::string name;
		};

		void DoInviteFriend();
		void DoGameReadyState(std::shared_ptr<PlayerState> playerState, bool gameReadyState);
		void ShowMsg(const char* msg, ULONGLONG nTime);
		const byte IncrementShipColor(byte shipColor);

		bool m_ready;
		std::string m_strMsg;
		bool m_showMsg;
		bool m_invitingFriend;
		bool m_exiting;
		float m_countdownTimer;

		GameState m_lastState;
		TextureHandle m_inGameTexture;
		TextureHandle m_readyTexture;

		std::string m_connectFailMessage;
	};

}
