//--------------------------------------------------------------------------------------
// MainMenuScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"

namespace NetRumble
{
	class MainMenuScreen : public MenuScreen
	{
	public:
		MainMenuScreen();
		virtual ~MainMenuScreen() = default;

		virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
		virtual void HandleInput(float elapsedTime) override;
		virtual void Draw(float totalTime, float elapsedTime) override;

	protected:
		virtual void OnCancel() override;

	private:
		void Intializing();
		void InitializeMainMenu();

		enum class State
		{
			Initializing,
			MainMenu
		};

		State m_state;
		TextureHandle m_title;
	};
}
