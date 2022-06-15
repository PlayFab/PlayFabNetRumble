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
		Unknown = 0,

		// Game messages
		GameStart = 1,
		GameOver = 2,

		PlayerJoined = 11,
		SynPlayerData = 12,
		PlayerState = 13,
		PlayerLeftGame = 14,

		PowerUpSpawn = 15,
		ShipSpawn = 16,
		ShipInput = 17,
		ShipData = 18,
		ShipDeath = 19,

		WorldSetup = 21,
		WorldData = 22,

		ServerWorldSetup = 31,
		ServerUpdateWorldData = 32,

		HostGameFailed = 33,
		CreateLobbyFailed = 34,
		JoiningGame = 41,
		OnlineDisconnect = 42,
		MatchmakingCanceled = 43,
		MatchmakingFailed = 44,
		JoinGameFailed = 45,
		JoinedGameComplete = 46,
		LeaveGameComplete = 47,

		// Player has multiplayer privilege
		MPPrivilegeError = 48,
		RegionLatency = 49,
		MigrateRegion = 50,

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
}
