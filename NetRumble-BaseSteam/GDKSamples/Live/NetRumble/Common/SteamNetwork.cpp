#include "pch.h"

using namespace NetRumble;

namespace {
	const uint32 MAX_MESSAGE_NUM_FETCHED_FROM_CONNECTION = 32;
}
#define STRINGIFY(x) #x
const char* NetRumble::MessageTypeString(GameMessageType type)
{
	switch (type)
	{
	case GameMessageType::Unknown:            return STRINGIFY(GameMessageType::Unknown);
	case GameMessageType::GameStart:          return STRINGIFY(GameMessageType::GameStart);
	case GameMessageType::GameOver:           return STRINGIFY(GameMessageType::GameOver);
	case GameMessageType::PlayerJoined:       return STRINGIFY(GameMessageType::PlayerJoined);
	case GameMessageType::PlayerInfo:         return STRINGIFY(GameMessageType::PlayerInfo);
	case GameMessageType::PowerUpSpawn:       return STRINGIFY(GameMessageType::PowerUpSpawn);
	case GameMessageType::ServerWorldSetup:         return STRINGIFY(GameMessageType::ServerWorldSetup);
	case GameMessageType::ServerUpdateWorldData:    return STRINGIFY(GameMessageType::ServerUpdateWorldData);
	case GameMessageType::ShipSpawn:          return STRINGIFY(GameMessageType::ShipSpawn);
	case GameMessageType::ShipInput:          return STRINGIFY(GameMessageType::ShipInput);
	case GameMessageType::ShipData:           return STRINGIFY(GameMessageType::ShipData);
	case GameMessageType::ShipDeath:          return STRINGIFY(GameMessageType::ShipDeath);
	case GameMessageType::ServerSendInfo:     return STRINGIFY(GameMessageType::ServerSendInfo);
	case GameMessageType::ServerFailAuthentication:     return STRINGIFY(GameMessageType::ServerFailAuthentication);
	case GameMessageType::ServerPassAuthentication:     return STRINGIFY(GameMessageType::ServerPassAuthentication);
	case GameMessageType::ServerStateExiting:     return STRINGIFY(GameMessageType::ServerStateExiting);
	case GameMessageType::ClientBeginAuthentication:     return STRINGIFY(GameMessageType::ClientBeginAuthentication);
	case GameMessageType::P2PSendingTicket:     return STRINGIFY(GameMessageType::P2PSendingTicket);
	case GameMessageType::VoiceChatData:     return STRINGIFY(GameMessageType::VoiceChatData);
	default:                                  return STRINGIFY(GameMessageType::Unknown);
	}
}

bool SteamOnlineManager::GameMessageResultStateLog(const EResult resultCode)
{
	switch (resultCode)
	{
	case k_EResultOK:
	case k_EResultIgnored:
		break;

	case k_EResultInvalidParam:
		DEBUGLOG("Failed sending msg to server: Invalid connection handle, or the individual msg is too big\n");
		return false;
	case k_EResultInvalidState:
		DEBUGLOG("Failed sending messagesg to server: Connection is in an invalid state\n");
		return false;
	case k_EResultNoConnection:
		DEBUGLOG("Failed sending msg to server: Connection has ended\n");
		return false;
	case k_EResultLimitExceeded:
		DEBUGLOG("Failed sending msg to server: There was already too much msg queued to be sent\n");
		return false;
	default:
	{
		DEBUGLOG("SendMessageToConnection returned %d\n", resultCode);
		return false;
	}
	}
	return true;
}

