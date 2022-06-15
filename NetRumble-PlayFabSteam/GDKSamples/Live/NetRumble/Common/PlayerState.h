//--------------------------------------------------------------------------------------
// PlayerState.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
	const int MaxSteamUserNameLength = 32;
	const int MaxEntityIdLength = 256;

	struct PlayerStateData
	{
		bool inGame;
		bool inLobby;
		bool lobbyReady;
		byte shipColorIndex;
		byte shipVariation;
		char displayName[MaxSteamUserNameLength];	// Steam user name max length only has 32 bytes
		char entityId[MaxEntityIdLength];
	};

	class PlayerState final
	{
	public:
		PlayerState(std::string_view displayName = "");

		// Lobby functions
		void DeactivatePlayer();
		void EnterLobby();
		void ReactivatePlayer();

		void DeserializePlayerStateData(const std::vector<uint8_t>& data);
		std::vector<uint8_t> SerializePlayerStateData() const;

		void SetRegionLatency(std::string_view region, uint64_t latency);
		std::vector<std::pair<std::string, uint64_t>> GetRegionLatencyList() const { return m_latencyList; }

		// State properties
		std::string DisplayName;
		bool IsLocalPlayer = false;
		bool InGame;
		bool InLobby;
		bool LobbyReady;
		uint64_t PeerId;
		std::string EntityId;

		byte ShipColor() const { return m_shipColor; }
		void ShipColor(byte colorIndex)
		{
			if (colorIndex >= Ship::Colors.size())
			{
				colorIndex = 0;
			}
			m_shipColor = colorIndex;
			if (m_playerShip)
			{
				m_playerShip->Color = Ship::Colors[colorIndex];
			}
		}

		byte ShipVariation() const { return m_shipVariation; }
		void ShipVariation(byte variation)
		{
			if (variation >= Ship::MaxVariations)
			{
				variation = 0;
			}
			m_shipVariation = variation;
			if (m_playerShip)
			{
				m_playerShip->SetShipTexture(variation);
			}
		}

		bool IsInactive() const { return m_isInactive; }

		std::shared_ptr<Ship> GetShip() const { return m_playerShip; }

	private:
		std::shared_ptr<Ship> m_playerShip;
		byte m_shipColor = 0;
		byte m_shipVariation = 0;
		bool m_isInactive = false;
		std::vector<std::pair<std::string, uint64_t>> m_latencyList;
	};

}
