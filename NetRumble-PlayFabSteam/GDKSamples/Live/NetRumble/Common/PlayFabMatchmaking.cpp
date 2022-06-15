//--------------------------------------------------------------------------------------
// PlayFabMatchmaking.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

namespace
{
	static GameMessage OnlineDisconnect(GameMessageType::OnlineDisconnect, 0);
	static GameMessage MatchmakingFailed(GameMessageType::MatchmakingFailed, 0);
	static GameMessage MatchmakingCanceled(GameMessageType::MatchmakingCanceled, 0);
	constexpr std::string_view DEFAULT_QUEUE{ "Default" };      // This is the name of the queue configured in Game Manager
	constexpr uint32_t MATCHMAKING_TIMEOUT_IN_SECONDS{ 60 };     // The timeout for attempting to find a match, in seconds
	constexpr uint32_t COUNT_OF_LOCAL_USERS{ 1 };
	constexpr uint32_t COUNT_OF_OTHER_MEMBER_TO_MATCH_WITH{ 0 }; // The number of other users expected to join the ticket
}

extern const char* GetPlayFabErrorMessage(HRESULT errorCode);

void PlayFabMatchmaking::DoWork()
{
	uint32_t stateChangeCount;
	const PFMatchmakingStateChange* const* stateChanges;
	const OnlineManager* onlineManager = Managers::Get<OnlineManager>();
	if (!onlineManager)
	{
		DEBUGLOG("Failed to get OnlineManager");
		return;
	}

	PFMultiplayerHandle pfMultiplayerHandle = onlineManager->m_pfMultiplayerHandle;

	HRESULT hr = PFMultiplayerStartProcessingMatchmakingStateChanges(pfMultiplayerHandle, &stateChangeCount, &stateChanges);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to start processing matchmaking state changes: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		return;
	}

	const PFMatchmakingStateChange* stateChange{ nullptr };
	for (uint32_t i = 0; i < stateChangeCount; ++i)
	{
		stateChange = stateChanges[i];
		if (!stateChange)
		{
			continue;
		}

		switch (stateChange->stateChangeType)
		{
		case PFMatchmakingStateChangeType::TicketStatusChanged: OnTicketStatusChanged(stateChange); break;
		case PFMatchmakingStateChangeType::TicketCompleted:OnTicketCompleted(stateChange); break;
		default:
			break;
		}
	}

	hr = PFMultiplayerFinishProcessingMatchmakingStateChanges(pfMultiplayerHandle, stateChangeCount, stateChanges);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to finish processing matchmaking state changes: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		return;
	}
}

bool PlayFabMatchmaking::StartMatchmaking()
{
	return CreateMatchmakingTicket();
}

bool PlayFabMatchmaking::CreateMatchmakingTicket()
{
	PFMatchmakingTicketConfiguration configuration{};
	configuration.timeoutInSeconds = MATCHMAKING_TIMEOUT_IN_SECONDS;
	configuration.queueName = DEFAULT_QUEUE.data(); // The ID of a match queue
	configuration.membersToMatchWithCount = COUNT_OF_OTHER_MEMBER_TO_MATCH_WITH;

	PFMatchmakingTicketHandle ticketHandle{};
	const char* localUserAttributes = "";
	// Start matchmaking for a single local user
	HRESULT hr = PFMultiplayerCreateMatchmakingTicket(
		Managers::Get<OnlineManager>()->m_pfMultiplayerHandle, // The handle of the PFMultiplayer API instance
		COUNT_OF_LOCAL_USERS,                                  // The count of local users to include in the ticket
		&Managers::Get<OnlineManager>()->m_playfabLogin.GetPFLoginEntityKey(),     // The array of local users to include in the ticket
		&localUserAttributes,                                  // The array of local user attribute strings
		&configuration,                                        // The ticket configuration
		nullptr,                                               // An optional asyncContext
		&ticketHandle);                                        // The resulting ticket object

	m_curTicketHandle = ticketHandle;
	if (FAILED(hr))
	{
		g_game->WriteDebugLogMessage("Failed to create matchmaking ticket: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		auto onlineManager = Managers::Get<OnlineManager>();
		onlineManager->m_messageHandler(onlineManager->GetLocalEntityId(), &MatchmakingFailed);
		return false;
	}

	return true;
}

void PlayFabMatchmaking::CancelMatchmaking()
{
	if (!m_curTicketHandle)
	{
		DEBUGLOG("Failed to cancel matchmaking, current ticket handle is nullptr");
		return;
	}

	HRESULT hr = PFMatchmakingTicketCancel(m_curTicketHandle);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to cancel matchmaking ticket: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
	}
}

