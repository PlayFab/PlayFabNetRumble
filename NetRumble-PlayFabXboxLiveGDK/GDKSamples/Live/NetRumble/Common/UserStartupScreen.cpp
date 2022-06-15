//--------------------------------------------------------------------------------------
// UserStartupScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

UserStartupScreen::UserStartupScreen() :
	m_state{ State::Initializing }
{
	m_transitionOnTime = 1.0;
	m_transitionOffTime = 1.0;
	m_exitWhenHidden = false;

	m_title = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\title.png");
}

void UserStartupScreen::ExitScreen(bool immediate)
{
	MenuScreen::ExitScreen(immediate);
}

void UserStartupScreen::HandleInput(float elapsedTime)
{
	MenuScreen::HandleInput(elapsedTime);
}

void UserStartupScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
	MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);

	if (m_state == State::Initializing)
	{
		AcquireUserMenu();
	}
}

void UserStartupScreen::Draw(float totalTime, float elapsedTime)
{
	if (IsActive())
	{
		// Draw the title texture
		if (m_title.Texture)
		{
			auto renderManager = Managers::Get<RenderManager>();
			auto renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);

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
	}
}

void UserStartupScreen::Reset()
{
	m_state = State::Initializing;
	m_menuEntries.clear();
}

void UserStartupScreen::OnCancel()
{
	// Can't back out of the start menu
}

void UserStartupScreen::AcquireUserMenu()
{
	m_state = State::AcquireUser;
	m_menuEntries.clear();

	m_menuEntries.emplace_back("LOGIN WITH CUSTOM ID",
		[this]()
		{
			m_menuEntries.clear();
			m_menuEntries.emplace_back("LOGIN WITH CUSTOM ID...");

			LoginWithCustomId();
		});

	m_menuEntries.emplace_back("LOGIN WITH XBOX LIVE",
		[this]()
		{
			m_menuEntries.clear();
			m_menuEntries.emplace_back("LOGIN WITH XBOX LIVE...");

			LoginWithXboxLive();
		});
}

void UserStartupScreen::Intializing()
{
	m_menuEntries.emplace_back("INITIALIZING...");
}

void UserStartupScreen::LoginWithCustomId()
{
	Managers::Get<GameStateManager>()->SwitchToState(GameState::Login);
	if (Managers::Get<OnlineManager>()->GetLoginType() == PlayFabLoginType::LoginWithXboxLive)
	{
		Managers::Get<ScreenManager>()->ShowError("Please login with Xbox Live", []() {
			Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
			});
		return;
	}

	Managers::Get<OnlineManager>()->LoginWithCustomID(
		[this](bool success, const std::string& userId)
		{
			UNREFERENCED_PARAMETER(userId);
			if (success)
			{
				SwitchToMainUI();
			}
			else
			{
				DEBUGLOG("Unable to login with Custom ID");
				Managers::Get<ScreenManager>()->ShowError("Unable to login with Custom ID", []() {
					Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
					});
			}
		});
}

void UserStartupScreen::LoginWithXboxLive()
{
	Managers::Get<GameStateManager>()->SwitchToState(GameState::Login);
	if (Managers::Get<OnlineManager>()->GetLoginType() == PlayFabLoginType::LoginWithCustomID)
	{
		Managers::Get<ScreenManager>()->ShowError("Please login with Custom ID", []() {
			Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
			});
		return;
	}

	Managers::Get<OnlineManager>()->LoginWithXboxLive(
		[this](bool success, const std::string& errorMessage)
		{
			if (success)
			{
				SwitchToMainUI();
			}
			else
			{
				DEBUGLOG(errorMessage.c_str());
				Managers::Get<ScreenManager>()->ShowError(errorMessage, []() {
					Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
					});
			}
		});
}

void UserStartupScreen::SwitchToMainUI()
{
	Managers::Get<OnlineManager>()->InitializePlayfabParty();
	if (Managers::Get<OnlineManager>()->IsPartyInitialized())
	{
		Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
	}
	else
	{
		DEBUGLOG("Unable to initialize party\n");
	}
}

