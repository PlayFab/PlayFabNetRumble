#include "pch.h"

using namespace NetRumble;

NetRumbleServer::NetRumbleServer() noexcept :
	m_connectedToSteam(false),
	m_lastGameWinner(0),
	m_lastServerUpdateTick(0),
	m_playerCount(0),
	m_gameState(ServerGameState::SvrGameStateWaitingPlayers)
{
	// Seed random num generator
	srand((uint32)time(nullptr));

	const char* pchGameDir = "NetRumble";

#ifdef USE_GS_AUTH_API
	// Server authentication and secure mode
	EServerMode eMode = eServerModeAuthenticationAndSecure;
#else
	// Use your own authentication
	EServerMode eMode = eServerModeNoAuthentication;
#endif
	if (!SteamGameServer_Init(INADDR_ANY, NETRUMBLE_SERVER_PORT, NETRUMBLE_MASTER_SERVER_UPDATER_PORT, eMode, NETRUMBLE_SERVER_VERSION))
	{
		DEBUGLOG("SteamGameServer_Init call failed\n");
	}

	if (SteamGameServer())
	{
		SteamGameServer()->SetModDir(pchGameDir);
		SteamGameServer()->SetProduct("NetRumble!");
		SteamGameServer()->SetGameDescription("NetRumble example");

		// Initiate Anonymous logon.
		SteamGameServer()->LogOnAnonymous();
		SteamNetworkingUtils()->InitRelayNetworkAccess();

#ifdef USE_GS_AUTH_API
		SteamGameServer()->SetAdvertiseServerActive(true);
#endif
	}
	else
	{
		DEBUGLOG("Invalid SteamGameServer() interface\n");
	}

	// Zero the client connection data
	memset(&m_clientData, 0, sizeof(m_clientData));
	memset(&m_pendingClientData, 0, sizeof(m_pendingClientData));

	// Create the listen socket for listening for new connecting from clients
	m_listenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);

	// Create the poll group used to receive messages from all clients at once
	m_netPollGroup = SteamGameServerNetworkingSockets()->CreatePollGroup();
}

NetRumbleServer::~NetRumbleServer()
{
	MsgServerExiting_t msg;
	SendMessageToAll((char*)&msg, sizeof(msg));

	SteamGameServerNetworkingSockets()->CloseListenSocket(m_listenSocket);
	SteamGameServerNetworkingSockets()->DestroyPollGroup(m_netPollGroup);

	// Disconnect from the Steam servers
	SteamGameServer()->LogOff();

	// Release our reference to the Steam client library
	SteamGameServer_Shutdown();
}

