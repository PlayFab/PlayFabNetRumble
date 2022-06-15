//--------------------------------------------------------------------------------------
// Ship.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

namespace
{
	// The full speed possible for the ship.
	constexpr float c_fullSpeed = 400.0f;

	// The amount of drag applied to velocity per second,
	// as a percentage of velocity.
	constexpr float c_dragPerSecond = 0.7f;

	// The amount that the right stick must be pressed to fire, squared so that
	// we can use LengthSquared instead of Length.
	constexpr float c_fireThresholdSquared = 0.25f;

	// The number of radians that the ship can turn in a second at full left-stick.
	constexpr float c_rotationRadiansPerSecond = 6.0f;

	// The maximum length of the velocity vector on a ship.
	constexpr float c_velocityMaximum = 400.0f;

	// The maximum strength of the shield.
	constexpr float c_shieldMaximum = 100.0f;

	// The maximum opacity for the shield, when it's fully recharged.
	constexpr float c_shieldAlphaMaximum = 150.0f / 255.0f;

	// How much the shield recharges per second.
	constexpr float c_shieldRechargePerSecond = 50.0f;

	// The maximum value of the "safe" timer.
	constexpr float c_safeTimerMaximum = 4.0f;

	// The maximum amount of life that a ship can have.
	constexpr float c_lifeMaximum = 25.0f;

	// The value of the spawn timer set when the ship dies.
	constexpr float c_respawnTimerOnDeath = 5.0f;

	// The value of velocity squared is less than 0.0001f, we consider ship stopped.
	constexpr float c_minimumVelocityThreshold = 0.0001f;
}

const std::array<DirectX::XMVECTORF32, 18> Ship::Colors =
{
	DirectX::Colors::Lime,      DirectX::Colors::CornflowerBlue, DirectX::Colors::Fuchsia,
	DirectX::Colors::Red,       DirectX::Colors::LightSeaGreen,  DirectX::Colors::LightGray,
	DirectX::Colors::Gold,      DirectX::Colors::ForestGreen,    DirectX::Colors::Beige,
	DirectX::Colors::LightPink, DirectX::Colors::Lavender,       DirectX::Colors::OrangeRed,
	DirectX::Colors::Plum,      DirectX::Colors::Tan,            DirectX::Colors::YellowGreen,
	DirectX::Colors::Azure,     DirectX::Colors::Aqua,           DirectX::Colors::Salmon
};

Ship::Ship()
{
	Life = 0.0f;
	Radius = 24.0f;
	Mass = 32.0f;

	m_shieldTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\shipShields.png");
}

void Ship::Initialize(bool isLocal, bool useSpawnEffect)
{
	// Set the initial gameplay data values
	m_active = false;
	Input = ShipInput();
	Rotation = 0;
	Velocity = SimpleMath::Vector2::Zero;

	IsLocal = isLocal;

	LastDamagedBy = nullptr;
	Life = c_lifeMaximum;
	Shield = c_shieldMaximum;
	m_safeTimer = c_safeTimerMaximum;
	m_shieldPulseTime = 0.0f;
	m_shieldRechargeTimer = 0.0f;
	PrimaryWeapon = std::make_shared<LaserWeapon>(this);
	DroppedWeapon = std::make_shared<MineWeapon>(this);

	if (useSpawnEffect)
	{
		// Play the player-spawn sound
		Managers::Get<AudioManager>()->PlaySound(L"player_spawn");

		// Add the ship-spawn particle effect
		Managers::Get<ParticleEffectManager>()->SpawnEffect(ParticleEffectType::ShipSpawn, this->shared_from_this());
	}

	// Clear out the projectiles list
	Projectiles.clear();

	GameplayObject::Initialize();
}

