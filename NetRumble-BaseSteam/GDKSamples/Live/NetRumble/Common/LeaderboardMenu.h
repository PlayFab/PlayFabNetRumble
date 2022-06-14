//--------------------------------------------------------------------------------------
// LeaderboardMenu.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"

namespace NetRumble
{
	class LeaderboardMenu : public MenuScreen
	{
	public:
		LeaderboardMenu();
		virtual ~LeaderboardMenu() = default;
		virtual void Draw(float totalTime, float elapsedTime) override;
		virtual void OnCancel() override;

		void FindLeaderboardCallBack(std::vector<std::shared_ptr<LeaderboardUser>> users);

	private:
		std::vector <std::shared_ptr<LeaderboardUser>> m_leaderBoardUser;
	};
}
