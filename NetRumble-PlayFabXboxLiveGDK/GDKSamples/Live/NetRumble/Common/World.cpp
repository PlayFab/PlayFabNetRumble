//--------------------------------------------------------------------------------------
// World.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

World::World()
{
	m_isGameInProgress = false;
	WinningScore = 5;

	ResetDefaults();

	m_starfield = std::make_unique<Starfield>(SimpleMath::Vector2());

	// Set outer barrier and world dimensions
	m_outerBarrierCounts = XMINT2(c_barrierCount, c_barrierCount);
	m_worldDimensions.left = c_barrierSize;
	m_worldDimensions.right = m_outerBarrierCounts.x * c_barrierSize;
	m_worldDimensions.top = c_barrierSize;
	m_worldDimensions.bottom = m_outerBarrierCounts.y * c_barrierSize;

	// Load world barrier textures
	ContentManager* contentManager = Managers::Get<ContentManager>();
	m_cornerBarrierTexture = contentManager->LoadTexture(L"Assets\\Textures\\barrierEnd.png");
	m_horizontalBarrierTexture = contentManager->LoadTexture(L"Assets\\Textures\\barrierRed.png");
	m_verticalBarrierTexture = contentManager->LoadTexture(L"Assets\\Textures\\barrierPurple.png");

	// Initialize the CollisionManager
	CollisionManager* collisionManager = Managers::Get<CollisionManager>();
	collisionManager->SetDimensions(m_worldDimensions);
	m_worldDimensions.top = m_worldDimensions.left = 0;

	// Setup the collision version of the world barriers
	auto MakeRect = [](int x, int y, int w, int h)
	{
		RECT r = { x, y, x + w, y + h };
		return r;
	};

	collisionManager->Barriers().clear();
	collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.top, m_worldDimensions.right - m_worldDimensions.left, c_barrierSize)); // Top edge
	collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.bottom, m_worldDimensions.right - m_worldDimensions.left, c_barrierSize)); // Bottom edge
	collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.top, c_barrierSize, m_worldDimensions.bottom - m_worldDimensions.top)); // Left edge
	collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.right, m_worldDimensions.top, c_barrierSize, m_worldDimensions.bottom - m_worldDimensions.top)); // Right edge

	// Setup the rendering version of the world barriers
	m_cornerBarriers.clear();
	m_cornerBarriers.push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.top, 4 * c_barrierSize, 4 * c_barrierSize)); // Top-left corner
	m_cornerBarriers.push_back(MakeRect(m_worldDimensions.right, m_worldDimensions.top, 4 * c_barrierSize, 4 * c_barrierSize)); // Top-right corner
	m_cornerBarriers.push_back(MakeRect(m_worldDimensions.right, m_worldDimensions.bottom, 4 * c_barrierSize, 4 * c_barrierSize)); // Bottom-right corner
	m_cornerBarriers.push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.bottom, 4 * c_barrierSize, 4 * c_barrierSize)); // Bottom-left corner

	m_horizontalBarriers.clear();
	for (int i = 2; i < m_outerBarrierCounts.x - 1; i++)
	{
		m_horizontalBarriers.push_back(MakeRect(m_worldDimensions.left + c_barrierSize * i, m_worldDimensions.top, c_barrierSize, c_barrierSize)); // Top edge
		m_horizontalBarriers.push_back(MakeRect(m_worldDimensions.left + c_barrierSize * i, m_worldDimensions.top + m_worldDimensions.right - m_worldDimensions.left, c_barrierSize, c_barrierSize)); // Bottom edge
	}

	m_verticalBarriers.clear();
	for (int i = 2; i < m_outerBarrierCounts.y - 1; i++)
	{
		m_verticalBarriers.push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.top + c_barrierSize * i, c_barrierSize, c_barrierSize)); // Left edge
		m_verticalBarriers.push_back(MakeRect(m_worldDimensions.right, m_worldDimensions.top + c_barrierSize * i, c_barrierSize, c_barrierSize)); // Right edge
	}
}

