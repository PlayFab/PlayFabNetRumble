//--------------------------------------------------------------------------------------
// PlayFabParty.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "PartyImpl.h"
#include "STTOverlayScreen.h"

using namespace NetRumble;
using namespace Party;

namespace
{
#define REPORT_PARTY_FAILED(err) PARTY_FAILED(err) && ReportPartyError(err)

	PartyString GetErrorMessage(PartyError error)
	{
		PartyString errString = nullptr;

		PartyError err = PartyManager::GetErrorMessage(error, &errString);
		if (PARTY_FAILED(err))
		{
			DEBUGLOG("Failed to get error message for error %lu.\n", error);
			return "[ERROR]";
		}

		return errString;
	}

	std::string PartyStateChangeResultToReasonString(PartyStateChangeResult result)
	{
		switch (result)
		{
		case PartyStateChangeResult::Succeeded: return "Succeeded";
		case PartyStateChangeResult::UnknownError: return "An unknown error occurred";
		case PartyStateChangeResult::InternetConnectivityError: return "The local device has internet connectivity issues which caused the operation to fail";
		case PartyStateChangeResult::PartyServiceError: return "The CommunicationFabric service is unable to create a new network at this time";
		case PartyStateChangeResult::NoServersAvailable: return "There are no available servers in the regions specified by the call to PartyManager::CreateNewNetwork()";
		case PartyStateChangeResult::CanceledByTitle: return "Operation canceled by title.";
		case PartyStateChangeResult::UserCreateNetworkThrottled: return "The PartyLocalUser specified in the call to PartyManager::CreateNewNetwork() has created too many networks and cannot create new networks at this time";
		case PartyStateChangeResult::TitleNotEnabledForParty: return "The title has not been configured properly in the Party portal";
		case PartyStateChangeResult::NetworkLimitReached: return "The network is full and is not allowing new devices or users to join";
		case PartyStateChangeResult::NetworkNoLongerExists: return "The network no longer exists";
		case PartyStateChangeResult::NetworkNotJoinable: return "The network is not currently allowing new devices or users to join";
		case PartyStateChangeResult::VersionMismatch: return "The network uses a version of the CommunicationFabric library that is incompatible with this library";
		case PartyStateChangeResult::UserNotAuthorized: return "The specified user was not authorized";
		case PartyStateChangeResult::LeaveNetworkCalled: return "The network was gracefully exited by the local device";
		}
		return "Unknown enumeration value";
	}

	template <typename result_type>
	void LogResult(const result_type& err)
	{
		if (PARTY_FAILED(err->errorDetail))
		{
			DEBUGLOG("Error Detail: %hs\n", GetErrorMessage(err->errorDetail));
		}
		if (err->result != PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(err->result).c_str());
		}
	}

	std::string GetPartyStateChangeTypeString(PartyStateChangeType Type)
	{
		switch (Type)
		{
		case PartyStateChangeType::RegionsChanged:return "RegionsChanged"; break;
		case PartyStateChangeType::DestroyLocalUserCompleted:return "DestroyLocalUserCompleted"; break;
		case PartyStateChangeType::CreateNewNetworkCompleted:return "CreateNewNetworkCompleted"; break;
		case PartyStateChangeType::ConnectToNetworkCompleted:return "ConnectToNetworkCompleted"; break;
		case PartyStateChangeType::AuthenticateLocalUserCompleted:return "AuthenticateLocalUserCompleted"; break;
		case PartyStateChangeType::NetworkConfigurationMadeAvailable:return "NetworkConfigurationMadeAvailable"; break;
		case PartyStateChangeType::NetworkDescriptorChanged:return "NetworkDescriptorChanged"; break;
		case PartyStateChangeType::LocalUserRemoved:return "LocalUserRemoved"; break;
		case PartyStateChangeType::RemoveLocalUserCompleted:return "RemoveLocalUserCompleted"; break;
		case PartyStateChangeType::LocalUserKicked:return "LocalUserKicked"; break;
		case PartyStateChangeType::CreateEndpointCompleted:return "CreateEndpointCompleted"; break;
		case PartyStateChangeType::DestroyEndpointCompleted:return "DestroyEndpointCompleted"; break;
		case PartyStateChangeType::EndpointCreated:return "EndpointCreated"; break;
		case PartyStateChangeType::EndpointDestroyed:return "EndpointDestroyed"; break;
		case PartyStateChangeType::RemoteDeviceCreated:return "RemoteDeviceCreated"; break;
		case PartyStateChangeType::RemoteDeviceDestroyed:return "RemoteDeviceDestroyed"; break;
		case PartyStateChangeType::RemoteDeviceJoinedNetwork:return "RemoteDeviceJoinedNetwork"; break;
		case PartyStateChangeType::RemoteDeviceLeftNetwork:return "RemoteDeviceLeftNetwork"; break;
		case PartyStateChangeType::DevicePropertiesChanged:return "DevicePropertiesChanged"; break;
		case PartyStateChangeType::LeaveNetworkCompleted:return "LeaveNetworkCompleted"; break;
		case PartyStateChangeType::NetworkDestroyed:return "NetworkDestroyed"; break;
		case PartyStateChangeType::EndpointMessageReceived:return "EndpointMessageReceived"; break;
		case PartyStateChangeType::DataBuffersReturned:return "DataBuffersReturned"; break;
		case PartyStateChangeType::EndpointPropertiesChanged:return "EndpointPropertiesChanged"; break;
		case PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted:return "SynchronizeMessagesBetweenEndpointsCompleted"; break;
		case PartyStateChangeType::CreateInvitationCompleted:return "CreateInvitationCompleted"; break;
		case PartyStateChangeType::RevokeInvitationCompleted:return "RevokeInvitationCompleted"; break;
		case PartyStateChangeType::InvitationCreated:return "InvitationCreated"; break;
		case PartyStateChangeType::InvitationDestroyed:return "InvitationDestroyed"; break;
		case PartyStateChangeType::NetworkPropertiesChanged:return "NetworkPropertiesChanged"; break;
		case PartyStateChangeType::KickDeviceCompleted:return "KickDeviceCompleted"; break;
		case PartyStateChangeType::KickUserCompleted:return "KickUserCompleted"; break;
		case PartyStateChangeType::CreateChatControlCompleted:return "CreateChatControlCompleted"; break;
		case PartyStateChangeType::DestroyChatControlCompleted:return "DestroyChatControlCompleted"; break;
		case PartyStateChangeType::ChatControlCreated:return "ChatControlCreated"; break;
		case PartyStateChangeType::ChatControlDestroyed:return "ChatControlDestroyed"; break;
		case PartyStateChangeType::SetChatAudioEncoderBitrateCompleted:return "SetChatAudioEncoderBitrateCompleted"; break;
		case PartyStateChangeType::ChatTextReceived:return "ChatTextReceived"; break;
		case PartyStateChangeType::VoiceChatTranscriptionReceived:return "VoiceChatTranscriptionReceived"; break;
		case PartyStateChangeType::SetChatAudioInputCompleted:return "SetChatAudioInputCompleted"; break;
		case PartyStateChangeType::SetChatAudioOutputCompleted:return "SetChatAudioOutputCompleted"; break;
		case PartyStateChangeType::LocalChatAudioInputChanged:return "LocalChatAudioInputChanged"; break;
		case PartyStateChangeType::LocalChatAudioOutputChanged:return "LocalChatAudioOutputChanged"; break;
		case PartyStateChangeType::SetTextToSpeechProfileCompleted:return "SetTextToSpeechProfileCompleted"; break;
		case PartyStateChangeType::SynthesizeTextToSpeechCompleted:return "SynthesizeTextToSpeechCompleted"; break;
		case PartyStateChangeType::SetLanguageCompleted:return "SetLanguageCompleted"; break;
		case PartyStateChangeType::SetTranscriptionOptionsCompleted:return "SetTranscriptionOptionsCompleted"; break;
		case PartyStateChangeType::SetTextChatOptionsCompleted:return "SetTextChatOptionsCompleted"; break;
		case PartyStateChangeType::ChatControlPropertiesChanged:return "ChatControlPropertiesChanged"; break;
		case PartyStateChangeType::ChatControlJoinedNetwork:return "ChatControlJoinedNetwork"; break;
		case PartyStateChangeType::ChatControlLeftNetwork:return "ChatControlLeftNetwork"; break;
		case PartyStateChangeType::ConnectChatControlCompleted:return "ConnectChatControlCompleted"; break;
		case PartyStateChangeType::DisconnectChatControlCompleted:return "DisconnectChatControlCompleted"; break;
		case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted:return "PopulateAvailableTextToSpeechProfilesCompleted"; break;
		case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted:return "ConfigureAudioManipulationVoiceStreamCompleted"; break;
		case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted:return "ConfigureAudioManipulationCaptureStreamCompleted"; break;
		case PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted:return "ConfigureAudioManipulationRenderStreamCompleted"; break;
		}

		return "Unknown PartyStateChangeType";
	}

	void LogPartyStateChangeType(const PartyStateChange* change)
	{
		if (change)
		{
			DEBUGLOG("PartyStateChange: PartyStateChangeType::%s \n", GetPartyStateChangeTypeString(change->stateChangeType).c_str());
		}
		else
		{
			DEBUGLOG("PlayFabParty::LogPartyStateChangeType: change was null \n");
		}
	}
}

