//--------------------------------------------------------------------------------------
// SteamVoiceChat.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

namespace {
	static constexpr uint64 VOICE_CHAT_ALIVE_TICKS = 2500000;
}
using namespace NetRumble;

void SteamVoiceChat::Tick()
{
	if (!m_isActive)
	{
		return;
	}

	// Set the chat voice data message update rate to 1,000,000 ticks
	if (m_lastTimeTalked + MIN_VOICE_DATA_UPDATE_INTERVAL > g_game->GetGameTickCount())
	{
		return;
	}

	// Read local microphone input
	uint32 bytesAvailable = 0;
	EVoiceResult result = SteamUser()->GetAvailableVoice(&bytesAvailable, NULL, 0);

	if (result == k_EVoiceResultOK && bytesAvailable > 0)
	{
		uint32 bytesWritten = 0;
		MsgVoiceChatData_t voicChatMsg;

		// Don't send more then 1 KB at a time
		memset(m_uncompressedVoiceBuffer.get(), 0, 1024 + sizeof(MsgVoiceChatData_t));

		result = SteamUser()->GetVoice(true, m_uncompressedVoiceBuffer.get() + sizeof(voicChatMsg), 1024, &bytesWritten, false, NULL, 0, NULL, 0);

		if (result == k_EVoiceResultOK && bytesWritten > 0)
		{
			// Assemble message. Note that we don't fill in the SteamID here
			// The server will know who sent
			voicChatMsg.SetDataLength(bytesWritten);
			memcpy_s(m_uncompressedVoiceBuffer.get(), VIOCE_CHAT_BUFFER_SIZE, &voicChatMsg, sizeof(voicChatMsg));

			// Send a message to the server with the data, server will broadcast this data on to all other clients.
			SteamNetworkingSockets()->SendMessageToConnection(m_connectedServerHandle, m_uncompressedVoiceBuffer.get(), sizeof(voicChatMsg) + bytesWritten, k_nSteamNetworkingSend_UnreliableNoDelay, nullptr);

			m_lastTimeTalked = g_game->GetGameTickCount();

#ifdef _DEBUG
			// If local voice loopback is enabled, play it back now
			if (m_voiceLoopback != 0)
			{
				// Uncompress the voice data, buffer holds up to 1 second of data
				uint32 numUncompressedBytes = 0;
				const uint8* voiceData = static_cast<const uint8*>(m_uncompressedVoiceBuffer.get());
				voiceData += sizeof(MsgVoiceChatData_t);

				result = SteamUser()->DecompressVoice(voiceData, bytesWritten,
					m_uncompressedVoice.get(), VIOCE_CHAT_BUFFER_BYTES, &numUncompressedBytes, VOICE_OUTPUT_SAMPLE_RATE);

				if (result == k_EVoiceResultOK && numUncompressedBytes > 0)
				{

					AddVoiceChatData(m_voiceLoopback, m_uncompressedVoice.get(), numUncompressedBytes);
				}
			}
#endif
		}
	}
}

// Handle received voice data
void SteamVoiceChat::HandleVoiceChatData(const void* rawChatData)
{
	const MsgVoiceChatData_t* msgVoiceData = static_cast<const MsgVoiceChatData_t*>(rawChatData);
	CSteamID fromSteamID = msgVoiceData->GetSteamID();

	const auto& iter = m_playerVchatConnMap.find(fromSteamID.ConvertToUint64());
	DEBUGLOG("SteamVoiceChat::HandleVoiceChatData fromSteamID == %llu, mapSize == %d\n", fromSteamID.ConvertToUint64(), m_playerVchatConnMap.size());
	if (iter == m_playerVchatConnMap.end())
	{
		DEBUGLOG("The SteamID does not exist in the connection map");
		return;
	}

	VoiceChatConnection& chatClient = iter->second;
	chatClient.m_lastReceiveVoiceTime = g_game->GetGameTickCount();

	// Uncompress the voice data, buffer holds up to 1 second of data
	memset(m_uncompressedVoiceBuffer.get(), 0, VIOCE_CHAT_BUFFER_BYTES);
	uint32 numUncompressedBytes = 0;
	const uint8* voiceData = static_cast<const uint8*>(rawChatData);
	voiceData += sizeof(MsgVoiceChatData_t);

	EVoiceResult result = SteamUser()->DecompressVoice(voiceData, msgVoiceData->GetDataLength(),
		m_uncompressedVoiceBuffer.get(), VIOCE_CHAT_BUFFER_BYTES, &numUncompressedBytes, VOICE_OUTPUT_SAMPLE_RATE);

	if (result == k_EVoiceResultOK && numUncompressedBytes > 0)
	{
		if (chatClient.m_voiceChannel == 0)
		{
			chatClient.m_voiceChannel = CreateVoiceChannel();
		}

		AddVoiceChatData(chatClient.m_voiceChannel, m_uncompressedVoiceBuffer.get(), numUncompressedBytes);
	}
}