// Clear the world of objects and reset default values
void World::ResetDefaults()
{
	m_asteroids.clear();

	m_isInitialized = false;
	m_updatesSinceWorldDataSent = 0;
	m_powerUp = nullptr;
	m_powerUpTimer = c_maximumPowerUpTimer;

	IsGameWon = false;
	WinnerName = "";
	WinningColor = Colors::White;

	if (g_game)
	{
		g_game->ClearPlayerScores();
	}

	Managers::Get<CollisionManager>()->Collection().ApplyPendingRemovals();
	Managers::Get<CollisionManager>()->Collection().clear();
}

// Generate the world, placing asteroids and all ships
void World::GenerateWorld()
{
	DEBUGLOG("World::GenerateWorld\n");
	// First reset world defaults from any prior game
	ResetDefaults();

	CollisionManager* collisionMgr = Managers::Get<CollisionManager>();

	// Initialize the ships, finding spawn points and resetting score
	for (const auto& playerState : g_game->GetAllPlayerStates())
	{
		if (playerState && playerState->LobbyReady)
		{
			std::shared_ptr<Ship> ship = playerState->GetShip();
			ship->Initialize(playerState->IsLocalPlayer);
			ship->Position = collisionMgr->FindSpawnPoint(ship.get(), ship->Radius);
		}
	}

	// Place the asteroids
	for (size_t i = 0; i < c_asteroids; ++i)
	{
		// Choose one of three radii and texture variations
		float radius = 32.0f;
		switch (RandomMath::RandomBetween(0, 2))
		{
		case 0:
			radius = 32.0f;
			break;
		case 1:
			radius = 60.0f;
			break;
		case 2:
			radius = 96.0f;
			break;
		}
		int variation = RandomMath::RandomBetween(0, Asteroid::c_Variations - 1);

		// Create the asteroid
		std::shared_ptr<Asteroid> asteroid = std::make_shared<Asteroid>(radius, variation);
		asteroid->Initialize();
		asteroid->Position = collisionMgr->FindSpawnPoint(asteroid.get(), radius);
		m_asteroids.push_back(asteroid);
	}

	std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
	if (localPlayerState)
	{
		m_starfield->Reset(localPlayerState->GetShip()->Position);
	}

	m_isInitialized = true;
}

std::vector<uint8_t> World::SerializeShipSpawn(std::string entityId) const
{
	std::shared_ptr<PlayerState> playerState = g_game->GetPlayerState(entityId);
	if (playerState != nullptr)
	{
		std::shared_ptr<Ship> ship = playerState->GetShip();
		if (ship != nullptr)
		{
			SimpleMath::Vector2 spawnPt = Managers::Get<CollisionManager>()->FindSpawnPoint(ship.get(), ship->Radius);
			DataBufferWriter dataWriter;

			dataWriter.WriteString(entityId);
			dataWriter.WriteStruct(spawnPt);

			return dataWriter.GetBuffer();
		}
	}

	return std::vector<uint8_t>();
}

void World::DeserializeShipSpawn(const std::vector<uint8_t>& data)
{
	DataBufferReader dataReader(data);

	std::string entityId = dataReader.ReadString();
	float x = dataReader.ReadSingle();
	float y = dataReader.ReadSingle();
	SimpleMath::Vector2 position = SimpleMath::Vector2(x, y);

	DEBUGLOG("Received entityId %s at (%f, %f)\n", entityId.c_str(), position.x, position.y);

	std::shared_ptr<PlayerState> playerState = g_game->GetPlayerState(entityId);
	if (playerState != nullptr)
	{
		std::shared_ptr<Ship> ship = playerState->GetShip();
		if (ship != nullptr)
		{
			ship->Position = position;
			ship->Initialize(playerState->IsLocalPlayer);
		}
	}
}

std::vector<unsigned char> World::SerializePowerUpSpawn() const
{
	DataBufferWriter dataWriter;

	dataWriter.WriteByte(static_cast<byte>(PowerUp::ChooseNextPowerUpType()));

	XMFLOAT2 spawnPt = Managers::Get<CollisionManager>()->FindSpawnPoint(nullptr, 50.0f);
	dataWriter.WriteStruct(spawnPt);

	return dataWriter.GetBuffer();
}

