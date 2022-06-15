//--------------------------------------------------------------------------------------
// PlayFabMatchmaking.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
	class PlayFabMatchmaking
	{
	public:
		enum class MatchmakingState
		{
			StartMatchmaking,
			CancelMatchmaking,
		};

		PlayFabMatchmaking() = default;
		~PlayFabMatchmaking() = default;
		PlayFabMatchmaking(const PlayFabMatchmaking&) = delete;
		PlayFabMatchmaking(PlayFabMatchmaking&&) = delete;
		PlayFabMatchmaking& operator=(const PlayFabMatchmaking&) = delete;
		PlayFabMatchmaking& operator=(PlayFabMatchmaking&&) = delete;

		void CancelMatchmaking();
		void DoWork();
		bool IsMatchmaking();
		bool StartMatchmaking();

	private:
		// PFMatchmakingStateChange Functions
		// A matchmaking ticket status has changed.
		void OnTicketStatusChanged(const PFMatchmakingStateChange* change);
		// A matchmaking ticket has completed.
		void OnTicketCompleted(const PFMatchmakingStateChange* change);

		bool CreateMatchmakingTicket();

		PFMatchmakingTicketHandle m_curTicketHandle{};
		const PFMatchmakingMatchDetails* m_curMatch{};
		PFMatchmakingTicketStatus m_curMatchmakingTicketStatus{ PFMatchmakingTicketStatus::Failed };
	};
}
