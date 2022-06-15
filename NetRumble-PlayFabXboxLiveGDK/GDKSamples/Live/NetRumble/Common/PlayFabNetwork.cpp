//--------------------------------------------------------------------------------------
// PlayFabNetwork.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

#define STRINGIFY(x) #x

const char* NetRumble::MessageTypeString(GameMessageType type)
{
	switch (type)
	{
	case GameMessageType::Unknown:            return STRINGIFY(GameMessageType::Unknown);
	case GameMessageType::GameStart:          return STRINGIFY(GameMessageType::GameStart);
	case GameMessageType::GameOver:           return STRINGIFY(GameMessageType::GameOver);
	case GameMessageType::PlayerJoined:       return STRINGIFY(GameMessageType::PlayerJoined);
	case GameMessageType::SynPlayerData:      return STRINGIFY(GameMessageType::SynPlayerData);
	case GameMessageType::PlayerState:        return STRINGIFY(GameMessageType::PlayerState);
	case GameMessageType::JoiningGame:        return STRINGIFY(GameMessageType::JoiningGame);
	case GameMessageType::PowerUpSpawn:       return STRINGIFY(GameMessageType::PowerUpSpawn);
	case GameMessageType::WorldSetup:         return STRINGIFY(GameMessageType::WorldSetup);
	case GameMessageType::WorldData:          return STRINGIFY(GameMessageType::WorldData);
	case GameMessageType::ShipSpawn:          return STRINGIFY(GameMessageType::ShipSpawn);
	case GameMessageType::ShipInput:          return STRINGIFY(GameMessageType::ShipInput);
	case GameMessageType::ShipData:           return STRINGIFY(GameMessageType::ShipData);
	case GameMessageType::ShipDeath:          return STRINGIFY(GameMessageType::ShipDeath);
	case GameMessageType::HostGameFailed:     return STRINGIFY(GameMessageType::HostGameFailed);
	case GameMessageType::OnlineDisconnect:   return STRINGIFY(GameMessageType::OnlineDisconnect);
	case GameMessageType::MatchmakingFailed:  return STRINGIFY(GameMessageType::MatchmakingFailed);
	case GameMessageType::JoinGameFailed:     return STRINGIFY(GameMessageType::JoinGameFailed);
	case GameMessageType::JoinedGameComplete: return STRINGIFY(GameMessageType::JoinedGameComplete);
	case GameMessageType::LeaveGameComplete:  return STRINGIFY(GameMessageType::LeaveGameComplete);
	case GameMessageType::MPPrivilegeError:   return STRINGIFY(GameMessageType::MPPrivilegeError);
	case GameMessageType::RegionLatency:      return STRINGIFY(GameMessageType::RegionLatency);
	case GameMessageType::MigrateRegion:      return STRINGIFY(GameMessageType::MigrateRegion);
	case GameMessageType::ServerUpdateWorldData:    return STRINGIFY(GameMessageType::ServerUpdateWorldData);
	default:                                  return STRINGIFY(GameMessageType::Unknown);
	}
}

void PlayFabOnlineManager::SendGameMessage(const GameMessage& message)
{
	DEBUGLOG("Sending message: %s\n", MessageTypeString(message.MessageType()));
	auto packet = message.Serialize();
	Managers::Get<OnlineManager>()->m_playfabParty.SendGameMessage(packet);
}