void SteamOnlineManager::OnReceiveServerInfo(CSteamID steamIDGameServer, bool VACSecure, const char* serverName)
{
	UNREFERENCED_PARAMETER(VACSecure);
	UNREFERENCED_PARAMETER(serverName);

	m_connectedState = ClientConnectionState::ClientConnectedPendingAuthentication;
	m_serverSteamID = steamIDGameServer;

	SteamNetConnectionInfo_t info;
	SteamNetworkingSockets()->GetConnectionInfo(m_connectedServerHandle, &info);
	m_serverIP = info.m_addrRemote.GetIPv4();
	m_serverPort = info.m_addrRemote.m_port;

	// Set how to connect to the game server, using the Rich Presence API
	// this lets our friends connect to this game via their friends list
	UpdateRichPresenceConnectionInfo();

	MsgClientBeginAuthentication_t msg;
#ifdef USE_GS_AUTH_API
	char token[1024];
	uint32 tokenLen = 0;
	m_authTicket = SteamUser()->GetAuthSessionTicket(token, sizeof(token), &tokenLen);
	msg.SetToken(token, tokenLen);

#else
	// When you aren't using Steam auth you can still call AdvertiseGame() so you can communicate presence data to the friends
	// system. Make sure to pass k_steamIDNonSteamGS
	uint32 tokenLen = SteamUser()->AdvertiseGame(k_steamIDNonSteamGS, m_unServerIP, m_usServerPort);
	msg.SetSteamID(SteamUser()->GetSteamID().ConvertToUint64());
#endif

	Steamworks_TestSecret();

	if (msg.GetTokenLen() < 1)
		DEBUGLOG("Warning: Looks like GetAuthSessionTicket didn't give us a good ticket\n");

	Managers::Get<OnlineManager>()->SendGameMessage(&msg, sizeof(msg), k_nSteamNetworkingSend_Reliable);
}

// Updates what we show to friends about what we're doing and how to connect
void SteamOnlineManager::UpdateRichPresenceConnectionInfo()
{
	// Connect string that will come back to us on the command line	when a friend tries to join our game
	char rgchConnectString[128];
	rgchConnectString[0] = 0;

	CSteamID steamIDLobby = Managers::Get<OnlineManager>()->GetSteamIDLobby();

	if (m_connectedState == ClientConnectedAndAuthenticated && m_serverIP && m_serverPort)
	{
		// Game server connection method
		sprintf_safe(rgchConnectString, "+connect %d:%d", m_serverIP, m_serverPort);
	}
	else if (steamIDLobby.IsValid())
	{
		// Lobby connection method
		sprintf_safe(rgchConnectString, "+connect_lobby %llu", steamIDLobby.ConvertToUint64());
	}
	else
	{
		DEBUGLOG("Failed to connect to game server and lobby is invalid.\n");
	}

	SteamFriends()->SetRichPresence("connect", rgchConnectString);
}

// Initiates a connection to a server via P2P (NAT-traversing) connection
void SteamOnlineManager::InitiateServerConnection(CSteamID steamIDGameServer)
{
	if (m_connectedServerHandle != 0)
	{
		return;
	}

	CSteamID steamIDLobby = Managers::Get<OnlineManager>()->GetSteamIDLobby();

	m_serverSteamID = steamIDGameServer;

	SteamNetworkingIdentity identity;
	identity.SetSteamID(steamIDGameServer);

	m_connectedServerHandle = SteamNetworkingSockets()->ConnectP2P(identity, 0, 0, nullptr);

	// Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
	// and so we will retry at appropriate intervals if packets drop
	m_lastNetworkDataReceivedTime = m_lastConnectionAttemptRetryTime = g_game->GetGameTickCount();
}

void SteamOnlineManager::SetLobbyGameServerAndConnect()
{
	// For debug, here chunk need move to other place.
	// See if we have a game server ready to play on
	if (g_game->IsHost() && g_game->GetGameServer()->IsConnectedToSteam())
	{
		CSteamID steamIDLobby = Managers::Get<OnlineManager>()->GetSteamIDLobby();

		// Server is up; tell everyone else to connect
		SteamMatchmaking()->SetLobbyGameServer(steamIDLobby, 0, 0, g_game->GetGameServer()->GetSteamID());
		// Start connecting ourself via localhost (this will automatically leave the lobby)
		InitiateServerConnection(g_game->GetGameServer()->GetSteamID());
	}
}

void SteamOnlineManager::OnReceiveServerAuthenticationResponse(bool success, uint32 playerPosition)
{
	if (!success)
	{
		DEBUGLOG("Connection failure.\nMultiplayer authentication failed\n");
		// It is simply output error log, disconnect network connection, 
		// later it should add logic to clear peers data and jump back to main interface
		DisconnectFromServer();
	}
	else
	{
		// Is this a duplicate message? If so ignore it...
		if (m_connectedState == ClientConnectedAndAuthenticated && m_playerShipIndex == playerPosition)
			return;

		m_playerShipIndex = playerPosition;
		m_connectedState = ClientConnectedAndAuthenticated;

		// Set information so our friends can join the lobby
		UpdateRichPresenceConnectionInfo();

		// Tell steam china duration control system that we are in a match and not to be interrupted
		SteamUser()->BSetDurationControlOnlineState(k_EDurationControlOnlineState_OnlineHighPri);
	}
}

