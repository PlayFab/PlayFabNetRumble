//--------------------------------------------------------------------------------------
// DoubleLaserWeapon.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "LaserProjectile.h"

#include <SimpleMath.h>

using namespace NetRumble;
using namespace DirectX::SimpleMath;

DoubleLaserWeapon::DoubleLaserWeapon(Ship* owner) : LaserWeapon(owner)
{
}

void DoubleLaserWeapon::CreateProjectiles(const Vector2& direction)
{
	// Calculate the spread of the laser bolts
	Vector2 cross = Vector2(-direction.y, direction.x) * c_laserSpread;

	// Create the first new projectile
	std::shared_ptr<LaserProjectile> projectile1 = std::make_shared<LaserProjectile>(m_owner, direction);
	projectile1->Initialize();
	projectile1->Position += cross;
	m_owner->Projectiles.push_back(projectile1);

	// Create the second projectile
	std::shared_ptr<LaserProjectile> projectile2 = std::make_shared<LaserProjectile>(m_owner, direction);
	projectile2->Initialize();
	projectile2->Position -= cross;
	m_owner->Projectiles.push_back(projectile2);
}
