//--------------------------------------------------------------------------------------
// LaserWeapon.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LaserWeapon.h"

#include "LaserProjectile.h"

using namespace NetRumble;

LaserWeapon::LaserWeapon(Ship* owner) : Weapon(owner)
{
	m_fireDelay = 0.15f;

	switch (owner->Variation % 3)
	{
	case 0:
		m_fireSoundEffect = L"fire_laser1";
		break;
	case 1:
		m_fireSoundEffect = L"fire_laser2";
		break;
	case 2:
		m_fireSoundEffect = L"fire_laser3";
		break;
	default:
		break;
	}
}

void LaserWeapon::CreateProjectiles(const DirectX::SimpleMath::Vector2& direction)
{
	// Create the new projectile
	std::shared_ptr<LaserProjectile> projectile = std::make_shared<LaserProjectile>(m_owner, direction);
	projectile->Initialize();

	m_owner->Projectiles.push_back(projectile);
}
