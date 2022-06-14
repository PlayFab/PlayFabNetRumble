//--------------------------------------------------------------------------------------
// RocketProjectile.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RocketProjectile.h"

using namespace NetRumble;
using namespace DirectX;

RocketProjectile::RocketProjectile(Ship* owner, DirectX::SimpleMath::Vector2 direction) :
	Projectile(owner, direction)
{
	// Set the gameplay data
	Velocity *= c_initialSpeed;

	// Set the collision data
	Radius = 8.0f;
	Mass = 10.0f;

	// Set the projectile data
	m_duration = 4.0f;
	m_damageAmount = 150.0f;
	m_damageRadius = 128.0f;
	m_damageOwner = false;
	Rotation += DirectX::XM_PI;

	m_rocketTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\rocket.png");
}

void RocketProjectile::Initialize()
{
	if (!Active())
	{
		m_rocketTrailEffect = Managers::Get<ParticleEffectManager>()->SpawnEffect(ParticleEffectType::RocketTrail, Position, shared_from_this());
		if (m_rocketTrailEffect)
		{ 
			m_rocketTrailEffect->SetFollowOffset(SimpleMath::Vector2(-Radius * 2.5f, 0));
		}
	}

	Projectile::Initialize();
}

void RocketProjectile::Draw(float elapsedTime, RenderContext* renderContext)
{
	GameplayObject::Draw(
		elapsedTime,
		renderContext,
		m_rocketTexture,
		m_owner != nullptr ? m_owner->Color : Colors::White
	);
}

void RocketProjectile::Die(GameplayObject* source, bool cleanupOnly)
{
	if (m_active)
	{
		if (!cleanupOnly)
		{
			// Play the rocket explosion sound
			Managers::Get<AudioManager>()->PlaySound(L"explosion_medium");

			// Display the rocket explosion
			Managers::Get<ParticleEffectManager>()->SpawnEffect(ParticleEffectType::RocketExplosion, Position);
		}

		// Stop the rocket trail effect
		if (m_rocketTrailEffect != nullptr)
		{
			m_rocketTrailEffect->Stop(false);
		}
	}

	Projectile::Die(source, cleanupOnly);
}
