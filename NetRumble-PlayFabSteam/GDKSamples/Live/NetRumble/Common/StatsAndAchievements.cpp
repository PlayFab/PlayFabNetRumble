#include "pch.h"

using namespace NetRumble;

#define _ACH_ID( id, name ) { id, #id, name, "", 0, 0 }

namespace {
	constexpr int BufferSize = 128;
	const char* strDeathCount = "stat_1";	// The number of times the player death
	const char* strVictoryCount = "stat_2";	// The number of times the player victories
	const char* strStartGameCount = "stat_3";	// The number of times the player starts
}

Achievement_t g_rgAchievements[] =
{
	_ACH_ID(EAchievements::ACH_FIRST_GAME, "Initial"),
	_ACH_ID(EAchievements::ACH_FIRST_DEATH, "Loser"),
};

std::map< EAchievements, Achievement_t> mAchievement{
	{EAchievements::ACH_FIRST_GAME, g_rgAchievements[0]},
	{EAchievements::ACH_FIRST_DEATH,g_rgAchievements[1]}
};

void StatsAndAchievements::InitializeStatsAndAchievements()
{
	// Only initialized once
	if (m_isInitialized)
	{
		return;
	}
	CSteamID localSteamID = Managers::Get<OnlineManager>()->GetLocalSteamID();
	bool bSuccess = SteamUserStats()->RequestUserStats(localSteamID);
	if (!bSuccess)
	{
		DEBUGLOG("Failed to initialize stats and achievements\n");
	}
	else
	{
		g_game->m_OutputMessage.clear();
		g_game->m_OutputMessage = "User stats have been requested successfully.";
		DEBUGLOG("%s\n", g_game->m_OutputMessage.c_str());
		m_isInitialized = true;
	}
}

void StatsAndAchievements::EvaluateAchievement()
{
	bool bActivation;
	for (auto iter = mAchievement.begin(); iter != mAchievement.end(); ++iter)
	{
		// Already have it
		if (iter->second.m_bAchieved)
		{
			break;
		}

		bActivation = false;

		switch (iter->first)
		{
		case EAchievements::ACH_FIRST_GAME:
			if (m_nStartGameCount == 1)
			{
				bActivation = true;
			}
			break;
		case EAchievements::ACH_FIRST_DEATH:
			if (m_nDeathCount >= 1)
			{
				bActivation = true;
			}
			break;
		default:
			break;
		}

		if (bActivation)
		{
			// Mark it down
			iter->second.m_bAchieved = true;
			SteamUserStats()->SetAchievement(iter->second.m_pchAchievementID);
		}
	}
}

void StatsAndAchievements::StoreStatsIfNecessary()
{
	// Set stats
	SteamUserStats()->SetStat(strDeathCount, m_nDeathCount);
	SteamUserStats()->SetStat(strVictoryCount, m_nVictoryCount);
	SteamUserStats()->SetStat(strStartGameCount, m_nStartGameCount);

	SteamUserStats()->StoreStats();

	EvaluateAchievement();
}

void StatsAndAchievements::SetDeathCount()
{
	m_nDeathCount++;
	StoreStatsIfNecessary();
}

void StatsAndAchievements::SetVictoryCount()
{
	m_nVictoryCount++;
	StoreStatsIfNecessary();
}

void StatsAndAchievements::SetStartGameCount()
{
	m_nStartGameCount++;
	StoreStatsIfNecessary();
}

void StatsAndAchievements::OnUserStatsReceived(UserStatsReceived_t* pCallback)
{
	if (!SteamUserStats())
	{
		return;
	}

	if (SteamUtils()->GetAppID() == pCallback->m_nGameID)
	{
		if (EResult::k_EResultOK == pCallback->m_eResult)
		{
			DEBUGLOG("Success fetching the stats\n");

			// Load achievements
			for (auto iter = mAchievement.begin(); iter != mAchievement.end(); ++iter)
			{
				Achievement_t& ach = iter->second;
				SteamUserStats()->GetAchievement(ach.m_pchAchievementID, &ach.m_bAchieved);
				sprintf_safe(ach.m_rgchName, "%s",
					SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, "name"));
			}

			// Load stats
			SteamUserStats()->GetStat(strDeathCount, &m_nDeathCount);
			SteamUserStats()->GetStat(strVictoryCount, &m_nVictoryCount);
			SteamUserStats()->GetStat(strStartGameCount, &m_nStartGameCount);
		}
		else
		{
			char buffer[BufferSize];
			sprintf_safe(buffer, "Failed to request stats: %d\n", pCallback->m_eResult);
			buffer[sizeof(buffer) - 1] = 0;
			DEBUGLOG(buffer);
		}
	}
}

void StatsAndAchievements::OnUserStatsStored(UserStatsStored_t* pCallback)
{
	if (SteamUtils()->GetAppID() == pCallback->m_nGameID)
	{
		if (k_EResultOK == pCallback->m_eResult)
		{
			DEBUGLOG("UserStatStored success\n");
		}
		else
		{
			char buffer[BufferSize];
			sprintf_safe(buffer, "Failed to store stats: %d\n", pCallback->m_eResult);
			buffer[sizeof(buffer) - 1] = 0;
			DEBUGLOG(buffer);
		}
	}
}

void StatsAndAchievements::OnAchievementStored(UserAchievementStored_t* pCallback)
{
	if (SteamUtils()->GetAppID() == pCallback->m_nGameID)
	{
		if (pCallback->m_nMaxProgress == 0)
		{
			char buffer[BufferSize];
			sprintf_safe(buffer, "Achievement '%s' unlocked!", pCallback->m_rgchAchievementName);
			buffer[sizeof(buffer) - 1] = 0;
			DEBUGLOG(buffer);
		}
		else
		{
			char buffer[BufferSize];
			sprintf_safe(buffer, "Achievement '%s' progress callback, (%d,%d)\n",
				pCallback->m_rgchAchievementName, pCallback->m_nCurProgress, pCallback->m_nMaxProgress);
			buffer[sizeof(buffer) - 1] = 0;
			DEBUGLOG(buffer);
		}
	}
}