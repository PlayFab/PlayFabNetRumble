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
		LoginWithSteam,
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

		void LoginWithCustomId(std::function<void(bool, const std::string&)> callback = nullptr);
		void LoginWithSteam(std::function<void(bool, const std::string&)> callback = nullptr);

		void TryEntityTokenRefresh();

		void SetSteamAuthTicketHandle(HAuthTicket hAuthTicket);

		bool GetUserId(std::string& outUserId) const;
		bool GetUserName(std::string& outUserName) const;
		const std::string& GetLocalPlayerName() const;
		void SetLocalPlayerName();
		void SetLoginProperties(const LoginResult& loginResult);
		const PlayFabLoginType& GetLoginType() { return m_loginType; };
		void Cleanup();

		inline const PlayFab::ClientModels::EntityKey& GetEntityKey() const { return m_entityKey; }
		inline const std::string GetEntityToken() const { return m_entityToken; }
		inline const PFEntityKey& GetPFLoginEntityKey() const { return m_pfLoginEntityKey; }

	private:
		void ClearHostNetwork();
		void DoLoginToSteam();
		std::string BuildHexString(unsigned char* byteArray, uint32 length) const;

		// Get authentication token for user
		void GetSteamTicket();
		void CancelSteamTicket();

		STEAM_CALLBACK(PlayFabLogin, OnGetAuthSessionTicketResponse, GetAuthSessionTicketResponse_t);

		PlayFabLoginType m_loginType{ PlayFabLoginType::Undefine };
		PFEntityKey m_pfLoginEntityKey{ nullptr, nullptr };
		EntityTokenResponse m_pfEntityToken;
		PlayFab::ClientModels::EntityKey  m_entityKey;
		std::string m_entityToken;

		// Size of Steam authentication ticket buffer
		static constexpr int TICKET_BUFFER_SIZE = 1024;
		// Steam authentication ticket buffer
		unsigned char m_bSteamTicketBuffer[TICKET_BUFFER_SIZE] = "";
		// Steam authentication ticket real length
		uint32 m_ticketLen;
		// Steam authentication ticket handle
		HAuthTicket m_steamTicket;
		// Name of a user that logged in
		std::string m_localPlayerName;
		// Id of a user that logged in
		std::string m_playfabUserId;
		// Login with Steam result call back
		std::function<void(bool, const std::string&)> m_loginCallback;
	};
}