bool SteamVoiceChat::AddVoiceChatData(int channel, const uint8* data, const uint32 dataSize)
{
	const auto iter = m_voiceChannelMap.find(channel);
	if (iter == m_voiceChannelMap.end())
	{
		DEBUGLOG("Unable to AddVoiceChatData, receive voice data from unknown source, channel: %d\n", channel);
		// Channel not found
		return false; 
	}

	const auto& voiceContext = iter->second;

	// At this point we have a buffer full of audio and enough room to submit it, so
	// let's submit it and get another read request going.
	uint8* voiceChatDataBuffer = static_cast<uint8*>(malloc(dataSize));
	memcpy_s(voiceChatDataBuffer, dataSize, data, dataSize);

	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = dataSize;
	buffer.pAudioData = voiceChatDataBuffer;
	buffer.pContext = voiceChatDataBuffer; // The buffer is freed again in VoiceContext::OnBufferEnd

	if (voiceContext->m_sourceVoice->SubmitSourceBuffer(&buffer) < 0)
	{
		free(voiceChatDataBuffer);
		DEBUGLOG("Fail to SubmitSourceBuffer when AddVoiceChatData, channel: %d, dataSize: %d\n", channel, dataSize);
		return false;
	}

	return true;
}

void SteamVoiceChat::MarkAllPlayersInactive()
{
	for (auto iter = m_playerVchatConnMap.begin(); iter != m_playerVchatConnMap.end(); ++iter)
	{
		iter->second.m_active = false;
	}
}

void SteamVoiceChat::MarkPlayerAsActive(CSteamID steamID)
{
	if (!m_isActive)
	{
		DEBUGLOG("Voice system of local player is not active.\n");
		return;
	}

	if (m_localUserSteamID == steamID)
	{
		return;
	}

	const uint64 digitalSteamID = steamID.ConvertToUint64();
	const auto& iter = m_playerVchatConnMap.find(digitalSteamID);
	if (iter != m_playerVchatConnMap.end())
	{
		// Player already has a session object, no new object created
		iter->second.m_active = true;
		return;
	}

	DEBUGLOG("SteamVoiceChat add player:[ %s ] to session.\n", SteamFriends()->GetFriendPersonaName(steamID));

	VoiceChatConnection session;
	session.m_lastReceiveVoiceTime = 0;
	session.m_voiceChannel = 0;
	session.m_active = true;

	m_playerVchatConnMap[digitalSteamID] = session;

	return;
}

bool SteamVoiceChat::IsPlayerTalking(const CSteamID steamID) const
{
	if (steamID == m_localUserSteamID)
	{
		// That's ourself
		// We assume user talked less then 250ms ago is active
		if (m_lastTimeTalked + VOICE_CHAT_ALIVE_TICKS > g_game->GetGameTickCount())
		{
			return true;
		}
	}
	else
	{
		const auto& iter = m_playerVchatConnMap.find(steamID.ConvertToUint64());
		if (iter != m_playerVchatConnMap.end())
		{
			if ((iter->second.m_lastReceiveVoiceTime + VOICE_CHAT_ALIVE_TICKS) > g_game->GetGameTickCount())
			{
				return true;
			}
		}
	}

	return false;
}

