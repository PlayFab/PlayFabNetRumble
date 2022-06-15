//--------------------------------------------------------------------------------------
// PlayFabLogin.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace PlayFab;
using namespace ClientModels;
using namespace NetRumble;

PlayFabLogin::PlayFabLogin() :
	m_ticketLen(0),
	m_steamTicket(0),
	m_loginCallback(nullptr)
{
	PlayFabSettings::staticSettings->titleId = NETRUMBLE_PLAYFAB_TITLE_ID;
}

void PlayFabLogin::ClearHostNetwork()
{
	DEBUGLOG("PlayFabLogin::ClearHostNetwork()\n");

	Json::Value param;

	param["entity"] = m_entityKey.ToJson();

	ClientModels::ExecuteCloudScriptRequest request;

	request.FunctionName = "clear_game_network";
	request.FunctionParameter = param;

	PlayFabClientAPI::ExecuteCloudScript(
		request,
		[](const ClientModels::ExecuteCloudScriptResult& result, void*)
		{
			if (result.Error.notNull())
			{
				DEBUGLOG("CloudScript error occured: %hs\n", result.Error->Message.c_str());
			}
			else
			{
				DEBUGLOG("ClearGameNetwork CloudScript succeeded\n");
			}
		},
		[](const PlayFabError& error, void*)
		{
			DEBUGLOG("ExecuteCloudScript failed: %hs\n", error.ErrorMessage.c_str());
		});
}

const std::string& PlayFabLogin::GetLocalPlayerName() const
{
	return m_localPlayerName;
}

void PlayFabLogin::SetLocalPlayerName()
{
	char machineName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD nameSize = ARRAYSIZE(machineName);
	GetComputerNameA(machineName, &nameSize);
	m_localPlayerName = machineName;
}

void PlayFabLogin::SetLoginProperties(const LoginResult& loginResult)
{
	m_playfabUserId = loginResult.PlayFabId;
	m_localPlayerName = loginResult.InfoResultPayload->AccountInfo->Username;

	m_pfEntityToken = loginResult.EntityToken;
	if (m_pfEntityToken.Entity.notNull())
	{
		// Login was successful
		m_entityKey = m_pfEntityToken.Entity;
		m_entityToken = m_pfEntityToken.EntityToken;
		m_pfLoginEntityKey = { m_entityKey.Id.c_str(), m_entityKey.Type.c_str() };

		// Initialize Multiplayer
		const auto& onlineManager = Managers::Get<OnlineManager>();
		onlineManager->InitializeMultiplayer();
		onlineManager->SetLocalUserId(std::stoull(m_playfabUserId, nullptr, 16));
		onlineManager->SetLocalEntityId(m_entityKey.Id);
		DEBUGLOG("PlayFab login succeeded, entityKey.Id = %s, entityKey.Type = %s, entityToken = %s, playfabUserId = %llx\n", m_entityKey.Id.c_str(), m_entityKey.Type.c_str(), m_entityToken.c_str(), onlineManager->GetLocalUserId());

		onlineManager->SetPartyLocalEntityId(m_entityKey.Id);
		onlineManager->SetPartyLocalEntityToken(m_entityToken);
		onlineManager->SetPartyEntityTokenExpireTime(m_pfEntityToken.TokenExpiration);

		g_game->m_isLoggedIn = true;
		g_game->LocalPlayerInitialize();
	}
}

void PlayFabLogin::Cleanup()
{
	m_pfLoginEntityKey = { nullptr, nullptr };
	m_entityToken.clear();
	m_localPlayerName.clear();
	m_playfabUserId.clear();
	m_loginCallback = nullptr;
	g_game->m_isLoggedIn = false;
}

void PlayFabLogin::Tick(float delta)
{
	UNREFERENCED_PARAMETER(delta);
	PlayFabClientAPI::Update();
}

void PlayFabLogin::LoginWithCustomId(std::function<void(bool, const std::string&)> callback)
{
	// Set local player name to its computer name when using LoginWithCustomId
	if (m_localPlayerName.empty())
	{
		SetLocalPlayerName();
	}

	LoginWithCustomIDRequest loginRequest;
	loginRequest.CreateAccount = true;
	loginRequest.CustomId = m_localPlayerName;

	GetPlayerCombinedInfoRequestParams reqParams;
	reqParams.GetUserAccountInfo = true;

	loginRequest.InfoRequestParameters = reqParams;

	PlayFabClientAPI::LoginWithCustomID(
		loginRequest,
		[this, callback](const LoginResult& loginResult, void*)
		{
			// Login was successful
			DEBUGLOG("PlayFab login with Custom ID callback\n");
			SetLoginProperties(loginResult);
			m_loginType = PlayFabLoginType::LoginWithCustomID;
			std::string displayName;
			if (loginResult.InfoResultPayload.notNull() &&
				loginResult.InfoResultPayload->AccountInfo.notNull() &&
				loginResult.InfoResultPayload->AccountInfo->TitleInfo.notNull())
			{
				displayName = loginResult.InfoResultPayload->AccountInfo->CustomIdInfo->CustomId;
			}

			if (displayName.empty())
			{
				UpdateUserTitleDisplayNameRequest updateDisplayNameRequest;

				// Set display name
				updateDisplayNameRequest.DisplayName = m_localPlayerName;

				PlayFabClientAPI::UpdateUserTitleDisplayName(
					updateDisplayNameRequest,
					[callback](const UpdateUserTitleDisplayNameResult& result, void*)
					{
						if (callback != nullptr)
						{
							callback(true, result.DisplayName);
						}
					},
					[callback](const PlayFabError& error, void*)
					{
						if (callback != nullptr)
						{
							std::string message(error.ErrorMessage);

							callback(false, message);
						}
					});
			}
			else
			{
				m_localPlayerName = displayName;

				// The user already has a display name set, trigger the success callback
				if (callback != nullptr)
				{
					std::string message;

					callback(true, message);
				}
			}
		},
		[this, callback](const PlayFabError& error, void*)
		{
			if (callback != nullptr)
			{
				std::string message(error.ErrorMessage);
				DEBUGLOG("Failed to login with Custom ID due to ERROR:[ %s ]", message.c_str());
				callback(false, message);
			}
		});
}

