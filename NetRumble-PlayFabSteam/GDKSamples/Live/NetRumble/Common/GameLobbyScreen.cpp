//--------------------------------------------------------------------------------------
// GameLobbyScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

const char* gameSessionInstructions = "Press X to toggle ready, Y to invite, LB/RB to change ship color/style, Press B to exit";
GameLobbyScreen::GameLobbyScreen() noexcept :
	m_ready{ false },
	m_showMsg{ false },
	m_invitingFriend{ false },
	m_exiting{ false },
	m_countdownTimer{ 5.0f },
	m_lastState{ GameState::Initialize }
{
	m_transitionOnTime = 1.0;
	m_transitionOffTime = 1.0;

	Managers::Get<AudioManager>()->PlaySoundTrack(false);

	m_inGameTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\Xbox_One_Controller_Front.png");
	m_readyTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\ready.png");

	Managers::Get<ScreenManager>()->SetBackgroundsVisible(true);
}

void GameLobbyScreen::HandleInput(float elapsedTime)
{
	if (!m_exiting)
	{
		MenuScreen::HandleInput(elapsedTime);

		if (Managers::Get<GameStateManager>()->GetState() != GameState::Lobby)
		{
			return;
		}

		InputManager* inputManager = Managers::Get<InputManager>();
		std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();

		if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::LeftShoulder) || inputManager->IsNewKeyPress(Keyboard::Keys::Left))
		{
			// Change ship color
			const byte newColor = IncrementShipColor(localPlayerState->ShipColor());
			localPlayerState->ShipColor(newColor);
			Managers::Get<OnlineManager>()->SendGameMessage(
				GameMessage(
					GameMessageType::PlayerState,
					g_game->GetLocalPlayerState()->SerializePlayerStateData()
				)
			);
		}
		else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::RightShoulder) || inputManager->IsNewKeyPress(Keyboard::Keys::Right))
		{
			// Change ship design
			byte newShip = (localPlayerState->ShipVariation() + 1) % Ship::MaxVariations;
			localPlayerState->ShipVariation(newShip);
			Managers::Get<OnlineManager>()->SendGameMessage(
				GameMessage(
					GameMessageType::PlayerState,
					g_game->GetLocalPlayerState()->SerializePlayerStateData()
				)
			);
		}
		else if (inputManager->IsNewKeyPress(Keyboard::Keys::Y))
		{
			DoInviteFriend();
		}
		else if (inputManager->IsNewKeyPress(Keyboard::Keys::A))
		{
			std::shared_ptr<PlayerState> playerState = g_game->GetLocalPlayerState();
			DoGameReadyState(playerState, !playerState->LobbyReady);
		}
	}
}

void GameLobbyScreen::DoInviteFriend()
{
	m_menuEntries.clear();
	m_invitingFriend = true;
	ISteamFriends* pSteamFriend = SteamFriends();
	int nFriends = pSteamFriend->GetFriendCount(k_EFriendFlagImmediate);
	int nMaxFriendCount = MAXCOUNT_SHOW_ONLINE_USER;
	for (int nIndex = 0; nIndex < nFriends; nIndex++)
	{
		CSteamID steamIDFriend = pSteamFriend->GetFriendByIndex(nIndex, k_EFriendFlagImmediate);
		if (pSteamFriend->GetFriendPersonaState(steamIDFriend) == k_EPersonaStateOffline)
		{
			continue;
		}
		const char* pName = pSteamFriend->GetFriendPersonaName(steamIDFriend);
		if (!pName)
		{
			continue;
		}
		std::shared_ptr<SteamFriend> pFriend = std::make_shared<SteamFriend>();
		if (pFriend)
		{
			pFriend->id = steamIDFriend;
			pFriend->name = pName;
			nMaxFriendCount--;
			if (nMaxFriendCount == 0)
			{
				break;
			}
			m_menuEntries.push_back(MenuEntry(pFriend->name, [this, pFriend]()
				{
					if (Managers::Get<OnlineManager>()->InviteSteamFriend(pFriend->id))
					{
						m_invitingFriend = false;
						SetSelectedEntry(INDEX_MENU_INVITE_USER);
					}
					else
					{
						std::string strErrMessage = "Invite user[" + pFriend->name;
						strErrMessage += "] Failed,Please check your network";
						ShowMsg(strErrMessage.c_str(), TIME_ERROR_SHOW);
					}
				}
			));
		}
	}
	if (m_menuEntries.size() > 0)
	{
		SetSelectedEntry(INDEX_MENU_DEFAULT);
	}
}