uint64 SteamOnlineManager::GetLastServerUpdateTick() const
{
	if (!g_game->IsHost())
	{
		return 0;
	}

	return g_game->GetGameServer()->GetLastServerUpdateTick();
}

void SteamOnlineManager::SetLastServerUpdateTick(uint64 newTick)
{
	if (!g_game->IsHost())
	{
		return;
	}

	g_game->GetGameServer()->SetLastServerUpdateTick(newTick);
}

void SteamOnlineManager::CreateGameServer()
{
	if (g_game->GetGameServer())
	{
		DisconnectFromServer();
	}

	g_game->GetGameServer() = std::make_unique<NetRumbleServer>();

	// We'll have to wait until the game server connects to the Steam server back-end 
	// before telling all the lobby members to join (so that the NAT traversal code has a path to contact the game server)
	DEBUGLOG("Game server being created; game will start soon.\n");
}

void SteamOnlineManager::DisconnectFromServer()
{

	if (m_connectedState != ClientConnectionState::ClientNotConnected)
	{
#ifdef USE_GS_AUTH_API
		if (m_authTicket != k_HAuthTicketInvalid)
		{
			SteamUser()->CancelAuthTicket(m_authTicket);
		}
		m_authTicket = k_HAuthTicketInvalid;
#else
		SteamUser()->AdvertiseGame(k_steamIDNil, 0, 0);
#endif

		// Tell steam china duration control system that we are no longer in a match
		SteamUser()->BSetDurationControlOnlineState(k_EDurationControlOnlineState_Offline);

		m_connectedState = ClientConnectionState::ClientNotConnected;
	}

	if (m_connectedServerHandle != k_HSteamNetConnection_Invalid)
	{
		SteamNetworkingSockets()->CloseConnection(m_connectedServerHandle, DisconnectReason::ClientDisconnect, nullptr, false);
	}
	m_serverSteamID = CSteamID();
	m_connectedServerHandle = k_HSteamNetConnection_Invalid;

	if (Managers::Get<OnlineManager>()->IsServer() && g_game->GetGameServer())
	{
		g_game->GetGameServer().reset();
	}
}

void SteamOnlineManager::OnReceiveServerExiting()
{
#ifdef USE_GS_AUTH_API
	if (m_authTicket != k_HAuthTicketInvalid)
	{
		SteamUser()->CancelAuthTicket(m_authTicket);
	}
	m_authTicket = k_HAuthTicketInvalid;
#else
	SteamUser()->AdvertiseGame(k_steamIDNil, 0, 0);
#endif

	m_connectedState = ClientNotConnected;

	DEBUGLOG("Game server has exited.");
}

void SteamOnlineManager::OnReceiveServerFullResponse()
{
	DEBUGLOG("Connection failure.\nServer is full\n");
	DisconnectFromServer();
}

bool SteamOnlineManager::SendGameMessage(const void* msg, const uint32 msgSize, int sendFlag)
{
	DEBUGLOG("Sending msg: %s\n", MessageTypeString(*(GameMessageType*)msg));
	const EResult resultCode = SteamNetworkingSockets()->SendMessageToConnection(GetConnectedServerHandle(), msg, msgSize, sendFlag, nullptr);

#ifdef DEBUG_LOGGING
	return GameMessageResultStateLog(resultCode);
#else
	return resultCode == k_EResultOK;
#endif
}