void World::DeserializePowerUpSpawn(const std::vector<uint8_t>& data)
{
	DataBufferReader dataReader(data);

	PowerUpType powerUpType = static_cast<PowerUpType>(dataReader.ReadByte());
	SimpleMath::Vector2 position;
	dataReader.ReadStruct(position);
	SpawnPowerUp(powerUpType, position);
}

// Prepare the world data for the ServerUpdateWorldData packet
std::vector<unsigned char> World::SerializeWorldData() const
{
	DataBufferWriter dataWriter;

	// Write the asteroid data
	for (size_t i = 0; i < c_asteroids; ++i)
	{
		dataWriter.WriteStruct(m_asteroids[i]->Position);
		dataWriter.WriteStruct(m_asteroids[i]->Velocity);
	}

	return dataWriter.GetBuffer();
}

void World::DeserializeWorldData(const std::vector<uint8_t>& data)
{
	DataBufferReader dataReader(data);

	// Update the asteroid data
	for (size_t i = 0; i < c_asteroids; ++i)
	{
		dataReader.ReadStruct(m_asteroids[i]->Position);
		dataReader.ReadStruct(m_asteroids[i]->Velocity);
	}
}

// Prepare the member ships and world data for the ServerWorldSetup packet
std::vector<uint8_t> World::SerializeWorldSetup(std::map<std::string, std::shared_ptr<Ship>>& ships) const
{
	DataBufferWriter dataWriter;

	dataWriter.WriteUInt32(static_cast<uint32_t>(ships.size()));
	// Write active ship data
	for (auto& activeShipPair : ships)
	{
		std::string entityId = activeShipPair.first;
		std::shared_ptr<Ship> ship = activeShipPair.second;

		dataWriter.WriteString(entityId);
		dataWriter.WriteStruct(ship->Position);

		WeaponType currentWeaponType = WeaponType::Unknown;
		if (ship->PrimaryWeapon != nullptr)
		{
			currentWeaponType = ship->PrimaryWeapon->GetWeaponType();
		}
		dataWriter.WriteByte(static_cast<uint8_t>(currentWeaponType));

		DEBUGLOG("SerializeWorldSetup() sending entityId %s at (%f, %f) with weapon %u\n", entityId.c_str(), ship->Position.x, ship->Position.y, currentWeaponType);
	}
	// Write the asteroid data
	for (size_t i = 0; i < c_asteroids; ++i)
	{
		dataWriter.WriteSingle(m_asteroids[i]->Radius);
		dataWriter.WriteByte(static_cast<uint8_t>(m_asteroids[i]->Variation));
		dataWriter.WriteStruct(m_asteroids[i]->Position);
		dataWriter.WriteStruct(m_asteroids[i]->Velocity);
	}
	// Write the powerUp data
	if (m_powerUp != nullptr)
	{
		dataWriter.WriteByte(static_cast<uint8_t>(m_powerUp->GetPowerUpType()));
		dataWriter.WriteStruct(m_powerUp->Position);
	}
	else
	{
		dataWriter.WriteByte(static_cast<uint8_t>(PowerUpType::Unknown));
		dataWriter.WriteSingle(0.0f);
		dataWriter.WriteSingle(0.0f);
	}
	// Write the game mode and winning score (redundant with the GameSettings packet, but that packet isn't sent to those who join in progress)
	dataWriter.WriteInt32(WinningScore);

	return dataWriter.GetBuffer();
}

