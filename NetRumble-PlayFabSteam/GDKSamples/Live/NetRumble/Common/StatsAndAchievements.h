//--------------------------------------------------------------------------------------
// StatsAndAchievements.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
	class StatsAndAchievements
	{
	public:
		StatsAndAchievements() = default;
		~StatsAndAchievements() = default;

		void InitializeStatsAndAchievements();

		void SetDeathCount();
		void SetVictoryCount();
		void SetStartGameCount();
		inline int GetStartGameCount() const { return m_nStartGameCount; }
		inline int GetDeathCount() const { return m_nDeathCount; }
		inline int GetVictoryCount() const { return m_nVictoryCount; }

	private:
		int m_nDeathCount;
		int m_nVictoryCount;
		int m_nStartGameCount;
		bool m_isInitialized{ false };

		STEAM_CALLBACK(StatsAndAchievements, OnUserStatsReceived, UserStatsReceived_t);
		STEAM_CALLBACK(StatsAndAchievements, OnUserStatsStored, UserStatsStored_t);
		STEAM_CALLBACK(StatsAndAchievements, OnAchievementStored, UserAchievementStored_t);

		void StoreStatsIfNecessary();
		void EvaluateAchievement();
	};
}
