//--------------------------------------------------------------------------------------
// RocketPowerUp.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "PowerUp.h"

namespace NetRumble
{
	class RocketPowerUp final : public PowerUp
	{
	public:
		RocketPowerUp();

		virtual bool OnTouch(GameplayObject* target) override;
		virtual void Draw(float elapsedTime, RenderContext* renderContext) override;
		virtual PowerUpType GetPowerUpType() const override { return PowerUpType::Rocket; }

	private:
		TextureHandle m_powerUpTexture;
	};

}
