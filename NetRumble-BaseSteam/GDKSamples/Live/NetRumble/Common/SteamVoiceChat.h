//--------------------------------------------------------------------------------------
// SteamVoiceChat.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble {
	static constexpr uint32 VOICE_OUTPUT_SAMPLE_RATE{ 11000 };	// Real sample rate is 11025 but for XAudio2 it must be a multiple of XAUDIO2_QUANTUM_DENOMINATOR
	static constexpr uint32 VOICE_OUTPUT_SAMPLE_RATE_IDEAL{ 11025 };
	static constexpr uint64 MIN_VOICE_DATA_UPDATE_INTERVAL{ DX::StepTimer::TicksPerSecond / 10 };
	static constexpr WORD BYTES_PER_SAMPLE{ 2 };
	static constexpr WORD BITS_PER_SAMPLE{ BYTES_PER_SAMPLE * 8 };
	static constexpr uint64 VIOCE_CHAT_BUFFER_SIZE{ VOICE_OUTPUT_SAMPLE_RATE * BYTES_PER_SAMPLE };
	static constexpr uint64 VIOCE_CHAT_BUFFER_BYTES{ sizeof(uint8) * VOICE_OUTPUT_SAMPLE_RATE * BYTES_PER_SAMPLE };
	static constexpr WORD NUMBER_OF_CHANNELS = 1;   // Mono, number of channels in the waveform-audio data. 
													// Monaural data uses one channel and stereo data uses two channels
	static constexpr WORD BLOCK_ALIGNMENT_IN_BYTES{ NUMBER_OF_CHANNELS * BYTES_PER_SAMPLE };

	struct VoiceContext : public IXAudio2VoiceCallback
	{
		VoiceContext() :
			m_bufferEndEvent(CreateEvent(nullptr, false, false, nullptr))
		{
		}

		virtual ~VoiceContext()
		{
			CloseHandle(m_bufferEndEvent);
		}

		STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) {}
		STDMETHOD_(void, OnVoiceProcessingPassEnd)() {}
		STDMETHOD_(void, OnStreamEnd)() {}
		STDMETHOD_(void, OnBufferStart)(void*) {}

		STDMETHOD_(void, OnBufferEnd)(void* context)
		{
			free(context); // Free the sound buffer
			context = nullptr;
			SetEvent(m_bufferEndEvent);
		}

		STDMETHOD_(void, OnLoopEnd)(void*) {}
		STDMETHOD_(void, OnVoiceError)(void*, HRESULT) {}

		HANDLE m_bufferEndEvent;
		IXAudio2SourceVoice* m_sourceVoice{ nullptr };
	};

	struct VoiceChatConnection
	{
		bool m_active;
		int m_voiceChannel;	// Engine voice channel for this player
		uint64 m_lastReceiveVoiceTime;
	};

	class SteamVoiceChat
	{
	public:
		SteamVoiceChat() :
			m_uncompressedVoice(std::make_unique<uint8[]>(VIOCE_CHAT_BUFFER_SIZE)), 
			m_uncompressedVoiceBuffer(std::make_unique<uint8[]>(VIOCE_CHAT_BUFFER_SIZE))
		{
		}
		~SteamVoiceChat() = default;
		SteamVoiceChat(const SteamVoiceChat&) = delete;
		SteamVoiceChat& operator=(const SteamVoiceChat&) = delete;

		// Chat control
		void MarkAllPlayersInactive();
		void MarkPlayerAsActive(CSteamID steamID);
		bool IsPlayerTalking(const CSteamID steamID) const;
		bool StartVoiceChat();
		bool StopVoiceChat();

		// Chat engine
		void Tick();
		void HandleVoiceChatData(const void* pMessage);
		bool AddVoiceChatData(int channel, const uint8* voiceData, const uint32 dataSize);

		// Voice channel control 
		int CreateVoiceChannel();
		void DeleteVoiceChannel(int channel);

	private:
		// Map of voice chat sessions with other players
		std::map<uint64, VoiceChatConnection> m_playerVchatConnMap;
		HSteamNetConnection m_connectedServerHandle{ k_HSteamNetConnection_Invalid };

		CSteamID m_localUserSteamID{ k_steamIDNil };
		bool m_isActive{ false }; // Is voice chat system active
		uint64 m_lastTimeTalked{ 0 }; // Last time we've talked ourself
#ifdef _DEBUG
		int m_voiceLoopback{ 0 }; // Enable optional local voice loopback
#endif
		std::unique_ptr<uint8[]> m_uncompressedVoice;
		std::unique_ptr<uint8[]> m_uncompressedVoiceBuffer;
		// Map of voice channels to voice objects
		std::map<int, std::unique_ptr<VoiceContext>> m_voiceChannelMap;
		uint8 m_voiceChannelCount{ 0 };
	};
	using OnlineVoiceChatImpl = SteamVoiceChat;
}