bool SteamOnlineManager::SendGameMessage(const GameMessage& message)
{
	DEBUGLOG("Sending msg: %s\n", MessageTypeString(message.MessageType()));
	const std::vector<uint8_t>& msgData = message.Serialize();
	const GameMessageType& messageType = message.MessageType();
	int sendFlag = k_nSteamNetworkingSend_Reliable;
	if (messageType == GameMessageType::ShipInput || messageType == GameMessageType::ShipData)
	{
		sendFlag = k_nSteamNetworkingSend_Unreliable;
	}
	HSteamNetConnection con = GetConnectedServerHandle();
	DEBUGLOG("NetRumbleServer::SendMessageToAll m_connectionHandle == %llu\n", con);
	const EResult resultCode = SteamNetworkingSockets()->SendMessageToConnection(GetConnectedServerHandle(), msgData.data(), static_cast<uint32>(msgData.size()), sendFlag, nullptr);

#ifdef DEBUG_LOGGING
	return GameMessageResultStateLog(resultCode);
#else
	return resultCode == k_EResultOK;
#endif
}

bool SteamOnlineManager::SendGameMessageWithSourceID(const GameMessage& message)
{
	DEBUGLOG("Sending msg: %s\n", MessageTypeString(message.MessageType()));
	const std::vector<uint8_t>& msgData = message.SerializeWithSourceID();
	const GameMessageType& messageType = message.MessageType();
	int sendFlag = k_nSteamNetworkingSend_Reliable;
	if (messageType == GameMessageType::ShipInput || messageType == GameMessageType::ShipData)
	{
		sendFlag = k_nSteamNetworkingSend_Unreliable;
	}
	const EResult resultCode = SteamNetworkingSockets()->SendMessageToConnection(GetConnectedServerHandle(), msgData.data(), msgData.size(), sendFlag, nullptr);

#ifdef DEBUG_LOGGING
	return GameMessageResultStateLog(resultCode);
#else
	return resultCode == k_EResultOK;
#endif
}

bool SteamOnlineManager::IsConnected() const
{
	// TODO
	return true;
}

uint64_t SteamOnlineManager::GetNetworkId() const
{
	return 0;
}

// Server msg process (current client is the host of the server)
// Send the same msg to all clients, except the ignored connection if any
// default value of hConnIgnore will not ignore any connections to the server 
bool SteamOnlineManager::ServerSendMessageToAll(const GameMessage& message, bool serializeWithSourceID, int sendFlags) const
{
	if (!IsServer())
	{
		DEBUGLOG("Server msg should be processed by server host\n");
		return false;
	}
	DEBUGLOG("Server sends msg: %s\n", MessageTypeString(message.MessageType()));

	if (serializeWithSourceID)
	{
		const std::vector<uint8_t>& data = message.SerializeWithSourceID();
		// uint64 is the size of message sender ID
		if (data.size() < sizeof(GameMessageType) + sizeof(uint64))
		{
			DEBUGLOG("Ill-formed GameMessage msg!\n Message size is smaller than an empty signal msg with only msg type and source ID in it.\n");
			return false;
		}
		g_game->GetGameServer()->SendMessageToAll(data.data(), data.size(), sendFlags);
	}
	else
	{
		const std::vector<uint8_t>& data = message.Serialize();
		if (data.size() < sizeof(GameMessageType))
		{
			DEBUGLOG("Ill-formed GameMessage msg!\n Message size is smaller than an empty signal msg with only msg type in it.\n");
			return false;
		}
		g_game->GetGameServer()->SendMessageToAll(data.data(), data.size(), sendFlags);
	}

	return true;
}

bool SteamOnlineManager::ServerSendMessageToAllIgnore(const GameMessage& message, std::set<HSteamNetConnection>& ignoreConnectionSet, bool serializeWithSourceID, int sendFlags) const
{
	if (!IsServer())
	{
		DEBUGLOG("Server msg should be processed by server host\n");
		return false;
	}

	DEBUGLOG("Server sends msg: %s\n", MessageTypeString(message.MessageType()));

	if (serializeWithSourceID)
	{
		const std::vector<uint8_t>& data = message.SerializeWithSourceID();
		if (data.size() < sizeof(GameMessageType) + sizeof(uint64))
		{
			return false;
		}

		g_game->GetGameServer()->SendMessageToAllIgnore(data.data(), data.size(), ignoreConnectionSet, sendFlags);
	}
	else
	{
		const std::vector<uint8_t>& data = message.Serialize();
		if (data.size() < sizeof(GameMessageType))
		{
			return false;
		}

		g_game->GetGameServer()->SendMessageToAllIgnore(data.data(), data.size(), ignoreConnectionSet, sendFlags);
	}
	return true;
}

