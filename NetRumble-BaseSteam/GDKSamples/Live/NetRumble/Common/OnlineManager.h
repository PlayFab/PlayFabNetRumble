#pragma once

#include "Json.h"

namespace NetRumble
{
	class GameMessage;
	using OnlineMessageHandler = std::function<void(uint64_t, GameMessage*)>;

	struct OnlineUser
	{
		std::string Name;
		uint64_t Id;
	};

	struct LeaderboardUser
	{
		int32 GlobakRank;
		std::string Name;
		int32 Score;
	};

	enum class LeaderboardsTypes
	{
		EnteryGameTimesLeaderboard = 0
	};

	enum class EAchievements
	{
		ACH_FIRST_GAME = 0,
		ACH_FIRST_DEATH = 2	// These numbers match our defined achievements in the portal
	};

	struct Achievement_t
	{
		EAchievements m_eAchievementID;
		const char* m_pchAchievementID;
		char m_rgchName[128];
		char m_rgchDescription[256];
		bool m_bAchieved;
		int m_iIconImage;
	};

	class IOnlineManager : public Manager
	{
	public:
		virtual ~IOnlineManager() noexcept = default;

		virtual void StartMatchmaking() = 0;
		virtual bool IsMatchmaking() = 0;
		virtual void CancelMatchmaking() = 0;

		virtual void LeaveMultiplayerGame() = 0;

		virtual bool SendGameMessage(const GameMessage&) = 0;

		virtual bool IsNetworkAvailable() const = 0;

		virtual bool IsServer() const = 0;
		virtual bool IsConnected() const = 0;

		virtual void Tick(float delta) = 0;

		virtual uint64_t GetNetworkId() const = 0;
	};

}
