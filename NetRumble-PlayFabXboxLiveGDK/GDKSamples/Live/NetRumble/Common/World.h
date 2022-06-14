//--------------------------------------------------------------------------------------
// World.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace NetRumble
{
	struct ShipSerialization
	{
		float xPos;
		float yPos;
		uint8_t weapon;
	};

	struct AsteroidSerialization
	{
		float radius;
		byte variation;
		float xPos;
		float yPos;
		float xVel;
		float yVel;
	};

	struct PowerUpSerialization
	{
		byte type;
		float xPos;
		float yPos;
	};

	class World final
	{
	public:
		World();

		// Clear the world of objects and reset default values
		void ResetDefaults();

		// Generate the world, placing asteroids and all ships
		void GenerateWorld();

		// Prepare the member ships and world data for the ServerWorldSetup packet
		std::vector<uint8_t> SerializeWorldSetup(std::map<std::string, std::shared_ptr<Ship>>& ships) const;

		// Initialize the member ships and world with the data from the ServerWorldSetup packet
		void DeserializeWorldSetup(const std::vector<uint8_t>& data);

		// Prepare the world data for the ServerUpdateWorldData packet
		std::vector<uint8_t> SerializeWorldData() const;

		// Update the world with the data from the ServerUpdateWorldData packet
		void DeserializeWorldData(const std::vector<uint8_t>& data);

		// Serialize powerUp spawn packet
		std::vector<uint8_t> SerializePowerUpSpawn() const;

		// Handle powerUp spawn packet
		void DeserializePowerUpSpawn(const std::vector<uint8_t>& data);

		// Serialize a suitable ship spawn point for the indicated player
		std::vector<uint8_t> SerializeShipSpawn(std::string entityId) const;

		// Spawn ship for indicated player
		void DeserializeShipSpawn(const std::vector<uint8_t>& data);

		// Prepare local ship death packet
		std::vector<uint8_t> SerializeShipDeath(std::shared_ptr<Ship> localShip) const;

		// Handle ship death packet
		void DeserializeShipDeath(std::string entityId, const std::vector<uint8_t>& data);

		// Serialize game over packet
		std::vector<uint8_t> SerializeGameOver() const;

		// Handle game over packet
		void DeserializeGameOver(const std::vector<uint8_t>& data);

		inline bool IsGameInProgress() const { return m_isGameInProgress; }
		inline bool IsInitialized() const { return m_isInitialized; }
		inline void SetGameInProgress(bool isGameInProgress) { m_isGameInProgress = isGameInProgress; }
		inline void SetInitialized(bool isInitialized) { m_isInitialized = isInitialized; }

		void Update(float totalTime, float elapsedTime);
		void Draw(float elapsedTime) const;

		bool IsGameWon;
		std::string WinnerName;
		DirectX::XMVECTORF32 WinningColor;
		int WinningScore;

		static constexpr int c_barrierCount = 50;           // Default number of edge barriers per side
		static constexpr int c_barrierSize = 48;            // Default size of each edge barrier
		static constexpr float c_visualPadding = 150.0f;    // Distance to pull the center inwards so we don't see a large amount of outside space when local ship is near an edge
		static constexpr size_t c_asteroids = 15;           // Default number of asteroids

		// The length of time it takes for another power-up to spawn.
		static constexpr float c_maximumPowerUpTimer = 10.0f;

	private:
		void SpawnPowerUp(PowerUpType type, const DirectX::SimpleMath::Vector2& position);

		bool m_isGameInProgress;
		bool m_isInitialized;
		float m_powerUpTimer;
		int m_updatesSinceWorldDataSent;

		// World contents
		RECT m_worldDimensions;
		DirectX::XMINT2 m_outerBarrierCounts;
		std::vector<RECT> m_cornerBarriers;
		std::vector<RECT> m_horizontalBarriers;
		std::vector<RECT> m_verticalBarriers;
		std::unique_ptr<Starfield> m_starfield;
		std::vector<std::shared_ptr<Asteroid>> m_asteroids;
		std::shared_ptr<PowerUp> m_powerUp;

		// World textures
		TextureHandle m_cornerBarrierTexture;
		TextureHandle m_horizontalBarrierTexture;
		TextureHandle m_verticalBarrierTexture;
	};
}
