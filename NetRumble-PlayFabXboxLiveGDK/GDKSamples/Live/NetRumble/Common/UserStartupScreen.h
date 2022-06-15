//--------------------------------------------------------------------------------------
// UserStartupScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"

namespace NetRumble
{
	class UserStartupScreen : public MenuScreen
	{
	public:
		UserStartupScreen();
		virtual ~UserStartupScreen() = default;

		void Reset() override;
		void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
		void HandleInput(float elapsedTime) override;
		void Draw(float totalTime, float elapsedTime) override;
		void ExitScreen(bool immediate = false) override;

	protected:
		void OnCancel() override;

	private:
		void Intializing();
		void AcquireUserMenu();

		void LoginWithCustomId();
		void LoginWithXboxLive();
		void SwitchToMainUI();

		enum class State
		{
			Initializing,
			AcquireUser
		};

		State m_state;
		TextureHandle m_title;
	};
}