PlayFabParty::~PlayFabParty()
{
	DEBUGLOG("PlayFabParty::~PlayFabParty()\n");
}

void PlayFabParty::Initialize()
{
	DEBUGLOG("PlayFabParty::Initialize()\n");

	PartyManager& partyManager = PartyManager::GetSingleton();
	PartyError err;

	// Only initialize the Party manager once
	if (m_partyInitialized == false)
	{
		// Initialize PlayFab Party
		err = partyManager.Initialize(NETRUMBLE_PLAYFAB_TITLE_ID.data());
		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("Initialize failed: %hs\n", GetErrorMessage(err));
			return;
		}

		m_partyInitialized = true;
		CreateLocalUser();
	}
}

// Create a local user object
void PlayFabParty::CreateLocalUser()
{
	PartyManager& partyManager = PartyManager::GetSingleton();
	PartyError err;

	// Only create a local user object if it doesn't exist
	if (m_localUser == nullptr)
	{
		DEBUGLOG("CreateLocalUser with entityId %s\n", m_localEntityId.c_str());

		// Create a local user object
		err = partyManager.CreateLocalUser(
			m_localEntityId.c_str(),                    // User id
			m_localEntityToken.c_str(),                 // User entity token
			&m_localUser                                // OUT local user object
		);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("CreateLocalUser failed: %hs\n", GetErrorMessage(err));
			return;
		}
	}
}

void PlayFabParty::CreatePlayFabParty(const std::string& networkId)
{
	CreateAndConnectToNetwork(
		networkId.c_str(),
		[this](std::string descriptor)
		{
			DEBUGLOG("Create PlayFab Party complete\n");
			Managers::Get<OnlineManager>()->OnPFPartyNetworkCreated(descriptor);
		});
}

void PlayFabParty::AddRemoteUser(uint64_t uid, const char* entityId)
{
	m_uidToEntityId[uid] = entityId;
	m_entityIdToUid[entityId] = uid;
}

void PlayFabParty::RemoveRemoteUser(uint64_t uid, const char* entityId)
{
	m_uidToEntityId.erase(uid);
	m_entityIdToUid.erase(entityId);
}

// Create a chat control and set the audio input and output channel where Party will receive or forward data
// The chat control object manages the chat operations for the user on the specific device
void PlayFabParty::CreateLocalChatControl()
{
	// Only create local chat controls if they don't exist
	if (m_localChatControl == nullptr)
	{
		PartyManager& partyManager = PartyManager::GetSingleton();
		PartyLocalDevice* localDevice = nullptr;

		// Retrieve the local device
		PartyError err = partyManager.GetLocalDevice(&localDevice);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("GetLocalDevice failed: %hs\n", GetErrorMessage(err));
			return;
		}

		// Create a chat control for the local user on the local device
		err = localDevice->CreateChatControl(
			m_localUser,                                // Local user object
			m_languageCode.c_str(),                     // Language id
			nullptr,                                    // Async identifier
			&m_localChatControl                         // OUT local chat control
		);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("CreateChatControl failed: %hs\n", GetErrorMessage(err));
			return;
		}

		uint64_t xuid = Managers::Get<OnlineManager>()->m_playfabLogin.GetXboxUserXuid();
		auto xuidString = std::to_string(xuid);

		// Use automatic settings for the audio input device
		err = m_localChatControl->SetAudioInput(
			PartyAudioDeviceSelectionType::PlatformUserDefault, // Selection type
			xuidString.c_str(),                                        	// Device id
			nullptr                                             // Async identifier
		);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("SetAudioInput failed: %hs\n", GetErrorMessage(err));
			return;
		}

		// Use automatic settings for the audio output device
		err = m_localChatControl->SetAudioOutput(
			PartyAudioDeviceSelectionType::PlatformUserDefault, // Selection type
			xuidString.c_str(),                                        // Device id
			nullptr                                             // Async identifier
		);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("SetAudioOutput failed: %hs\n", GetErrorMessage(err));
		}

		// Get the available list of text to speech profiles
		err = m_localChatControl->PopulateAvailableTextToSpeechProfiles(nullptr);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("Populating available TextToSpeechProfiles failed: %s \n", GetErrorMessage(err));
		}
	}
}

void PlayFabParty::Shutdown()
{
	DEBUGLOG("PlayFabParty::Shutdown()\n");

	m_state = NetworkManagerState::Initialize;

	// This cleans up everything allocated in Initialize() and
	// should only be used when done with networking
	PartyManager::GetSingleton().Cleanup();

	m_localChatControl = nullptr;
	m_localEndpoint = nullptr;
	m_network = nullptr;
	m_localUser = nullptr;
	m_partyInitialized = false;
}

PartyInvitationConfiguration PlayFabParty::GetPartyInvitationConfiguration(const char* networkId)
{
	// Setup the network invitation configuration to use the network id as an invitation id and allow anyone to join.
	return PartyInvitationConfiguration{
		networkId,                                  // invitation identifier
		PartyInvitationRevocability::Anyone,        // revokability
		0,                                          // authorized user count
		nullptr                                     // authorized user list
	};
}

