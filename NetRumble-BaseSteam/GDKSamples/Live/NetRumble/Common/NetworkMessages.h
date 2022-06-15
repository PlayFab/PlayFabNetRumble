//--------------------------------------------------------------------------------------
// NetworkMessages.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <map>

#include "ServerConfig.h"

namespace NetRumble
{
	// Enum for various client connection states
	enum ClientConnectionState
	{
		ClientNotConnected,							// Initial state, not connected to a server
		ClientConnectedPendingAuthentication,		// We've established communication with the server, but it hasn't authed us yet
		ClientConnectedAndAuthenticated,				// Final phase, server has authed us, we are actually able to play on it
	};

	enum class GameMessageType : DWORD
	{
		Unknown						= 0,

		// Game messages
		GameStart					= 1,
		GameOver					= 2,

		PlayerJoined				= 11,
		PlayerInfo					= 12,
		PlayerState					= 13,
		PlayerLeft					= 14,

		PowerUpSpawn				= 15,
		ShipSpawn					= 16,
		ShipInput					= 17,
		ShipData					= 18,
		ShipDeath					= 19,

		ServerWorldSetup			= 31,
		ServerUpdateWorldData		= 32,

		// Steam message
		// Server login and authentication messages
		ServerMessageBegin			= 200,
		ServerSendInfo				= 201,
		ServerFailAuthentication	= 202,
		ServerPassAuthentication	= 203,
		ServerStateExiting			= 205,

		// Client login messages
		ClientBeginAuthentication	= 501,

		// P2P authentication messages
		P2PSendingTicket			= 601,

		// Voice data from another player
		VoiceChatData				= 701,

		// Force 32-bit size enum so the wire protocol doesn't get outgrown later
		ForceDWORD = 0x7fffffff
	};

	static constexpr size_t MsgTypeSize = sizeof(GameMessageType);
	class GameMessage final
	{
	public:
		GameMessage() = default;
		GameMessage(GameMessageType type, uint32_t data);
		GameMessage(GameMessageType type, std::string_view data);
		GameMessage(GameMessageType type, const std::vector<uint8_t>& data);
		GameMessage(const std::vector<uint8_t>& data);

		inline const GameMessageType MessageType() const { return m_type; }
		inline void MessageType(GameMessageType type) { m_type = type; }

		inline const std::vector<uint8_t>& RawData() const { return m_data; }
		inline void RawData(const std::vector<uint8_t>& data) { m_data = data; }

		std::string StringValue() const;
		uint32_t UnsignedValue() const;

		std::vector<uint8_t> Serialize() const;
		// Used for game server host to dispatch the message
		std::vector<uint8_t> SerializeWithSourceID() const;

	private:
		GameMessageType m_type = GameMessageType::Unknown;
		std::vector<uint8_t> m_data;
	};

	// Defines the wire protocol for the game
#pragma pack( push, 1 )

	enum DisconnectReason
	{
		ClientDisconnect = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_App_Min + 1,
		ServerClosed	 = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_App_Min + 2,
		ServerReject	 = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_App_Min + 3,
		ServerFull		 = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_App_Min + 4,
		ClientKicked	 = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_App_Min + 5,
		RemoteTimeout	 = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_Remote_Timeout,
		MiscGeneric		 = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_Misc_Generic,
		MiscP2PRendezvous = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_Misc_P2P_Rendezvous,
		MiscPeerSentNoConnection = ESteamNetConnectionEnd::k_ESteamNetConnectionEnd_Misc_PeerSentNoConnection
	};

	// Msg from the server to the client which is sent right after communications are established
	// and tells the client what SteamID the game server is using as well as whether the server is secure
	struct MsgServerSendInfo_t
	{
		MsgServerSendInfo_t() : m_msgType(GameMessageType::ServerSendInfo) {}
		const GameMessageType GetMessageType() const { return (m_msgType); }

		void SetSteamIDServer(uint64 SteamID) { m_SteamIDServer = SteamID; }
		const uint64 GetSteamIDServer() const { return m_SteamIDServer; }

		void SetSecure(bool secure) { m_isVACSecure = secure; }
		const bool GetSecure() const { return m_isVACSecure; }

