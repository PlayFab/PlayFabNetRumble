//--------------------------------------------------------------------------------------
// Game.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace NetRumble
{
	const int ERRORMESSAGE_LENGTH = 256;
	const int MAX_ERRORMESSAGE_COUNT = 4;

	class P2PAuthedGame;

	// A basic game implementation that creates a D3D11 device and
	// provides a game loop.
	class Game
	{
	public:
		Game() noexcept = default;
		Game(Game&) = delete;
		Game(Game&&) noexcept = delete;
		Game& operator=(const Game&) = delete;
		Game& operator=(Game&&) = delete;
		~Game();

		// Initialization and management
		void Initialize(HWND window);

		int GetWindowWidth() const { return m_outputWidth; }
		int GetWindowHeight() const { return m_outputHeight; }

		// Basic game loop
		void Tick();

		// Messages
		void OnSuspending();
		void OnResuming();

		void OnWindowMoved() {};
		void OnWindowSizeChanged(int width, int height);

		// Game World
		void StartGame();
		void DrawWorld(float elapsedTime);
		void UpdateWorld(float totalTime, float elapsedTime);
		void ExitGame();
		void CleanupOnlineServices();

		// Game Player Management
		std::shared_ptr<PlayerState> GetPlayerState(const std::string& peer);
		std::shared_ptr<PlayerState> GetLocalPlayerState();
		std::vector<std::shared_ptr<PlayerState>> GetAllPlayerStates();
		void ClearPlayerScores();
		void LocalPlayerInitialize();
		void RemovePlayerFromLobbyPeers(std::string playerId);
		void AddPlayerToLobbyPeers(std::shared_ptr<PlayerState> player);
		void DeleteOtherPlayerInPeersAndExitLobby();
		bool CheckAllPlayerReady();

		inline bool IsGameWon() const { return m_world->IsGameWon; }
		inline std::unique_ptr<World>& GetWorld() { return m_world; }
		inline std::string_view GetWinnerName() const { return m_world->WinnerName; }
		inline std::string_view GetLocalPlayerName() const { return m_localPlayerName; }
		inline std::map<std::string, std::shared_ptr<PlayerState>>& GetPeers() { return m_peers; }
		inline DirectX::XMVECTORF32 GetWinningColor() const { return m_world->WinningColor; }
		template<typename ...Types>
		void WriteDebugLogMessage(const Types& ...args);

		void ResetGameplayData();

		bool m_isLoggedIn{ false };

		// Only output the message to toggle stats overlay
		std::string m_OutputMessage;

		std::vector<std::string> m_DebugLogMessageList;
		char m_DebugLogMessage[ERRORMESSAGE_LENGTH] = {};

	private:
		void CleanupUser();
		void AddHandleToWaitSet(HANDLE handle);
		void WaitForAndCleanupHandles();

		void Update(DX::StepTimer const& timer);
		void Render();

		void PrefetchContent();

		uint32_t m_signInCallbackToken{ 0 };
		uint32_t m_signOutCallbackToken{ 0 };

		std::vector<HANDLE> m_waitHandles;

		// Device resources
		int m_outputWidth{ 0 };
		int m_outputHeight{ 0 };
		std::shared_ptr<DirectX::DescriptorPile> m_descriptors;

		// World
		std::unique_ptr<World> m_world;

		// Players
		std::string m_localPlayerName;
		std::map<std::string, std::shared_ptr<PlayerState>> m_peers;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};

	template<typename ...Types>
	void Game::WriteDebugLogMessage(const Types& ...args)
	{
		sprintf_s(m_DebugLogMessage, ERRORMESSAGE_LENGTH, args...);
		DEBUGLOG(m_DebugLogMessage);
		m_DebugLogMessageList.insert(m_DebugLogMessageList.begin(), m_DebugLogMessage);
		memset(m_DebugLogMessage, '\0', ERRORMESSAGE_LENGTH);

		if (m_DebugLogMessageList.size() > MAX_ERRORMESSAGE_COUNT)
		{
			m_DebugLogMessageList.erase(m_DebugLogMessageList.end() - 1);
		}
	}
}

extern std::unique_ptr<NetRumble::Game> g_game;