void GameLobbyScreen::DoGameReadyState(std::shared_ptr<PlayerState> playerState, bool gameReadyState)
{
	m_ready = gameReadyState;
	playerState->LobbyReady = gameReadyState;

	Managers::Get<OnlineManager>()->SendGameMessage(
		GameMessage(
			GameMessageType::PlayerState,
			playerState->SerializePlayerStateData()
		)
	);
}

void GameLobbyScreen::ShowMsg(const char* msg, ULONGLONG nTime)
{
	m_showMsg = true;
	m_strMsg = msg;
	m_menuEntries.clear();
	m_menuEntries.push_back(MenuEntry(m_strMsg.c_str()));
	SetSelectedEntry(INDEX_MENU_DEFAULT);
}

void GameLobbyScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
	GameState state = Managers::Get<GameStateManager>()->GetState();
	std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
	if (m_showMsg)
	{
		MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
		return;
	}
	if (!m_exiting)
	{
		if (!m_invitingFriend)
		{
			m_menuEntries.clear();
		}
		switch (state)
		{
		case GameState::MPHostGame:
		{
			m_menuEntries.emplace_back("Starting host session");
			break;
		}
		case GameState::LeavingToMainMenu:
		{
			m_menuEntries.emplace_back("Leaving lobby");
			break;
		}
		case GameState::MPMatchmaking:
		{
			m_menuEntries.emplace_back("Matchmaking: Waiting for players");
			break;
		}
		case GameState::JoinGameFromLobby:
		{
			m_menuEntries.emplace_back("Join game from lobby");
			break;
		}
		case GameState::MPJoinLobby:
		{
			m_menuEntries.emplace_back("Joining lobby");
			break;
		}
		case GameState::Lobby:
		{
			if (Managers::Get<OnlineManager>()->IsJoiningArrangedLobby())
			{
				m_menuEntries.emplace_back("Match found, synchronizing player data");
				break;
			}
			if (m_ready)
			{
				std::vector<std::shared_ptr<PlayerState>> players = g_game->GetAllPlayerStates();

				std::string message = players.size() == 1 ? "Waiting for more players" : "Waiting for game to start";

				m_menuEntries.emplace_back(message, [this, localPlayerState]()
					{
						DoGameReadyState(localPlayerState, false);
					});
			}
			else
			{
				if (!m_invitingFriend)
				{
					m_menuEntries.emplace_back("Press (A) to ready", [this, localPlayerState]()
						{
							DoGameReadyState(localPlayerState, true);
						});
					m_menuEntries.emplace_back("Press (Y) to invite friend", [this]()
						{
							DoInviteFriend();
						});
				}
			}

			bool lobbyReady = g_game->CheckAllPlayerReady();
			if (lobbyReady)
			{
				Managers::Get<GameStateManager>()->SwitchToState(GameState::MatchingGame);
			}
			break;
		}
		case GameState::MatchingGame:
		{
			m_countdownTimer -= elapsedTime;
			std::string message;
			if (m_countdownTimer < 0.5f)
			{
				if (Managers::Get<OnlineManager>()->IsHost())
				{
					Managers::Get<OnlineManager>()->SendGameMessage(
						GameMessage(
							GameMessageType::JoiningGame,
							0
						)
					);
					Managers::Get<OnlineManager>()->PopulatePartyRegionLatencies();

					Managers::Get<GameStateManager>()->SwitchToState(GameState::JoinGameFromLobby);
					Managers::Get<OnlineManager>()->UpdateLobbyState(PFLobbyAccessPolicy::Private);
				}
				else
				{
					m_countdownTimer = 0.0f;
				}
				message = "Game starting...";
			}
			else
			{
				message = "Match starts in ";
				message += std::to_string((int)m_countdownTimer);
				message += " seconds";
			}
			m_menuEntries.emplace_back(message);
			break;
		}
		case GameState::GameConnecting:
		{
			if (g_game->GetWorld()->IsGameInProgress())
			{
				Managers::Get<GameStateManager>()->SwitchToState(GameState::JoinGameFromLobby);
			}
			else
			{
				m_menuEntries.emplace_back("Game starting...");
			}

			break;
		}
		case GameState::InternetConnectivityError:
		{
			Managers::Get<ScreenManager>()->ShowError("Internet connectivity error. Returning to main menu.", []() {
				Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
				});
			break;
		}
		case GameState::NetworkNoLongerExists:
		{
			Managers::Get<ScreenManager>()->ShowError("Network no longer exists. Returning to main menu.", []() {
				Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
				});
			break;
		}
		case GameState::GameConnectFail:
		{
			m_countdownTimer -= elapsedTime;
			if (m_countdownTimer < 0.5f)
			{
				m_countdownTimer = 0.0f;
				m_connectFailMessage.clear();
				OnCancel();
			}
			else
			{
				m_menuEntries.emplace_back("Connect game fail:" + m_connectFailMessage);
			}
			break;
		}
		default:
			break;
		}
	}
	m_lastState = state;

	MenuScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
}

