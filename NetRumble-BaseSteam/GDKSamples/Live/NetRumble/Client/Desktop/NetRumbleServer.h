#pragma once

#include "pch.h"
#include "NetworkMessages.h"

namespace NetRumble
{
	struct ClientConnectionData
	{
		bool m_active;							// Slot is in available or not for new connections
		bool m_inGame;
		CSteamID m_SteamIDUser;					// Player SteamID
		uint64 m_tickCountLastData;				// Last time server got data from the player
		HSteamNetConnection m_connectionHandle;	// The handle of player connection
	};

	class NetRumbleServer final
	{
	public:
		NetRumbleServer() noexcept;
		NetRumbleServer(const NetRumbleServer&) = delete;
		NetRumbleServer(NetRumbleServer&&) noexcept = delete;
		NetRumbleServer& operator=(const NetRumbleServer&) = delete;
		NetRumbleServer& operator=(const NetRumbleServer&&) = delete;

		~NetRumbleServer();

		// Server tick will be called in game loop every tick
		void Tick();

		void SetGameState(ServerGameState state);

		// Last server update tick
		inline const uint64 GetLastServerUpdateTick() const { return m_lastServerUpdateTick; }
		void SetLastServerUpdateTick(uint64 newTick) { m_lastServerUpdateTick = newTick; }

		// Check for any incoming network messages, then dispatch them to poll group
		void ProcessNetworkMessageOnPollGroup();

		// Reset player scores (occurs when starting a new game)
		void ResetScores();

		// Kick a given player off the server
		void KickPlayerOffServer(CSteamID steamID);

		// Whether the connection to steam has already formed
		bool IsConnectedToSteam() const { return m_connectedToSteam; }
		CSteamID GetSteamID() const;

		// Send the same message to all clients, except the ignored connection if any
		void SendMessageToAll(const void* msg, uint32 msgSize, int sendFlags = k_nSteamNetworkingSend_UnreliableNoDelay);
		void SendMessageToAllIgnore(const void* msg, uint32 msgSize, std::set<HSteamNetConnection>& ignoreConnectionSet, int sendFlags = k_nSteamNetworkingSend_UnreliableNoDelay);

		// Removes a player from the server
		void RemovePlayerFromServer(uint32 shipPosition, DisconnectReason reason);

		void RemoveDroppedUser(ESteamNetworkingConnectionState eState, CSteamID droppedUser);

	private:
		// Connection and authentication callback functions:
		// Steam will call these callback functions to let us know about events related to our
		// connection to the Steam servers.

		// Called when successfully connected to Steam
		STEAM_GAMESERVER_CALLBACK(NetRumbleServer, OnSteamServersConnected, SteamServersConnected_t);

		// Called when there was a failure to connect to Steam
		STEAM_GAMESERVER_CALLBACK(NetRumbleServer, OnSteamServersConnectFailure, SteamServerConnectFailure_t);

		// Called when we have been logged out of Steam
		STEAM_GAMESERVER_CALLBACK(NetRumbleServer, OnSteamServersDisconnected, SteamServersDisconnected_t);

		// Called when Steam has set our security policy (VAC on or off)
		STEAM_GAMESERVER_CALLBACK(NetRumbleServer, OnPolicyResponse, GSPolicyResponse_t);

		// Client validation Callback functions:
		// Steam will call to let us know about whether we should
		// allow clients to play or we should kick/deny them.

		// Called when a client has been authenticated and approved to play by Steam (passes auth, license check, VAC status, etc...)
		STEAM_GAMESERVER_CALLBACK(NetRumbleServer, OnValidateAuthTicketResponse, ValidateAuthTicketResponse_t);

		// Client connection state
		// All connection changes are handled through this callback
		STEAM_GAMESERVER_CALLBACK(NetRumbleServer, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);

		// Tell Steam about our servers details
		void SendUpdatedServerDetailsToSteam();

		// Send msg to a client at the given ship index
		bool SendMessageToClientAtIndex(uint32 index, char* message, uint32 msgSize);

		void OnClientBeginAuthentication(CSteamID steamIDClient, HSteamNetConnection connectionID, void* pToken, int tokenLen);

		// Handles authentication completing for a client
		void OnAuthCompleted(bool authSuccess, uint32 pendingAuthIndex);

		// Send world update to all clients
		void SendUpdateDataToAllClients();

		// Track whether our server is connected to Steam ok (meaning we can restrict who plays based on 
		// ownership and VAC bans, etc...)
		bool m_connectedToSteam;

		// Player scores
		uint32 m_playerScores[MAX_PLAYERS_PER_SERVER]{ 0 };

		// Server name
		std::string m_serverName;

		// Who just won the game? Should be set if we go into the GameWinner state
		uint32 m_lastGameWinner;

		// Last time state changed
		uint64 m_lastStateTransitionTime;

		// Last time we sent clients an update
		uint64 m_lastServerUpdateTick;

		// Number of players currently connected, updated each tick
		uint32 m_playerCount;

		// Current game state
		ServerGameState m_gameState;

		// Client connections
		ClientConnectionData m_clientData[MAX_PLAYERS_PER_SERVER];

		// Client connections which are pending auth
		ClientConnectionData m_pendingClientData[MAX_PLAYERS_PER_SERVER];

		// Socket to listen for new connections on 
		HSteamListenSocket m_listenSocket;

		// Poll group used to receive messages from all clients at once
		HSteamNetPollGroup m_netPollGroup;
	};
}