void SteamOnlineManager::ServerProcessNetworkMessage()
{
	if (!IsServer())
	{
		DEBUGLOG("Server msg should be processed by server host\n");
		return;
	}

	g_game->GetGameServer()->ProcessNetworkMessageOnPollGroup();
};

// Client side network msg processor
void SteamOnlineManager::ClientProcessNetworkMessage()
{
	if (!SteamNetworkingSockets())
	{
		return;
	}

	const HSteamNetConnection connectedServerHandle = GetConnectedServerHandle();
	if (connectedServerHandle == k_HSteamNetConnection_Invalid)
	{
		return;
	}

	std::unique_ptr<World>& world = g_game->GetWorld();
	if (!world)
	{
		DEBUGLOG("The world is empty\n");
		return;
	}
	const uint64 localUserID = GetLocalSteamID().ConvertToUint64();
	const GameState gameState = Managers::Get<GameStateManager>()->GetState();

	SteamNetworkingMessage_t* msgs[MAX_MESSAGE_NUM_FETCHED_FROM_CONNECTION];
	// Fetch the next available msg(s) from the connection, if any.
	// Returns the number of messages returned into your array, up to nMaxMessages.
	// If the connection handle is invalid, -1 is returned.
	int msgNum = SteamNetworkingSockets()->ReceiveMessagesOnConnection(connectedServerHandle, msgs, MAX_MESSAGE_NUM_FETCHED_FROM_CONNECTION);
	for (int i = 0; i < msgNum; i++)
	{
		SteamNetworkingMessage_t* msg = msgs[i];
		uint32 msgSize = msg->GetSize();
		const GameMessageType msgType = (GameMessageType)(*(GameMessageType*)msg->GetData());

		//	Make sure we're connected
		if (GetConnectedState() == ClientNotConnected && gameState != GameState::JoinGameFromLobby)
		{
			if (msgType != GameMessageType::ServerSendInfo)
			{
				msg->Release();
				continue;
			}
		}

		if (msgSize < sizeof(GameMessageType))
		{
			DEBUGLOG("Got garbage on client socket, too short\n");
			msg->Release();
			continue;
		}

		if (msgType > GameMessageType::ServerMessageBegin)
		{
			auto messageData = msg->GetData();
			switch (msgType)
			{
			case GameMessageType::ServerSendInfo:
			{
				if (msgSize != sizeof(MsgServerSendInfo_t))
				{
					DEBUGLOG("Bad server info msg\n");
					break;
				}
				MsgServerSendInfo_t* message = (MsgServerSendInfo_t*)messageData;

				// Pull the IP address of the user from the socket
				OnReceiveServerInfo(CSteamID(message->GetSteamIDServer()), message->GetSecure(), message->GetServerName());
				break;
			}
			case GameMessageType::ServerPassAuthentication:
			{
				if (msgSize != sizeof(MsgServerPassAuthentication_t))
				{
					DEBUGLOG("Bad accept connection msg\n");
					break;
				}
				MsgServerPassAuthentication_t* message = (MsgServerPassAuthentication_t*)messageData;

				// Our game client doesn't really care about whether the server is secure, or what its 
				// steamID is, but if it did we would pass them in here as they are part of the accept msg
				OnReceiveServerAuthenticationResponse(true, message->GetPlayerPosition());
				break;
			}
			case GameMessageType::ServerFailAuthentication:
			{
				OnReceiveServerAuthenticationResponse(false, 0);
				break;
			}
			case GameMessageType::VoiceChatData:
			{
				// Here we just assume the message is the right size
				const AudioManager* audioManager = Managers::Get<AudioManager>();
				if (audioManager && audioManager->IsVoiceChatActive())
				{
					std::shared_ptr<SteamVoiceChat> voiceChat = OnlineVoiceChat::GetInstance().GetVoiceChat();
					if (voiceChat)
					{
						voiceChat->HandleVoiceChatData(messageData);
					}
				}
				break;
			}
			default:
			{
				DEBUGLOG("Unhandled message:[%s] from server\n", MessageTypeString(msgType));
				break;
			}
			}
		}
		else
		{
			uint64 sourceId = msg->m_identityPeer.GetSteamID64();
			const size_t msgTypeSize = sizeof(GameMessageType);
			uint32 msgSize = msg->GetSize() - msgTypeSize;

			if (msgType == GameMessageType::ShipInput ||
				msgType == GameMessageType::ShipSpawn ||
				msgType == GameMessageType::ShipData ||
				msgType == GameMessageType::ShipDeath ||
				msgType == GameMessageType::PlayerInfo ||
				msgType == GameMessageType::PlayerJoined)
			{
				// These messages contain both message type and sourceId as appended header
				// Please take a look at GameMessage::SerializeWithSourceID()
				// This is because these messages dispatch from game server
				// Therefore, msg->m_identityPeer.GetSteamID64() will give game server's SteamId instead of the original source of the real message sender 
				sourceId = *(uint64_t*)((char*)msg->GetData() + MsgTypeSize);
				const GameMessageType msgType = (GameMessageType)(*(GameMessageType*)msg->GetData());
				const size_t sourceIdSize = sizeof(sourceId);
				// Size of the payload. 
				msgSize = msg->GetSize() - msgTypeSize - sourceIdSize;
				const uint8_t* dataBegin = static_cast<const uint8_t*>(msg->GetData()) + msgTypeSize + sourceIdSize;
				const uint8_t* dataEnd = dataBegin + msgSize;
				// Message payload
				const std::vector<uint8_t>& messageData = std::vector<uint8_t>(dataBegin, dataEnd);
				std::shared_ptr<PlayerState> playerState = g_game->GetPlayerState(sourceId);

				switch (msgType)
				{
				case GameMessageType::PlayerJoined:
				{
					DEBUGLOG("Received a PlayerJoined msg from %u\n", sourceId);
					if (playerState == nullptr)
					{
						playerState = std::make_shared<PlayerState>();
						playerState->DeserializePlayerStateData(messageData);
						playerState->PeerId = sourceId;
						g_game->AddPlayerToLobbyPeers(playerState);
					}
					else
					{
						playerState->DeserializePlayerStateData(messageData);
					}

					if (sourceId != localUserID && playerState->InGame == true && playerState->InLobby == false)
					{
						AudioManager* audioManager = Managers::Get<AudioManager>();
						if (audioManager)
						{
							audioManager->StartVoiceChat();
							OnlineVoiceChat::GetInstance().GetVoiceChat()->MarkPlayerAsActive(sourceId);
						}
					}
					break;
				}
				case GameMessageType::PlayerInfo:
				{
					if (sourceId == localUserID)
					{
						DEBUGLOG("Local players have all their own PlayerInfo %u\n", sourceId);
						break;
					}
					const auto& peers = g_game->GetPeers();
					const auto& itr = peers.find(sourceId);
					// Player has joined already 
					if (itr == peers.end())
					{
						DEBUGLOG("Received PlayerInfo for unknown peer %u\n", sourceId);
						break;
					}

					// Message comes from one of other player joins in same game
					auto& newPlayerState = itr->second;
					newPlayerState->DisplayName = GameMessage(messageData).StringValue().c_str();
					DEBUGLOG("Received PlayerInfo for: %ws\n", newPlayerState->DisplayName.c_str());
					break;
				}
				case GameMessageType::ShipData:
				{
					if (world->IsInitialized())
					{
						if (playerState != nullptr)
						{
							playerState->GetShip()->Deserialize(messageData);
						}
					}
					else
					{
						DEBUGLOG("Received a ShipData msg\n...World not initialized!\n");
					}
					break;
				}
				case GameMessageType::ShipSpawn:
				{
					DEBUGLOG("Received a ShipSpawn msg\n");
					if (world->IsInitialized())
					{
						world->DeserializeShipSpawn(messageData);
					}
					else
					{
						DEBUGLOG("ShipSpawn ...World not initialized!\n");
					}
					break;
				}
				case GameMessageType::ShipDeath:
				{
					DEBUGLOG("Received a ShipDeath msg from %u\n", sourceId);
					if (world->IsInitialized())
					{
						if (playerState != nullptr)
						{
							world->DeserializeShipDeath(sourceId, messageData);
						}
						else
						{
							DEBUGLOG("PlayerState not found for %u\n", sourceId);
						}
					}
					else
					{
						DEBUGLOG("ShipDeath ...World not initialized!\n");
					}
					break;
				}
				case GameMessageType::ShipInput:
				{
					if (world->IsInitialized())
					{
						if (playerState != nullptr)
						{
							playerState->GetShip()->Deserialize(messageData);
						}
					}
					else
					{
						DEBUGLOG("ShipInput ...World not initialized!\n");
					}
					break;
				}
				}
			}
			else
			{
				const uint8_t* dataBegin = static_cast<const uint8_t*>(msg->GetData()) + msgTypeSize;
				const uint8_t* dataEnd = dataBegin + msgSize;
				// Message payload
				const std::vector<uint8_t>& messageData = std::vector<uint8_t>(dataBegin, dataEnd);

				switch (msgType)
				{
				case GameMessageType::GameStart:
				{
					DEBUGLOG("Received a GameStart msg\n");
					std::shared_ptr<PlayerState> localPlayerState = g_game->GetPlayerState(localUserID);
					if (!localPlayerState->InGame)
					{
						world->SetGameInProgress(true);
					}
					break;
				}
				case GameMessageType::GameOver:
				{
					if (world->IsInitialized())
					{
						DEBUGLOG("Received a GameOver message\n");

						std::shared_ptr<PlayerState> localPlayer = g_game->GetLocalPlayerState();
						if (localPlayer->InGame)
						{
							world->IsGameWon = true;
							world->SetGameInProgress(false);
							world->DeserializeGameOver(messageData);
						}
						else
						{
							g_game->ResetGameplayData();
						}
					}
					break;
				}
				case GameMessageType::PlayerLeft:
				{
					DEBUGLOG("Received a PlayerLeft msg from %u\n", sourceId);
					CSteamID steamID = Managers::Get<OnlineManager>()->DeserializePlayerDisconnect(messageData);
					if (steamID.ConvertToUint64() != localUserID)
					{
						g_game->RemovePlayerFromGamePeers(steamID.ConvertToUint64());
					}
					break;
				}
				case GameMessageType::PowerUpSpawn:
				{
					DEBUGLOG("Received a PowerUpSpawn msg\n");
					if (world->IsInitialized())
					{
						world->DeserializePowerUpSpawn(messageData);
					}
					else
					{
						DEBUGLOG("PowerUpSpawn ...World not initialized!\n");
					}
					break;
				}
				case GameMessageType::ServerUpdateWorldData:
				{
					if (world->IsInitialized())
					{
						world->DeserializeWorldData(messageData);
					}
					else
					{
						DEBUGLOG("ServerUpdateWorldData ...World not initialized!\n");
					}
					break;
				}
				case GameMessageType::ServerWorldSetup:
				{
					DEBUGLOG("Received a WorldSetup msg\n");
					if (!world->IsInitialized())
					{
						world->DeserializeWorldSetup(messageData);
					}
					else
					{
						DEBUGLOG("...World is already initialized!\n");
					}
					break;
				}
				default:
					DEBUGLOG("Unhandled msg from server\n");
					break;
				}
			}
			msg->Release();
		}

		// If we're hosting a server
		if (Managers::Get<OnlineManager>()->IsServer())
		{
			Managers::Get<OnlineManager>()->ServerProcessNetworkMessage();
		}
	}
}

