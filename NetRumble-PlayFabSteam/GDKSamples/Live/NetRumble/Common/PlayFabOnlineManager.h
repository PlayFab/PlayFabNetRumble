//--------------------------------------------------------------------------------------
// PlayFabOnlineManager.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "StatsAndAchievements.h"
#include "SteamInventory.h"
#include "SteamLeaderboard.h"

namespace NetRumble
{
	class User;
	class PlayFabParty;
	class PlayFabLobby;
	class PlayFabMatchmaking;

	enum class OnlineState
	{
		Ready,
		Matchmaking,
		Searching,
		Hosting,
		Joining,
		Canceling,
		InGame
	};

	class PlayFabOnlineManager final : public IOnlineManager
	{
	public:
		PlayFabOnlineManager() = default;
		~PlayFabOnlineManager() noexcept = default;
		PlayFabOnlineManager(PlayFabOnlineManager&&) = delete;
		PlayFabOnlineManager(const PlayFabOnlineManager&) = delete;
		PlayFabOnlineManager& operator=(const PlayFabOnlineManager&) = delete;
		PlayFabOnlineManager& operator=(PlayFabOnlineManager&&) = delete;

		// IOnlineManager
		virtual void StartMatchmaking() override;
		virtual bool IsMatchmaking() override;
		virtual void CancelMatchmaking() override;
		// Handling when leaving the game
		virtual void LeaveMultiplayerGame() override;
		// Send in-game messages to other players
		virtual void SendGameMessage(const GameMessage& message) override;
		// Registers a GameMessage message handle
		virtual void RegisterOnlineMessageHandler(OnlineMessageHandler handler) override;
		virtual bool IsNetworkAvailable() const override;
		virtual bool IsConnected() const override;
		virtual void Tick(float delta) override;
		virtual std::string GetNetworkId() const override;

		// Initialize PlayFab Multiplayer
		void InitializeMultiplayer();
		// Uninitialize PlayFab Multiplayer
		void UninitializeMultiplayer();

		// HostMultiplayerGame is the entry point for hosting a joinable game lobby.
		inline void HostMultiplayerGame(bool isPrivateGame = false) { isPrivateGame ? HostMultiplayerGame(PFLobbyAccessPolicy::Private) : HostMultiplayerGame(PFLobbyAccessPolicy::Public); };
		uint64_t GetLocalUserId() const { return m_localUserId; }
		std::string GetLocalEntityId() const { return m_localEntityId; }
		void SetLocalUserId(uint64_t userId) { m_localUserId = userId; }
		void SetLocalEntityId(std::string entityId) { m_localEntityId = entityId; }
		void LoginWithSteam(std::function<void(bool, const std::string&)> callback = nullptr);
		void LoginWithCustomID(std::function<void(bool, const std::string&)> callback = nullptr);
		const PlayFabLoginType& GetLoginType() { return m_playfabLogin.GetLoginType(); }
		OnlineMessageHandler& GetOnlineMessageHandler() { return m_messageHandler; }
		void JoinMultiplayerGame(const std::string& connectionString);
		// Checks for any incoming network data, then dispatches it
		void ProcessGameNetworkMessage(std::string sourceId, GameMessage* message);
		inline void SwitchToOnlineState(OnlineState toState) { m_onlineState = toState; }
		inline void SetPartyLocalEntityId(std::string& entityId) { m_playfabParty.SetPartyLocalEntityId(entityId); }
		inline void SetPartyLocalEntityToken(std::string& entityId) { m_playfabParty.SetPartyLocalEntityToken(entityId); }
		inline void SetPartyEntityTokenExpireTime(time_t expireTime) { m_playfabParty.SetPartyEntityTokenExpireTime(expireTime); }
		void PlayfabPartyDoWork() { m_playfabParty.DoWork(); }
		void InitializePlayfabParty() { m_playfabParty.Initialize(); }
		void PopulatePartyRegionLatencies(bool send = true) { m_playfabParty.PopulatePartyRegionLatencies(); }
		bool IsHost() const { return m_playfabParty.IsHost(); }
		void SetHost(bool isHost) { m_playfabParty.SetHost(isHost); }
		bool IsPartyInitialized() const { return m_playfabParty.IsPartyInitialized(); }
		const char* GetLocalUserEntityId() const { return m_playfabParty.GetLocalUserEntityId(); }
		inline void FindLobbies() { m_pfLobby.FindLobbies(); }
		void UpdateLobbyState(const PFLobbyAccessPolicy accessPolicy) { m_pfLobby.UpdateLobbyState(accessPolicy); }
		void SetFindLobbyCallback(FindLobbyCallback completionCallback) { m_pfLobby.SetFindLobbyCallback(completionCallback); }
		void GetAllItems() { m_inventory.GetAllItems(); }
		inline const PlayFab::ClientModels::EntityKey& GetEntityKey() { return m_playfabLogin.GetEntityKey(); }
		bool IsJoiningArrangedLobby() { return m_isJoiningArrangedLobby; }
		// Leaves and shuts down PlayFab Party
		void CleanupOnlineServices();
		// Cleans up manager and its components
		void Cleanup();