PartyNetworkConfiguration PlayFabParty::GetPartyNetworkConfiguration()
{
	PartyNetworkConfiguration cfg{};

	// Setup the network to allow 8 single-device players of any device type
	cfg.maxDeviceCount = 8;
	cfg.maxDevicesPerUserCount = 1;
	cfg.maxEndpointsPerDeviceCount = 1;
	cfg.maxUserCount = 8;
	cfg.maxUsersPerDeviceCount = 1;

	return cfg;
}

void PlayFabParty::CreateAndConnectToNetwork(const char* networkId, std::function<void(std::string)> callback)
{
	DEBUGLOG("PlayFabParty::CreateAndConnectToNetwork()\n");

	const auto& networkConfiguration = GetPartyNetworkConfiguration();
	const auto& invitationConfiguration = GetPartyInvitationConfiguration(networkId);
	PartyNetworkDescriptor networkDescriptor;

	// Create a new network descriptor
	PartyError err = PartyManager::GetSingleton().CreateNewNetwork(
		m_localUser,                                // Local User
		&networkConfiguration,                      // Network Config
		0,                                          // Region List Count
		nullptr,                                    // Region List
		&invitationConfiguration,                   // Invitation configuration
		nullptr,                                    // Async Identifier
		&networkDescriptor,                         // OUT network descriptor
		nullptr                                     // Applied initial invitation identifier
	);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("CreateNewNetwork failed: %hs\n", GetErrorMessage(err));
		return;
	}

	Managers::Get<OnlineManager>()->m_networkId = networkId;

	// Connect to the new network
	if (InternalConnectToNetwork(networkId, networkDescriptor, &m_network, &m_localEndpoint))
	{
		m_state = NetworkManagerState::WaitingForNetwork;
		m_onNetworkCreated = callback;
	}
}

void PlayFabParty::ConnectToNetwork(const char* networkId, const char* descriptor, std::function<void(void)> callback)
{
	DEBUGLOG("PlayFabParty::ConnectToNetwork()\n");

	PartyNetworkDescriptor networkDescriptor = {};

	// Deserialize the remote network's descriptor
	PartyError err = PartyManager::DeserializeNetworkDescriptor(descriptor, &networkDescriptor);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("ConnectToNetwork failed to deserialize descriptor: %hs\n", GetErrorMessage(err));
		return;
	}

	Managers::Get<OnlineManager>()->m_networkId = networkId;

	// Connect to the remote network
	if (InternalConnectToNetwork(networkId, networkDescriptor, &m_network, &m_localEndpoint))
	{
		m_state = NetworkManagerState::WaitingForNetwork;
		m_onNetworkConnected = callback;
	}
}

bool PlayFabParty::InternalConnectToNetwork(const char* networkId, PartyNetworkDescriptor& descriptor, PartyNetwork** network, PartyLocalEndpoint** endpoint)
{
	// This portion of connecting to the network is the same for
	// both creating a new and joining an existing network.

	PartyError err = PartyManager::GetSingleton().ConnectToNetwork(
		&descriptor,                                // Network descriptor
		nullptr,                                    // Async identifier
		network                                     // OUT network
	);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("ConnectToNetwork failed: %hs\n", GetErrorMessage(err));
		return false;
	}

	// Authenticate the local user on the network so we can participate in it
	err = (*network)->AuthenticateLocalUser(
		m_localUser,                                // Local user
		networkId,                                  // Invite value
		nullptr                                     // Async identifier
	);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("AuthenticateLocalUser failed: %hs\n", GetErrorMessage(err));
		return false;
	}

	CreateLocalChatControl();

	// Connect the local user chat control to the network so we can use VOIP
	err = (*network)->ConnectChatControl(
		m_localChatControl,                         // Local chat control
		nullptr                                     // Async identifier
	);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("ConnectChatControl failed: %hs\n", GetErrorMessage(err));
		return false;
	}

	// Establish a network endpoint for game message traffic
	err = (*network)->CreateEndpoint(
		m_localUser,                                // Local user
		0,                                          // Property Count
		nullptr,                                    // Property name keys
		nullptr,                                    // Property Values
		nullptr,                                    // Async identifier
		endpoint                                    // OUT local endpoint
	);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("Failed to CreateEndpoint: %hs\n", GetErrorMessage(err));
		return false;
	}

	return true;
}

void PlayFabParty::SendGameMessage(const GameMessage& message)
{
	if (m_localEndpoint)
	{
		std::vector<uint8_t> packet = message.Serialize();

		// Form the data packet into a data buffer structure
		PartyDataBuffer data[] = {
			{
				static_cast<const void*>(packet.data()),
				static_cast<uint32_t>(packet.size())
			},
		};

		PartySendMessageOptions deliveryOptions;

		// ShipInput and ShipData messages don't need to be sent reliably
		// or sequentially, but the rest are needed for gameplay
		switch (message.MessageType())
		{
		case GameMessageType::ShipInput:
		case GameMessageType::ShipData:
			deliveryOptions = PartySendMessageOptions::Default;
			break;

		default:
			deliveryOptions = PartySendMessageOptions::GuaranteedDelivery |
				PartySendMessageOptions::SequentialDelivery;
		}

		// Send out the message to all other peers
		PartyError err = m_localEndpoint->SendMessage(
			0,                                      // endpoint count; 0 = broadcast
			nullptr,                                // endpoint list
			deliveryOptions,                        // send message options
			nullptr,                                // configuration
			1,                                      // buffer count
			data,                                   // buffer
			nullptr                                 // async identifier
		);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("Failed to SendMessage: %hs\n", GetErrorMessage(err));
		}
	}
}

void PlayFabParty::SetGameMessageHandler(std::function<void(std::string, std::shared_ptr<GameMessage>)> callback)
{
	m_onMessageReceived = callback;
}

void PlayFabParty::SetEndpointChangeHandler(std::function<void(uint64_t, bool)> callback)
{
	m_onEndpointChanged = callback;
}

void PlayFabParty::SendTextMessage(std::string text)
{
	if (m_localChatControl != nullptr)
	{
		DEBUGLOG("Send text message: %hs\n", text.c_str());

		std::vector<PartyChatControl*> targets;

		for (const auto& item : m_chatControls)
		{
			PartyLocalChatControl* local = nullptr;

			item.second->GetLocal(&local);

			if (local == nullptr)
			{
				targets.push_back(item.second);
			}
		}

		PartyError err = m_localChatControl->SendText(
			static_cast<uint32_t>(targets.size()),  // Count of target controls
			targets.data(),                         // Target controls
			text.c_str(),                           // Text to synthesize
			0,                                      // Data buffer size
			nullptr                                 // Data buffer
		);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("Failed to SendText: %hs\n", GetErrorMessage(err));
		}

		// Toast the text on the screen
		Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(
			DisplayNameFromChatControl(m_localChatControl),
			text.c_str(),
			false
		);
	}
}

void PlayFabParty::SendTextAsVoice(std::string text)
{
	if (m_localChatControl != nullptr)
	{
		if (m_enableCognitiveServices)
		{
			DEBUGLOG("Requesting synthesis of: %hs\n", text.c_str());

			PartyError err = m_localChatControl->SynthesizeTextToSpeech(
				PartySynthesizeTextToSpeechType::VoiceChat,
				text.c_str(),                           // Text to synthesize
				nullptr                                 // Async identifier
			);

			if (REPORT_PARTY_FAILED(err))
			{
				g_game->WriteDebugLogMessage("Failed to SynthesizeTextToSpeech: %hs\n", GetErrorMessage(err));
			}
		}
	}
}

