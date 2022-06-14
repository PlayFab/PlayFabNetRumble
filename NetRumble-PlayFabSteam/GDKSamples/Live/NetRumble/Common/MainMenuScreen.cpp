//--------------------------------------------------------------------------------------
// MainMenuScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

MainMenuScreen::MainMenuScreen() :
	m_state{ State::Initializing }
{
	m_transitionOnTime = 1.0;
	m_transitionOffTime = 1.0;
	m_exitWhenHidden = false;

	m_title = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\title.png");
}

void MainMenuScreen::HandleInput(float elapsedTime)
{
	MenuScreen::HandleInput(elapsedTime);
}

void MainMenuScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
	MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);

	if (m_state == State::Initializing)
	{
		if (Managers::Get<OnlineManager>()->IsNetworkAvailable())
		{
			InitializeMainMenu();
		}
	}
}

void MainMenuScreen::Draw(float totalTime, float elapsedTime)
{
	if (IsActive())
	{
		// Draw the title texture
		if (m_title.Texture)
		{
			RenderManager* renderManager = Managers::Get<RenderManager>();
			std::unique_ptr<RenderContext> renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);

			float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
			float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
			float scale = GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
			SimpleMath::Vector2 titlePosition = SimpleMath::Vector2(viewportWidth / 2.0f, (viewportHeight / 2.0f) - (185.f * scale));
			titlePosition.y -= powf(TransitionPosition(), 2) * titlePosition.y;
			SimpleMath::Vector4 color = { 1.0f, 1.0f, 1.f, TransitionAlpha() };

			renderContext->Begin();
			renderContext->Draw(
				m_title,
				titlePosition,
				0.0f,
				scale,
				color);
			renderContext->End();
		}

		MenuScreen::Draw(totalTime, elapsedTime);

		DrawCurrentUser();
	}
}

void MainMenuScreen::OnCancel()
{
	// Can't back out of the main menu
}

void MainMenuScreen::InitializeMainMenu()
{
	m_state = State::MainMenu;
	m_menuEntries.clear();

	bool hasMultiplayerPrivileges = true;
	if (hasMultiplayerPrivileges)
	{
		m_menuEntries.emplace_back("QUICK PLAY", []()
			{
				Managers::Get<GameStateManager>()->SwitchToState(GameState::MPMatchmaking);
			});

		m_menuEntries.emplace_back("HOST GAME", []()
			{
				Managers::Get<GameStateManager>()->SwitchToState(GameState::MPHostGame);
			});

		m_menuEntries.emplace_back("JOIN GAME", []()
			{
				Managers::Get<ScreenManager>()->AddGameScreen<JoinFriendsMenu>();
			});
		m_menuEntries.emplace_back("LEADERBOARDS", []()
			{
				Managers::Get<ScreenManager>()->AddGameScreen<LeaderboardMenu>();
			});
	}
	else
	{
		m_menuEntries.emplace_back("Your account does not have multiplayer privileges");
	}

	m_menuEntries.emplace_back("OPTIONS", []()
		{
			Managers::Get<ScreenManager>()->AddGameScreen<OptionsPopUpScreen>();
		});

	// This example itself does not have a complete inventory system, 
	// it is just a demonstration of how to get information about all of the player's items. 
	// As a game with an inventory system, we should capture the player's inventory before they use it.
	Managers::Get<OnlineManager>()->GetAllItems();
}

void MainMenuScreen::Intializing()
{
	m_menuEntries.emplace_back("INITIALIZING");
}
