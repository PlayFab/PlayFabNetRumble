//--------------------------------------------------------------------------------------
// ErrorScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

JoinFriendsMenu::JoinFriendsMenu()
{
	m_transitionOnTime = 1.0;
	m_transitionOffTime = 1.0;

	FindLobbyCallback findLobbyCallback = std::bind(&JoinFriendsMenu::FindLobbyCallBack, this, std::placeholders::_1);

	Managers::Get<OnlineManager>()->SetFindLobbyCallback(findLobbyCallback);
	Managers::Get<OnlineManager>()->FindLobbies();
}

void JoinFriendsMenu::Draw(float totalTime, float elapsedTime)
{
	UNREFERENCED_PARAMETER(totalTime);
	UNREFERENCED_PARAMETER(elapsedTime);

	m_menuEntries.clear();

	if (m_friendsGames.empty())
	{
		m_menuEntries.push_back(MenuEntry{ "Loading Friends' Games" });
	}
	else
	{
		for (size_t i = 0; i < 5 && i < m_friendsGames.size(); ++i)
		{
			std::shared_ptr<OnlineUser> lobby = m_friendsGames[i];
			uint64_t lobbyId = lobby->Id;
			m_menuEntries.push_back(MenuEntry(lobby->Name, [lobbyId]()
				{
					Managers::Get<OnlineManager>()->JoinLobby(CSteamID(lobbyId));
				}));
		}
	}


	std::unique_ptr<RenderContext> renderContext = Managers::Get<RenderManager>()->GetRenderContext();

	ContentManager* contentManager = Managers::Get<ContentManager>();

	std::shared_ptr<DirectX::SpriteFont> spriteFont = contentManager->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());

	// Calculate position and size of error message
	XMVECTORF32 errorMsgColor = Colors::Yellow;
	const float scale = 0.5f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

	renderContext->Begin();

	// Draw a background color for the rectangle
	XMVECTORF32 color = Colors::DarkSlateGray;
	color.f[3] = .25f;

	if (m_state == ScreenStateType::Active)
	{
		// Draw error message in the middle of the screen
		SimpleMath::Vector2 errorMsgPosition = SimpleMath::Vector2(0, viewportHeight / 2.0f - 10);
		XMVECTOR size = spriteFont->MeasureString("Select Friend to Join");
		errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX(size) / 2.0f * scale;
		renderContext->DrawString(spriteFont, "Select Friend to Join", errorMsgPosition, errorMsgColor, 0, DirectX::XMFLOAT2{ 0,0 }, scale);
	}
	renderContext->End();

	MenuScreen::Draw(totalTime, elapsedTime);

	DrawCurrentUser();
}

void JoinFriendsMenu::OnCancel()
{
	ExitScreen(false);
}

void NetRumble::JoinFriendsMenu::FindLobbyCallBack(std::vector<std::shared_ptr<OnlineUser>> users)
{
	m_friendsGames = users;
}