bool SteamVoiceChat::StartVoiceChat()
{
	if (!(Managers::Get<AudioManager>()->GetAudioEngine()))
	{
		DEBUGLOG("Unable to StartVoiceChat(), failed to GetAudioEngine().\n");
		return false;
	}

	if (m_isActive)
	{
		DEBUGLOG("Unable to StartVoiceChat(), voice chat has started already.\n");
		return false;
	}

	const HSteamNetConnection serverHandle = Managers::Get<OnlineManager>()->GetConnectedServerHandle();
	if (serverHandle == k_HSteamNetConnection_Invalid)
	{
		DEBUGLOG("Unable to StartVoiceChat(), invalid connected server handle\n");
		return false;
	}

	m_connectedServerHandle = serverHandle;
	m_localUserSteamID = SteamUser()->GetSteamID();
	SteamUser()->StartVoiceRecording();
	m_isActive = true;
	Managers::Get<AudioManager>()->SetVoiceChatActive(true);

#ifdef _DEBUG
	// Here you can enable optional local voice loopback:		
	m_voiceLoopback = CreateVoiceChannel();
#endif
	return true;
}

bool SteamVoiceChat::StopVoiceChat()
{
	if (!m_isActive)
	{
		DEBUGLOG("Unable to StopVoiceChat(), voice chat has stopped already\n");
		return false;
	}

	for (auto iter = m_playerVchatConnMap.begin(); iter != m_playerVchatConnMap.end(); ++iter)
	{
		DeleteVoiceChannel(iter->second.m_voiceChannel);
	}
	m_playerVchatConnMap.clear();

#ifdef _DEBUG
	if (m_voiceLoopback)
	{
		DeleteVoiceChannel(m_voiceLoopback);
		m_voiceLoopback = 0;
	}
#endif
	SteamUser()->StopVoiceRecording();
	m_isActive = false;
	Managers::Get<AudioManager>()->SetVoiceChatActive(false);
	return true;
}

int SteamVoiceChat::CreateVoiceChannel()
{
	const std::shared_ptr<DirectX::AudioEngine>& audioEngine = Managers::Get<AudioManager>()->GetAudioEngine();
	if (!audioEngine)
	{
		DEBUGLOG("Unable to create voice channel, unable to get AudioEngine\n");
		return 0;
	}

	const auto& engineInterface = audioEngine->GetInterface();
	if (!engineInterface)
	{
		DEBUGLOG("Unable to create voice channel, unable to get audio engine interface\n");
		return 0;
	}

	std::unique_ptr<VoiceContext> voiceContext = std::make_unique<VoiceContext>();

	// The Format we sample voice in
	WAVEFORMATEX voicesampleformat =
	{
		WAVE_FORMAT_PCM,							// Format type 
		NUMBER_OF_CHANNELS,							// Number of channels (i.e. mono, stereo...)
		VOICE_OUTPUT_SAMPLE_RATE,					// Sample rate 
		VOICE_OUTPUT_SAMPLE_RATE * BYTES_PER_SAMPLE,// For buffer estimation 
		BLOCK_ALIGNMENT_IN_BYTES,					// Block size of data 
		8 * BYTES_PER_SAMPLE,						// Number of bits per sample of mono data
		sizeof(WAVEFORMATEX)						// The count in bytes of the size of extra information
	};

	if (engineInterface->CreateSourceVoice(&voiceContext->m_sourceVoice, &voicesampleformat, 0, 1.0f, voiceContext.get()) < 0)
	{
		DEBUGLOG("Failed to CreateSourceVoice when CreateVoiceChannel\n");
		return 0;
	}

	voiceContext->m_sourceVoice->Start(0, 0);

	m_voiceChannelMap[++m_voiceChannelCount] = std::move(voiceContext);

	return m_voiceChannelCount;
}

void SteamVoiceChat::DeleteVoiceChannel(int channel)
{
	const auto& iter = m_voiceChannelMap.find(channel);
	if (iter == m_voiceChannelMap.end())
	{
		DEBUGLOG("Invalid voice channel:%d\n", channel);
		return;
	}

	const auto& voiceContext = iter->second;
	XAUDIO2_VOICE_STATE state;

	while (true)
	{
		voiceContext->m_sourceVoice->GetState(&state);
		if (!state.BuffersQueued)
		{
			break;
		}

		WaitForSingleObject(voiceContext->m_bufferEndEvent, INFINITE);
	}

	voiceContext->m_sourceVoice->Stop(0);
	voiceContext->m_sourceVoice->DestroyVoice();

	m_voiceChannelMap.erase(iter);
	--m_voiceChannelCount;
}

