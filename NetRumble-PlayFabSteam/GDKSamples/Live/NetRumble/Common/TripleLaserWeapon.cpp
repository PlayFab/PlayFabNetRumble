//--------------------------------------------------------------------------------------
// TripleLaserWeapon.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LaserProjectile.h"

using namespace NetRumble;
using namespace DirectX::SimpleMath;

TripleLaserWeapon::TripleLaserWeapon(Ship* owner) :
	LaserWeapon(owner)
{
	m_fireDelay = 0.3f;
}

void TripleLaserWeapon::CreateProjectiles(const DirectX::SimpleMath::Vector2& direction)
{
	// Calculate the direction vectors for the second and third projectiles
	Vector2 directionV2{ direction.x, direction.y };
	float rotation = std::acosf(directionV2.Dot(Vector2(0.0f, -1.0f)));
	rotation *= ((-Vector2::UnitY).Dot(Vector2(direction.y, -direction.x)) > 0.0f) ? 1.0f : -1.0f;

	Vector2 direction2{ std::sinf(rotation - c_laserSpreadRadians), -std::cosf(rotation - c_laserSpreadRadians) };
	Vector2 direction3{ std::sinf(rotation + c_laserSpreadRadians), -std::cosf(rotation + c_laserSpreadRadians) };

	// Create the first projectile
	std::shared_ptr<LaserProjectile> projectile1 = std::make_shared<LaserProjectile>(m_owner, direction);
	projectile1->Initialize();
	m_owner->Projectiles.push_back(projectile1);

	// Create the second projectile
	std::shared_ptr<LaserProjectile> projectile2 = std::make_shared<LaserProjectile>(m_owner, direction2);
	projectile2->Initialize();
	m_owner->Projectiles.push_back(projectile2);

	// Create the second projectile
	std::shared_ptr<LaserProjectile> projectile3 = std::make_shared<LaserProjectile>(m_owner, direction3);
	projectile3->Initialize();
	m_owner->Projectiles.push_back(projectile3);
}
