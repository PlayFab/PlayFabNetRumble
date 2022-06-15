//--------------------------------------------------------------------------------------
// ErrorScreen.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"

namespace NetRumble
{
	class JoinFriendsMenu : public MenuScreen
	{
	public:
		JoinFriendsMenu();
		virtual ~JoinFriendsMenu() = default;

		virtual void Draw(float totalTime, float elapsedTime) override;
		virtual void OnCancel() override;

		void FindLobbyCallBack(std::vector<std::shared_ptr<LobbySearchResult>> users);

	private:
		std::vector<OnlineUser> m_friendsGames;
	};

}
