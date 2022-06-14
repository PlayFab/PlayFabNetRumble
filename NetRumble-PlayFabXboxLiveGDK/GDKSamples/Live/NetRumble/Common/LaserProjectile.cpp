//--------------------------------------------------------------------------------------
// LaserProjectile.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LaserProjectile.h"

using namespace NetRumble;
using namespace DirectX;

LaserProjectile::LaserProjectile(Ship* owner, DirectX::SimpleMath::Vector2 direction) : Projectile(owner, direction)
{
	// Set the gameplay data
	Velocity *= initialSpeed;

	// Set the collision data
	Radius = 4.0f;
	Mass = 0.5f;

	// Set the projectile data
	m_duration = 5.0f;
	m_damageAmount = 20.0f;
	m_damageRadius = 0.0f;
	m_damageOwner = false;

	m_texture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\laser.png");
}

void LaserProjectile::Draw(float elapsedTime, RenderContext* renderContext)
{
	// Ignore the parameter color if we have an owner
	GameplayObject::Draw(elapsedTime, renderContext, m_texture, m_owner != nullptr ? m_owner->Color : Colors::White);
}

void LaserProjectile::Die(GameplayObject* source, bool cleanupOnly)
{
	if (m_active)
	{
		// Display the laser explosion
		if (!cleanupOnly)
		{
			Managers::Get<ParticleEffectManager>()->SpawnEffect(ParticleEffectType::LaserExplosion, Position);
		}
	}

	Projectile::Die(source, cleanupOnly);
}
