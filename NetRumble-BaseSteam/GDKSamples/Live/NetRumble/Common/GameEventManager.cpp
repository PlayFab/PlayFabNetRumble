//--------------------------------------------------------------------------------------
// GameEventManager.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

GameEventManager::~GameEventManager()
{
	Release();
}

void GameEventManager::RegisterEvent(GameEventType eventType, void* pInstance, CallBack callback)
{
	auto event = m_eventList.find(eventType);
	if (event != m_eventList.end())
	{
		auto iterator = event->second.begin();
		while (iterator != event->second.end())
		{
			if (iterator->first == pInstance)
			{
				break;
			}
			iterator++;
		}
		if (iterator == event->second.end())
		{
			event->second.push_back(std::pair<void*, CallBack>(pInstance, callback));
		}
	}
	else
	{
		std::vector<std::pair<void*, CallBack>> callBackList;
		callBackList.push_back(std::pair<void*, CallBack>(pInstance, callback));
		m_eventList[eventType] = callBackList;
	}
}

void GameEventManager::RemoveEvent(void* pInstance)
{
	for (auto event : m_eventList)
	{
		auto iterator = event.second.begin();
		while (iterator != event.second.end())
		{
			if (iterator->first == pInstance)
			{
				event.second.erase(iterator);
				break;
			}
			iterator++;
		}
	}
}

void GameEventManager::RemoveEvent(GameEventType eventType, void* pInstance)
{
	auto event = m_eventList.find(eventType);
	if (event != m_eventList.end())
	{
		auto iterator = event->second.begin();
		while (iterator != event->second.end())
		{
			if (iterator->first == pInstance)
			{
				event->second.erase(iterator);
				break;
			}
			iterator++;
		}
	}
	else
	{
		DEBUGLOG("The signal was not registered");
	}
}

void GameEventManager::DispatchEvent(GameEventMessage message)
{
	auto event = m_eventList.find(message.m_type);
	if (event != m_eventList.end())
	{
		auto callBackList = event->second;
		for (auto callBack : callBackList)
		{
			CallBack call = callBack.second;
			call(message);
		}
	}
	else
	{
		DEBUGLOG("The signal was not registered");
	}
}

void GameEventManager::Release() 
{
	for (auto event : m_eventList)
	{
		DeregisterEvent(event.first);
	}
	m_eventList.clear();
}

void GameEventManager::DeregisterEvent(GameEventType eventType)
{
	auto event = m_eventList.find(eventType);
	if (event != m_eventList.end())
	{
		event->second.clear();
		m_eventList.erase(event);
	}
	else
	{
		DEBUGLOG("The signal was not registered");
	}
}