void World::DeserializeWorldSetup(const std::vector<uint8_t>& data)
{
	DataBufferReader dataReader(data);

	// First reset world defaults from any prior game
	ResetDefaults();

	// Read the members' ship data
	uint32_t memberSize = dataReader.ReadUInt32();
	for (uint32_t i = 0; i < memberSize; ++i)
	{
		std::string entityId = dataReader.ReadString();

		SimpleMath::Vector2 position;
		dataReader.ReadStruct(position);

		WeaponType currentWeaponType = static_cast<WeaponType>(dataReader.ReadByte());

		DEBUGLOG("DeserializeWorldSetup() received entityId %s at (%f, %f) with weapon %u\n", entityId.c_str(), position.x, position.y, currentWeaponType);

		std::shared_ptr<PlayerState> playerState = g_game->GetPlayerState(entityId);
		if (playerState != nullptr)
		{
			std::shared_ptr<Ship> ship = playerState->GetShip();

			ship->Initialize(playerState->IsLocalPlayer);
			ship->Position = position;

			switch (currentWeaponType)
			{
			case WeaponType::Unknown:
				break;
			case WeaponType::Laser:
				ship->PrimaryWeapon = std::make_shared<LaserWeapon>(ship.get());
				break;
			case WeaponType::DoubleLaser:
				ship->PrimaryWeapon = std::make_shared<DoubleLaserWeapon>(ship.get());
				break;
			case WeaponType::TripleLaser:
				ship->PrimaryWeapon = std::make_shared<TripleLaserWeapon>(ship.get());
				break;
			case WeaponType::Rocket:
				ship->PrimaryWeapon = std::make_shared<RocketWeapon>(ship.get());
				break;
			case WeaponType::Mine:
				ship->PrimaryWeapon = std::make_shared<MineWeapon>(ship.get());
				break;
			default:
				break;
			}
		}
	}

	// Read the asteroid data
	for (size_t i = 0; i < c_asteroids; ++i)
	{
		float radius = dataReader.ReadSingle();
		m_asteroids.push_back(std::make_shared<Asteroid>(radius, dataReader.ReadByte()));
		m_asteroids[i]->Initialize();
		dataReader.ReadStruct(m_asteroids[i]->Position);
		dataReader.ReadStruct(m_asteroids[i]->Velocity);
	}

	// Read the powerUp data
	PowerUpType powerUpType = static_cast<PowerUpType>(dataReader.ReadByte());
	SimpleMath::Vector2 position;
	dataReader.ReadStruct(position);
	SpawnPowerUp(powerUpType, position);

	// Read the game mode and winning score
	WinningScore = dataReader.ReadInt32();

	std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
	if (localPlayerState && localPlayerState->GetShip())
	{
		m_starfield->Reset(localPlayerState->GetShip()->Position);
	}

	m_isInitialized = true;
}

std::vector<uint8_t> World::SerializeShipDeath(std::shared_ptr<Ship> localShip) const
{
	if (localShip)
	{
		DataBufferWriter dataWriter;

		GameplayObject* lastDamagedBy = localShip->LastDamagedBy;
		std::string killer = "";
		if (lastDamagedBy != nullptr &&
			lastDamagedBy->GetType() == GameplayObjectType::Ship &&
			lastDamagedBy != localShip.get())
		{
			for (const auto& playerState : g_game->GetAllPlayerStates())
			{
				if (playerState && playerState->InGame)
				{
					std::shared_ptr<Ship> ship = playerState->GetShip();
					if (ship && ship.get() == lastDamagedBy)
					{
						killer = playerState->EntityId;
						break;
					}
				}
			}
		}
		dataWriter.WriteString(killer);

		return dataWriter.GetBuffer();
	}

	return std::vector<unsigned char>();
}

void World::DeserializeShipDeath(std::string entityId, const std::vector<uint8_t>& data)
{
	DEBUGLOG("DeserializeShipDeath() received for entityId %s\n", entityId.c_str());

	std::shared_ptr<PlayerState> playerState = g_game->GetPlayerState(entityId);
	if (playerState == nullptr)
	{
		return;
	}

	std::shared_ptr<Ship> shipKilled = playerState->GetShip();
	if (shipKilled == nullptr)
	{
		return;
	}

	DataBufferReader dataReader(data);

	std::shared_ptr<Ship> killerShip = nullptr;
	std::string killerid = dataReader.ReadString();
	if (!killerid.empty())
	{
		std::shared_ptr<PlayerState> killerState = g_game->GetPlayerState(killerid);
		if (killerState)
		{
			killerShip = killerState->GetShip();
		}
	}

	shipKilled->Die(killerShip.get(), false);
}

