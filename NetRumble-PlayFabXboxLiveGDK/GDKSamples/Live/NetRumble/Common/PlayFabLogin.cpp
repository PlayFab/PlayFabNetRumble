//--------------------------------------------------------------------------------------
// PlayFabLogin.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include <XGame.h>

using namespace PlayFab;
using namespace ClientModels;
using namespace NetRumble;

namespace {
	constexpr char PF_AUTH_URL[] = "https://playfabapi.com";
}

PlayFabLogin::PlayFabLogin() :
	m_loginCallback(nullptr)
{
	PlayFabSettings::staticSettings->titleId = NETRUMBLE_PLAYFAB_TITLE_ID;
}

static const char* ToHexString(uint32_t value)
{
	std::stringstream ss;
	ss << std::hex << value;
	return ss.str().c_str();;
}

void PlayFabLogin::Initialize()
{
	XblInitArgs xblArgs = {};
	uint32_t scid = 0;
	XGameGetXboxTitleId(&scid);
	xblArgs.scid = ToHexString(scid); // Add your scid here that you got from the Partner Center web portal.
	const HRESULT hr = XblInitialize(&xblArgs);
	if (FAILED(hr))
	{
		// handle error: couldn't initialize Xbox Live.
		DEBUGLOG("Failed to get XblInitialize\n");
	}

	DEBUGLOG("XblInitialize succeeded\n");
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

void PlayFabLogin::SetLocalPlayerNameForCustomId()
{
	char machineName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD nameSize = ARRAYSIZE(machineName);
	GetComputerNameA(machineName, &nameSize);
	m_localPlayerName = machineName;
}

void PlayFabLogin::SetLoginProperties(const LoginResult& loginResult)
{
	m_playfabUserId = loginResult.PlayFabId;
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
	xblUser = nullptr;
	g_game->m_isLoggedIn = false;
}

void PlayFabLogin::HandleLoginFailure()
{
	const std::string errorMessage = "Unable to sign in";
	m_loginCallback(false, errorMessage);
}

void PlayFabLogin::Tick(float delta)
{
	UNREFERENCED_PARAMETER(delta);
	PlayFabClientAPI::Update();
}

void PlayFabLogin::LoginWithCustomId(std::function<void(bool, const std::string&)> callback)
{
	DEBUGLOG("PlayFabLogin::LoginWithCustomId()\n");

	// Set local player name to its computer name when using LoginWithCustomId
	if (m_localPlayerName.empty())
	{
		SetLocalPlayerNameForCustomId();
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

HRESULT RequestTokenComplete(XAsyncBlock* asyncBlock)
{
	size_t bufferSize;
	HRESULT hr = XUserGetTokenAndSignatureResultSize(asyncBlock, &bufferSize);

	if (FAILED(hr))
	{
		DEBUGLOG("Failed to get XUserGetTokenAndSignatureResultSize: 0x%08X\n", static_cast<unsigned int>(hr));
	}

	std::vector<uint8_t> buffer(bufferSize);
	XUserGetTokenAndSignatureData* data = nullptr;

	if (SUCCEEDED(hr))
	{
		hr = XUserGetTokenAndSignatureResult(asyncBlock, buffer.size(), buffer.data(), &data, nullptr /*bufferUsed*/);

		if (FAILED(hr))
		{
			DEBUGLOG("Failed to get XUserGetTokenAndSignatureResult: 0x%08X\n", static_cast<unsigned int>(hr));
		}

		DEBUGLOG("XUserGetTokenAndSignatureResult succeeded.\n\tToken: %s\n\tSignature: %s\n", data ? data->token : "nullptr", data ? data->signature : "nullptr");

		Managers::Get<OnlineManager>()->LoginWithXboxLiveUserId(data->token);
	}

	return S_OK;
}

HRESULT RequestTokenAsync(XblUserHandle user, XTaskQueueHandle queue, const char* url, bool forceRefresh)
{
	auto asyncBlock = std::make_unique<XAsyncBlock>();
	ZeroMemory(asyncBlock.get(), sizeof(*asyncBlock));
	asyncBlock->queue = queue;
	asyncBlock->callback = [](XAsyncBlock* ab)
	{
		RequestTokenComplete(ab);
		delete ab;
	};

	const XUserGetTokenAndSignatureOptions options = forceRefresh ? XUserGetTokenAndSignatureOptions::ForceRefresh : XUserGetTokenAndSignatureOptions::None;

	static const XUserGetTokenAndSignatureHttpHeader headers[] =
	{
		{ "Accept", "application/json"},
	};

	const HRESULT hr = XUserGetTokenAndSignatureAsync(
		user,
		options,
		"GET",
		url,
		ARRAYSIZE(headers),
		headers,
		0,
		nullptr,
		asyncBlock.get());

	if (SUCCEEDED(hr))
	{
		// The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
		// If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
		asyncBlock.release();
	}

	return S_OK;
}

void CALLBACK Identity_TrySignInDefaultUserSilently_Callback(_In_ XAsyncBlock* asyncBlock)
{
	XUserHandle user = nullptr;
	HRESULT hr = XUserAddResult(asyncBlock, &user);

	if (SUCCEEDED(hr))
	{
		DEBUGLOG("XUserAddResult succeeded\n");

		uint64_t xuid = {};
		hr = XUserGetId(user, &xuid);
		if (SUCCEEDED(hr))
		{
			Managers::Get<OnlineManager>()->SetXboxUser(user, xuid);
			RequestTokenAsync(user, asyncBlock->queue, PF_AUTH_URL, false);
		}
	}
	else
	{
		Managers::Get<OnlineManager>()->HandlingLoginFailure();
	}

	delete asyncBlock;
}

HRESULT Identity_TrySignInDefaultUserSilently(_In_ XTaskQueueHandle asyncQueue)
{
	XAsyncBlock* asyncBlock = new XAsyncBlock();
	asyncBlock->queue = asyncQueue;
	asyncBlock->callback = Identity_TrySignInDefaultUserSilently_Callback;

	// Request to silently sign in the default user.
	const HRESULT hr = XUserAddAsync(XUserAddOptions::None, asyncBlock);

	if (FAILED(hr))
	{
		DEBUGLOG("Failed to get XUserAddAsync\n");
		delete asyncBlock;
	}

	DEBUGLOG("XUserAddAsync succeeded\n");

	return hr;
}

void PlayFabLogin::LoginWithXboxLive(std::function<void(bool, const std::string&)> callback)
{
	if (callback)
	{
		m_loginCallback = callback;
	}

	Identity_TrySignInDefaultUserSilently(nullptr);
}

void PlayFabLogin::LoginWithXboxLiveUserId(std::string xboxToken)
{
	LoginWithXboxRequest loginRequest;
	loginRequest.XboxToken = xboxToken;
	loginRequest.CreateAccount = true;

	GetPlayerCombinedInfoRequestParams reqParams;
	reqParams.GetUserAccountInfo = true;
	reqParams.GetPlayerProfile = true;

	loginRequest.InfoRequestParameters = reqParams;
	PlayFabClientAPI::LoginWithXbox(
		loginRequest,
		[this](const LoginResult& loginResult, void*)
		{
			char displayName[XUserGamertagComponentClassicMaxBytes] = {};
			const HRESULT hr = XUserGetGamertag(xblUser, XUserGamertagComponent::Classic, XUserGamertagComponentClassicMaxBytes, displayName, nullptr);
			if (SUCCEEDED(hr))
			{
				m_localPlayerName = displayName;
				m_loginType = PlayFabLoginType::LoginWithXboxLive;
				SetLoginProperties(loginResult);
				if (!m_localPlayerName.empty())
				{
					SetXboxUser(xblUser, xblUserXuid);

					if (m_loginCallback != nullptr)
					{
						std::string message;
						m_loginCallback(true, message);
					}
				}
			}
		},
		[this](const PlayFabError& error, void*)
		{
			if (m_loginCallback != nullptr)
			{
				std::string message(error.ErrorMessage);
				DEBUGLOG("Failed to login with Xbox Live due to ERROR:[ %s ]", message.c_str());
				m_loginCallback(false, message);
			}
		}
		);
}

void PlayFabLogin::SetXboxUser(XUserHandle user, uint64_t xuid)
{
	xblUserXuid = xuid;
	if (!user)
	{
		return;
	}
	if (!xblUser || 0 != XUserCompare(user, xblUser))
	{
		xblUser = user;
	}
}

void PlayFabLogin::XblCleanup()
{
	static XAsyncBlock emptyBlock{ nullptr, nullptr, nullptr };
	XblCleanupAsync(&emptyBlock);
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
	LoginWithXboxRequest request;
	request.CreateAccount = false;
	request.InfoRequestParameters = params;
	request.TitleId = NETRUMBLE_PLAYFAB_TITLE_ID;
	request.XboxToken = GetEntityToken();
	PlayFabClientAPI::LoginWithXbox(
		request,
		[this](const LoginResult& loginResult, void*)
		{
			char displayName[XUserGamertagComponentClassicMaxBytes] = {};
			const HRESULT hr = XUserGetGamertag(xblUser, XUserGamertagComponent::Classic, XUserGamertagComponentClassicMaxBytes, displayName, nullptr);
			if (SUCCEEDED(hr))
			{
				m_localPlayerName = displayName;
				SetLoginProperties(loginResult);
				if (!m_localPlayerName.empty())
				{
					SetXboxUser(xblUser, xblUserXuid);

					if (m_loginCallback != nullptr)
					{
						std::string message;
						m_loginCallback(true, message);
					}
				}
			}
		},
		[this](const PlayFabError& error, void*)
		{
			if (m_loginCallback != nullptr)
			{
				std::string message(error.ErrorMessage);
				DEBUGLOG("Failed to login with Xbox Live due to ERROR:[ %s ]", message.c_str());
				m_loginCallback(false, message);
			}
		}
		);
}
