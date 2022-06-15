//--------------------------------------------------------------------------------------
// PlayFabLogin.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "pch.h"

using namespace PlayFab;
using namespace ClientModels;

namespace NetRumble
{
	enum class PlayFabLoginType : int
	{
		LoginWithCustomID,
		LoginWithXboxLive,
		Undefine
	};

	class PlayFabLogin
	{
	public:
		PlayFabLogin();
		~PlayFabLogin() = default;
		PlayFabLogin(const PlayFabLogin&) = delete;
		PlayFabLogin& operator=(const PlayFabLogin&) = delete;
		PlayFabLogin(PlayFabLogin&&) = delete;
		PlayFabLogin& operator=(PlayFabLogin&&) = delete;

		void Tick(float delta);
		void Initialize();
		void HandleLoginFailure();

		void LoginWithCustomId(std::function<void(bool, const std::string&)> callback = nullptr);
		void LoginWithXboxLive(std::function<void(bool, const std::string&)> callback = nullptr);
		void LoginWithXboxLiveUserId(std::string xboxToken);
		void SetXboxUser(XUserHandle user, uint64_t xuid);
		uint64_t GetXboxUserXuid() const { return xblUserXuid; }
		void XblCleanup();

		void TryEntityTokenRefresh();

		bool GetUserId(std::string& outUserId) const;
		bool GetUserName(std::string& outUserName) const;
		const std::string& GetLocalPlayerName() const;
		void SetLocalPlayerNameForCustomId();
		void SetLoginProperties(const LoginResult& loginResult);
		const PlayFabLoginType& GetLoginType() { return m_loginType; };
		void Cleanup();

		inline const PlayFab::ClientModels::EntityKey& GetEntityKey() const { return m_entityKey; }
		inline const std::string GetEntityToken() const { return m_entityToken; }
		inline const PFEntityKey& GetPFLoginEntityKey() const { return m_pfLoginEntityKey; }

	private:
		void ClearHostNetwork();

		PlayFabLoginType m_loginType{ PlayFabLoginType::Undefine };
		PFEntityKey m_pfLoginEntityKey{ nullptr, nullptr };
		EntityTokenResponse m_pfEntityToken;
		PlayFab::ClientModels::EntityKey  m_entityKey;
		std::string m_entityToken;
		// Name of a user that logged in
		std::string m_localPlayerName;
		// Id of a user that logged in
		std::string m_playfabUserId;
		std::function<void(bool, const std::string&)> m_loginCallback;
		// xbox user
		XUserHandle xblUser{};
		uint64_t xblUserXuid{ 0 };
	};
}