std::vector<unsigned char> World::SerializeGameOver() const
{
	DataBufferWriter dataWriter;

	dataWriter.WriteStruct(WinningColor);
	dataWriter.WriteString(WinnerName);

	return dataWriter.GetBuffer();
}

void World::DeserializeGameOver(const std::vector<uint8_t>& data)
{
	DataBufferReader dataReader(data);

	dataReader.ReadStruct(WinningColor);

	WinnerName = dataReader.ReadString();

	DEBUGLOG("DeserializeGameOver() received with winner %ws and color (%f, %f, %f, %f)\n", WinnerName.c_str(), WinningColor.f[0], WinningColor.f[1], WinningColor.f[2], WinningColor.f[3]);
}

void World::Update(float totalTime, float elapsedTime)
{
	UNREFERENCED_PARAMETER(totalTime);

	if (!IsGameWon)
	{
		int highScore = MININT;
		std::string highScoreName = "";
		DirectX::XMVECTORF32 highScoreColor = Colors::White;

		std::vector<std::shared_ptr<PlayerState>> playerStates = g_game->GetAllPlayerStates();

		if (playerStates.size() < 0)
		{
			highScore = WinningScore;
			highScoreName = "Insufficient Players";
			highScoreColor = DirectX::Colors::DarkOrange;
		}
		else
		{
			for (const auto& playerState : playerStates)
			{
				if (playerState && playerState->InGame)
				{
					std::shared_ptr<Ship> ship = playerState->GetShip();
					if (!ship)
					{
						continue;
					}

					// Get current high score (and scorer)
					if (ship->Score > highScore)
					{
						highScore = ship->Score;
						highScoreName = playerState->DisplayName;
						highScoreColor = ship->Color;
					}

					// Respawn players
					if (!ship->Active() && ship->RespawnTimer <= 0.0f)
					{
						// Send ship spawn message and immediately process locally
						if (playerState->EntityId == Managers::Get<OnlineManager>()->GetLocalEntityId())
						{
							std::vector<uint8_t> messageData = SerializeShipSpawn(playerState->EntityId);
							Managers::Get<OnlineManager>()->SendGameMessage(
								GameMessage(
									GameMessageType::ShipSpawn,
									messageData
								)
							);
							DeserializeShipSpawn(messageData);
						}
					}
				}
			}
		}

		// Check if game has been won
		if (highScore >= WinningScore)
		{
			DEBUGLOG("Game over\n");

			WinnerName = highScoreName;
			WinningColor = highScoreColor;

			Managers::Get<OnlineManager>()->SendGameMessage(
				GameMessage(
					GameMessageType::GameOver,
					SerializeGameOver()
				)
			);

			SetGameInProgress(false);
			IsGameWon = true;
		}
	}
	// Update all player ships based on last input
	for (const auto& playerState : g_game->GetAllPlayerStates())
	{
		if (playerState && playerState->InGame)
		{
			std::shared_ptr<Ship> ship = playerState->GetShip();
			if (ship)
			{
				if (ship->Active())
				{
					ship->Update(elapsedTime);

					// Check for ship death
					// Server is authority on death of own ship
					if (playerState->IsLocalPlayer && ship->Life < 0 && !IsGameWon)
					{
						// Send local ship death message and immediately process locally
						std::vector<uint8_t> messageData = SerializeShipDeath(ship);
						Managers::Get<OnlineManager>()->SendGameMessage(
							GameMessage(
								GameMessageType::ShipDeath,
								messageData
							)
						);
						DeserializeShipDeath(playerState->EntityId, messageData);
					}
				}
				else if (ship->RespawnTimer > 0.0f)
				{
					// Everyone (not just the host) updates all respawn timers so we can display respawn countdown in HUD
					ship->RespawnTimer -= elapsedTime;
				}
			}
		}
	}

	// Update all asteroids
	for (auto& asteroid : m_asteroids)
	{
		if (asteroid->Active())
		{
			asteroid->Update(elapsedTime);
		}
	}

	// Update the power-up
	if (m_powerUp != nullptr)
	{
		if (m_powerUp->Active())
		{
			m_powerUp->Update(elapsedTime);
		}
		else
		{
			m_powerUp = nullptr;
		}
	}

	// Update collision manager to apply all physics
	Managers::Get<CollisionManager>()->Update(elapsedTime);

	// Update the particle-effect manager
	Managers::Get<ParticleEffectManager>()->Update(elapsedTime);

	if (Managers::Get<OnlineManager>()->IsHost())
	{
		Managers::Get<OnlineManager>()->SendGameMessage(
			GameMessage(
				GameMessageType::ServerUpdateWorldData,
				SerializeWorldData()
			)
		);
	}
}