void GameLobbyScreen::Draw(float totalTime, float elapsedTime)
{
	if (!IsActive())
	{
		return;
	}

	if (State() == ScreenStateType::Active && !m_exiting)
	{
		RenderManager* renderManager = Managers::Get<RenderManager>();
		std::unique_ptr<RenderContext> renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);
		std::shared_ptr<DirectX::SpriteFont> spriteFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
		float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
		float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
		float scale = 0.35f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

		renderContext->Begin();
		if (m_invitingFriend)
		{
			XMVECTORF32 errorMsgColor = Colors::Yellow;
			SimpleMath::Vector2 errorMsgPosition = SimpleMath::Vector2(0, viewportHeight / 2.0f - 10);
			XMVECTOR size = spriteFont->MeasureString("Select friend to invite");
			float titleScale = scale * 1.5f;
			errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX(size) / 2.0f * titleScale;
			renderContext->DrawString(spriteFont, "Select friend to invite", errorMsgPosition, errorMsgColor, 0, DirectX::XMFLOAT2{ 0,0 }, titleScale);
		}

		std::vector<std::shared_ptr<PlayerState>> playerStates = g_game->GetAllPlayerStates();
		if (playerStates.size() > 0)
		{
			// Draw session members centered
			XMFLOAT2 memberPositions[] =
			{
				XMFLOAT2(viewportWidth * 0.15f, viewportHeight * 0.7f),
				XMFLOAT2(viewportWidth * 0.38f, viewportHeight * 0.7f),
				XMFLOAT2(viewportWidth * 0.61f, viewportHeight * 0.7f),
				XMFLOAT2(viewportWidth * 0.84f, viewportHeight * 0.7f)
			};

			float inGameTextureScale = 0.9f * scale * spriteFont->GetLineSpacing() / m_inGameTexture.Texture->Height();
			XMFLOAT2 inGameTextureOrigin = XMFLOAT2((float)m_inGameTexture.Texture->Width(), (float)m_inGameTexture.Texture->Height() / 2.0f);

			float readyTextureScale = 0.9f * scale * spriteFont->GetLineSpacing() / m_readyTexture.Texture->Height();
			XMFLOAT2 readyTextureOrigin = XMFLOAT2((float)m_readyTexture.Texture->Width(), (float)m_readyTexture.Texture->Height() / 2.0f);

			UINT memberNumber = 0;
			for (const auto& playerState : playerStates)
			{
				if (!playerState)
				{
					continue;
				}

				// Get member display name
				std::string memberName = "(Reserved)";
				if (!playerState->DisplayName.empty())
				{
					memberName = playerState->DisplayName;

					if (playerState->IsLocalPlayer)
					{
						memberName.append(" (You)");
					}
				}

				memberName = DX::ChsToUtf8(memberName);
				XMVECTOR memberNameLen = spriteFont->MeasureString(memberName.c_str());
				XMFLOAT2 memberOrigin = XMFLOAT2(XMVectorGetX(memberNameLen) / 2.0f, spriteFont->GetLineSpacing() / 2.0f);

				// Draw member display name
				if (playerState != nullptr && !playerState->IsInactive())
				{
					renderContext->DrawString(spriteFont, memberName.c_str(), memberPositions[memberNumber % 4], Ship::Colors[playerState->ShipColor()], 0, memberOrigin, scale);
				}
				else
				{
					renderContext->DrawString(spriteFont, memberName.c_str(), memberPositions[memberNumber % 4], Colors::LightGray, 0, memberOrigin, scale);
				}

				if (playerState != nullptr)
				{
					// Draw member status icons and ship
					XMFLOAT2 glyphCenterPosition = XMFLOAT2(memberPositions[memberNumber % 4].x, memberPositions[memberNumber % 4].y + spriteFont->GetLineSpacing() * 1.7f * scale);

					if (playerState->InGame)
					{
						// Draw inGame texture
						renderContext->Draw(m_inGameTexture, glyphCenterPosition, 0.0f, inGameTextureScale, Colors::White, TexturePosition::Centered);
					}
					else if (playerState->LobbyReady)
					{
						// Draw ready texture if playerState is ready
						renderContext->Draw(m_readyTexture, glyphCenterPosition, 0.0f, readyTextureScale, Colors::White, TexturePosition::Centered);
					}

					// Draw ship
					std::shared_ptr<Ship> ship = playerState->GetShip();
					if (ship)
					{
						XMFLOAT2 oldShipPosition = ship->Position;
						float oldShipRotation = ship->Rotation;

						ship->Position = glyphCenterPosition;
						ship->Position.x += ship->Radius + 10.0f;
						ship->Rotation = 0.0f;

						ship->Draw(0.0f, renderContext.get(), true, GetScaleMultiplierForViewport(viewportWidth, viewportHeight));

						ship->Rotation = oldShipRotation;
						ship->Position = oldShipPosition;
					}
				}

				// Adjust the column for the next row
				memberPositions[memberNumber % 4].y += spriteFont->GetLineSpacing() * scale * 3.7f;
				memberNumber++;
			}

			if (IsActive())
			{
				// Draw instructions centered at bottom of screen
				XMFLOAT2 position = XMFLOAT2(viewportWidth / 2.0f, viewportHeight * 0.95f);

				if (Managers::Get<GameStateManager>()->GetState() == GameState::Lobby)
				{
					XMVECTOR size = spriteFont->MeasureString(gameSessionInstructions);
					XMFLOAT2 origin = XMFLOAT2(XMVectorGetX(size) / 2.0f, spriteFont->GetLineSpacing() / 2.0f);
					renderContext->DrawString(spriteFont, gameSessionInstructions, position, Colors::White, 0, origin, scale);
				}
			}
		}

		renderContext->End();
	}

	MenuScreen::Draw(totalTime, elapsedTime);

	DrawCurrentUser();
}

const byte GameLobbyScreen::IncrementShipColor(byte shipColor)
{
	// Increment color of ship 
	const byte color = static_cast<byte>((static_cast<size_t>(shipColor) + 1) % Ship::Colors.size());
	return color;
}

void GameLobbyScreen::OnCancel()
{
	m_menuEntries.clear();
	if (m_invitingFriend)
	{
		m_invitingFriend = false;
		return;
	}

	Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
}