// Get authentication token for user
void PlayFabLogin::GetSteamTicket()
{
	m_steamTicket = SteamUser()->GetAuthSessionTicket(m_bSteamTicketBuffer, PlayFabLogin::TICKET_BUFFER_SIZE, &m_ticketLen);
}

void PlayFabLogin::CancelSteamTicket()
{
	SteamUser()->CancelAuthTicket(m_steamTicket);
	m_steamTicket = 0;
	ZeroMemory(m_bSteamTicketBuffer, TICKET_BUFFER_SIZE);
	m_ticketLen = 0;
}

void PlayFabLogin::LoginWithSteam(std::function<void(bool, const std::string&)> callback)
{
	if (callback)
	{
		m_loginCallback = callback;
	}
	GetSteamTicket();
}

// Record Steam authenication ticket and login to Steam, called from GetSteamTicket callback method OnGetAuthSessionTicketResponse
void PlayFabLogin::SetSteamAuthTicketHandle(HAuthTicket hAuthTicket)
{
	m_steamTicket = hAuthTicket;

	DoLoginToSteam();
}

// Build a hexadecimal string from a byte array
std::string PlayFabLogin::BuildHexString(unsigned char* byteArray, uint32 length) const
{
	std::stringstream s;
	s << std::hex << std::setfill('0');

	for (size_t i = 0; i < length; i++)
	{
		s << std::setw(2) << int(byteArray[i]);
	}

	return s.str();
}

// Login to Steam using a ticket
void PlayFabLogin::DoLoginToSteam()
{
	GetPlayerCombinedInfoRequestParams params;
	params.GetUserAccountInfo = true;
	params.GetTitleData = true;

	LoginWithSteamRequest request;
	request.CreateAccount = true;
	request.InfoRequestParameters = params;
	request.TitleId = NETRUMBLE_PLAYFAB_TITLE_ID;
	request.SteamTicket = BuildHexString(m_bSteamTicketBuffer, m_ticketLen);

	PlayFabClientAPI::LoginWithSteam(
		request,
		[this](const LoginResult& loginResult, void*)
		{
			DEBUGLOG("PlayFab login with Steam callback\n");
			SetLoginProperties(loginResult);
			m_loginType = PlayFabLoginType::LoginWithSteam;
			// The user already has a display name set, trigger the success callback
			if (m_loginCallback != nullptr)
			{
				std::string message;

				m_loginCallback(true, message);
			}
			CancelSteamTicket();
		},
		[this](const PlayFabError& error, void* customData)
		{
			UNREFERENCED_PARAMETER(customData);
			if (m_loginCallback != nullptr)
			{
				m_loginCallback(false, error.ErrorMessage);
			}

			CancelSteamTicket();
		});
}

void PlayFabLogin::OnGetAuthSessionTicketResponse(GetAuthSessionTicketResponse_t* pResponse)
{
	if (pResponse->m_eResult == k_EResultOK)
	{
		SetSteamAuthTicketHandle(pResponse->m_hAuthTicket);
	}
	else
	{
		CancelSteamTicket();

		if (m_loginCallback != nullptr)
		{
			m_loginCallback(false, "Failed to get a Steam auth ticket");
		}
	}
}

bool PlayFabLogin::GetUserId(std::string& outUserId) const
{
	if (!PlayFabClientAPI::IsClientLoggedIn())
	{
		return false;
	}

	outUserId = m_playfabUserId;
	return true;
}

bool PlayFabLogin::GetUserName(std::string& outUserName) const
{
	if (!PlayFabClientAPI::IsClientLoggedIn())
	{
		return false;
	}

	outUserName = m_localPlayerName;
	return true;
}

void PlayFabLogin::TryEntityTokenRefresh()
{
	GetPlayerCombinedInfoRequestParams params;
	params.GetUserAccountInfo = true;
	params.GetTitleData = true;

	LoginWithSteamRequest request;
	request.CreateAccount = false;
	request.InfoRequestParameters = params;
	request.TitleId = NETRUMBLE_PLAYFAB_TITLE_ID;
	request.SteamTicket = BuildHexString(m_bSteamTicketBuffer, m_ticketLen);

	PlayFabClientAPI::LoginWithSteam(
		request,
		[this](const LoginResult& loginResult, void*)
		{
			m_playfabUserId = loginResult.PlayFabId;
			m_localPlayerName = loginResult.InfoResultPayload->AccountInfo->Username;

			const auto& entityToken = loginResult.EntityToken;
			if (entityToken.notNull() && entityToken->Entity.notNull())
			{
				// Login was successful
				m_entityKey = entityToken->Entity;
				m_entityToken = entityToken->EntityToken;
				m_pfLoginEntityKey = { m_entityKey.Id.c_str(), m_entityKey.Type.c_str() };
			}

			if (m_loginCallback != nullptr)
			{
				m_loginCallback(true, m_playfabUserId);
			}

			CancelSteamTicket();
		},
		[this](const PlayFabError& error, void* customData)
		{
			UNREFERENCED_PARAMETER(customData);
			if (m_loginCallback != nullptr)
			{
				m_loginCallback(false, error.ErrorMessage);
			}

			CancelSteamTicket();
		});
}