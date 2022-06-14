//--------------------------------------------------------------------------------------
// PlayerState.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

PlayerState::PlayerState(std::string_view displayName)
{
	m_playerShip = std::make_shared<Ship>();

	if (displayName.empty())
	{
		DisplayName = "Player";
	}
	else
	{
		DisplayName = displayName;
	}

	EnterLobby();
}

void PlayerState::DeactivatePlayer()
{
	m_isInactive = true;
	m_playerShip->Score = 0;
	EnterLobby();
}

void PlayerState::EnterLobby()
{
	if (InGame)
	{
		m_playerShip->Die(nullptr, true);
		m_playerShip->RespawnTimer = 0.0f;
		InGame = false;
		InLobby = true;
	}

	LobbyReady = false;
}

void PlayerState::ReactivatePlayer()
{
	m_isInactive = false;
}

void PlayerState::DeserializePlayerStateData(const std::vector<uint8_t>& data)
{
	PlayerStateData rcvdPlayerStateData = PlayerStateData();
	CopyMemory(&rcvdPlayerStateData, data.data(), sizeof(PlayerStateData));

	DEBUGLOG("Received player state data: DisplayName = %s; EntityId = %s; InGame = %u; InLobby = %u;LobbyReady = %u; ColorIndex = %u; ColorVariation = %u\n",
		rcvdPlayerStateData.displayName,
		rcvdPlayerStateData.entityId,
		rcvdPlayerStateData.inGame,
		rcvdPlayerStateData.inLobby,
		rcvdPlayerStateData.lobbyReady,
		rcvdPlayerStateData.shipColorIndex,
		rcvdPlayerStateData.shipVariation);

	if (rcvdPlayerStateData.displayName[MaxUserNameLength - 1] != '\0')
	{
		std::string nameBuffer(rcvdPlayerStateData.displayName, rcvdPlayerStateData.displayName + MaxUserNameLength);
		DisplayName = nameBuffer;
	}
	else
	{
		DisplayName = rcvdPlayerStateData.displayName;
	}
	if (rcvdPlayerStateData.entityId[MaxUserNameLength - 1] != '\0')
	{
		std::string nameBuffer(rcvdPlayerStateData.entityId, rcvdPlayerStateData.entityId + MaxUserNameLength);
		EntityId = nameBuffer;
	}
	else
	{
		EntityId = rcvdPlayerStateData.entityId;
	}

	InGame = rcvdPlayerStateData.inGame;
	InLobby = rcvdPlayerStateData.inLobby;
	LobbyReady = rcvdPlayerStateData.lobbyReady;
	ShipColor(rcvdPlayerStateData.shipColorIndex);
	ShipVariation(rcvdPlayerStateData.shipVariation);
}

std::vector<uint8_t> PlayerState::SerializePlayerStateData() const
{
	PlayerStateData playerStateData = { InGame, InLobby, LobbyReady, m_shipColor, m_shipVariation };

	CopyMemory(playerStateData.displayName, DisplayName.c_str(), DisplayName.length());
	CopyMemory(playerStateData.entityId, EntityId.c_str(), EntityId.length());

	DEBUGLOG("Serializing player state data: PlayerName = %s; EntityId = %s; InGame = %u; InLobby = %u; LobbyReady = %u; ColorIndex = %d; ColorVariation = %d\n",
		DisplayName.c_str(),
		EntityId.c_str(),
		InGame,
		InLobby,
		LobbyReady,
		m_shipColor,
		m_shipVariation);

	std::vector<uint8_t> data(sizeof(PlayerStateData));
	CopyMemory(data.data(), &playerStateData, sizeof(PlayerStateData));

	return data;
}

void PlayerState::SetRegionLatency(std::string_view region, uint64_t latency)
{
	auto it = std::find_if(
		std::begin(m_latencyList),
		std::end(m_latencyList),
		[&](const std::pair<std::string, uint64_t>& item)
		{
			return item.first == region;
		});

	if (it != std::end(m_latencyList))
	{
		(*it).second = latency;
	}
	else
	{
		m_latencyList.emplace_back(std::pair<std::string, uint64_t>{ region, latency });
	}
}