// Handle any connection status change
void NetRumbleServer::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback)
{
	// Connection handle
	HSteamNetConnection hConn = pCallback->m_hConn;

	// Full connection info
	SteamNetConnectionInfo_t info = pCallback->m_info;

	// Previous state.  (Current state is in m_info.m_eState)
	ESteamNetworkingConnectionState eOldState = pCallback->m_eOldState;

	// Check if a client has connected
	if (info.m_hListenSocket &&
		eOldState == k_ESteamNetworkingConnectionState_None &&
		info.m_eState == k_ESteamNetworkingConnectionState_Connecting)
	{
		// Search an available slot for new client connection  
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			if (!m_clientData[i].m_active && !m_pendingClientData[i].m_connectionHandle)
			{
				// Found one.  "Accept" the connection.
				EResult res = SteamGameServerNetworkingSockets()->AcceptConnection(hConn);
				if (res != k_EResultOK)
				{
					DEBUGLOG("AcceptConnection returned %d", res);
					SteamGameServerNetworkingSockets()->CloseConnection(hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection", false);
					return;
				}

				m_pendingClientData[i].m_connectionHandle = hConn;

				// Add the user to the poll group
				SteamGameServerNetworkingSockets()->SetConnectionPollGroup(hConn, m_netPollGroup);

				// Send them the server info as a reliable msg
				MsgServerSendInfo_t msg;
				msg.SetSteamIDServer(SteamGameServer()->GetSteamID().ConvertToUint64());
#ifdef USE_GS_AUTH_API
				// You can only make use of VAC when using the Steam authentication system
				msg.SetSecure(SteamGameServer()->BSecure());
#endif
				msg.SetServerName(m_serverName.c_str());
				SteamGameServerNetworkingSockets()->SendMessageToConnection(hConn, &msg, sizeof(MsgServerSendInfo_t), k_nSteamNetworkingSend_Reliable, nullptr);

				return;
			}
		}

		// No empty slot. Server is full!
		DEBUGLOG("Rejecting connection, server is full");
		SteamGameServerNetworkingSockets()->CloseConnection(hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Server full!", false);
	}
	// Check if a client has disconnected
	else if ((eOldState == k_ESteamNetworkingConnectionState_Connecting || eOldState == k_ESteamNetworkingConnectionState_Connected || eOldState == k_ESteamNetworkingConnectionState_FindingRoute)
		&& (info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer || info.m_eState == k_ESteamNetworkingConnectionState_None || info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally))
	{
		CSteamID identityRemote = info.m_identityRemote.GetSteamID();
		// Handle disconnecting a client
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			// If there is no ship, skip
			if (!m_clientData[i].m_active)
			{
				continue;
			}

			if (m_clientData[i].m_SteamIDUser == identityRemote)
			{
				RemoveDroppedUser(info.m_eState, m_clientData[i].m_SteamIDUser);
				RemovePlayerFromServer(i, DisconnectReason::ClientDisconnect);
				break;
			}
		}

		auto& peers = g_game->GetPeers();
		auto droppedUser = peers.find(identityRemote.ConvertToUint64());
		if (droppedUser != peers.end())
		{
			RemoveDroppedUser(info.m_eState, identityRemote);
		}
	}
	else
	{
		DEBUGLOG("Unknown connection status[%d]\n", info.m_eState);
	}
}

// Handle sending msg to a client at a given index
bool NetRumbleServer::SendMessageToClientAtIndex(uint32 index, char* msg, uint32 msgSize)
{
	if (index >= MAX_PLAYERS_PER_SERVER)
	{
		return false;
	}

	int64 messageOut;
	if (!SteamGameServerNetworkingSockets()->SendMessageToConnection(m_clientData[index].m_connectionHandle, msg, msgSize, k_nSteamNetworkingSend_Unreliable, &messageOut))
	{
		DEBUGLOG("Failed sending msg to a client\n");
		return false;
	}
	return true;
}

// Handle a new client connecting
void NetRumbleServer::OnClientBeginAuthentication(CSteamID steamIDClient, HSteamNetConnection connectionID, void* token, int tokenLen)
{
	// First, check this isn't a duplicate and we already have a user logged on from the same steamid
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (m_clientData[i].m_connectionHandle == connectionID)
		{
			return;
		}
	}

	// Second, do we have room to let them in
	uint32 nPendingOrActivePlayerCount = 0;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (m_pendingClientData[i].m_active)
		{
			++nPendingOrActivePlayerCount;
		}

		if (m_clientData[i].m_active)
		{
			++nPendingOrActivePlayerCount;
		}
	}

	// We are full (or will be if the pending players auth), deny new login
	if (nPendingOrActivePlayerCount >= MAX_PLAYERS_PER_SERVER)
	{
		SteamGameServerNetworkingSockets()->CloseConnection(connectionID, DisconnectReason::ServerFull, "Server is full", false);
	}

	// If we get here there is room, add the player as pending
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (!m_pendingClientData[i].m_active)
		{
			m_pendingClientData[i].m_tickCountLastData = g_game->GetGameTickCount();
#ifdef USE_GS_AUTH_API
			// Authenticate the user with the Steam back-end servers
			EBeginAuthSessionResult res = SteamGameServer()->BeginAuthSession(token, tokenLen, steamIDClient);
			if (res != k_EBeginAuthSessionResultOK)
			{
				SteamGameServerNetworkingSockets()->CloseConnection(connectionID, DisconnectReason::ServerReject, "BeginAuthSession failed", false);
				break;
			}

			m_pendingClientData[i].m_SteamIDUser = steamIDClient;
			m_pendingClientData[i].m_active = true;
			m_pendingClientData[i].m_connectionHandle = connectionID;
			break;
#else
			m_pendingClientData[i].m_active = true;
			// Tell the server our Steam id in the non-auth case, so we stashed it in the login msg, pull it back out
			m_pendingClientData[i].m_userSteamID = *(CSteamID*)pToken;
			m_pendingClientData[i].m_connectionHandle = connectionID;
			// You would typically do your own authentication method here and later call OnAuthCompleted
			// In this sample we just automatically auth anyone who connects
			OnAuthCompleted(true, i);
			break;
#endif
		}
	}
}