void PlayFabOnlineManager::ProcessGameNetworkMessage(std::string sourceId, GameMessage* message)
{
	std::string localId = Managers::Get<OnlineManager>()->m_playfabParty.GetLocalUserEntityId();
	std::unique_ptr<World>& world = g_game->GetWorld();
	std::shared_ptr<PlayerState> player = g_game->GetPlayerState(sourceId);

	switch (message->MessageType())
	{
	case GameMessageType::MPPrivilegeError:
	{
		Managers::Get<ScreenManager>()->ShowError("Your account does not have multiplayer privileges");
		break;
	}
	case GameMessageType::RegionLatency:
	{
		DEBUGLOG("Received a RegionLatency message\n");
		if (player != nullptr)
		{
			std::string raw = message->StringValue();
			std::string region = raw.substr(0, raw.find(":"));
			uint64_t latency = std::strtoull(raw.substr(raw.find(":") + 1).c_str(), nullptr, 10);

			player->SetRegionLatency(region, latency);

			DEBUGLOG("Received RegionLatency from %s: %s - %d\n", player->DisplayName.c_str(), region.c_str(), latency);
		}
		break;
	}
	case GameMessageType::MigrateRegion:
	{
		DEBUGLOG("Received a MigrateRegion message\n");
		Managers::Get<GameStateManager>()->SwitchToState(GameState::MigratingNetwork);
		break;
	}
	case GameMessageType::GameOver:
	{
		DEBUGLOG("Received a GameOver message\n");
		if (world->IsInitialized())
		{
			std::shared_ptr<PlayerState> localPlayer = g_game->GetPlayerState(localId);
			if (localPlayer->InGame)
			{
				world->IsGameWon = true;
				world->SetGameInProgress(false);
				world->DeserializeGameOver(message->RawData());
			}
			else
			{
				g_game->ResetGameplayData();
			}
		}
		break;
	}
	case GameMessageType::GameStart:
	{
		DEBUGLOG("Received a GameStart message\n");
		std::shared_ptr<PlayerState> localPlayer = g_game->GetPlayerState(localId);
		if (!localPlayer->InGame)
		{
			world->SetGameInProgress(true);
		}
		break;
	}
	case GameMessageType::ServerUpdateWorldData:
	{
		DEBUGLOG("Received a ServerUpdateWorldData message\n");
		if (world->IsInitialized())
		{
			world->DeserializeWorldData(message->RawData());
		}
		else
		{
			DEBUGLOG("World not initialized!\n");
		}
		break;
	}
	case GameMessageType::PlayerJoined:
	{
		DEBUGLOG("Received a PlayerJoined message\n");
		auto& peers = g_game->GetPeers();
		const auto& itr = peers.find(sourceId);
		auto entityid = sourceId;
		if (itr == peers.end())
		{
			auto playerState = std::make_shared<PlayerState>();
			playerState->DeserializePlayerStateData(message->RawData());

			peers[sourceId] = playerState;

			Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(
				GameMessageType::PlayerState,
				g_game->GetLocalPlayerState()->SerializePlayerStateData()
			));
			Managers::Get<OnlineManager>()->m_isJoiningArrangedLobby = false;
		}
		break;
	}
	case GameMessageType::SynPlayerData:
	{
		DEBUGLOG("Received a SynPlayerData message\n");
		if (sourceId == localId)
		{
			break;
		}
		if (player != nullptr)
		{
			player->DeserializePlayerStateData(message->RawData());
		}
		else
		{
			DEBUGLOG("No PlayerState exists for peer %u!\n", sourceId);
		}
		break;
	}
	case GameMessageType::PlayerState:
	{
		DEBUGLOG("Received a PlayerState message from %u\n", sourceId);
		if (player != nullptr)
		{
			player->DeserializePlayerStateData(message->RawData());
		}
		else
		{
			player = std::make_shared<PlayerState>();
			player->DeserializePlayerStateData(message->RawData());
			g_game->AddPlayerToLobbyPeers(player);
		}
		break;
	}
	case GameMessageType::PowerUpSpawn:
	{
		DEBUGLOG("Received a PowerUpSpawn message\n");
		if (world->IsInitialized())
		{
			world->DeserializePowerUpSpawn(message->RawData());
		}
		else
		{
			DEBUGLOG("World not initialized!\n");
		}
		break;
	}
	case GameMessageType::ShipData:
	{
		DEBUGLOG("Received a ShipData message\n");
		if (world->IsInitialized())
		{
			if (player != nullptr)
			{
				player->GetShip()->Deserialize(message->RawData());
			}
		}
		else
		{
			DEBUGLOG("World not initialized!\n");
		}
		break;
	}
	case GameMessageType::ShipDeath:
	{
		DEBUGLOG("Received a ShipDeath message from %u\n", sourceId);
		if (world->IsInitialized())
		{
			if (player != nullptr)
			{
				world->DeserializeShipDeath(sourceId, message->RawData());
			}
			else
			{
				DEBUGLOG("PlayerState not found for %u\n", sourceId);
			}
		}
		else
		{
			DEBUGLOG("World not initialized!\n");
		}
		break;
	}
	case GameMessageType::ShipInput:
	{
		if (world->IsInitialized())
		{
			if (player != nullptr)
			{
				player->GetShip()->Deserialize(message->RawData());
			}
		}
		else
		{
			DEBUGLOG("World not initialized!\n");
		}
		break;
	}
	case GameMessageType::ShipSpawn:
	{
		DEBUGLOG("Received a ShipSpawn message\n");
		if (world->IsInitialized())
		{
			world->DeserializeShipSpawn(message->RawData());
		}
		else
		{
			DEBUGLOG("World not initialized!\n");
		}
		break;
	}
	case GameMessageType::WorldData:
	{
		DEBUGLOG("Received a WorldData message\n");
		if (world->IsInitialized())
		{
			world->DeserializeWorldData(message->RawData());
		}
		else
		{
			DEBUGLOG("World not initialized!\n");
		}
		break;
	}
	case GameMessageType::WorldSetup:
	{
		DEBUGLOG("Received a WorldSetup message\n");
		if (!world->IsInitialized())
		{
			world->DeserializeWorldSetup(message->RawData());
		}
		else
		{
			DEBUGLOG("World is already initialized!\n");
		}
		break;
	}
	case GameMessageType::HostGameFailed:
	{
		DEBUGLOG("Received a HostGameFailed message\n");
		break;
	}
	case GameMessageType::JoiningGame:
	{
		DEBUGLOG("Received a JoiningGame message\n");
		Managers::Get<GameStateManager>()->SwitchToState(GameState::JoinGameFromLobby);
		break;
	}
	case GameMessageType::MatchmakingFailed:
	{
		DEBUGLOG("Received a MatchmakingFailed message\n");
		Managers::Get<ScreenManager>()->ShowError("Failed to matchmaking. Returning to main menu.", []() {
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
			});
		break;
	}
	case GameMessageType::JoinGameFailed:
	{
		DEBUGLOG("Received a JoinGameFailed message\n");
		Managers::Get<ScreenManager>()->ShowError("Failed to join game. Returning to main menu.", []() {
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
			});
		break;
	}
	case GameMessageType::OnlineDisconnect:
	{
		DEBUGLOG("Received a OnlineDisconnect message\n");
		GameState state = Managers::Get<GameStateManager>()->GetState();
		if (state != GameState::LeavingToMainMenu && state != GameState::MainMenu)
		{
			Managers::Get<ScreenManager>()->ShowError("Lost online connection. Returning to main menu.", []() {
				Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
				});
		}
		else if (state == GameState::LeavingToMainMenu)
		{
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
		}
		break;
	}
	case GameMessageType::JoinedGameComplete:
	{
		DEBUGLOG("Received a JoinedGameComplete message\n");
		Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);
		break;
	}
	case GameMessageType::MatchmakingCanceled:
	{
		DEBUGLOG("Received a MatchmakingCanceled message\n");
		Managers::Get<GameStateManager>()->SwitchToState(GameState::LeavingToMainMenu);
		Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
		break;
	}
	case GameMessageType::LeaveGameComplete:
	{
		DEBUGLOG("Received a LeaveGameComplete message\n");
		// Remove all remote peers
		std::vector<std::string> remotePlayers;
		auto& peers = g_game->GetPeers();
		for (auto&& [id, playerState] : peers)
		{
			if (!playerState->IsLocalPlayer)
			{
				remotePlayers.push_back(id);
			}
			else
			{
				g_game->ResetGameplayData();
			}
		}
		for (auto& id : remotePlayers)
		{
			peers.erase(id);
		}

		if (Managers::Get<OnlineManager>()->IsHost())
		{
			Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
			Managers::Get<OnlineManager>()->SetHost(false);
		}
		break;
	}
	default:
		break;
	}
}

bool PlayFabOnlineManager::IsConnected() const
{
	return Managers::Get<OnlineManager>()->m_playfabParty.IsConnected();
}

std::string PlayFabOnlineManager::GetNetworkId() const
{
	return m_networkId;
}

void PlayFabOnlineManager::RegisterOnlineMessageHandler(OnlineMessageHandler handler)
{
	m_messageHandler = handler;

	Managers::Get<OnlineManager>()->m_playfabParty.SetGameMessageHandler(
		[this](std::string uid, std::shared_ptr<GameMessage> message)
		{
			DEBUGLOG("Received message: %s\n", MessageTypeString(message->MessageType()));
			m_messageHandler(uid, message.get());
		});
}
