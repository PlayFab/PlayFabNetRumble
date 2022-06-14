#pragma once

namespace NetRumble
{
	using FindLobbyCallback = std::function<void(std::vector<std::shared_ptr<OnlineUser>>)>;

	class Lobby
	{
	public:

		void CreateLobby();
		void FindLobbies();
		void JoinLobby(CSteamID steamLobbyID);
		void LeaveLobby();

		void SetReadyStateInLobby(bool readyState);
		void SetShipColorInLobby(byte shipColor);
		void SetShipVariationInLobby(byte ShipVariation);

		void PreparingGameServer();
		void SetFindLobbyCallback(FindLobbyCallback completionCallback);
		void SetIsServer(bool isServer) { m_isServer = isServer; }
		bool IsServer() const { return m_isServer; };
		inline const CSteamID GetSteamIDLobby() { return m_steamIDLobby; }
		void AcceptInvitationWhenGameNotStart(const char* pLobbyID);
		void InviteFriendsIntoLobby();

	private:
		CSteamID m_steamIDLobby;
		bool m_isServer = false;

		void OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure);
		CCallResult<Lobby, LobbyCreated_t> m_SteamCallResultLobbyCreated;

		void OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure);
		CCallResult<Lobby, LobbyEnter_t> m_SteamCallResultLobbyEntered;

		void OnLobbyMatchListCallback(LobbyMatchList_t* pLobbyMatchList, bool bIOFailure);
		CCallResult<Lobby, LobbyMatchList_t> m_SteamCallResultLobbyMatchList;
		FindLobbyCallback m_FindLobbyCallback;

		STEAM_CALLBACK(Lobby, OnLobbyDataUpdate, LobbyDataUpdate_t);
		STEAM_CALLBACK(Lobby, OnLobbyChatUpdate, LobbyChatUpdate_t);

		STEAM_CALLBACK(Lobby, OnLobbyGameCreated, LobbyGameCreated_t);

		STEAM_CALLBACK(Lobby, OnPersonaStateChange, PersonaStateChange_t);
		STEAM_CALLBACK(Lobby, OnGameJoinRequested, GameRichPresenceJoinRequested_t);

		// Callbacks for Invitations
		STEAM_CALLBACK(Lobby, AcceptFriendInvitationsInMenuOrLobby, GameLobbyJoinRequested_t);

		bool CheckAllPlayerReady();
		void AddPlayerStateToPeers(uint64_t steamUserID, bool isLocalPlayer = false);
		void UserBeInvitedIntoLobby(CSteamID invitedLobbyID);

	};

}