void PlayFabParty::LeaveNetwork(std::function<void(void)> callback)
{
	DEBUGLOG("PlayFabParty::LeaveNetwork()\n");

	m_network->DestroyEndpoint(m_localEndpoint, nullptr);
	if (m_state != NetworkManagerState::Leaving && m_network != nullptr)
	{
		m_state = NetworkManagerState::Leaving;
		m_onNetworkDestroyed = callback;

		// First destroy the chat control
		PartyLocalDevice* localDevice = nullptr;

		// Retrieve the local device
		PartyError err = PartyManager::GetSingleton().GetLocalDevice(&localDevice);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("GetLocalDevice failed: %hs\n", GetErrorMessage(err));
			m_network->LeaveNetwork(nullptr);
			return;
		}

		err = localDevice->DestroyChatControl(m_localChatControl, nullptr);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("DestroyChatControl failed: %hs\n", GetErrorMessage(err));
			m_network->LeaveNetwork(nullptr);
		}
	}
	else
	{
		if (callback != nullptr)
		{
			callback();
		}
	}
	Managers::Get<OnlineManager>()->m_networkId.clear();
	Managers::Get<OnlineManager>()->m_networkDescriptor.clear();
	Managers::Get<OnlineManager>()->m_connectionString.clear();
}

void PlayFabParty::MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool, const char*)> onMigrationComplete)
{
	DEBUGLOG("PlayFabParty::MigrateToNewNetwork()\n");

	if (regionList.size() == 0)
	{
		DEBUGLOG("No regions to migrate to.\n");
		if (onMigrationComplete)
		{
			onMigrationComplete(false, nullptr);
		}
		return;
	}

	auto networkConfiguration = GetPartyNetworkConfiguration();
	auto invitationConfiguration = GetPartyInvitationConfiguration(Managers::Get<OnlineManager>()->m_networkId.c_str());
	auto networkDescriptor = PartyNetworkDescriptor{};
	auto partyRegion = PartyRegion{};

	// We want to create the new network in the 'best' region for all players
	regionList.at(0).copy(partyRegion.regionName, regionList.at(0).size());

	// Create a new network descriptor
	PartyError err = PartyManager::GetSingleton().CreateNewNetwork(
		m_localUser,                                // Local User
		&networkConfiguration,                      // Network Config
		1,                                          // Region List Count
		&partyRegion,                               // Region List
		&invitationConfiguration,                   // Invitation configuration
		nullptr,                                    // Async Identifier
		&networkDescriptor,                         // OUT network descriptor
		nullptr                                     // Applied initial invitation identifier
	);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("CreateNewNetwork failed: %hs\n", GetErrorMessage(err));
		if (onMigrationComplete)
		{
			onMigrationComplete(false, nullptr);
		}
		return;
	}

	// Connect to the new network
	if (InternalConnectToNetwork(Managers::Get<OnlineManager>()->m_networkId.c_str(), networkDescriptor, &m_newNetwork, &m_newLocalEndpoint))
	{
		m_state = NetworkManagerState::Migrating;
		m_onRegionMigrated = onMigrationComplete;
	}
}

void PlayFabParty::MigrateToNetwork(const char* descriptor, std::function<void(bool)> callback)
{
	DEBUGLOG("PlayFabParty::MigrateToNetwork()\n");

	PartyNetworkDescriptor networkDescriptor = {};

	// Deserialize the remote network's descriptor
	PartyError err = PartyManager::DeserializeNetworkDescriptor(descriptor, &networkDescriptor);

	if (REPORT_PARTY_FAILED(err))
	{
		g_game->WriteDebugLogMessage("MigrateToNetwork failed to deserialize descriptor: %hs\n", GetErrorMessage(err));
		return;
	}

	// Connect to the remote network
	if (InternalConnectToNetwork(Managers::Get<OnlineManager>()->m_networkId.c_str(), networkDescriptor, &m_newNetwork, &m_newLocalEndpoint))
	{
		m_state = NetworkManagerState::Migrating;
		m_onNetworkMigrated = callback;
	}
}

uint64_t PlayFabParty::GetUidFromEntityId(const char* entityId)
{
	auto it = m_entityIdToUid.find(entityId);
	if (it != m_entityIdToUid.end())
	{
		return it->second;
	}

	return 0;
}