// A new client that connected has had their authentication processed
void NetRumbleServer::OnAuthCompleted(bool authSuccessful, uint32 pendingAuthIndex)
{
	if (!m_pendingClientData[pendingAuthIndex].m_active)
	{
		DEBUGLOG("Got auth completed callback for client who is not pending\n");
		return;
	}

	if (!authSuccessful)
	{
#ifdef USE_GS_AUTH_API
		// Tell the Game Server the user is leaving the server
		SteamGameServer()->EndAuthSession(m_pendingClientData[pendingAuthIndex].m_SteamIDUser);
#endif
		// Send a deny for the client, and zero out the pending msg
		MsgServerFailAuthentication_t msg;
		int64 outMessage;
		SteamGameServerNetworkingSockets()->SendMessageToConnection(m_pendingClientData[pendingAuthIndex].m_connectionHandle, &msg, sizeof(msg), k_nSteamNetworkingSend_Reliable, &outMessage);
		memset(&m_pendingClientData[pendingAuthIndex], 0, sizeof(ClientConnectionData));
		return;
	}

	bool addedOk = false;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (!m_clientData[i].m_active)
		{
			// Copy over the data from the pending array
			memcpy(&m_clientData[i], &m_pendingClientData[pendingAuthIndex], sizeof(ClientConnectionData));
			memset(&m_pendingClientData[pendingAuthIndex], 0, sizeof(ClientConnectionData));
			m_clientData[i].m_tickCountLastData = g_game->GetGameTickCount();

			MsgServerPassAuthentication_t msg;
			msg.SetPlayerPosition(i);
			SendMessageToClientAtIndex(i, (char*)&msg, sizeof(msg));

			addedOk = true;

			break;
		}
	}

	// If we just successfully added the player, check if they are #2 so we can restart the round
	if (addedOk)
	{
		uint32 players = 0;
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			if (m_clientData[i].m_active)
			{
				++players;
			}
		}
	}
}

// Used to reset scores (at start of a new game usually)
void NetRumbleServer::ResetScores()
{
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		m_playerScores[i] = 0;
	}
}

// Removes a player at the given position
void NetRumbleServer::RemovePlayerFromServer(uint32 shipPosition, DisconnectReason reason)
{
	if (shipPosition >= MAX_PLAYERS_PER_SERVER)
	{
		DEBUGLOG("Trying to remove player at invalid position\n");
		return;
	}

	DEBUGLOG("Removing player at index: %d from server, disconnectReason: %d\n", shipPosition, reason);

	m_playerScores[shipPosition] = 0;
	// Close the connection
	SteamGameServerNetworkingSockets()->CloseConnection(m_clientData[shipPosition].m_connectionHandle, reason, nullptr, false);

#ifdef USE_GS_AUTH_API
	// Tell the game server the user is leaving the server
	SteamGameServer()->EndAuthSession(m_clientData[shipPosition].m_SteamIDUser);
#endif
	memset(&m_clientData[shipPosition], 0, sizeof(ClientConnectionData));
}

// Used to transition game state
void NetRumbleServer::SetGameState(ServerGameState eState)
{
	if (m_gameState == eState)
	{
		return;
	}

	m_gameState = eState;
}

