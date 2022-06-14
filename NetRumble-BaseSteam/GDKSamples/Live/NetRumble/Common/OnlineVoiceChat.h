//--------------------------------------------------------------------------------------
// OnlineVoiceChatManager.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
	class OnlineVoiceChat 
	{
	public:
		static OnlineVoiceChat& GetInstance();
		
		inline std::shared_ptr<OnlineVoiceChatImpl> GetVoiceChat() { return m_voiceChat; }
		bool StartVoiceChat();
		bool StopVoiceChat();

	private:
		std::shared_ptr<OnlineVoiceChatImpl> m_voiceChat{ nullptr };
		OnlineVoiceChat(OnlineVoiceChat&) = delete;
		OnlineVoiceChat& operator=(const OnlineVoiceChat&) = delete;
		OnlineVoiceChat() noexcept = default;
		static OnlineVoiceChat m_audioInstance;
	};
}