void PlayFabParty::DoWork()
{
	// Check for entity token refresh
	TryEntityTokenRefresh();

	uint32_t count;
	PartyStateChangeArray changes;

	if (m_partyInitialized == false)
	{
		DEBUGLOG("Party initialization failed\n");
		return;
	}

	// Start processing messages from PlayFab Party
	auto err = PartyManager::GetSingleton().StartProcessingStateChanges(
		&count,
		&changes
	);

	if (REPORT_PARTY_FAILED(err))
	{
		DEBUGLOG("StartProcessingStateChanges failed: %hs\n", GetErrorMessage(err));
		return;
	}

	for (uint32_t i = 0; i < count; i++)
	{
		const PartyStateChange* change = changes[i];
		if (change)
		{
			switch (change->stateChangeType)
			{
			case PartyStateChangeType::RegionsChanged: OnRegionsChanged(change); break;
			case PartyStateChangeType::DestroyLocalUserCompleted: OnDestroyLocalUserCompleted(change); break;
			case PartyStateChangeType::CreateNewNetworkCompleted: OnCreateNewNetworkCompleted(change); break;
			case PartyStateChangeType::ConnectToNetworkCompleted: OnConnectToNetworkCompleted(change); break;
			case PartyStateChangeType::AuthenticateLocalUserCompleted: OnAuthenticateLocalUserCompleted(change); break;
			case PartyStateChangeType::NetworkConfigurationMadeAvailable: OnNetworkConfigurationMadeAvailable(change); break;
			case PartyStateChangeType::NetworkDescriptorChanged: OnNetworkDescriptorChanged(change); break;
			case PartyStateChangeType::LocalUserRemoved: OnLocalUserRemoved(change); break;
			case PartyStateChangeType::RemoveLocalUserCompleted: OnRemoveLocalUserCompleted(change); break;
			case PartyStateChangeType::LocalUserKicked: OnLocalUserKicked(change); break;
			case PartyStateChangeType::CreateEndpointCompleted: OnCreateEndpointCompleted(change); break;
			case PartyStateChangeType::DestroyEndpointCompleted: OnDestroyEndpointCompleted(change); break;
			case PartyStateChangeType::EndpointCreated: OnEndpointCreated(change); break;
			case PartyStateChangeType::EndpointDestroyed: OnEndpointDestroyed(change); break;
			case PartyStateChangeType::RemoteDeviceCreated: OnRemoteDeviceCreated(change); break;
			case PartyStateChangeType::RemoteDeviceDestroyed: OnRemoteDeviceDestroyed(change); break;
			case PartyStateChangeType::RemoteDeviceJoinedNetwork: OnRemoteDeviceJoinedNetwork(change); break;
			case PartyStateChangeType::RemoteDeviceLeftNetwork: OnRemoteDeviceLeftNetwork(change); break;
			case PartyStateChangeType::DevicePropertiesChanged: OnDevicePropertiesChanged(change); break;
			case PartyStateChangeType::LeaveNetworkCompleted: OnLeaveNetworkCompleted(change); break;
			case PartyStateChangeType::NetworkDestroyed: OnNetworkDestroyed(change); break;
			case PartyStateChangeType::EndpointMessageReceived: OnEndpointMessageReceived(change); break;
			case PartyStateChangeType::DataBuffersReturned: OnDataBuffersReturned(change); break;
			case PartyStateChangeType::EndpointPropertiesChanged: OnEndpointPropertiesChanged(change); break;
			case PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted: OnSynchronizeMessagesBetweenEndpointsCompleted(change); break;
			case PartyStateChangeType::CreateInvitationCompleted: OnCreateInvitationCompleted(change); break;
			case PartyStateChangeType::RevokeInvitationCompleted: OnRevokeInvitationCompleted(change); break;
			case PartyStateChangeType::InvitationCreated: OnInvitationCreated(change); break;
			case PartyStateChangeType::InvitationDestroyed: OnInvitationDestroyed(change); break;
			case PartyStateChangeType::NetworkPropertiesChanged: OnNetworkPropertiesChanged(change); break;
			case PartyStateChangeType::KickDeviceCompleted: OnKickDeviceCompleted(change); break;
			case PartyStateChangeType::KickUserCompleted: OnKickUserCompleted(change); break;
			case PartyStateChangeType::CreateChatControlCompleted: OnCreateChatControlCompleted(change); break;
			case PartyStateChangeType::DestroyChatControlCompleted: OnDestroyChatControlCompleted(change); break;
			case PartyStateChangeType::ChatControlCreated: OnChatControlCreated(change); break;
			case PartyStateChangeType::ChatControlDestroyed: OnChatControlDestroyed(change); break;
			case PartyStateChangeType::SetChatAudioEncoderBitrateCompleted: OnSetChatAudioEncoderBitrateCompleted(change); break;
			case PartyStateChangeType::ChatTextReceived: OnChatTextReceived(change); break;
			case PartyStateChangeType::VoiceChatTranscriptionReceived: OnVoiceChatTranscriptionReceived(change); break;
			case PartyStateChangeType::SetChatAudioInputCompleted: OnSetChatAudioInputCompleted(change); break;
			case PartyStateChangeType::SetChatAudioOutputCompleted: OnSetChatAudioOutputCompleted(change); break;
			case PartyStateChangeType::LocalChatAudioInputChanged: OnLocalChatAudioInputChanged(change); break;
			case PartyStateChangeType::LocalChatAudioOutputChanged: OnLocalChatAudioOutputChanged(change); break;
			case PartyStateChangeType::SetTextToSpeechProfileCompleted: OnSetTextToSpeechProfileCompleted(change); break;
			case PartyStateChangeType::SynthesizeTextToSpeechCompleted: OnSynthesizeTextToSpeechCompleted(change); break;
			case PartyStateChangeType::SetLanguageCompleted: OnSetLanguageCompleted(change); break;
			case PartyStateChangeType::SetTranscriptionOptionsCompleted: OnSetTranscriptionOptionsCompleted(change); break;
			case PartyStateChangeType::SetTextChatOptionsCompleted: OnSetTextChatOptionsCompleted(change); break;
			case PartyStateChangeType::ChatControlPropertiesChanged: OnChatControlPropertiesChanged(change); break;
			case PartyStateChangeType::ChatControlJoinedNetwork: OnChatControlJoinedNetwork(change); break;
			case PartyStateChangeType::ChatControlLeftNetwork: OnChatControlLeftNetwork(change); break;
			case PartyStateChangeType::ConnectChatControlCompleted: OnConnectChatControlCompleted(change); break;
			case PartyStateChangeType::DisconnectChatControlCompleted: OnDisconnectChatControlCompleted(change); break;
			case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted: OnPopulateAvailableTextToSpeechProfilesCompleted(change); break;
			case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted: OnConfigureAudioManipulationVoiceStreamCompleted(change); break;
			case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted: OnConfigureAudioManipulationCaptureStreamCompleted(change); break;
			case PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted: OnConfigureAudioManipulationRenderStreamCompleted(change); break;
			}
		}
	}

	// Return the processed changes back to the PartyManager
	err = PartyManager::GetSingleton().FinishProcessingStateChanges(count, changes);

	if (REPORT_PARTY_FAILED(err))
	{
		DEBUGLOG("FinishProcessingStateChanges failed: %hs\n", GetErrorMessage(err));
	}
}

void PlayFabParty::PopulatePartyRegionLatencies(bool send)
{
	uint32_t regionCount;
	const PartyRegion* regionList;

	PartyError err = PartyManager::GetSingleton().GetRegions(&regionCount, &regionList);

	if (PARTY_SUCCEEDED(err))
	{
		DEBUGLOG("Populating Party Regions (%lu)\n", regionCount);

		std::string uiString;

		uiString.reserve(1024);
		uiString = "Party Regions:\n";

		for (uint32_t x = 0; x < regionCount; x++)
		{
			uiString += regionList[x].regionName;
			uiString += ":  ";
			uiString += std::to_string(regionList[x].roundTripLatencyInMilliseconds);
			uiString += " ms\n";

			DEBUGLOG("%20hs:  %lu ms\n", regionList[x].regionName, regionList[x].roundTripLatencyInMilliseconds);

			if (send)
			{
				// Tell the host about it
				std::string messageStr;

				messageStr += regionList[x].regionName;
				messageStr += ":";
				messageStr += std::to_string(regionList[x].roundTripLatencyInMilliseconds);

				Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::RegionLatency, messageStr));
			}

			// Track our latencies
			g_game->GetLocalPlayerState()->SetRegionLatency(
				regionList[x].regionName,
				regionList[x].roundTripLatencyInMilliseconds);
		}
	}
	else
	{
		DEBUGLOG("GetRegions() failed with %hs\n", GetErrorMessage(err));
	}
}

void PlayFabParty::OnRegionsChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyRegionsChangedStateChange* result = static_cast<const PartyRegionsChangedStateChange*>(change);
	if (result)
	{
		LogResult(result);
	}
}