// Process incoming network messages from connected clients 
void NetRumbleServer::ProcessNetworkMessageOnPollGroup()
{
	SteamNetworkingMessage_t* msgs[SERVER_MAX_MESSAGE_NUMBER_PROCESS];
	int numMsgs = SteamGameServerNetworkingSockets()->ReceiveMessagesOnPollGroup(m_netPollGroup, msgs, SERVER_MAX_MESSAGE_NUMBER_PROCESS);
	for (int i = 0; i < numMsgs; i++)
	{
		SteamNetworkingMessage_t* msg = msgs[i];
		// Message payload size
		const uint32 msgSize = msg->GetSize();

		if (msgSize < sizeof(GameMessageType))
		{
			DEBUGLOG("Got garbage on server socket, too short\n");
			msg->Release();
			msg = nullptr;
			continue;
		}

		// Message payload data
		const auto msgData = msg->GetData();
		CSteamID steamIDRemote = msg->m_identityPeer.GetSteamID();
		HSteamNetConnection senderConnectionHandle = msg->m_conn;
		GameMessageType msgType = (GameMessageType)(*(GameMessageType*)msgData);

		for (uint32 j = 0; j < MAX_PLAYERS_PER_SERVER; ++j)
		{
			if (m_clientData[j].m_connectionHandle == senderConnectionHandle)
			{
				DEBUGLOG("Server receives message:[%s] from client at index:[%d] SteamIDUser:[%llu] \n", MessageTypeString(msgType), j, m_clientData[j].m_SteamIDUser);
			}
		}

		// Steam authentication and login message structure is different from game play message(GameMessage.Serialize())
		// We have to process them seperately
		if (msgType > GameMessageType::ServerMessageBegin) // Steam auth and login related messages
		{
			switch (msgType)
			{
			case GameMessageType::ClientBeginAuthentication:
			{
				if (msgSize != sizeof(MsgClientBeginAuthentication_t))
				{
					DEBUGLOG("Bad connection attempt msg\n");
					msg->Release();
					msg = nullptr;
					continue;
				}
				MsgClientBeginAuthentication_t* pMsg = (MsgClientBeginAuthentication_t*)msgData;
#ifdef USE_GS_AUTH_API
				OnClientBeginAuthentication(steamIDRemote, senderConnectionHandle, (void*)pMsg->GetTokenPtr(), static_cast<int>(pMsg->GetTokenLen()));
#else
				OnClientBeginAuthentication(connection, 0);
#endif
			}
			break;
			case GameMessageType::VoiceChatData:
			{
				// Received voice chat messages, broadcast to all other players
				MsgVoiceChatData_t* voiceChatMsg = (MsgVoiceChatData_t*)msgData;
				// Make sure sender steam ID is set.
				voiceChatMsg->SetSteamID(msg->m_identityPeer.GetSteamID());
				std::set<HSteamNetConnection> ignoreConnectionSet = { senderConnectionHandle };
				SendMessageToAllIgnore(voiceChatMsg, msgSize, ignoreConnectionSet);
				break;
			}
			case GameMessageType::P2PSendingTicket:
			{
				// Received a P2P auth ticket, forward it to the intended recipient
				MsgP2PSendingTicket_t msgP2PSendingTicket;
				memcpy(&msgP2PSendingTicket, msgData, sizeof(MsgP2PSendingTicket_t));
				CSteamID toSteamID = msgP2PSendingTicket.GetSteamID();

				HSteamNetConnection toHConn = 0;
				for (int index = 0; index < MAX_PLAYERS_PER_SERVER; index++)
				{
					if (toSteamID == m_clientData[index].m_SteamIDUser)
					{
						// Mutate the msg,waw replacing the destination SteamID with the sender's SteamID
						msgP2PSendingTicket.SetSteamID(msg->m_identityPeer.GetSteamID64());

						SteamNetworkingSockets()->SendMessageToConnection(m_clientData[index].m_connectionHandle, &msgP2PSendingTicket, sizeof(msgP2PSendingTicket), k_nSteamNetworkingSend_Reliable, nullptr);
						break;
					}
				}

				if (toHConn == 0)
				{
					DEBUGLOG("msgP2PSendingTicket received with no valid target to send to.");
				}
			}
			break;

			default:
				// We can check miss msg here. neo
				DEBUGLOG("Invalid msg %x\n", msgType);
			}
		}
		else
		{
			// These messages need to be broadcasted by the server, they contain a sourceID in their message header
			// Message structure: GameMessageType|SourceID|MessagePayLoad
			if (msgType == GameMessageType::ShipInput ||
				msgType == GameMessageType::ShipSpawn ||
				msgType == GameMessageType::ShipData ||
				msgType == GameMessageType::ShipDeath ||
				msgType == GameMessageType::PlayerInfo ||
				msgType == GameMessageType::PlayerJoined)
			{
				// Dispatch the message to all players except for message sender and the server
				int sendFlag = k_nSteamNetworkingSend_Reliable;
				if (msgType == GameMessageType::ShipInput || msgType == GameMessageType::ShipData)
				{
					sendFlag = k_nSteamNetworkingSend_Unreliable;
				}
				std::set<HSteamNetConnection> ignoreSet = { senderConnectionHandle, Managers::Get<OnlineManager>()->GetConnectedServerHandle() };
				SendMessageToAllIgnore(msgData, msgSize, ignoreSet, sendFlag);

				// The server host will process the message immediately
				uint64 sourceId = msg->m_identityPeer.GetSteamID64();

				std::shared_ptr<PlayerState> playerState = g_game->GetPlayerState(sourceId);
				if (!g_game->GetWorld()->IsInitialized())
				{
					DEBUGLOG("Server receive [%s] from client when world is not initialized!\n", MessageTypeString(msgType));
				}
				else if (playerState == nullptr)
				{
					DEBUGLOG("Server receive [%s] when peerState is nullptr!\n", MessageTypeString(msgType));
				}
				else
				{
					DEBUGLOG("Received [%s] from %u\n", MessageTypeString(msgType), sourceId);
					const size_t msgTypeSize = sizeof(GameMessageType);
					// Size of the payload. These messages contain both message type and sourceId as appended header.
					const size_t sourceIdSize = sizeof(sourceId);
					const size_t gamePlayMsgSize = msg->GetSize() - msgTypeSize - sourceIdSize;
					// Message payload start pos
					const uint8_t* dataBegin = static_cast<const uint8_t*>(msg->GetData()) + msgTypeSize + sourceIdSize;
					const uint8_t* dataEnd = dataBegin + gamePlayMsgSize;
					// Message payload
					const std::vector<uint8_t>& messageData = std::vector<uint8_t>(dataBegin, dataEnd);
					if (msgType == GameMessageType::ShipInput)
					{
						if (sourceId != g_game->GetLocalPlayerState()->PeerId)
						{
							playerState->GetShip()->Deserialize(messageData);
						}
					}
					else if (msgType == GameMessageType::ShipData)
					{
						playerState->GetShip()->Deserialize(messageData);
					}
					else if (msgType == GameMessageType::ShipDeath)
					{
						g_game->GetWorld()->DeserializeShipDeath(sourceId, messageData);
					}
					else if (msgType == GameMessageType::ShipSpawn)
					{
						g_game->GetWorld()->DeserializeShipSpawn(messageData);
					}
					else if (msgType == GameMessageType::PlayerJoined)
					{
						if (!playerState->IsLocalPlayer)
						{
							playerState->DeserializePlayerStateData(messageData);
						}

						if (playerState->InGame == true && playerState->InLobby == false)
						{
							const AudioManager* audioManager = Managers::Get<AudioManager>();
							if (audioManager)
							{
								OnlineVoiceChat& voiceChatManager = OnlineVoiceChat::GetInstance();
								if (!audioManager->IsVoiceChatActive())
								{
									voiceChatManager.StartVoiceChat();
								}
								voiceChatManager.GetVoiceChat()->MarkPlayerAsActive(sourceId);
							}
						}
					}
					else if (msgType == GameMessageType::PlayerInfo)
					{
						if (!playerState->IsLocalPlayer)
						{
							// Msg comes from one of other player joins in same game
							playerState->DisplayName = GameMessage(messageData).StringValue().c_str();

							// Server will dispatch the msg to all connected clients execept for the msg sender and the server host player
							std::set<HSteamNetConnection> ignoreSet = { static_cast<HSteamNetConnection>(sourceId) };
							for (auto& [id, peer] : g_game->GetPeers())
							{
								// Server host player
								if (peer->IsLocalPlayer)
								{
									ignoreSet.emplace(static_cast<HSteamNetConnection>(id));
								}
							}

							const bool serializeWithSourceID = true;
							Managers::Get<OnlineManager>()->ServerSendMessageToAllIgnore(GameMessage(
								GameMessageType::PlayerInfo,
								playerState->DisplayName
							), ignoreSet, serializeWithSourceID);
						}
						DEBUGLOG("Received PlayerInfo for: %ws\n", playerState->DisplayName.c_str());
					}
					else
					{
						DEBUGLOG("Unhandled msg:[%s] from client:[%d]\n", MessageTypeString(msgType), sourceId);
					}
				}
			}
		}
		msg->Release();
		msg = nullptr;
	}
}

