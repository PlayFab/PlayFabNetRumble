//--------------------------------------------------------------------------------------
// MineWeapon.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MineWeapon.h"

#include "MineProjectile.h"

using namespace NetRumble;

MineWeapon::MineWeapon(Ship* owner) : Weapon(owner)
{
	m_fireDelay = 3.0f;
}

void MineWeapon::CreateProjectiles(const DirectX::SimpleMath::Vector2& direction)
{
	// Create the new projectile
	std::shared_ptr<MineProjectile> projectile = std::make_shared<MineProjectile>(m_owner, direction);
	projectile->Initialize();

	m_owner->Projectiles.push_back(projectile);

	// Move the mine out from the ship
	float magnitude = m_owner->Radius + projectile->Radius + c_mineSpawnDistance;
	projectile->Position += direction * magnitude;
}
