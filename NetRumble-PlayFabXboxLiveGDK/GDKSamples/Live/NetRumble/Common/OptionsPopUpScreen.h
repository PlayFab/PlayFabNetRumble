//--------------------------------------------------------------------------------------
// OptionsPopUpScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
	class OptionsPopUpScreen : public MenuScreen
	{
	public:
		OptionsPopUpScreen();
		virtual ~OptionsPopUpScreen() = default;

		virtual void HandleInput(float elapsedTime) override;
		virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
		virtual void Draw(float totalTime, float elapsedTime) override;
		virtual void LoadContent() override;

	protected:
		virtual void OnCancel() override;
		virtual void ComputeMenuBounds(float viewportWidth, float viewportHeight) override;

	private:
		TextureHandle m_backgroundTexture;
		int m_currentIndex;
	};
}
