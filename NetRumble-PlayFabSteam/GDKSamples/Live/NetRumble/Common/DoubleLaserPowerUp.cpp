//--------------------------------------------------------------------------------------
// DoubleLaserPowerUp.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

DoubleLaserPowerUp::DoubleLaserPowerUp()
{
	_powerUpTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\powerupDoubleLaser.png");
}

bool DoubleLaserPowerUp::OnTouch(GameplayObject* target)
{
	// Check the target, if we have one
	if (target != nullptr)
	{
		if (target->GetType() == GameplayObjectType::Ship)
		{
			Ship* ship = static_cast<Ship*>(target);
			ship->PrimaryWeapon = std::make_shared<DoubleLaserWeapon>(ship);
		}
	}

	return PowerUp::OnTouch(target);
}

void DoubleLaserPowerUp::Draw(float elapsedTime, RenderContext* renderContext)
{
	PowerUp::Draw(elapsedTime, renderContext, _powerUpTexture, Colors::White);
}
