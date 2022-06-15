#pragma once

namespace NetRumble
{
	class StatsAndAchievements
	{
	public:
		StatsAndAchievements();
		~StatsAndAchievements();

		void InitializeStatsAndAchievements();

		void SetDeathCount();
		void SetVictoryCount();
		void SetStartGameCount();
		inline int GetStartGameCount() { return m_nStartGameCount; }
		inline int GetDeathCount() { return m_nDeathCount; }
		inline int GetVictoryCount() { return m_nVictoryCount; }

	private:
		int m_nDeathCount;
		int m_nVictoryCount;
		int m_nStartGameCount;

		STEAM_CALLBACK(StatsAndAchievements, OnUserStatsReceived, UserStatsReceived_t);
		STEAM_CALLBACK(StatsAndAchievements, OnUserStatsStored, UserStatsStored_t);
		STEAM_CALLBACK(StatsAndAchievements, OnAchievementStored, UserAchievementStored_t);
		
		void StoreStatsIfNecessary();
		void EvaluateAchievement();
	};
}
