//--------------------------------------------------------------------------------------
// AudioManager.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

AudioManager::AudioManager() noexcept :
	SoundTrackOn(true),
	PlaySoundEffects(true)
{
}

void AudioManager::LoadSound(const std::wstring& name, const std::wstring& path)
{
	// Stuff the data into our map
	m_soundEffects[name] = std::make_shared<SoundEffect>(m_audEngine.get(), path.c_str());
}

void AudioManager::Initialize()
{
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_UseMasteringLimiter;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif

	m_audEngine.reset(new AudioEngine(eflags));

	// The sound assets in NR are all too loud at default volume
	// One simple option to correct this is to adjust the master volume
	SetMasterVolume(0.02f);

	try
	{
		m_backgroundSound.reset(new SoundEffect(m_audEngine.get(), L"Assets\\Audio\\OneStepBeyond.xwma"));
	}
	catch (std::exception)
	{
		m_backgroundSound.reset();
	}

	if (m_backgroundSound)
	{
		m_backgroundSoundInstance = m_backgroundSound->CreateInstance();
	}
}

void AudioManager::Suspend()
{
	if (m_audEngine)
	{
		m_audEngine->Suspend();
	}
}

void AudioManager::Resume()
{
	if (m_audEngine)
	{
		m_audEngine->Resume();
	}
}

void AudioManager::Tick()
{
	if (!m_audEngine)
	{
		return;
	}

	if (!m_audEngine->Update())
	{
		// No audio device is active
		if (m_audEngine->IsCriticalError())
		{
			// This would only happen if we were rendering to a headset that was disconnected
			// The sample always renders to the 'default' system audio device, not to headsets
			DEBUGLOG("Audio engine encounters critical error");
			assert(false);
		}
	}
}

void AudioManager::PlaySoundTrack(bool play)
{
	if (!(m_audEngine && m_backgroundSoundInstance))
	{
		return;
	}

	if (play)
	{
		m_backgroundSoundInstance->Play(true);
	}
	else
	{
		m_backgroundSoundInstance->Stop();
	}

	SoundTrackOn = play;
}

void AudioManager::PlaySound(const std::wstring& soundName, bool loop)
{
	if (!PlaySoundEffects)
	{
		return;
	}

	UNREFERENCED_PARAMETER(loop);

	// Try to find the sound data for the sound
	auto itr = m_soundEffects.find(soundName);
	if (itr == m_soundEffects.end())
	{
		return;
	}

	itr->second->Play();
}

void AudioManager::SetMasterVolume(float volume)
{
	if (!m_audEngine)
	{
		return;
	}

	IXAudio2MasteringVoice* masterVoice = m_audEngine->GetMasterVoice();
	if (masterVoice)
	{
		masterVoice->SetVolume(volume);
	}
}