// Tick function, updates the state of the server 
void NetRumbleServer::Tick()
{
	// Run any Steam Game Server API callbacks
	SteamGameServer_RunCallbacks();

	// Update our server details
	SendUpdatedServerDetailsToSteam();

	// Timeout stale player connections, also update player count msg
	uint32 playerCount = 0;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		// If there is no ship, skip
		if (!m_clientData[i].m_active)
		{
			continue;
		}

		// Integer format represent time using StepTimer::TicksPerSecond = 10'000'000 ticks per second
		// Server timeout is 5 seconds = 10'000'000 * 5 ticks
		if (g_game->GetGameTickCount() - m_clientData[i].m_tickCountLastData > SERVER_TIMEOUT_MILLISECONDS * 1000)
		{
			DEBUGLOG("Timing out player connection\n");
			RemovePlayerFromServer(i, DisconnectReason::ClientKicked);
		}
		else
		{
			++playerCount;
		}
	}
	m_playerCount = playerCount;

	switch (m_gameState)
	{
	case ServerGameState::SvrGameStateWaitingPlayers:
		// Wait a few seconds (so everyone can join if a lobby just started this server)
		if (g_game->GetGameTickCount() - m_lastStateTransitionTime >= MILLISECONDS_BETWEEN_ROUNDS)
		{
			// Just keep waiting until at least one ship is active
			for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
			{
				if (m_clientData[i].m_active)
				{
					// Transition to active
					DEBUGLOG("Server going active after waiting for players\n");
					SetGameState(ServerGameState::SvrGameStateActive);
				}
			}
		}
		break;
	case ServerGameState::SvrGameStateActive:
	{
		// Update all the entities...
	}
		break;
	case ServerGameState::SvrGameStateExiting:
		break;
	default:
		break;
	}
}

