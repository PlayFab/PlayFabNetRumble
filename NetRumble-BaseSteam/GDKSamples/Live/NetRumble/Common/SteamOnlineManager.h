#pragma once

#include "pch.h"
#include "NetRumbleServer.h"
#include "NetworkMessages.h"
#include "StatsAndAchievements.h"
#include "SteamLobby.h"
#include "SteamInventory.h"
#include "SteamMatchmaking.h"
#include "SteamLeaderboard.h"

namespace NetRumble
{
	// This class manages steam Lobby, Matchmaking, and Network Message Processing methods
	class SteamOnlineManager final : public IOnlineManager
	{
	public:
		SteamOnlineManager();
		virtual ~SteamOnlineManager() override;

		virtual void Tick(float elapsedTime) override;

		virtual bool IsNetworkAvailable() const override;

		virtual void LeaveMultiplayerGame() override;

		// Client message
		// All the game play message will be send this way
		virtual bool SendGameMessage(const GameMessage& message) override;
		// This is used for client message that will be dispatched by game server
		virtual bool SendGameMessageWithSourceID(const GameMessage& message);
		// Steam login and authentication message will be sent this way, most of these type of message should be reliable
		bool SendGameMessage(const void* msg, const uint32 msgSize, int sendFlag);
		void ClientProcessNetworkMessage();

		// Server message (local player is the host of the game server)
		// Send the same message to all clients, except the ignored connection if any
		// Some dispatched message from client will require the original sender ID, in such case the last bool should be set to true
		bool ServerSendMessageToAll(const GameMessage& message, bool serializeWithSourceID = false, int sendFlags = k_nSteamNetworkingSend_UnreliableNoDelay) const;
		bool ServerSendMessageToAllIgnore(const GameMessage& message, std::set<HSteamNetConnection>& ingoreConnectionSet, bool serializeWithSourceID = false, int sendFlags = k_nSteamNetworkingSend_UnreliableNoDelay) const;
		void ServerProcessNetworkMessage();

		virtual bool IsConnected() const override;

		virtual uint64_t GetNetworkId() const override;

		// Get the steam id for the local user at this client
		inline CSteamID GetLocalSteamID() const { return m_localPlayerSteamID; }
		inline void SetLocalSteamID(CSteamID playerID) { m_localPlayerSteamID = playerID; };

		inline const HSteamNetConnection GetConnectedServerHandle() const { return m_connectedServerHandle; }
		inline const ClientConnectionState GetConnectedState() const { return m_connectedState; }

		// Connect to a server at a given IP address or game server steamID
		void InitiateServerConnection(CSteamID steamIDGameServer);

		// Receive a response from the server for a connection attempt
		void OnReceiveServerAuthenticationResponse(bool success, uint32 playerPosition);

		void SetLobbyGameServerAndConnect();

		// Receive a response from the server for a connection attempt
		void OnReceiveServerInfo(CSteamID steamIDGameServer, bool bVACSecure, const char* pchServerName);

		uint64 GetLastServerUpdateTick() const;
		void SetLastServerUpdateTick(uint64 newTick);

		// Handle the server exiting
		void OnReceiveServerExiting();

		void CreateGameServer();

		// Received a response that the server is full
		void OnReceiveServerFullResponse();

		// Disconnects from a server (telling it so) if we are connected
		void DisconnectFromServer();

		// Updates what we show to friends about what we're doing and how to connect
		void UpdateRichPresenceConnectionInfo();

		void InviteFriendsIntoLobby();

		void AcceptInvitationWhenGameNotStart(const char* pLobbyID);

		void DispatchMessageInLobby(std::string& message);

		std::vector<uint8_t> SerializePlayerDisconnect(CSteamID steamID ) const;
		CSteamID DeserializePlayerDisconnect(const std::vector<uint8_t>& data);

		// StatsAndAchievements
		void InitializeStatsAndAchievements();
		void SetDeathCount();
		void SetVictoryCount();
		void SetStartGameCount();
		int GetStartGameCount();
		int GetDeathCount();
		int GetVictoryCount();

		// Lobby
		void CreateLobby();
		void FindLobbies();
		void JoinLobby(CSteamID steamLobbyID);
		void LeaveLobby();

		void SetReadyStateInLobby(bool readyState);
		void SetShipColorInLobby(byte shipColor);
		void SetShipVariationInLobby(byte shipVariation);

		void PreparingGameServer();
		void SetFindLobbyCallback(FindLobbyCallback completionCallback);
		void SetIsServer(bool isServer);
		virtual bool IsServer() const override;
		const CSteamID GetSteamIDLobby();

		// Steam Inventory
		void CheckForItemDrops();
		void GetAllItems();

		// Matchmaking
		virtual void StartMatchmaking() override;
		virtual bool IsMatchmaking() override;
		virtual void CancelMatchmaking() override;

		// Leaderboard
		void SetFindLeaderboardCallback(FindLeaderboardCallback completionCallback);
		void GetLeaderboards(LeaderboardsTypes typeOfLeaderboard);
		void UpdateLeaderboards(LeaderboardsTypes typeOfLeaderboard);

	private:
		// Steam
		CSteamID m_localPlayerSteamID;

		HSteamNetConnection m_connectedServerHandle{ k_HSteamNetConnection_Invalid };

		// Track whether we are connected to a server (and what specific state that connection is in)
		ClientConnectionState m_connectedState;

		// m_serverSteamID will always be valid when you connect to a game server
		CSteamID m_serverSteamID;
		uint32 m_serverIP{ 0 };
		uint16 m_serverPort{ 0 };
		HAuthTicket m_authTicket{ k_HAuthTicketInvalid };

		// Time we started our last connection attempt
		uint64 m_lastConnectionAttemptRetryTime;

		// Time we last got data from the server
		uint64 m_lastNetworkDataReceivedTime;

		// Our ship position in the array below
		uint32 m_playerShipIndex;

		// Callbacks for Steam connection state
		STEAM_CALLBACK(SteamOnlineManager, OnSteamServersConnected, SteamServersConnected_t);
		STEAM_CALLBACK(SteamOnlineManager, OnSteamServersDisconnected, SteamServersDisconnected_t);
		STEAM_CALLBACK(SteamOnlineManager, OnSteamServerConnectFailure, SteamServerConnectFailure_t);

		// Called when we get new connections, or the state of a connection changes
		STEAM_CALLBACK(SteamOnlineManager, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);

		
		bool GameMessageResultStateLog(const EResult result);

		StatsAndAchievements m_statsAndAchievements;
		Lobby m_lobby;
		Inventory m_inventory;
		Matchmaking m_matchmaking;
		Leaderboard m_leaderboard;
	};

	extern const char* MessageTypeString(GameMessageType type);
	using OnlineManager = SteamOnlineManager;
}