void PlayFabParty::OnDestroyLocalUserCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnCreateNewNetworkCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyCreateNewNetworkCompletedStateChange* result = static_cast<const PartyCreateNewNetworkCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("CreateNewNetworkCompleted:  SUCCESS\n");
			PartyString entityId;
			result->localUser->GetEntityId(&entityId);
			DEBUGLOG("CreateNewNetworkCompleted:  EntityId: %s\n", entityId);
		}
		else
		{
			DEBUGLOG("CreateNewNetworkCompleted:  FAIL:  %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnConnectToNetworkCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyConnectToNetworkCompletedStateChange* result = static_cast<const PartyConnectToNetworkCompletedStateChange*>(change);
	if (!result)
	{
		return;
	}

	switch (result->result)
	{
	case PartyStateChangeResult::Succeeded:
	{
		DEBUGLOG("ConnectToNetworkCompleted:  SUCCESS\n");
		if (m_state != NetworkManagerState::Migrating)
		{
			m_state = NetworkManagerState::NetworkConnected;

			// Callback if ConnectToNetwork() was called
			if (m_onNetworkConnected)
			{
				m_onNetworkConnected();
			}

			// Callback if CreateAndConnectToNetwork() was called
			if (m_onNetworkCreated)
			{
				char descriptor[c_maxSerializedNetworkDescriptorStringLength + 1] = {};

				// Serialize our local network descriptor for other peers to use
				PartyError err = PartyManager::SerializeNetworkDescriptor(
					&result->networkDescriptor,
					descriptor
				);

				if (REPORT_PARTY_FAILED(err))
				{
					g_game->WriteDebugLogMessage("Failed to serialize network descriptor: %hs\n", GetErrorMessage(err));
					m_onNetworkCreated(std::string());
				}

				DEBUGLOG("Serialized value: %hs\n", descriptor);
				// Callback with the descriptor to be shared with connecting clients
				m_onNetworkCreated(std::string(descriptor));
			}
		}
		break;
	}
	case PartyStateChangeResult::InternetConnectivityError:
	{
		DEBUGLOG("Internet Connectivity Error");
		Managers::Get<GameStateManager>()->SwitchToState(GameState::InternetConnectivityError);
		break;
	}
	case PartyStateChangeResult::NetworkNoLongerExists:
	{
		DEBUGLOG("Network No Longer Exists");
		Managers::Get<GameStateManager>()->SwitchToState(GameState::NetworkNoLongerExists);
		break;
	}
	default:
		DEBUGLOG("ConnectToNetworkCompleted:  FAIL:  %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
		break;
	}
}

void PlayFabParty::OnAuthenticateLocalUserCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyAuthenticateLocalUserCompletedStateChange* result = static_cast<const PartyAuthenticateLocalUserCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("OnAuthenticateLocalUserCompleted succeeded\n");
		}
		else
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnNetworkConfigurationMadeAvailable(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnNetworkDescriptorChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnLocalUserRemoved(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	if (m_state != NetworkManagerState::Leaving && m_state != NetworkManagerState::Migrating)
	{
		DEBUGLOG("Unexpected local user removal!\n");
	}
}

void PlayFabParty::OnRemoveLocalUserCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnLocalUserKicked(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnCreateEndpointCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyCreateEndpointCompletedStateChange* result = static_cast<const PartyCreateEndpointCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("CreateEndpointCompleted:  SUCCESS\n");

			if (m_state == NetworkManagerState::Migrating)
			{
				// We can leave our old network now
				m_network->LeaveNetwork(nullptr);
			}
		}
		else
		{
			DEBUGLOG("CreateEndpointCompleted:  FAIL:  %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnDestroyEndpointCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnEndpointCreated(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyEndpointCreatedStateChange* result = static_cast<const PartyEndpointCreatedStateChange*>(change);
	if (result && m_state != NetworkManagerState::Migrating)
	{
		PartyString user = nullptr;
		PartyError err = result->endpoint->GetEntityId(&user);

		if (REPORT_PARTY_FAILED(err) || user == nullptr)
		{
			g_game->WriteDebugLogMessage("Unable to retrieve user id from endpoint: %hs\n", GetErrorMessage(err));
		}
		else
		{
			DEBUGLOG("Established endpoint with user %s\n", user);

			uint64_t xuid = GetUidFromEntityId(user);
			if (xuid == 0)
			{
				uint64_t  uid = std::stoull(user, nullptr, 16);
				AddRemoteUser(uid, user);
				DEBUGLOG("Add remote user %s, %d\n", user, uid);
			}
		}
	}
}

void PlayFabParty::OnEndpointDestroyed(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyEndpointDestroyedStateChange* result = static_cast<const PartyEndpointDestroyedStateChange*>(change);
	if (result)
	{
		DEBUGLOG("Endpoint is %hs\n", result->endpoint == m_localEndpoint ? "local" : "remote");
		DEBUGLOG("Reason: %d\n", result->reason);
		DEBUGLOG("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));

		if (result->endpoint == m_localEndpoint)
		{
			// Our endpoint was disconnected
			m_localEndpoint = nullptr;

			if (Managers::Get<GameStateManager>()->GetState() == GameState::MigratingNetwork)
			{
				DEBUGLOG("Migrating: Old endpoint destroyed, swapping to new\n");
				m_localEndpoint = m_newLocalEndpoint;
				m_newLocalEndpoint = nullptr;
			}
		}
		else
		{
			// Another user has disconnected
			PartyString user = nullptr;
			PartyError err = result->endpoint->GetEntityId(&user);

			if (REPORT_PARTY_FAILED(err))
			{
				g_game->WriteDebugLogMessage("Unable to retrieve user id from endpoint: %hs\n", GetErrorMessage(err));
				return;
			}

			uint64_t xuid = GetUidFromEntityId(user);

			if (xuid == 0)
			{
				uint64_t uid = std::stoull(user, nullptr, 16);
				RemoveRemoteUser(uid, user);
				DEBUGLOG("Remove remote user %s, %d\n", user, uid);
			}
		}
	}
}

void PlayFabParty::OnRemoteDeviceCreated(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnRemoteDeviceDestroyed(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnRemoteDeviceJoinedNetwork(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnRemoteDeviceLeftNetwork(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnDevicePropertiesChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnLeaveNetworkCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	if (m_state == NetworkManagerState::Migrating)
	{
		m_state = NetworkManagerState::NetworkConnected;

		if (Managers::Get<OnlineManager>()->IsHost())
		{
			PartyNetworkDescriptor descriptor{};

			PartyError err = m_network->GetNetworkDescriptor(&descriptor);

			if (REPORT_PARTY_FAILED(err))
			{
				g_game->WriteDebugLogMessage("Failed to get migrating network descriptor: %hs\n", GetErrorMessage(err));
				if (m_onRegionMigrated)
				{
					m_onRegionMigrated(false, nullptr);
				}
				return;
			}

			char descriptorString[c_maxSerializedNetworkDescriptorStringLength + 1] = {};

			// Serialize our local network descriptor for other peers to use
			err = PartyManager::SerializeNetworkDescriptor(
				&descriptor,
				descriptorString
			);

			if (REPORT_PARTY_FAILED(err))
			{
				g_game->WriteDebugLogMessage("Failed to serialize network descriptor: %hs\n", GetErrorMessage(err));
				if (m_onRegionMigrated)
				{
					m_onRegionMigrated(false, nullptr);
				}
				return;
			}

			if (m_onRegionMigrated)
			{
				m_onRegionMigrated(true, descriptorString);
			}
		}
		else
		{
			if (m_onRegionMigrated)
			{
				m_onRegionMigrated(true, nullptr);
			}
		}
	}
	else
	{
		m_state = NetworkManagerState::Initialize;

		if (m_onNetworkDestroyed)
		{
			m_onNetworkDestroyed();
			m_onNetworkCreated = nullptr;
			m_onNetworkConnected = nullptr;
			m_onNetworkDestroyed = nullptr;
			m_onNetworkMigrated = nullptr;
		}
	}
}

void PlayFabParty::OnNetworkDestroyed(const PartyStateChange* change)
{
	const PartyNetworkDestroyedStateChange* result = static_cast<const PartyNetworkDestroyedStateChange*>(change);
	if (result->reason == PartyDestroyedReason::Disconnected)
	{
		const char* errorMsg = GetErrorMessage(result->errorDetail);
		DEBUGLOG(errorMsg);

		Managers::Get<ScreenManager>()->ShowError("Failed to get network connection. Returning to start menu.", [this]() {
			Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
			Managers::Get<OnlineManager>()->SwitchToOnlineState(OnlineState::Ready);
			Managers::Get <OnlineManager>()->LeaveMultiplayerGame();
			m_state = NetworkManagerState::Leaving;
			Managers::Get<OnlineManager>()->Cleanup();
			});
		return;
	}
	LogPartyStateChangeType(change);

	m_network = nullptr;

	if (m_state == NetworkManagerState::Migrating)
	{
		DEBUGLOG("Migrating: Old network destroyed, swapping to new\n");
		m_network = m_newNetwork;
		m_newNetwork = nullptr;
	}
	else if (m_state != NetworkManagerState::Leaving)
	{
		Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
		Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
		DEBUGLOG("Unexpected network destruction!\n");
	}
}

void PlayFabParty::OnEndpointMessageReceived(const PartyStateChange* change)
{
	// This is spammy, but can be useful when debugging
	const PartyEndpointMessageReceivedStateChange* result = static_cast<const PartyEndpointMessageReceivedStateChange*>(change);
	if (result)
	{
		const uint8_t* buffer = static_cast<const uint8_t*>(result->messageBuffer);
		std::shared_ptr<GameMessage> packet = std::make_shared<GameMessage>(
			std::vector<uint8_t>(buffer, buffer + result->messageSize)
			);

		PartyString sender = nullptr;
		PartyError err = result->senderEndpoint->GetEntityId(&sender);

		if (PARTY_SUCCEEDED(err))
		{
			// Give the message to the game engine
			if (m_onMessageReceived)
			{
				if (sender != nullptr)
				{
					m_onMessageReceived(sender, packet);
				}
				else
				{
					DEBUGLOG("Message '%s' received from entityid %s but we don't know their xuid yet.\n", MessageTypeString(packet->MessageType()), sender);
				}
			}
		}
		else
		{
			DEBUGLOG("GetEntityId failed: %hs\n", GetErrorMessage(err));
		}
	}
}

void PlayFabParty::OnDataBuffersReturned(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnEndpointPropertiesChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnSynchronizeMessagesBetweenEndpointsCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnCreateInvitationCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnRevokeInvitationCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnInvitationCreated(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnInvitationDestroyed(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnNetworkPropertiesChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnKickDeviceCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnKickUserCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnCreateChatControlCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyCreateChatControlCompletedStateChange* result = static_cast<const PartyCreateChatControlCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("OnCreateChatControlCompleted succeeded\n");
		}
		else
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			DEBUGLOG("Error detail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnDestroyChatControlCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyDestroyChatControlCompletedStateChange* result = static_cast<const PartyDestroyChatControlCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("OnDestroyChatControlCompleted succeeded\n");
		}
		else
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			DEBUGLOG("Error detail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnChatControlCreated(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyChatControlCreatedStateChange* result = static_cast<const PartyChatControlCreatedStateChange*>(change);
	if (result)
	{
		PartyString sender = nullptr;
		PartyError err = result->chatControl->GetEntityId(&sender);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("GetEntityId failed: %hs\n", GetErrorMessage(err));
		}
		else
		{
			DEBUGLOG("Created ChatControl for %hs\n", sender);
			m_chatControls[sender] = result->chatControl;

			PartyLocalChatControl* local = nullptr;
			err = result->chatControl->GetLocal(&local);

			if (REPORT_PARTY_FAILED(err))
			{
				g_game->WriteDebugLogMessage("Failed to get LocalChatControl: %hs\n", GetErrorMessage(err));
			}
			else if (local == nullptr)
			{
				DEBUGLOG("ChatControl is remote\n");

				// Remote ChatControl added, set chat permissions
				err = m_localChatControl->SetPermissions(
					result->chatControl,
					PartyChatPermissionOptions::ReceiveAudio |
					PartyChatPermissionOptions::ReceiveText |
					PartyChatPermissionOptions::SendAudio
				);

				if (REPORT_PARTY_FAILED(err))
				{
					g_game->WriteDebugLogMessage("Failed to SetPermissions on ChatControl: %hs\n", GetErrorMessage(err));
				}
			}
		}
	}
}

void PlayFabParty::OnChatControlDestroyed(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyChatControlDestroyedStateChange* result = static_cast<const PartyChatControlDestroyedStateChange*>(change);
	if (result)
	{
		PartyString sender = nullptr;
		PartyError err = result->chatControl->GetEntityId(&sender);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("GetEntityId failed: %hs\n", GetErrorMessage(err));
		}
		else
		{
			DEBUGLOG("Destroyed ChatControl from %hs\n", sender);

			if (g_game->GetLocalPlayerState()->EntityId != sender)
			{
				g_game->RemovePlayerFromLobbyPeers(sender);
			}
			if (result->chatControl == m_localChatControl)
			{
				DEBUGLOG("Local ChatControl destroyed\n");
				m_localChatControl = nullptr;

				if (m_state == NetworkManagerState::Leaving)
				{
					// Continue the LeaveNetwork process
					m_network->LeaveNetwork(nullptr);
				}
			}
			else
			{
				m_chatControls.erase(sender);
			}
		}
	}
}

void PlayFabParty::OnSetChatAudioEncoderBitrateCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnChatTextReceived(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyChatTextReceivedStateChange* result = static_cast<const PartyChatTextReceivedStateChange*>(change);
	if (result)
	{
		// Toast the text on the screen
		std::string message;

		// First look for translations
		if (result->translationCount > 0)
		{
			// Since we only have one local chat control, there will only be one translation
			if (result->translations[0].result == PartyStateChangeResult::Succeeded)
			{
				message = result->translations[0].translation;
			}
			else
			{
				DEBUGLOG("Translation failed: %hs\n", PartyStateChangeResultToReasonString(result->translations[0].result).c_str());
			}
		}

		if (message.empty())
		{
			message = result->chatText;
		}

		Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(
			DisplayNameFromChatControl(result->senderChatControl),
			message,
			false
		);

		DEBUGLOG("Chat Text: %hs\n", message.c_str());
	}
}

void PlayFabParty::OnVoiceChatTranscriptionReceived(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyVoiceChatTranscriptionReceivedStateChange* result = static_cast<const PartyVoiceChatTranscriptionReceivedStateChange*>(change);
	if (result)
	{
		if (REPORT_PARTY_FAILED(result->errorDetail))
		{
			g_game->WriteDebugLogMessage("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));
		}

		if (result->result != PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
		}
		else if (result->transcription == nullptr)
		{
			DEBUGLOG("Transcription is null\n");
		}
		else
		{
			// Toast the text on the screen
			std::string message;

			// First look for translations
			if (result->translationCount > 0)
			{
				// Since we only have one local chat control, there will only be one translation
				if (result->translations[0].result == PartyStateChangeResult::Succeeded)
				{
					message = result->translations[0].translation;
				}
				else
				{
					DEBUGLOG("Translation failed: %hs\n", PartyStateChangeResultToReasonString(result->translations[0].result).c_str());
				}
			}

			if (message.empty())
			{
				message = result->transcription;
			}

			Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(
				DisplayNameFromChatControl(result->senderChatControl),
				message,
				true,
				result->type == PartyVoiceChatTranscriptionPhraseType::Hypothesis
			);

			DEBUGLOG("Chat Transcription: %hs\n", message.c_str());
		}
	}
}

void PlayFabParty::OnSetChatAudioInputCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartySetChatAudioInputCompletedStateChange* result = static_cast<const PartySetChatAudioInputCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("OnSetChatAudioInputCompleted succeeded\n");
		}
		else
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			Managers::Get<ScreenManager>()->ShowError("Voice chat failure");
		}
	}
}

void PlayFabParty::OnSetChatAudioOutputCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartySetChatAudioOutputCompletedStateChange* result = static_cast<const PartySetChatAudioOutputCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("OnSetChatAudioOutputCompleted succeeded\n");
		}
		else
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
			Managers::Get<ScreenManager>()->ShowError("Voice chat failure");
		}
	}
}