// Sends updates to all connected clients
void NetRumbleServer::SendUpdateDataToAllClients()
{
	// Limit the rate at which we update, even if our internal frame rate is higher
	if (g_game->GetGameTickCount() - m_lastServerUpdateTick < DX::StepTimer::TicksPerSecond / SERVER_UPDATE_SEND_RATE)
	{
		return;
	}

	m_lastServerUpdateTick = g_game->GetGameTickCount();

	for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (!m_clientData[i].m_active)
		{
			continue;
		}
	}
}

void NetRumbleServer::SendMessageToAll(const void* msg, uint32 msgSize, int sendFlags)
{
	// Reliable msg send. Can send up to k_cbMaxSteamNetworkingSocketsMessageSizeSend bytes in a single msg. 
	if (msgSize >= k_cbMaxSteamNetworkingSocketsMessageSizeSend)
	{
		DEBUGLOG("Message size is %d larger than max size (k_cbMaxSteamNetworkingSocketsMessageSizeSend = 512*1024) of a single msg that we can SEND.\n", msgSize);
		return;
	}

	for (int i = 0; i < MAX_PLAYERS_PER_SERVER; i++)
	{
		if (m_clientData[i].m_connectionHandle != k_HSteamNetConnection_Invalid)
		{
			DEBUGLOG("NetRumbleServer::SendMessageToAll m_connectionHandle == %llu\n", m_clientData[i].m_connectionHandle);
			SteamNetworkingSockets()->SendMessageToConnection(m_clientData[i].m_connectionHandle, msg, msgSize, sendFlags, nullptr);
		}
	}
}

