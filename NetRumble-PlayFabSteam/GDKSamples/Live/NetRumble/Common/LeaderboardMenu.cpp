//--------------------------------------------------------------------------------------
// LeaderboardMenu.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

namespace {
	constexpr int MaxLeaderboardDisplayUserCount = 5;
	const char* strLeaderboards = "Leaderboards";
}

LeaderboardMenu::LeaderboardMenu()
{
	m_transitionOnTime = 1.0;
	m_transitionOffTime = 1.0;

	// Get Leaderboard Datas
	FindLeaderboardCallback findLeaderboardCallback = std::bind(&LeaderboardMenu::FindLeaderboardCallBack, this, std::placeholders::_1);

	Managers::Get<OnlineManager>()->SetFindLeaderboardCallback(findLeaderboardCallback);
	Managers::Get<OnlineManager>()->GetLeaderboards(LeaderboardsTypes::EnteryGameTimesLeaderboard);
}

void LeaderboardMenu::Draw(float totalTime, float elapsedTime)
{
	UNREFERENCED_PARAMETER(totalTime);
	UNREFERENCED_PARAMETER(elapsedTime);

	m_menuEntries.clear();
	if (m_leaderBoardUser.empty())
	{
		m_menuEntries.push_back(MenuEntry{ "Loading Leaderboards" });
	}
	else
	{
		bool findUser = false;
		std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
		std::shared_ptr<LeaderboardUser> localPlayerLeaderboards = std::make_shared<LeaderboardUser>();
		size_t displayUserCount = MaxLeaderboardDisplayUserCount < m_leaderBoardUser.size() ? MaxLeaderboardDisplayUserCount : m_leaderBoardUser.size();
		for (size_t i = 0; i < displayUserCount; ++i)
		{
			std::shared_ptr<LeaderboardUser> user = m_leaderBoardUser[i];
			std::string userMsg = "{" + std::to_string(user->GlobakRank) + "} " + user->Name + " - " + std::to_string(user->Score);

			if (findUser == false && localPlayerState->DisplayName == user->Name)
			{
				localPlayerLeaderboards = user;
				findUser = true;
			}
			m_menuEntries.push_back(MenuEntry{ userMsg });
		}

		std::string userMsg;
		if (findUser)
		{
			userMsg = "You:{" + std::to_string(localPlayerLeaderboards->GlobakRank) + "}" + localPlayerLeaderboards->Name + " - " + std::to_string(localPlayerLeaderboards->Score);
		}
		else
		{
			int startGameCounts = Managers::Get<OnlineManager>()->GetStartGameCount();
			userMsg = "You're not on the leaderboards, your score is " + std::to_string(startGameCounts);
		}
		m_menuEntries.push_back(MenuEntry{ " " }); //space line
		m_menuEntries.push_back(MenuEntry{ userMsg });
	}

	std::unique_ptr<RenderContext> renderContext = Managers::Get<RenderManager>()->GetRenderContext();

	ContentManager* contentManager = Managers::Get<ContentManager>();

	std::shared_ptr<DirectX::SpriteFont> spriteFont = contentManager->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());

	// Calculate position and size of error message
	const DirectX::XMVECTORF32 errorMsgColor = Colors::Yellow;
	const float scale = 0.5f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

	renderContext->Begin();

	if (m_state == ScreenStateType::Active)
	{
		// Draw error message in the middle of the screen
		SimpleMath::Vector2 errorMsgPosition = SimpleMath::Vector2(0, viewportHeight / 2.0f - 10);
		XMVECTOR size = spriteFont->MeasureString(strLeaderboards);
		errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX(size) / 2.0f * scale;
		renderContext->DrawString(spriteFont, strLeaderboards, errorMsgPosition, errorMsgColor, 0, DirectX::XMFLOAT2{ 0,0 }, scale);
	}

	renderContext->End();

	MenuScreen::Draw(totalTime, elapsedTime);
}

void LeaderboardMenu::OnCancel()
{
	ExitScreen(false);
}

void LeaderboardMenu::FindLeaderboardCallBack(std::vector<std::shared_ptr<LeaderboardUser>> users)
{
	m_leaderBoardUser = users;
}
