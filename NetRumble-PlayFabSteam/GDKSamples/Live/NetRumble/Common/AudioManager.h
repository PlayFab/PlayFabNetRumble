//--------------------------------------------------------------------------------------
// AudioManager.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace NetRumble
{
	class AudioManager : public Manager
	{
	public:
		AudioManager() noexcept;

		void Initialize();
		void Suspend();
		void Resume();
		void Tick();
		void PlaySoundTrack(bool play);
		void PlaySound(const std::wstring& soundName, bool loop = false);
		void SetMasterVolume(float volume);
		inline bool IsVoiceChatActive() const { return m_voiceChatActive; }
		inline void SetVoiceChatActive(bool activeState) { m_voiceChatActive = activeState; }

		bool SoundTrackOn;
		bool PlaySoundEffects;

		void LoadSound(const std::wstring& name, const std::wstring& path);

	private:
		std::shared_ptr<DirectX::AudioEngine>                         m_audEngine;
		std::unique_ptr<DirectX::SoundEffect>                         m_backgroundSound;
		std::unique_ptr<DirectX::SoundEffectInstance>                 m_backgroundSoundInstance;
		std::map<std::wstring, std::shared_ptr<DirectX::SoundEffect>> m_soundEffects;

		bool m_voiceChatActive{ false };
	};

}