void PlayFabParty::OnLocalChatAudioInputChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyLocalChatAudioInputChangedStateChange* result = static_cast<const PartyLocalChatAudioInputChangedStateChange*>(change);
	if (result)
	{
		if (REPORT_PARTY_FAILED(result->errorDetail))
		{
			g_game->WriteDebugLogMessage("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnLocalChatAudioOutputChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyLocalChatAudioOutputChangedStateChange* result = static_cast<const PartyLocalChatAudioOutputChangedStateChange*>(change);
	if (result)
	{
		if (REPORT_PARTY_FAILED(result->errorDetail))
		{
			g_game->WriteDebugLogMessage("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnSetTextToSpeechProfileCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartySetTextToSpeechProfileCompletedStateChange* result = static_cast<const PartySetTextToSpeechProfileCompletedStateChange*>(change);
	if (result)
	{
		LogResult(result);
	}
}

void PlayFabParty::OnSynthesizeTextToSpeechCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartySynthesizeTextToSpeechCompletedStateChange* result = static_cast<const PartySynthesizeTextToSpeechCompletedStateChange*>(change);
	if (result)
	{
		LogResult(result);
	}
}

void PlayFabParty::OnSetLanguageCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnSetTranscriptionOptionsCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartySetTranscriptionOptionsCompletedStateChange* result = static_cast<const PartySetTranscriptionOptionsCompletedStateChange*>(change);
	if (result)
	{
		LogResult(result);
	}
}

void PlayFabParty::OnSetTextChatOptionsCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartySetTextChatOptionsCompletedStateChange* result = static_cast<const PartySetTextChatOptionsCompletedStateChange*>(change);
	if (result)
	{
		LogResult(result);
	}
}

void PlayFabParty::OnChatControlPropertiesChanged(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnChatControlJoinedNetwork(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnChatControlLeftNetwork(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnConnectChatControlCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyConnectChatControlCompletedStateChange* result = static_cast<const PartyConnectChatControlCompletedStateChange*>(change);
	if (result)
	{
		if (result->result == PartyStateChangeResult::Succeeded)
		{
			DEBUGLOG("OnConnectChatControlCompleted succeeded\n");
		}
		else
		{
			DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
			DEBUGLOG("Error detail: %hs\n", GetErrorMessage(result->errorDetail));
		}
	}
}

void PlayFabParty::OnDisconnectChatControlCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnPopulateAvailableTextToSpeechProfilesCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);

	const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange* result = static_cast<const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange*>(change);
	if (result)
	{
		uint32_t profileCount = 0;
		PartyTextToSpeechProfileArray profileList = nullptr;

		// Get the profile list
		PartyError err = result->localChatControl->GetAvailableTextToSpeechProfiles(
			&profileCount,
			&profileList
		);

		if (REPORT_PARTY_FAILED(err))
		{
			g_game->WriteDebugLogMessage("GetAvailableTextToSpeechProfiles failed: %s\n", GetErrorMessage(err));
			return;
		}

		// Loop the profiles looking for one that matches our language
		for (uint32_t j = 0; j < profileCount; j++)
		{
			PartyTextToSpeechProfile* profile = profileList[j];

			PartyString languageCode = nullptr;

			err = profile->GetLanguageCode(&languageCode);

			if (REPORT_PARTY_FAILED(err))
			{
				g_game->WriteDebugLogMessage("GetLanguageCode failed: %s\n", GetErrorMessage(err));
				continue;
			}

			if (m_languageCode == languageCode)
			{
				// Get the profile name
				PartyString profileName = nullptr;
				err = profile->GetIdentifier(&profileName);

				if (REPORT_PARTY_FAILED(err))
				{
					g_game->WriteDebugLogMessage("GetIdentifier failed: %s\n", GetErrorMessage(err));
				}

				DEBUGLOG("Setting TTS profile to: %s\n", profileName);

				// Set the profile to the first we find for our language
				// Ideally, the user would be able to select one
				err = result->localChatControl->SetTextToSpeechProfile(
					PartySynthesizeTextToSpeechType::VoiceChat,
					profileName,
					nullptr
				);

				if (REPORT_PARTY_FAILED(err))
				{
					g_game->WriteDebugLogMessage("SetTextToSpeechProfile to '%s' failed: %s\n", profileName, GetErrorMessage(err));
				}
				break;
			}
		}
	}
}

void PlayFabParty::OnConfigureAudioManipulationVoiceStreamCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnConfigureAudioManipulationCaptureStreamCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

void PlayFabParty::OnConfigureAudioManipulationRenderStreamCompleted(const PartyStateChange* change)
{
	LogPartyStateChangeType(change);
}

std::string PlayFabParty::DisplayNameFromChatControl(PartyChatControl* control)
{
	std::string sttuser;
	PartyString sender = nullptr;

	PartyError err = control->GetEntityId(&sender);
	if (REPORT_PARTY_FAILED(err) || sender == nullptr)
	{
		g_game->WriteDebugLogMessage("GetEntityId failed: %hs\n", GetErrorMessage(err));
		sttuser = "[ERROR]";
	}
	else
	{
		// Get the display name of the sender
		std::shared_ptr<PlayerState> playerInfo = g_game->GetPlayerState(sender);

		if (sender != 0 && playerInfo != nullptr)
		{
			sttuser = playerInfo->DisplayName;
		}
		else
		{
			sttuser = sender;
		}
	}

	return sttuser;
}

bool PlayFabParty::ReportPartyError(const PartyError& error)
{
	bool reportError = true;
	PartyString errString = nullptr;
	PartyError err = PartyManager::GetErrorMessage(error, &errString);

	if (std::string(errString).find("Unmapped", 0) != EOF)
	{
		reportError = false;
		DEBUGLOG("Unmapped error (%lu): %hs\n", error, errString);
	}

	return reportError;
}

void PlayFabParty::TryEntityTokenRefresh()
{
	// Do not refresh if we are in the middle of refreshing
	if (m_isRefreshingEntityToken)
	{
		return;
	}

	// Do not refresh if we haven't done the initial login
	if (m_playfabLoginComplete == false || m_localEntityTokenExpirationTime == 0)
	{
		return;
	}

	// Refresh if the token is about to expire
	time_t currentTime = std::time(nullptr);
	if (currentTime > m_localEntityTokenExpirationTime)
	{
		m_isRefreshingEntityToken = true;
		Managers::Get<OnlineManager>()->m_playfabLogin.TryEntityTokenRefresh();
	}
}