		void SetServerName(const char* name) { strncpy_s(m_serverName, name, sizeof(m_serverName)); }
		const char* GetServerName() const { return m_serverName; }

	private:
		const GameMessageType m_msgType;
		uint64 m_SteamIDServer;
		bool m_isVACSecure;
		char m_serverName[128];
	};

	// Msg from the server to the client when refusing a connection
	struct MsgServerFailAuthentication_t
	{
		MsgServerFailAuthentication_t() : m_msgType(GameMessageType::ServerFailAuthentication) {}
		const GameMessageType GetMessageType() const { return m_msgType; }
	private:
		const GameMessageType m_msgType;
	};

	// Msg from the server to client when accepting a pending connection
	struct MsgServerPassAuthentication_t
	{
		MsgServerPassAuthentication_t() : m_msgType(GameMessageType::ServerPassAuthentication) {}
		const GameMessageType GetMessageType() const { return m_msgType; }

		void SetPlayerPosition(uint32 pos) { m_playerPosition = pos; }
		const uint32 GetPlayerPosition() const { return m_playerPosition; }

	private:
		const GameMessageType m_msgType;
		uint32 m_playerPosition{ 0 };
	};

	// Msg from server to clients when it is exiting
	struct MsgServerExiting_t
	{
		MsgServerExiting_t() : m_msgType(GameMessageType::ServerStateExiting) {}
		const GameMessageType GetMessageType() const { return m_msgType; }

	private:
		const GameMessageType m_msgType;
	};

	// Msg from client to server when initiating authentication
	struct MsgClientBeginAuthentication_t
	{
		MsgClientBeginAuthentication_t() : m_msgType(GameMessageType::ClientBeginAuthentication) {}
		const GameMessageType GetMessageType() const { return m_msgType; }

		void SetToken(const char* token, uint32 tokenLen) { m_tokenLen = tokenLen; memcpy(m_token, token, MIN(tokenLen, sizeof(m_token))); }
		const uint32 GetTokenLen() const { return m_tokenLen; }
		const char* GetTokenPtr() const { return m_token; }

		void SetSteamID(uint64 SteamID) { m_SteamID = SteamID; }
		const uint64 GetSteamID() const { return m_SteamID; }

	private:
		const GameMessageType m_msgType;

		uint32 m_tokenLen;
#ifdef USE_GS_AUTH_API
		char m_token[1024];
#endif
		uint64 m_SteamID;
	};

	// Message sent from one peer to another, so peers authenticate directly with each other.
	// (In this example, the server is responsible for relaying the messages, but peers
	// are directly authenticating each other.)
	struct MsgP2PSendingTicket_t
	{
		MsgP2PSendingTicket_t() : m_msgType(GameMessageType::P2PSendingTicket) {}
		const GameMessageType GetMessageType() const { return m_msgType; }


		void SetToken(const void* token, uint32 length) { m_tokenLen = length; memcpy(m_token, token, MIN(length, sizeof(m_token))); }
		const uint32 GetTokenLen() const { return m_tokenLen; }
		const char* GetTokenPtr() const { return m_token; }

		// Sender or receiver (depending on context)
		void SetSteamID(uint64 SteamID) { m_SteamID = SteamID; }
		const uint64 GetSteamID() const { return m_SteamID; }

	private:
		const GameMessageType m_msgType;
		uint32 m_tokenLen;
		char m_token[1024];
		uint64 m_SteamID;
	};

	// Voice chat data.  This is relayed through the server
	struct MsgVoiceChatData_t
	{
		MsgVoiceChatData_t() : m_msgType(GameMessageType::VoiceChatData) {}
		const GameMessageType GetMessageType() const { return m_msgType; }

		void SetDataLength(uint32 length) { m_msgLength = length; }
		const uint32 GetDataLength() const { return m_msgLength; }

		void SetSteamID(CSteamID steamID) { m_fromSteamID = steamID; }
		const CSteamID GetSteamID() const { return m_fromSteamID; }

	private:
		const GameMessageType m_msgType;
		uint32 m_msgLength;
		CSteamID m_fromSteamID;
	};

#pragma pack( pop )

}