bool PlayFabMatchmaking::IsMatchmaking()
{
	return Managers::Get<GameStateManager>()->GetState() == GameState::MPMatchmaking;
}

void PlayFabMatchmaking::OnTicketStatusChanged(const PFMatchmakingStateChange* change)
{
	const auto ticketStatusChanged = static_cast<const PFMatchmakingTicketStatusChangedStateChange*>(change);

	// Retrieve the new ticket status
	PFMatchmakingTicketStatus status{};
	HRESULT hr = PFMatchmakingTicketGetStatus(ticketStatusChanged->ticket, &status);
	if (FAILED(hr))
	{
		g_game->WriteDebugLogMessage("Failed to get matchmaking status: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		return;
	}

	DEBUGLOG("Ticket status is: %u.\n", status);
	switch (status)
	{
	case PFMatchmakingTicketStatus::Creating:
	{
		break;
	}
	case PFMatchmakingTicketStatus::Joining:
	{
		break;
	}
	case PFMatchmakingTicketStatus::WaitingForPlayers:
	{
		break;
	}
	case PFMatchmakingTicketStatus::WaitingForMatch:
	{
		break;
	}
	case PFMatchmakingTicketStatus::Matched:
	{
		if (!m_curMatch)
		{
			hr = PFMatchmakingTicketGetMatch(ticketStatusChanged->ticket, &m_curMatch);
			if (FAILED(hr))
			{
				DEBUGLOG("Failed to get match details from ticket: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
				Managers::Get<OnlineManager>()->m_messageHandler(Managers::Get<OnlineManager>()->GetLocalEntityId(), &MatchmakingFailed);
				return;
			}
		}
		Managers::Get<OnlineManager>()->m_pfLobby.m_MatchmakingMemberCount = m_curMatch->memberCount;
		Managers::Get<OnlineManager>()->m_pfLobby.JoinArrangedLobby(m_curMatch->lobbyArrangementString);
		Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Joining);
		break;
	}
	case PFMatchmakingTicketStatus::Canceled:
	{
		break;
	}
	case PFMatchmakingTicketStatus::Failed:
	{
		g_game->WriteDebugLogMessage("The matchmaking ticket failed to find a match.");
		break;
	}
	default:
		break;
	}
}

void PlayFabMatchmaking::OnTicketCompleted(const PFMatchmakingStateChange* change)
{
	const OnlineManager* onlineManager = Managers::Get<OnlineManager>();
	OnlineMessageHandler pfMessageHandle = onlineManager->m_messageHandler;
	std::string curUserId = onlineManager->GetLocalEntityId();
	const auto ticketCompleted = static_cast<const PFMatchmakingTicketCompletedStateChange*>(change);

	HRESULT ticketResult = ticketCompleted->result;
	DEBUGLOG("PFMatchmaking completed with Result 0x%08X.\n", ticketResult);
	if (FAILED(ticketResult))
	{
		// Return the state change(s), bail out if we detected ticket failure.
		DEBUGLOG("Matchmaking ticket failure detected: 0x%08X %s\n", static_cast<unsigned int>(ticketResult), GetPlayFabErrorMessage(ticketResult));
		pfMessageHandle(curUserId, &MatchmakingFailed);
		return;
	}

	HRESULT hr = PFMatchmakingTicketGetMatch(ticketCompleted->ticket, &m_curMatch);
	if (FAILED(hr))
	{
		DEBUGLOG("Failed to get match details from ticket: 0x%08X %s\n", static_cast<unsigned int>(hr), GetPlayFabErrorMessage(hr));
		pfMessageHandle(curUserId, &MatchmakingFailed);
		return;
	}
}