		// Invite Firend
		STEAM_CALLBACK(PlayFabOnlineManager, OnInviteUserToGame, GameRichPresenceJoinRequested_t);
		void JoinGameFromInvite(const std::string& connectionString);
		bool JoinPendingInviteSession();
		bool ParseInviteParam(std::string& strParam);
		bool IsJoiningFromInvite() const { return m_pfLobby.IsJoiningFromInvite(); }
		bool HasPendingInviteSession() const { return m_pfLobby.HasPendingInviteSession(); }
		bool InviteSteamFriend(CSteamID steamID) { return m_pfLobby.InviteSteamFriend(steamID); }
		void SetHasPendingInvite(bool bHasPendingSession) { m_pfLobby.SetHasPendingInvite(bHasPendingSession); }

		// Steam Leaderboard
		void SetFindLeaderboardCallback(FindLeaderboardCallback completionCallback) { m_leaderboard.SetFindLeaderboardCallback(completionCallback); }
		void GetLeaderboards(LeaderboardsTypes typeOfLeaderboard) { m_leaderboard.GetLeaderboards(typeOfLeaderboard); }

		// Steam StatsAndAchievements
		void SetDeathCount() { m_statsAndAchievements.SetDeathCount(); }
		void SetVictoryCount() { m_statsAndAchievements.SetVictoryCount(); }
		void SetStartGameCount() { m_statsAndAchievements.SetStartGameCount(); }
		void InitializeStatsAndAchievements() { m_statsAndAchievements.InitializeStatsAndAchievements(); }
		int GetDeathCount() const { return m_statsAndAchievements.GetDeathCount(); }
		int GetVictoryCount() const { return m_statsAndAchievements.GetVictoryCount(); }
		int GetStartGameCount() const { return m_statsAndAchievements.GetStartGameCount(); }

		// Get the Steam id for the local user at this client
		inline CSteamID GetLocalSteamID() const { return m_localPlayerSteamID; }
		inline void SetLocalSteamID(CSteamID playerID) { m_localPlayerSteamID = playerID; };

	private:
		friend PlayFabLobby;
		friend PlayFabMatchmaking;
		friend PlayFabParty;

		// Steam
		Inventory m_inventory;
		Leaderboard m_leaderboard;
		CSteamID m_localPlayerSteamID;
		StatsAndAchievements m_statsAndAchievements;

		// PlayFab
		PlayFabLobby m_pfLobby{};
		PlayFabLogin m_playfabLogin{};
		PlayFabParty m_playfabParty{};
		PlayFabMatchmaking m_pfMatchmaking{};
		PFMultiplayerHandle m_pfMultiplayerHandle{ nullptr };

		inline void LeaveLobby() { m_pfLobby.LeaveLobby(); }
		inline void JoinLobby(const std::string& lobbyId) { m_pfLobby.JoinLobby(lobbyId); }
		inline void CreateLobby(PFLobbyAccessPolicy accessPolicy) { m_pfLobby.CreateLobby(accessPolicy); }
		inline OnlineState GetOnlineState() const { return m_onlineState; }
		inline std::string GetDescriptor()const { return m_networkDescriptor; }
		inline std::string GetConnectionString() const { return m_connectionString; }

		void CreateLobby();
		void MultiplayerTick();
		void CreatePlayFabParty();
		void MigrateToNewNetwork();
		void FindAndAddRemoteUsers();
		void FindAndRemoveRemoteUsers();
		void MultiplayerSDKInitialize();
		void MultiplayerSDKUninitialize();
		void InviteFinished() { m_pfLobby.InviteFinished(); }
		void HostMultiplayerGame(PFLobbyAccessPolicy accessPolicy);
		void SetSessionProperty(const char* name, const char* value);
		void SwitchToJoiningFromInvite() { m_pfLobby.SwitchToJoiningFromInvite(); }
		void FindAndConnectToNetwork(std::string_view networkId, std::string_view descriptor);
		void SetConnectionString(std::string strConnectString) { m_connectionString = strConnectString; };
		bool ResetNetwork();

		PFMultiplayerHandle& GetMultiplayerHandle() { return m_pfMultiplayerHandle; }
		void OnPFPartyNetworkCreated(const std::string& descriptor);

		uint64_t m_localUserId{ 0 };
		std::string m_networkId;
		std::string m_localEntityId;
		std::string m_connectionString;
		std::string m_networkDescriptor;
		OnlineMessageHandler m_messageHandler{};
		bool m_isJoiningArrangedLobby{ false };
		OnlineState m_onlineState{ OnlineState::Ready };
	};

	extern const char* MessageTypeString(GameMessageType type);
	using OnlineManager = PlayFabOnlineManager;
}
