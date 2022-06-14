//--------------------------------------------------------------------------------------
// NetworkMessages.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

GameMessage::GameMessage(GameMessageType type, uint32_t data) :
	m_type{ type }
{
	m_data.resize(sizeof(unsigned));
	memcpy(m_data.data(), &data, sizeof(data));
}

GameMessage::GameMessage(GameMessageType type, std::string_view data) :
	m_type{ type }
{
	m_data.resize(data.length() * sizeof(char));
	memcpy(m_data.data(), reinterpret_cast<const uint8_t*>(data.data()), m_data.size());
}

GameMessage::GameMessage(GameMessageType type, const std::vector<uint8_t>& data) :
	m_type{ type },
	m_data{ data }
{
}

GameMessage::GameMessage(const std::vector<uint8_t>& data)
{
	if (data.size() < (MsgTypeSize + sizeof(uint8_t)))
	{
		DEBUGLOG("Ill-formed game message\n");
		return;
	}

	memcpy(&m_type, data.data(), MsgTypeSize);

	m_data.resize(data.size() - MsgTypeSize);
	memcpy(m_data.data(), data.data() + MsgTypeSize, m_data.size());
}

std::vector<uint8_t> GameMessage::Serialize() const
{
	if (m_type == GameMessageType::Unknown || m_data.empty())
	{
		return std::vector<uint8_t>();
	}

	const size_t length = MsgTypeSize + m_data.size();
	uint8_t* buffer = new uint8_t[length];

	memcpy(buffer, &m_type, MsgTypeSize);
	memcpy(buffer + MsgTypeSize, m_data.data(), m_data.size());

	std::vector<uint8_t> packet = std::vector<uint8_t>(buffer, buffer + length);

	delete[] buffer;

	return packet;
}

std::vector<uint8_t> GameMessage::SerializeWithSourceID() const
{
	if (m_type == GameMessageType::Unknown || m_data.empty())
	{
		return std::vector<uint8_t>();
	}

	if (g_game->GetLocalPlayerState() == nullptr)
	{
		DEBUGLOG("Serialize GameMessage with source ID without having valid player state\n");
		return std::vector<uint8_t>();
	}

	const uint64_t sourceID = g_game->GetLocalPlayerState()->PeerId;
	const size_t sourceIDSize = sizeof(sourceID);

	const size_t length = MsgTypeSize + sourceIDSize + m_data.size();
	uint8_t* buffer = new uint8_t[length];

	// Serialized message data will be: GameMessageType|SourceID|MessagePayload
	memcpy(buffer, &m_type, MsgTypeSize);	// GameMessageType
	memcpy(buffer + MsgTypeSize, &sourceID, sizeof(sourceID)); // Source ID
	memcpy(buffer + MsgTypeSize + sourceIDSize, m_data.data(), m_data.size()); // Data payload

	std::vector<uint8_t> packet = std::vector<uint8_t>(buffer, buffer + length);

	delete[] buffer;

	return packet;
}

std::string GameMessage::StringValue() const
{
	if (!m_data.empty())
	{
		const char* stringData = reinterpret_cast<const char*>(m_data.data());
		return std::string(stringData, stringData + (m_data.size() / sizeof(char)));
	}

	return "";
}

uint32_t GameMessage::UnsignedValue() const
{
	if (!m_data.empty())
	{
		return *(reinterpret_cast<const uint32_t*>(m_data.data()));
	}

	return 0;
}

