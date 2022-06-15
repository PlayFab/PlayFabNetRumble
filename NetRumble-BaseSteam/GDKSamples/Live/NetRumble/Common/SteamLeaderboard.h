#pragma once

namespace NetRumble
{
	static const int MaxLeaderboardEntries = 100;
	static const char* KeyEntryGameTimes = "Entry Game Times";

	using FindLeaderboardCallback = std::function<void(std::vector<std::shared_ptr<LeaderboardUser>>)>;

	class Leaderboard
	{
	public:
		Leaderboard();
		~Leaderboard();

		void SetFindLeaderboardCallback(FindLeaderboardCallback completionCallback);
		void GetLeaderboards(LeaderboardsTypes typeOfLeaderboard);
		
		void UpdateLeaderboards(LeaderboardsTypes typeOfLeaderboard);

	private:
		int m_nLeaderboardEntries;
		LeaderboardEntry_t m_leaderboardEntries[MaxLeaderboardEntries];

		SteamLeaderboard_t m_hEntryGameTimesLeaderboard;

		void FindLeaderboards();
		void OnFindLeaderboard(LeaderboardFindResult_t* pFindLearderboardResult, bool bIOFailure);
		CCallResult<Leaderboard, LeaderboardFindResult_t> m_SteamCallResultCreateLeaderboard;
		void ShowLeaderboard(SteamLeaderboard_t hLeaderboard, ELeaderboardDataRequest eLeaderboardData, int offset);

		void OnUploadScore(LeaderboardScoreUploaded_t* pFindLearderboardResult, bool bIOFailure);
		CCallResult<Leaderboard, LeaderboardScoreUploaded_t> m_SteamCallResultUploadScore;

		FindLeaderboardCallback m_FindLeaderboardCallback;
		CCallResult<Leaderboard, LeaderboardScoresDownloaded_t> m_callResultDownloadEntries;
		void OnLeaderboardDownloadedEntries(LeaderboardScoresDownloaded_t* pLeaderboardScoresDownloaded, bool bIOFailure);

	};
}