void World::Draw(float elapsedTime) const
{
	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
	std::shared_ptr<Ship> localShip = g_game->GetLocalPlayerState()->GetShip();

	XMFLOAT2 center = XMFLOAT2(localShip->Position.x - viewportWidth / 2.0f, localShip->Position.y - viewportHeight / 2.0f);

	// Pull the center inwards so that it doesn't show a ton of the space outside the game
	center.x = std::min(std::max(center.x, m_worldDimensions.left - c_visualPadding), m_worldDimensions.right - viewportWidth + c_visualPadding);
	center.y = std::min(std::max(center.y, m_worldDimensions.top - c_visualPadding), m_worldDimensions.bottom - viewportHeight + c_visualPadding);

	// Draw the starfield
	m_starfield->Draw(center);

	if (m_isInitialized)
	{
		std::unique_ptr<RenderContext> renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::NonPremultiplied);
		XMMATRIX transform = XMMatrixTranslation(-center.x, -center.y, 0);

		renderContext->Begin(transform);

		// Draw the barriers
		float rotation = 0;
		for (auto& corner : m_cornerBarriers)
		{
			renderContext->Draw(m_cornerBarrierTexture, corner, DirectX::Colors::White, rotation);
			rotation += DirectX::XM_PIDIV2;
		}
		std::for_each(m_horizontalBarriers.begin(), m_horizontalBarriers.end(), [&](RECT r) { renderContext->Draw(m_horizontalBarrierTexture, r); });
		std::for_each(m_verticalBarriers.begin(), m_verticalBarriers.end(), [&](RECT r) { renderContext->Draw(m_verticalBarrierTexture, r); });

		// Draw the powerup
		if (m_powerUp != nullptr && m_powerUp->Active())
		{
			m_powerUp->Draw(elapsedTime, renderContext.get());
		}

		// Draw the asteroids
		for (auto& asteroid : m_asteroids)
		{
			if (asteroid->Active())
			{
				asteroid->Draw(elapsedTime, renderContext.get());
			}
		}

		// Draw each ship
		for (const auto& playerState : g_game->GetAllPlayerStates())
		{
			if (playerState && playerState->InGame)
			{
				std::shared_ptr<Ship> ship = playerState->GetShip();
				if (ship && ship->Active())
				{
					ship->Draw(elapsedTime, renderContext.get(), false);
				}
			}
		}

		// Draw the alpha-blended particles
		Managers::Get<ParticleEffectManager>()->Draw(renderContext.get(), SpriteBlendMode::Alpha);

		renderContext->End();

		renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::Additive);

		// Draw the additive particles
		renderContext->Begin(transform, SpriteSortMode::SpriteSortMode_Texture);

		Managers::Get<ParticleEffectManager>()->Draw(renderContext.get(), SpriteBlendMode::Additive);

		renderContext->End();
	}
}

void World::SpawnPowerUp(PowerUpType type, const DirectX::SimpleMath::Vector2& position)
{
	switch (type)
	{
	case PowerUpType::DoubleLaser:
		m_powerUp = std::make_shared<DoubleLaserPowerUp>();
		break;
	case PowerUpType::TripleLaser:
		m_powerUp = std::make_shared<TripleLaserPowerUp>();
		break;
	case PowerUpType::Rocket:
		m_powerUp = std::make_shared<RocketPowerUp>();
		break;
	default:
		m_powerUp = nullptr;
		break;
	}

	if (m_powerUp != nullptr)
	{
		m_powerUp->Position = position;
		m_powerUp->Initialize();
	}
}
