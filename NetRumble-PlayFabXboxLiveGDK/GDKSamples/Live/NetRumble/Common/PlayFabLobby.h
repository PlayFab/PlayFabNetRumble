//--------------------------------------------------------------------------------------
// PlayFabLobby.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "pch.h"

namespace NetRumble
{
	static const char* LOBBY_PROPERTY_HOSTNAME = "string_key1";
	static const char* LOBBY_PROPERTY_NETWORKID = "string_key2";
	static const char* LOBBY_PROPERTY_DESCRIPTOR = "string_key3";
	static constexpr uint32_t MAX_LOBBY_COUNTS = 8; // Maximum count of lobby
	static constexpr uint32_t MAX_MEMBER_COUNT_PER_LOBBY = 4; // Maximum count of players the lobby
	static constexpr uint32_t LOBBY_PROPERTY_COUNT = 3;
	static constexpr uint32_t SEARCH_PROPERTY_COUNT = 1;

	struct LobbySearchResult
	{
		// The ID of the found lobby.
		std::string_view lobbyId;
		// The connection string of the found lobby.
		std::string_view connectionString;
		// The current owner of the lobby.
		std::string_view ownerEntityKeyId;
		std::string_view ownerEntityKeyType;
		// The maximum number of members that can be present in this lobby.
		uint32_t maxMemberCount{ 0 };
		// The current number of members that are present in this lobby.
		uint32_t currentMemberCount{ 0 };
		// The number of search properties associated with this lobby.
		uint32_t searchPropertyCount{ 0 };
		// The <searchPropertyKey, searchPropertyValue> pairs of the search properties associated with this lobby.
		std::unordered_map<std::string_view, std::string_view> searchProperties;
		// The number of friends in the found lobby.
		// If the lobby search which generated this search result was not performed with a
		// "PFLobbySearchFriendsFilter", this value will always be 0.
		// Only bidirectional friends will be returned in this search result.
		// That is, the user querying for the lobby and the user in the lobby must both be friends with each other.
		uint32_t friendCount{ 0 };
		// The list of friends in the found lobby, if the lobby search was performed with a "PFLobbySearchFriendsFilter".
		// { <FriendEntityKey1.Id, FriendEntityKey1.type>, <FriendEntityKey2.Id, FriendEntityKey2.type>, ..., <FriendEntityKeyN.Id, FriendEntityKeyN.type> } 
		std::unordered_map<std::string_view, std::string_view> friends;
	};

	using FindLobbyCallback = std::function<void(std::vector<std::shared_ptr<LobbySearchResult>>)>;

	// An example scenario of using lobby:
	// A player wants to play with other people.
	// Steps: 
	// 1. The player starts a multiplayer game
	// 2. Invites friends
	// 3. Waits for others to join
	class PlayFabLobby
	{
	public:
		PlayFabLobby() = default;
		~PlayFabLobby() = default;
		PlayFabLobby(const PlayFabLobby&) = delete;
		PlayFabLobby(PlayFabLobby&&) = delete;
		PlayFabLobby& operator=(const PlayFabLobby&) = delete;
		PlayFabLobby& operator=(PlayFabLobby&&) = delete;

		void DoWork();
		void CreateLobby(PFLobbyAccessPolicy accessPolicy);
		void FindLobbies();
		void JoinLobby(const std::string& connectionString);
		void LeaveLobby();
		void SetFindLobbyCallback(FindLobbyCallback completionCallback);
		static const char* GetCmdLineFlag() { static const char* pCmdLineFlag = "cmd:"; return pCmdLineFlag; }
		void UpdateLobbyState(PFLobbyAccessPolicy accessPolicy);
		void JoinArrangedLobby(const char* lobbyArrangementString);
		PFLobbyHandle& GetLobbyHandle() { return m_lobbyHandle; }
		void Cleanup();

		uint32_t m_CurrentMemberCount{ 0 };
		uint32_t m_MatchmakingMemberCount{ 0 };

	private:
		// PFLobbyStateChange Functions
		// The operation started by a previous call to "PFMultiplayerCreateAndJoinLobby()" completed.
		void OnCreateAndJoinLobbyCompleted(const PFLobbyStateChange* change);
		// The operation started by a previous call to "PFMultiplayerJoinLobby()" completed.
		void OnJoinLobbyCompleted(const PFLobbyStateChange* change);
		// A PlayFab entity was added to a lobby as a member.
		void OnMemberAdded(const PFLobbyStateChange* change);
		// The operation started by a previous call to "PFLobbyAddMember()" completed.
		void OnAddMemberCompleted(const PFLobbyStateChange* change);
		// A PlayFab entity was removed from a lobby as a member.
		void OnMemberRemoved(const PFLobbyStateChange* change);
		// A lobby was updated.
		void OnUpdated(const PFLobbyStateChange* change);
		// The client has disconnected from a lobby.
		void OnDisconnected(const PFLobbyStateChange* change);
		void OnLeaveLobbyCompleted(const PFLobbyStateChange* change);
		// The operation started by a previous call to "PFMultiplayerJoinArrangedLobby()" completed.
		void OnJoinArrangedLobbyCompleted(const PFLobbyStateChange* change);
		// The operation started by a previous call to "PFMultiplayerFindLobbies()" completed.
		void OnFindLobbiesCompleted(const PFLobbyStateChange* change);
		// An entity on this client has received an invite to a lobby.
		void OnInviteReceived(const PFLobbyStateChange* change);
		// An invite listener's status has changed.
		void OnInviteListenerStatusChanged(const PFLobbyStateChange* change);
		// The operation started by a previous call to "PFLobbySendInvite()" completed.
		void OnSendInviteCompleted(const PFLobbyStateChange* change);

		std::string GetConnectString();
		void RefreshLobbyOwner();
		void UpdateLobbyNetwork(std::string networkId, std::string descriptor);
		void UpdateArrangedLobbyNetwork();
		void TryProcessLobbyStateChanges();
		std::string m_lobbyId;
		PFLobbyHandle m_lobbyHandle{};
		std::vector<std::shared_ptr<LobbySearchResult>> m_pfLobbySearchResults;
		bool m_arrangedLobbyNetworkReady{ false };
		std::string m_arrangedLobbyNetworkId;
		std::string m_arrangedLobbyNetworkDescriptor;

		FindLobbyCallback m_FindLobbyCallback;
	};
}

