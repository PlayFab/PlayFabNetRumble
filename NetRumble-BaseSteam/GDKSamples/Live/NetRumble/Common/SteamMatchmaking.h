#pragma once
namespace NetRumble
{
	class Matchmaking
	{
	public:
		void StartMatchmaking();
		bool IsMatchmaking() { return m_onlineState == OnlineState::Matchmaking; }
		void CancelMatchmaking();
	private:

		enum class OnlineState
		{
			Ready,
			Matchmaking,
			Hosting,
			Joining,
			Canceling,
			InGame
		};

		OnlineState m_onlineState{ OnlineState::Ready };
	};

}

