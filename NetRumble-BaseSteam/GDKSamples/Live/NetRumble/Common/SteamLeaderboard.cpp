//--------------------------------------------------------------------------------------
// SteamLeaderboard.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

NetRumble::Leaderboard::Leaderboard()
	:m_hEntryGameTimesLeaderboard{ 0 }
{
	FindLeaderboards();
}

NetRumble::Leaderboard::~Leaderboard()
{
}

void NetRumble::Leaderboard::ShowLeaderboard(SteamLeaderboard_t hLeaderboard, ELeaderboardDataRequest eLeaderboardData, int offset)
{
	if(hLeaderboard)
	{
		SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(hLeaderboard, eLeaderboardData,
			offset, offset + MaxLeaderboardEntries);

		m_callResultDownloadEntries.Set(hSteamAPICall, this, &Leaderboard::OnLeaderboardDownloadedEntries);
	}
}

void NetRumble::Leaderboard::GetLeaderboards(LeaderboardsTypes typeOfLeaderboard)
{
	UNREFERENCED_PARAMETER(typeOfLeaderboard);

	if (m_hEntryGameTimesLeaderboard != 0)
	{
		ShowLeaderboard(m_hEntryGameTimesLeaderboard, k_ELeaderboardDataRequestGlobal, 0);
	}
}

void NetRumble::Leaderboard::FindLeaderboards()
{
	SteamAPICall_t hSteamAPICall = 0;

	if (!m_hEntryGameTimesLeaderboard)
	{
		hSteamAPICall = SteamUserStats()->FindOrCreateLeaderboard(KeyEntryGameTimes,
			k_ELeaderboardSortMethodAscending, k_ELeaderboardDisplayTypeNumeric);
	}

	if (hSteamAPICall != 0)
	{
		m_SteamCallResultCreateLeaderboard.Set(hSteamAPICall, this, &Leaderboard::OnFindLeaderboard);
	}
}

void NetRumble::Leaderboard::UpdateLeaderboards(LeaderboardsTypes typeOfLeaderboard)
{
	if (typeOfLeaderboard == LeaderboardsTypes::EnteryGameTimesLeaderboard && m_hEntryGameTimesLeaderboard)
	{
		int startGameCounts = Managers::Get<SteamOnlineManager>()->GetStartGameCount();
		SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore(m_hEntryGameTimesLeaderboard, k_ELeaderboardUploadScoreMethodKeepBest, startGameCounts, nullptr, 0);
		m_SteamCallResultUploadScore.Set(hSteamAPICall, this, &Leaderboard::OnUploadScore);
	}
}

void NetRumble::Leaderboard::OnUploadScore(LeaderboardScoreUploaded_t* pFindLearderboardResult, bool bIOFailure)
{
	if (!pFindLearderboardResult->m_bSuccess || bIOFailure)
	{
		DEBUGLOG("Score could not be uploaded to Steam\n");
	}

	// TODO: We can do something when we upload the score successfully
}

void NetRumble::Leaderboard::OnLeaderboardDownloadedEntries(LeaderboardScoresDownloaded_t* pLeaderboardScoresDownloaded, bool bIOFailure)
{
	UNREFERENCED_PARAMETER(bIOFailure);

	bool isGetLeaderboardData = false;

	m_nLeaderboardEntries = pLeaderboardScoresDownloaded->m_cEntryCount < MaxLeaderboardEntries ? pLeaderboardScoresDownloaded->m_cEntryCount : MaxLeaderboardEntries;
	for (int index = 0; index < m_nLeaderboardEntries; index++)
	{
		isGetLeaderboardData = SteamUserStats()->GetDownloadedLeaderboardEntry(pLeaderboardScoresDownloaded->m_hSteamLeaderboardEntries,
			index, &m_leaderboardEntries[index], nullptr, 0);
		if (!isGetLeaderboardData)
		{
			break;
		}
	}

	if (isGetLeaderboardData)
	{
		std::vector<std::shared_ptr<LeaderboardUser>> users;
		for (int index = 0; index < m_nLeaderboardEntries; index++)
		{
			std::shared_ptr<LeaderboardUser> user = std::make_shared<LeaderboardUser>();
			std::string userName = SteamFriends()->GetFriendPersonaName(m_leaderboardEntries[index].m_steamIDUser);
			user->GlobakRank = m_leaderboardEntries[index].m_nGlobalRank;
			user->Name = userName;
			user->Score = m_leaderboardEntries[index].m_nScore;
			users.push_back(user);
		}

		m_FindLeaderboardCallback(users);
	}
}

void NetRumble::Leaderboard::SetFindLeaderboardCallback(FindLeaderboardCallback completionCallback)
{
	m_FindLeaderboardCallback = completionCallback;
}

void NetRumble::Leaderboard::OnFindLeaderboard(LeaderboardFindResult_t* pFindLearderboardResult, bool bIOFailure)
{
	if (!pFindLearderboardResult->m_bLeaderboardFound || bIOFailure)
	{
		return;
	}

	const char* pchName = SteamUserStats()->GetLeaderboardName(pFindLearderboardResult->m_hSteamLeaderboard);
	if (strcmp(pchName, KeyEntryGameTimes) == 0)
	{
		m_hEntryGameTimesLeaderboard = pFindLearderboardResult->m_hSteamLeaderboard;
	}
}
