//--------------------------------------------------------------------------------------
// RocketWeapon.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RocketWeapon.h"

#include "RandomMath.h"
#include "RocketProjectile.h"

using namespace NetRumble;

RocketWeapon::RocketWeapon(Ship* owner) : Weapon(owner)
{
	m_fireDelay = 0.5f;

	if (RandomMath::RandomBetween(0, 1) == 0)
	{
		m_fireSoundEffect = L"fire_rocket1";
	}
	else
	{
		m_fireSoundEffect = L"fire_rocket2";
	}
}

void RocketWeapon::CreateProjectiles(const DirectX::SimpleMath::Vector2& direction)
{
	// Create the new projectile
	std::shared_ptr<RocketProjectile> projectile = std::make_shared<RocketProjectile>(m_owner, direction);
	projectile->Initialize();

	m_owner->Projectiles.push_back(projectile);
}
