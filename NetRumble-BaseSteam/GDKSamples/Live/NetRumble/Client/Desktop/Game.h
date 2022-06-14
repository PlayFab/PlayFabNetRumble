
#pragma once

#include "pch.h"

namespace NetRumble
{
	class P2PAuthedGame;

	// A basic game implementation that creates a D3D11 device and
	// provides a game loop.
	class Game
	{
	public:
		Game() noexcept;
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
		void OnActivated();
		void OnDeactivated();
		void OnSuspending();
		void OnResuming();

		void OnWindowMoved() {};
		void OnWindowSizeChanged(int width, int height);

		// Game World
		void StartGame();
		void ServerPrepareGameEnviroment();
		void DrawWorld(float elapsedTime);
		void UpdateWorld(float totalTime, float elapsedTime);

		// Game Player Management
		std::shared_ptr<PlayerState> GetPlayerState(uint64_t peer);
		std::shared_ptr<PlayerState> GetLocalPlayerState();
		void SetPlayerState(uint64_t id, std::shared_ptr<PlayerState> state);
		std::vector<std::shared_ptr<PlayerState>> GetAllPlayerStates();
		void ClearPlayerScores();
		bool RemovePlayerFromLobbyPeers(uint64 playerId);
		bool RemovePlayerFromGamePeers(uint64 playerId);
		bool RemovePlayerFromGamePeers(std::shared_ptr<PlayerState> player);
		void AddPlayerToLobbyPeers(uint64 PlayerId);
		void AddPlayerToLobbyPeers(std::shared_ptr<PlayerState> player);
		void DeleteOtherPlayerInPeersAndExitLobby();
		void RemovePeers(uint64 playerId);

		inline std::string_view GetLocalPlayerName() { return m_localPlayerName; }
		inline bool IsGameWon() { return m_world->IsGameWon; }

		void ResetGameplayData();

		inline std::string_view GetWinnerName() const { return m_world->WinnerName; }
		inline DirectX::XMVECTORF32 GetWinningColor() const { return m_world->WinningColor; }

		inline std::unique_ptr<World>& GetWorld() { return m_world; }
		inline std::map<uint64_t, std::shared_ptr<PlayerState>>& GetPeers() { return m_peers; }

		const uint64 GetGameTickCount() const { return m_timer.GetTotalTicks(); }

		// Game server is created by a local client and all the players who join this server will be connected via Steam P2P network
		// since we don't need a dedicated server and the max players number per server (2-16) is relatively small
		// If current client is the host of the game server, m_server will not be nullptr
		std::unique_ptr<NetRumbleServer>& GetGameServer() { return m_server; };
		inline bool IsHost() const { return m_server != nullptr; }

		// Only output the message to toggle stats overlay
		std::string m_OutputMessage;

	private:
		void CleanupUser();
		void LocalPlayerInitialize();

		void AddHandleToWaitSet(HANDLE handle);
		void WaitForAndCleanupHandles();

		// Checks for any incoming network data, then dispatches it
		void ProcessGameNetworkMessage(uint64_t steamID, GameMessage* message);

		void Update(DX::StepTimer const& timer);
		void Render();

		void PrefetchContent();

		uint32_t m_signInCallbackToken{ 0 };
		uint32_t m_signOutCallbackToken{ 0 };

		std::vector<HANDLE>                             m_waitHandles;

		// Device resources
		int                                             m_outputWidth{ 0 };
		int                                             m_outputHeight{ 0 };
		std::shared_ptr<DirectX::DescriptorPile>        m_descriptors;

		// World
		std::unique_ptr<World> m_world;

		// Players
		std::string m_localPlayerName;
		std::map<uint64_t, std::shared_ptr<PlayerState>> m_peers;

		// Server info
		// Server we create as the host, will be nullptr if the server was not created by us
		// For all the users connect to a valid server the member variable 
		std::unique_ptr<NetRumbleServer> m_server;

		// List of steamIDs for each player
		CSteamID m_SteamIDPlayers[MAX_PLAYERS_PER_SERVER];

		// Time the last state transition occurred (so we can count-down round restarts)
		uint64 m_stateTransitionTime;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}

extern std::unique_ptr<NetRumble::Game> g_game;