void NetRumbleServer::SendMessageToAllIgnore(const void* msg, uint32 msgSize, std::set<HSteamNetConnection>& ignoreConnectionSet, int sendFlags)
{
	if (msgSize >= k_cbMaxSteamNetworkingSocketsMessageSizeSend)
	{
		DEBUGLOG("Message size is %d, it is larger than max size (k_cbMaxSteamNetworkingSocketsMessageSizeSend = 512*1024) of a single msg that we can SEND.\n", msgSize);
		return;
	}
	// Invalid connection also have to be excluded 
	ignoreConnectionSet.emplace(k_HSteamNetConnection_Invalid);
	for (int i = 0; i < MAX_PLAYERS_PER_SERVER; i++)
	{
		// Send message to valid client connection handles that are not in the ignored set
		if (ignoreConnectionSet.find(m_clientData[i].m_connectionHandle) == ignoreConnectionSet.end())
		{
			SteamNetworkingSockets()->SendMessageToConnection(m_clientData[i].m_connectionHandle, msg, msgSize, sendFlags, nullptr);
		}
	}
}

// Take any action we need to on Steam notifying us we are now logged in
void NetRumbleServer::OnSteamServersConnected(SteamServersConnected_t* pLogonSuccess)
{
	UNREFERENCED_PARAMETER(pLogonSuccess);

	DEBUGLOG("NetRumble Server connected to Steam successfully\n");
	m_connectedToSteam = true;

	// Log on is not finished until OnPolicyResponse() is called
	// Tell Steam about our server details
	SendUpdatedServerDetailsToSteam();
}

// Callback from Steam when logon is fully completed and VAC secure policy is set
void NetRumbleServer::OnPolicyResponse(GSPolicyResponse_t* pPolicyResponse)
{
	UNREFERENCED_PARAMETER(pPolicyResponse);
}

// Called when we were previously logged into steam but get logged out
void NetRumbleServer::OnSteamServersDisconnected(SteamServersDisconnected_t* pLoggedOff)
{
	UNREFERENCED_PARAMETER(pLoggedOff);

	m_connectedToSteam = false;
	DEBUGLOG("NetRumble Server got logged out of Steam\n");
}

// Called when an attempt to login to Steam fails
void NetRumbleServer::OnSteamServersConnectFailure(SteamServerConnectFailure_t* pConnectFailure)
{
	UNREFERENCED_PARAMETER(pConnectFailure);

	m_connectedToSteam = false;
	DEBUGLOG("NetRumble Server failed to connect to Steam\n");
}