// Handles notification that we are now connected to Steam
void SteamOnlineManager::OnSteamServersConnected(SteamServersConnected_t* callback)
{
	UNREFERENCED_PARAMETER(callback);

	if (SteamUser()->BLoggedOn())
	{
		Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
	}
	else
	{
		DEBUGLOG("Got SteamServersConnected_t, but not logged on?\n");
	}
}

// Handles notification that we are now connected to Steam
void SteamOnlineManager::OnSteamServersDisconnected(SteamServersDisconnected_t* callback)
{
	UNREFERENCED_PARAMETER(callback);

	DEBUGLOG("Got SteamServersDisconnected_t\n");
}

// Handles notification that we are failed to connected to Steam
void SteamOnlineManager::OnSteamServerConnectFailure(SteamServerConnectFailure_t* callback)
{
	std::string message("failed to connected to Steam");
	DEBUGLOG("SteamServerConnectFailure_t: %d\n", callback->m_eResult);
	Managers::Get<GameEventManager>()->DispatchEvent(GameEventMessage{ GameEventType::ConnectionSteamFail, const_cast<char*>(message.c_str()) });
}


// Handle any connection status change
void SteamOnlineManager::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback)
{
	/// Connection handle
	HSteamNetConnection m_hConn = pCallback->m_hConn;

	/// Full connection info
	SteamNetConnectionInfo_t m_info = pCallback->m_info;

	/// Previous state.  (Current state is in m_info.m_eState)
	ESteamNetworkingConnectionState m_eOldState = pCallback->m_eOldState;

	std::string message;

	// Triggered when a server rejects our connection
	if ((m_eOldState == k_ESteamNetworkingConnectionState_Connecting || m_eOldState == k_ESteamNetworkingConnectionState_Connected || m_eOldState == k_ESteamNetworkingConnectionState_FindingRoute) &&
		(m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer || m_info.m_eState == k_ESteamNetworkingConnectionState_None || m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally))
	{
		// Close the connection with the server
		SteamNetworkingSockets()->CloseConnection(m_hConn, m_info.m_eEndReason, nullptr, false);
		switch (m_info.m_eEndReason)
		{
		case DisconnectReason::ClientDisconnect:
			DisconnectFromServer();
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
			break;
		case DisconnectReason::ServerReject:
			message = "Server rejects connection";
			Managers::Get<OnlineManager>()->OnReceiveServerAuthenticationResponse(false, 0);
			DispatchMessageInLobby(message);
			break;
		case DisconnectReason::ServerFull:
			message = "Server is full";
			Managers::Get<OnlineManager>()->OnReceiveServerFullResponse();
			DispatchMessageInLobby(message);
			break;
		case DisconnectReason::RemoteTimeout:
			message = "Connection to a remote server timed out";
			Managers::Get<GameEventManager>()->DispatchEvent(GameEventMessage{ GameEventType::ConnectionRemoteTimeout, const_cast<char*>(message.c_str()) });
			break;
		case DisconnectReason::MiscGeneric:
			message = "Misc generic failed";
			DispatchMessageInLobby(message);
			break;
		case DisconnectReason::MiscP2PRendezvous:
			message = "P2P rendezvous failed";
			DispatchMessageInLobby(message);
			break;
		case DisconnectReason::MiscPeerSentNoConnection:
			message = "Peer Sent No Connection";
			Managers::Get<GameEventManager>()->DispatchEvent(GameEventMessage{ GameEventType::PeerSentNoConnection, const_cast<char*>(message.c_str()) });
			break;
		default:
			DisconnectFromServer();
			break;
		}
		// Need to display the error message and return to the main screen
		DEBUGLOG("Server rejects our connection, quiting server\n");
	}
	// Triggered if our connection to the server fails
	else if ((m_eOldState == k_ESteamNetworkingConnectionState_Connecting || m_eOldState == k_ESteamNetworkingConnectionState_Connected) &&
		m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
	{
		// Failed, error out
		DEBUGLOG("Failed to make P2P connection, quiting server\n");
		SteamNetworkingSockets()->CloseConnection(m_hConn, m_info.m_eEndReason, nullptr, false);
		Managers::Get<OnlineManager>()->OnReceiveServerExiting();
		DisconnectFromServer();
		// Need to display the error message and return to the main screen
		message = "Failed to make P2P connection";
		DispatchMessageInLobby(message);
	}
}

void SteamOnlineManager::DispatchMessageInLobby(std::string& message)
{
	Managers::Get<GameEventManager>()->DispatchEvent(GameEventMessage{ GameEventType::ConnectionServerFail, const_cast<char*>(message.c_str()) });
	Managers::Get<GameStateManager>()->SwitchToState(GameState::GameConnectFail);
	Managers::Get<GameStateManager>()->SetImmutableState();
}

std::vector<uint8_t> SteamOnlineManager::SerializePlayerDisconnect(CSteamID steamID) const
{
	std::vector<uint8_t> data(sizeof(CSteamID));
	CopyMemory(data.data(), &steamID, sizeof(CSteamID));

	return data;
}

CSteamID NetRumble::SteamOnlineManager::DeserializePlayerDisconnect(const std::vector<uint8_t>& data)
{
	CSteamID rcvdsteamID = CSteamID();
	CopyMemory(&rcvdsteamID, data.data(), sizeof(CSteamID));
	return rcvdsteamID;
}
