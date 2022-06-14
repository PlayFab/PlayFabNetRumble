//--------------------------------------------------------------------------------------
// OnlineVoiceChat.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

OnlineVoiceChat OnlineVoiceChat::m_audioInstance;

OnlineVoiceChat& OnlineVoiceChat::GetInstance()
{
	return m_audioInstance;
}

bool OnlineVoiceChat::StartVoiceChat()
{
	if (!m_voiceChat)
	{
		m_voiceChat = std::make_shared<SteamVoiceChat>();
	}

	if (!m_voiceChat)
	{ 
		DEBUGLOG("Failed to StartVoiceChat, unable to create m_voiceChat\n");
		return false;
	}

	return m_voiceChat->StartVoiceChat();
}

bool OnlineVoiceChat::StopVoiceChat()
{
	if (!m_voiceChat)
	{
		DEBUGLOG("Unable to StopVoiceChat(), invalid m_voiceChat\n");
		return false;
	}

	return m_voiceChat->StopVoiceChat();
}