// Called once we are connected to Steam to tell it about our details
void NetRumbleServer::SendUpdatedServerDetailsToSteam()
{
	// Tell the Steam authentication servers about our game
	char serverName[128];
	if (g_game)
	{
		// If a client is running (should always be since we don't support a dedicated server)
		// then we'll form the name based off of it
		std::string_view name = g_game->GetLocalPlayerName();
		sprintf_safe(serverName, "%s's game", name.data());
	}
	else
	{
		sprintf_safe(serverName, "%s", "NetRumble!");
	}
	m_serverName = serverName;

	// Set state variables, relevant to any master server updates or client pings
	// These server state variables may be changed at any time.
	SteamGameServer()->SetMaxPlayerCount(MAX_PLAYERS_PER_SERVER);
	SteamGameServer()->SetPasswordProtected(false);
	SteamGameServer()->SetServerName(m_serverName.c_str());
	SteamGameServer()->SetMapName("Galaxy");

#ifdef USE_GS_AUTH_API
	// Update all the players names/scores
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (m_clientData[i].m_active)
		{
			SteamGameServer()->BUpdateUserData(m_clientData[i].m_SteamIDUser, SteamFriends()->GetFriendPersonaName(m_clientData[i].m_SteamIDUser), m_playerScores[i]);
		}
	}
#endif
}

// Tells us Steam3 (VAC and newer license checking) has accepted the user connection
void NetRumbleServer::OnValidateAuthTicketResponse(ValidateAuthTicketResponse_t* response)
{
	if (response->m_eAuthSessionResponse == k_EAuthSessionResponseOK)
	{
		DEBUGLOG("localPlayerID == %d\n", SteamUser()->GetSteamID().ConvertToUint64());
		// This is the final approval, and means we should let the client play (find the pending auth by steamid)
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			if (m_pendingClientData[i].m_active && m_pendingClientData[i].m_SteamIDUser == response->m_SteamID)
			{
				DEBUGLOG("Auth completed for a client, steamIdUser == %d, connectHandler == %d\n", m_pendingClientData[i].m_SteamIDUser.ConvertToUint64(), m_pendingClientData[i].m_connectionHandle);
				OnAuthCompleted(true, i);
				// If the verification is not completed until after the game has started due to network reasons,
				// the game should be notified to start again.
				g_game->ServerPrepareGameEnviroment();
				return;
			}
		}
	}
	else
	{
		// Looks like we shouldn't let this user play, kick them
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			if (m_pendingClientData[i].m_active && m_pendingClientData[i].m_SteamIDUser == response->m_SteamID)
			{
				OnAuthCompleted(false, i);
				DEBUGLOG("Auth failed for a client\n");
				return;
			}
		}
	}
}

// Returns the SteamID of the game server
CSteamID NetRumbleServer::GetSteamID() const
{
#ifdef USE_GS_AUTH_API
	return SteamGameServer()->GetSteamID();
#else
	// This is a placeholder steam id to use when not making use of Steam auth or matchmaking
	return k_steamIDNonSteamGS;
#endif
}

// Kicks a player off the server
void NetRumbleServer::KickPlayerOffServer(CSteamID steamID)
{
	uint32 playerCount = 0;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		// If there is no ship, skip
		if (!m_clientData[i].m_active)
		{ 
			continue;
		}

		if (m_clientData[i].m_SteamIDUser == steamID)
		{
			DEBUGLOG("Kicking player\n");
			RemovePlayerFromServer(i, DisconnectReason::ClientKicked);
			// Send him a kick msg
			MsgServerFailAuthentication_t msg;
			int64 outMessage;
			SteamGameServerNetworkingSockets()->SendMessageToConnection(m_clientData[i].m_connectionHandle, &msg, sizeof(msg), k_nSteamNetworkingSend_Reliable, &outMessage);
		}
		else
		{
			++playerCount;
		}
	}
	m_playerCount = playerCount;
}

void NetRumbleServer::RemoveDroppedUser(ESteamNetworkingConnectionState eState, CSteamID droppedUser)
{
	DEBUGLOG("Remove dropped user\n");
	if (eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
	{
		std::vector<uint8_t> message = Managers::Get<OnlineManager>()->SerializePlayerDisconnect(droppedUser);
		Managers::Get<OnlineManager>()->ServerSendMessageToAll(
			GameMessage(
				GameMessageType::PlayerLeft,
				message
			),
			false,
			k_nSteamNetworkingSend_Reliable
		);
	}

	g_game->RemovePlayerFromGamePeers(droppedUser.ConvertToUint64());
}
