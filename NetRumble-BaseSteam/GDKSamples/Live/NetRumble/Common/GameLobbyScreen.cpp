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

	Managers::Get<GameEventManager>()->RegisterEvent(NetRumble::GameEventType::ConnectionServerFail, this, std::bind(&GameLobbyScreen::SetServerConnectionEvent, this, std::placeholders::_1));
	Managers::Get<GameEventManager>()->RegisterEvent(NetRumble::GameEventType::ConnectionSteamFail, this, std::bind(&GameLobbyScreen::SetSteamConnectionEvent, this, std::placeholders::_1));
	Managers::Get<GameEventManager>()->RegisterEvent(NetRumble::GameEventType::LobbyFull, this, std::bind(&GameLobbyScreen::SetSteamConnectionEvent, this, std::placeholders::_1));
}

GameLobbyScreen::~GameLobbyScreen()
{
	Managers::Get<GameEventManager>()->RemoveEvent(NetRumble::GameEventType::ConnectionServerFail, this);
	Managers::Get<GameEventManager>()->RemoveEvent(NetRumble::GameEventType::ConnectionSteamFail, this);
	Managers::Get<GameEventManager>()->RemoveEvent(NetRumble::GameEventType::LobbyFull, this);
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
		
		// Check if the player has returned to the main menu
		auto peers = g_game->GetPeers();
		for (auto& [id, peer] : peers)
		{
			if (peer->IsReturnedToMainMenu == true)
			{
				g_game->RemovePeers(id);
			}
		}

		InputManager* inputManager = Managers::Get<InputManager>();
		std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();

		if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::LeftShoulder) || inputManager->IsNewKeyPress(Keyboard::Keys::Left))
		{
			// Change ship color
			const byte newColor = IncrementShipColor(localPlayerState->ShipColor());
			localPlayerState->ShipColor(newColor);
			Managers::Get<OnlineManager>()->SetShipColorInLobby(newColor);
		}
		else if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::RightShoulder) || inputManager->IsNewKeyPress(Keyboard::Keys::Right))
		{
			// Change ship design
			byte newShip = (localPlayerState->ShipVariation() + 1) % Ship::MaxVariations;
			localPlayerState->ShipVariation(newShip);

			Managers::Get<OnlineManager>()->SetShipVariationInLobby(newShip);
		}
		else if (inputManager->IsNewKeyPress(Keyboard::Keys::Y))
		{
			Managers::Get<OnlineManager>()->InviteFriendsIntoLobby();
		}
		else if (inputManager->IsNewKeyPress(Keyboard::Keys::A))
		{
			std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
			m_ready = true;
			localPlayerState->LobbyReady = true;
			Managers::Get<OnlineManager>()->SetReadyStateInLobby(true);
		}
	}
}

void GameLobbyScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
	GameState state = Managers::Get<GameStateManager>()->GetState();
	std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
	if (!m_exiting)
	{
		m_menuEntries.clear();

		switch (state)
		{
		case GameState::MPHostGame:
		{
			m_menuEntries.emplace_back("Starting host session");
			break;
		}
		case GameState::MPMatchmaking:
		{
			m_menuEntries.emplace_back("Matchmaking: Waiting for players");
			break;
		}
		case GameState::JoinGameFromLobby:
		{
			m_menuEntries.emplace_back("Join Game Frome Lobby");
			break;
		}
		case GameState::MPJoinLobby:
		{
			m_menuEntries.emplace_back("Joining Lobby");
			break;
		}
		case GameState::Lobby:
		{
			if (m_ready)
			{
				std::vector<std::shared_ptr<PlayerState>> players = g_game->GetAllPlayerStates();

				std::string message = players.size() == 1 ? "Waiting for more players" : "Waiting for game to start";

				m_menuEntries.emplace_back(message, [this, localPlayerState]()
					{
						m_ready = false;
						localPlayerState->LobbyReady = false;
						Managers::Get<OnlineManager>()->SetReadyStateInLobby(false);
					});
			}
			else
			{
				m_menuEntries.emplace_back("Press (A) to ready", [this, localPlayerState]()
					{
						m_ready = true;
						localPlayerState->LobbyReady = true;
						Managers::Get<OnlineManager>()->SetReadyStateInLobby(true);
					});

				m_menuEntries.emplace_back("Press (Y) to Invite Friend", []() {
						Managers::Get<OnlineManager>()->InviteFriendsIntoLobby();
					});
			}

			if (g_game->IsHost())
			{
				const std::vector<std::shared_ptr<PlayerState>>& playerStates = g_game->GetAllPlayerStates();

				if (playerStates.size() < 0)
				{
					break;
				}

				for (auto& playerState : playerStates)
				{
					if (!playerState->LobbyReady)
					{
						break;
					}
				}
			}
			break;
		}
		case GameState::MatchingGame:
		{
			m_countdownTimer -= elapsedTime;

			std::string message;

			if (m_countdownTimer < 0.5f)
			{

				if (Managers::Get<OnlineManager>()->IsServer())
				{
					Managers::Get<GameStateManager>()->SwitchToState(GameState::EvaluatingNetwork);
				}
				else
				{
					m_countdownTimer = 0.0f;
					Managers::Get<GameStateManager>()->SwitchToState(GameState::GameConnecting);
				}

				message = "Game starting...";
			}
			else
			{
				message = "Match starts in ";
				message += std::to_string((int)m_countdownTimer);
				message += " seconds";
			}

			Managers::Get<OnlineManager>()->SetLobbyGameServerAndConnect();

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
				m_menuEntries.emplace_back("Game Starting...");
			}

			break;
		}
		case GameState::EvaluatingNetwork:
		{
			m_menuEntries.emplace_back("Evaluating Network...");
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
				m_menuEntries.emplace_back("Connect Game fail:" + m_connectFailMessage);
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

const byte NetRumble::GameLobbyScreen::IncrementShipColor(byte shipColor)
{
	// Increment color of ship 
	const byte color = static_cast<byte>((static_cast<size_t>(shipColor) + 1) % Ship::Colors.size());
	return color;
}

void GameLobbyScreen::OnCancel()
{
	m_menuEntries.clear();

	Managers::Get<OnlineManager>()->LeaveLobby();
	Managers::Get<GameStateManager>()->CancelImmutableState();
	Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
	g_game->DeleteOtherPlayerInPeersAndExitLobby();
}

void NetRumble::GameLobbyScreen::SetServerConnectionEvent(GameEventMessage message)
{
	m_countdownTimer = 5.0f;
	m_connectFailMessage = static_cast<char*>(message.m_message);
}

void NetRumble::GameLobbyScreen::SetSteamConnectionEvent(GameEventMessage message)
{
	m_countdownTimer = 5.0f;
	m_connectFailMessage = static_cast<char*>(message.m_message);
	Managers::Get<GameStateManager>()->SwitchToState(GameState::GameConnectFail);
}