//--------------------------------------------------------------------------------------
// Managers.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <vcruntime_typeinfo.h>

using namespace NetRumble;

std::unordered_map<size_t, std::unique_ptr<Manager>> Managers::m_managersByType;

void Managers::Initialize()
{
	AddManager<AudioManager>();
	AddManager<RenderManager>();
	AddManager<InputManager>();

	AddManager<ContentManager>();
	AddManager<CollisionManager>();
	AddManager<GameStateManager>();
	AddManager<ParticleEffectManager>();
	AddManager<ScreenManager>();

	AddManager<OnlineManager>();

	AddManager<GameEventManager>();
	AddManager<PlayFabParty>();
}
