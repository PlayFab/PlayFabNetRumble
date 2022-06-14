//--------------------------------------------------------------------------------------
// ServerConfig.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
	// Remove this define to disable using the native Steam authentication and matchmaking system
	// You can use this as a sample of how to integrate your game without replacing an existing matchmaking system
	// When you un-define USE_GS_AUTH_API you get:
	// - Access to achievement/community API's
	// - P2P networking capability
	// You CANNOT use: as these function depend on using Steam authentication
	// - Strong user authentication and authorization
	// - VAC cheat protection
	// - Game server matchmaking
	// - Access to achievement/community API's
#define USE_GS_AUTH_API 

// Current game server version
#define NETRUMBLE_SERVER_VERSION "1.0.0.0"

	// This PlayFab title ID will need to be set, before building and running the game.
	// In the final code we will need to remove the hard - coded app specific information.PlatFabTitleId
	static constexpr std::string_view NETRUMBLE_PLAYFAB_TITLE_ID{ "00000" };

	// UDP port for the NetRumble server to listen on
	static constexpr uint32 NETRUMBLE_SERVER_PORT{ 27015 };

	// UDP port for the master server updater to listen on
	static constexpr uint32 NETRUMBLE_MASTER_SERVER_UPDATER_PORT{ 27016 };

	// How long to wait for a client to send an update before we drop its connection server side
	static constexpr uint64 SERVER_TIMEOUT_MILLISECONDS{ 50000000 };

	// Maximum number of players who can join a server and play simultaneously
	static constexpr uint32 MAX_PLAYERS_PER_SERVER{ 4 };

	// Time to pause wait after a round ends before starting a new one
	static constexpr uint32 MILLISECONDS_BETWEEN_ROUNDS{ 4000 };

	// How many times a second does the server send world updates to clients
	static constexpr uint32 SERVER_UPDATE_SEND_RATE{ 60 };

	// Max number of messages the server will process per server message processing function call 
	static constexpr uint32 SERVER_MAX_MESSAGE_NUMBER_PROCESS{ 128 };

	// Enum for possible game states on the server
	enum class ServerGameState : uint8
	{
		SvrGameStateWaitingPlayers,
		SvrGameStateActive,
		SvrGameStateDraw,
		SvrGameStateWinner,
		SvrGameStateExiting,
	};
}
