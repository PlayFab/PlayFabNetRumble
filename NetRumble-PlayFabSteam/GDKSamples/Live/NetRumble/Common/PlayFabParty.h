//--------------------------------------------------------------------------------------
// PlayFabParty.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Party.h"
#include "Manager.h"
#include "NetworkMessages.h"

namespace NetRumble
{
	enum class NetworkManagerState
	{
		Initialize,
		WaitingForNetwork,
		NetworkConnected,
		Migrating,
		NetworkDisconnected,
		Leaving
	};

	class PlayFabParty : public Manager
	{
	public:
		PlayFabParty() = default;
		virtual ~PlayFabParty();

		void Initialize();
		void CreatePlayFabParty(const std::string& networkId);
		void CreateLocalUser();
		void CreateAndConnectToNetwork(const char* networkId, std::function<void(std::string)> onNetworkCreated = nullptr);
		void ConnectToNetwork(const char* networkId, const char* descriptor, std::function<void(void)> onNetworkConnected = nullptr);
		void SendGameMessage(const GameMessage& message);
		void SetGameMessageHandler(std::function<void(std::string, std::shared_ptr<GameMessage>)> onMessageReceived);
		void SetEndpointChangeHandler(std::function<void(uint64_t, bool)> onEndpointChanged);
		void SendTextAsVoice(std::string text);
		void SendTextMessage(std::string text);
		void LeaveNetwork(std::function<void(void)> onNetworkDestroyed = nullptr);
		void MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool, const char*)> onMigrationComplete = nullptr);
		void MigrateToNetwork(const char* descriptor, std::function<void(bool)> callback = nullptr);
		void Shutdown();

		void DoWork();

		void TryEntityTokenRefresh();

		// PartyStateChange Functions
		void OnRegionsChanged(const Party::PartyStateChange* change);
		void OnDestroyLocalUserCompleted(const Party::PartyStateChange* change);
		void OnCreateNewNetworkCompleted(const Party::PartyStateChange* change);
		void OnConnectToNetworkCompleted(const Party::PartyStateChange* change);
		void OnAuthenticateLocalUserCompleted(const Party::PartyStateChange* change);
		void OnNetworkConfigurationMadeAvailable(const Party::PartyStateChange* change);
		void OnNetworkDescriptorChanged(const Party::PartyStateChange* change);
		void OnLocalUserRemoved(const Party::PartyStateChange* change);
		void OnRemoveLocalUserCompleted(const Party::PartyStateChange* change);
		void OnLocalUserKicked(const Party::PartyStateChange* change);
		void OnCreateEndpointCompleted(const Party::PartyStateChange* change);
		void OnDestroyEndpointCompleted(const Party::PartyStateChange* change);
		void OnEndpointCreated(const Party::PartyStateChange* change);
		void OnEndpointDestroyed(const Party::PartyStateChange* change);
		void OnRemoteDeviceCreated(const Party::PartyStateChange* change);
		void OnRemoteDeviceDestroyed(const Party::PartyStateChange* change);
		void OnRemoteDeviceJoinedNetwork(const Party::PartyStateChange* change);
		void OnRemoteDeviceLeftNetwork(const Party::PartyStateChange* change);
		void OnDevicePropertiesChanged(const Party::PartyStateChange* change);
		void OnLeaveNetworkCompleted(const Party::PartyStateChange* change);
		void OnNetworkDestroyed(const Party::PartyStateChange* change);
		void OnEndpointMessageReceived(const Party::PartyStateChange* change);
		void OnDataBuffersReturned(const Party::PartyStateChange* change);
		void OnEndpointPropertiesChanged(const Party::PartyStateChange* change);
		void OnSynchronizeMessagesBetweenEndpointsCompleted(const Party::PartyStateChange* change);
		void OnCreateInvitationCompleted(const Party::PartyStateChange* change);
		void OnRevokeInvitationCompleted(const Party::PartyStateChange* change);
		void OnInvitationCreated(const Party::PartyStateChange* change);
		void OnInvitationDestroyed(const Party::PartyStateChange* change);
		void OnNetworkPropertiesChanged(const Party::PartyStateChange* change);
		void OnKickDeviceCompleted(const Party::PartyStateChange* change);
		void OnKickUserCompleted(const Party::PartyStateChange* change);
		void OnCreateChatControlCompleted(const Party::PartyStateChange* change);
		void OnDestroyChatControlCompleted(const Party::PartyStateChange* change);
		void OnChatControlCreated(const Party::PartyStateChange* change);
		void OnChatControlDestroyed(const Party::PartyStateChange* change);
		void OnSetChatAudioEncoderBitrateCompleted(const Party::PartyStateChange* change);
		void OnChatTextReceived(const Party::PartyStateChange* change);
		void OnVoiceChatTranscriptionReceived(const Party::PartyStateChange* change);
		void OnSetChatAudioInputCompleted(const Party::PartyStateChange* change);
		void OnSetChatAudioOutputCompleted(const Party::PartyStateChange* change);
		void OnLocalChatAudioInputChanged(const Party::PartyStateChange* change);
		void OnLocalChatAudioOutputChanged(const Party::PartyStateChange* change);
		void OnSetTextToSpeechProfileCompleted(const Party::PartyStateChange* change);
		void OnSynthesizeTextToSpeechCompleted(const Party::PartyStateChange* change);
		void OnSetLanguageCompleted(const Party::PartyStateChange* change);
		void OnSetTranscriptionOptionsCompleted(const Party::PartyStateChange* change);
		void OnSetTextChatOptionsCompleted(const Party::PartyStateChange* change);
		void OnChatControlPropertiesChanged(const Party::PartyStateChange* change);
		void OnChatControlJoinedNetwork(const Party::PartyStateChange* change);
		void OnChatControlLeftNetwork(const Party::PartyStateChange* change);
		void OnConnectChatControlCompleted(const Party::PartyStateChange* change);
		void OnDisconnectChatControlCompleted(const Party::PartyStateChange* change);
		void OnPopulateAvailableTextToSpeechProfilesCompleted(const Party::PartyStateChange* change);
		void OnConfigureAudioManipulationVoiceStreamCompleted(const Party::PartyStateChange* change);
		void OnConfigureAudioManipulationCaptureStreamCompleted(const Party::PartyStateChange* change);
		void OnConfigureAudioManipulationRenderStreamCompleted(const Party::PartyStateChange* change);

		const char* GetLocalUserEntityId() const { return m_localEntityId.c_str(); }

		const char* GetLanguageCode() const { return m_languageCode.c_str(); }

		inline NetworkManagerState GetState() const { return m_state; }
		inline bool IsConnected() const { return m_state == NetworkManagerState::NetworkConnected; }
		inline bool IsHost() const { return m_host == true; }
		inline void SetHost(bool isHost) { m_host = isHost; }
		inline bool IsPartyInitialized() const { return m_partyInitialized; }
		inline void SetPartyLocalEntityId(std::string& entityId) { m_localEntityId = entityId; }
		inline void SetPartyLocalEntityToken(std::string& entityId) { m_localEntityToken = entityId; }
		inline void SetPartyEntityTokenExpireTime(time_t expireTime) { m_localEntityTokenExpirationTime = expireTime; }

		void AddRemoteUser(uint64_t uid, const char* entityId);
		void RemoveRemoteUser(uint64_t uid, const char* entityId);
		void PopulatePartyRegionLatencies(bool send = true);

		uint64_t GetUidFromEntityId(const char* entityId);

	private:
		Party::PartyInvitationConfiguration GetPartyInvitationConfiguration(const char* networkId);
		Party::PartyNetworkConfiguration GetPartyNetworkConfiguration();
		bool InternalConnectToNetwork(const char* networkId, Party::PartyNetworkDescriptor& descriptor, Party::PartyNetwork** network, Party::PartyLocalEndpoint** endpoint);
		void CreateLocalChatControl();
		std::string DisplayNameFromChatControl(Party::PartyChatControl* control);
		bool ReportPartyError(const PartyError& error);

		std::function<void(std::string)> m_onNetworkCreated;
		std::function<void(void)> m_onNetworkConnected;
		std::function<void(void)> m_onNetworkDestroyed;
		std::function<void(bool)> m_onNetworkMigrated;
		std::function<void(bool, const char*)> m_onRegionMigrated;
		std::function<void(std::string, std::shared_ptr<GameMessage>)> m_onMessageReceived;
		std::function<void(uint64_t, bool)> m_onEndpointChanged;
		NetworkManagerState m_state = NetworkManagerState::Initialize;
		std::map<std::string, Party::PartyChatControl*> m_chatControls;
		Party::PartyLocalEndpoint* m_localEndpoint = nullptr;
		Party::PartyNetwork* m_network = nullptr;
		Party::PartyLocalEndpoint* m_newLocalEndpoint = nullptr;
		Party::PartyNetwork* m_newNetwork = nullptr;
		Party::PartyLocalUser* m_localUser = nullptr;
		Party::PartyLocalChatControl* m_localChatControl = nullptr;
		bool m_partyInitialized = false;
		bool m_host = false;
		bool m_localUserReady = false;
		bool m_playfabLoginComplete = false;
		bool m_isRefreshingEntityToken = false;
		bool m_enableCognitiveServices = false;
		bool m_textChatFiltering = false;
		std::string m_languageCode = "en-US";
		std::string m_languageName = "English (United States)";
		std::string m_localEntityId;
		std::string m_localEntityToken;
		time_t m_localEntityTokenExpirationTime = 0;
		std::function<void(PartyError)> m_userCreatedCallback;
		std::map<std::string, uint64_t> m_entityIdToUid;
		std::map<uint64_t, std::string> m_uidToEntityId;
		Party::PartyNetworkDescriptor m_networkDescriptor{};
	};

}
