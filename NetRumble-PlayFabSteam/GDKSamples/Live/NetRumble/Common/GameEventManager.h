//--------------------------------------------------------------------------------------
// GameEventManager.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"

namespace NetRumble
{
	enum class GameEventType
	{
		LeavingGame					// Leave game
	};

	struct GameEventMessage
	{
		GameEventType m_type;
		void* m_message;
	};

	class GameEventManager : public Manager {
	public:
		using CallBack = std::function<void(GameEventMessage)>;
		GameEventManager() noexcept { }
		~GameEventManager();

		void RegisterEvent(GameEventType eventType, void* pInstance, CallBack callback);

		void RemoveEvent(void* pInstance);
		void RemoveEvent(GameEventType eventType, void* pInstance);

		void DispatchEvent(GameEventMessage message);

	private:
		std::map<GameEventType, std::vector<std::pair<void*, CallBack>>> m_eventList;

		void Release();
		void DeregisterEvent(GameEventType eventType);
	};
}