void Ship::Update(float elapsedTime)
{
	// Calculate the current forward vector
	SimpleMath::Vector2 forward = SimpleMath::Vector2{ std::sin(Rotation), -std::cos(Rotation) };
	SimpleMath::Vector2 right = SimpleMath::Vector2{ -forward.y, forward.x };

	// Calculate the new forward vector with the left stick
	Input.LeftStick.y *= -1.0f;

	float d = Input.LeftStick.LengthSquared();

	if (d > 0.0f)
	{
		d = sqrt(d);

		SimpleMath::Vector2 wantedForward = XMVectorScale(Input.LeftStick, 1.0f / d);
		float angleDiff = std::acos(wantedForward.Dot(forward));
		float facing = wantedForward.Dot(right) > 0.0f ? 1.0f : -1.0f;

		if (angleDiff > 0.001f)
		{
			Rotation += std::min<float>(angleDiff, facing * elapsedTime * c_rotationRadiansPerSecond);
		}
		Velocity += Input.LeftStick * elapsedTime * c_fullSpeed;

		d = Velocity.Length();

		if (d > c_velocityMaximum)
		{
			Velocity *= c_velocityMaximum / d;
		}
	}
	Input.LeftStick = SimpleMath::Vector2{ 0, 0 };

	// Apply drag to the velocity
	Velocity -= Velocity * (elapsedTime * c_dragPerSecond);
	if (Velocity.LengthSquared() <= c_minimumVelocityThreshold)
	{
		Velocity = XMFLOAT2(0, 0);
	}

	// Check for firing with the right stick
	Input.RightStick.y *= -1.0f;
	if (Input.RightStick.LengthSquared() > c_fireThresholdSquared)
	{
		SetSafe(false);
		PrimaryWeapon->Fire(Input.RightStick);
	}
	Input.RightStick = SimpleMath::Vector2{ 0, 0 };

	// Check for laying mines
	if (Input.MineFired)
	{
		// Fire behind the ship
		SetSafe(false);
		DroppedWeapon->Fire(-forward);
	}
	Input.MineFired = false;

	// Recharge the shields
	if (m_shieldRechargeTimer > 0.0f)
	{
		m_shieldRechargeTimer = std::max<float>(m_shieldRechargeTimer - elapsedTime, 0.0f);
	}
	if (m_shieldRechargeTimer <= 0.0f)
	{
		if (Shield < c_shieldMaximum)
		{
			Shield = std::min<float>(c_shieldMaximum, Shield + c_shieldRechargePerSecond * elapsedTime);
		}
	}

	// Update the radius based on the shield
	Radius = (Shield > 0.0f) ? 24.0f : 20.0f;

	// Update the weapons
	if (PrimaryWeapon)
	{
		PrimaryWeapon->Update(elapsedTime);
	}
	if (DroppedWeapon)
	{
		DroppedWeapon->Update(elapsedTime);
	}

	// Decrement the safe timer
	if (m_safeTimer > 0.0f)
	{
		m_safeTimer = std::max<float>(m_safeTimer - elapsedTime, 0.0f);
	}

	// Update the projectiles
	for (auto& projectile : Projectiles)
	{
		if (projectile->Active())
		{
			projectile->Update(elapsedTime);
		}
		else
		{
			Projectiles.QueuePendingRemoval(projectile);
		}
	}

	// Get any freshly inactive projectiles that are in the collision system out of there before we destroy them for good
	Managers::Get<CollisionManager>()->Collection().ApplyPendingRemovals();

	Projectiles.ApplyPendingRemovals();

	GameplayObject::Update(elapsedTime);
}

void Ship::Draw(float elapsedTime, RenderContext* renderContext, bool onlyDrawBody, float scale)
{
	if (!onlyDrawBody)
	{
		// Draw the projectiles
		for (auto& projectile : Projectiles)
		{
			projectile->Draw(elapsedTime, renderContext);
		}
	}

	float preservedRadius = Radius;
	Radius *= scale;

	GameplayObject::Draw(elapsedTime, renderContext, m_primaryTexture, Color);
	GameplayObject::Draw(elapsedTime, renderContext, m_overlayTexture, Colors::White);

	if (!onlyDrawBody)
	{
		if (Shield > 0)
		{
			// Draw the shield
			XMVECTORF32 shieldColor = Color;
			shieldColor.f[3] = c_shieldAlphaMaximum * Shield / c_shieldMaximum;
			GameplayObject::Draw(elapsedTime, renderContext, m_shieldTexture, shieldColor);
		}
	}

	// Reset the ship radius back to the preserved value
	Radius = preservedRadius;
}

