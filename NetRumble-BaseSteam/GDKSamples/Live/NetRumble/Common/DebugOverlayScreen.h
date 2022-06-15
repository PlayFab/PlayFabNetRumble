//--------------------------------------------------------------------------------------
// DebugOverlayScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameScreen.h"

namespace NetRumble
{

	class DebugOverlayScreen : public GameScreen
	{
	public:
		DebugOverlayScreen();
		virtual ~DebugOverlayScreen() = default;

		virtual void Draw(float totalTime, float elapsedTime) override;
	};

}