bool Ship::TakeDamage(GameplayObject* source, float damageAmount)
{
	// If the safe timer hasn't yet gone off, then the ship can't be hurt
	if ((m_safeTimer > 0.0f) || (damageAmount <= 0.0f))
	{
		return false;
	}

	// Once you're hit, the shield-recharge timer starts over
	m_shieldRechargeTimer = 2.5f;

	// Damage the shield first, then life
	if (Shield <= 0.0f)
	{
		Life -= damageAmount;
	}
	else
	{
		Shield -= damageAmount;
		if (Shield < 0.0f)
		{
			// Shield has the overflow value as a negative value, just add it
			Life += Shield;
			Shield = 0.0f;
		}
	}

	if (source->GetType() == GameplayObjectType::Projectile)
	{
		Projectile* sourceAsProjectile = static_cast<Projectile*>(source);
		LastDamagedBy = sourceAsProjectile->GetOwner();
	}
	else
	{
		LastDamagedBy = source;
	}

	return true;
}

void Ship::Die(GameplayObject* source, bool cleanupOnly)
{
	if (m_active)
	{
		if (!cleanupOnly)
		{
			// Update the score
			if (source != nullptr)
			{
				Ship* killerShip = nullptr;
				if (source->GetType() == GameplayObjectType::Projectile)
				{
					Projectile* projectile = static_cast<Projectile*>(source);
					killerShip = projectile->GetOwner();
				}
				else if (source->GetType() == GameplayObjectType::Ship)
				{
					killerShip = static_cast<Ship*>(source);
				}

				if (killerShip == this || killerShip == nullptr)
				{
					// Reduce the score, since i blew myself up, or something besides a projectile blew me up
					if (Score > 0)
					{
						Score--;
					}
				}
				else
				{
					// Add score to the ship who shot this object
					killerShip->Score++;
				}
			}
			else
			{
				// If the killer wasn't a ship, then this object loses score
				if (Score > 0)
				{
					Score--;
				}
			}

			// Play the player-death sound
			Managers::Get<AudioManager>()->PlaySound(L"explosion_shockwave");
			Managers::Get<AudioManager>()->PlaySound(L"explosion_large");

			// Display the ship explosion
			Managers::Get<ParticleEffectManager>()->SpawnEffect(ParticleEffectType::ShipExplosion, Position);
		}

		// Clear out the projectiles list
		for (auto projectile : Projectiles)
		{
			projectile->Die(nullptr, true);
		}

		// Get these projectiles out of the collision system before we destroy them for good
		Managers::Get<CollisionManager>()->Collection().ApplyPendingRemovals();

		Projectiles.clear();

		// Set the respawn timer
		RespawnTimer = c_respawnTimerOnDeath;
	}

	GameplayObject::Die(source, cleanupOnly);
}

void Ship::SetSafe(bool isSafe)
{
	if (isSafe)
	{
		m_safeTimer = c_safeTimerMaximum;
	}
	else
	{
		m_safeTimer = 0.0f;
	}
}

std::vector<unsigned char> Ship::Serialize()
{
	DataBufferWriter dataWriter;

	dataWriter.WriteStruct(Position);
	dataWriter.WriteStruct(Velocity);
	dataWriter.WriteSingle(Rotation);
	dataWriter.WriteSingle(Life);
	dataWriter.WriteSingle(Shield);
	dataWriter.WriteStruct(Input);

	return dataWriter.GetBuffer();
}

void Ship::Deserialize(const std::vector<unsigned char>& data)
{
	DataBufferReader dataReader(data);

	dataReader.ReadStruct(Position);
	dataReader.ReadStruct(Velocity);
	Rotation = dataReader.ReadSingle();
	Life = dataReader.ReadSingle();
	Shield = dataReader.ReadSingle();
	dataReader.ReadStruct(Input);
}

void Ship::SetShipTexture(uint32_t index)
{
	Variation = index;

	ContentManager* contentManager = Managers::Get<ContentManager>();
	wchar_t buffer[64] = {};
	swprintf_s(buffer, L"Assets\\Textures\\ship%d.png", index);
	m_primaryTexture = contentManager->LoadTexture(buffer);
	swprintf_s(buffer, L"Assets\\Textures\\ship%dOverlay.png", index);
	m_overlayTexture = contentManager->LoadTexture(buffer);
}
