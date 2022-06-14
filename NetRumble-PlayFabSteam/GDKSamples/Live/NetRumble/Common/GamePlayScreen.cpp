//--------------------------------------------------------------------------------------
// GamePlayScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;
using namespace DirectX::SimpleMath;

GamePlayScreen::GamePlayScreen() :
	GameScreen(),
	m_leavingGame{ false },
	m_countdownTimer{ 3.0f },
	m_frameTime{ 0 }
{
	m_playerFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
	m_scoreFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\NetRumble.spritefont");
	Managers::Get<GameEventManager>()->RegisterEvent(GameEventType::LeavingGame, this, std::bind(&GamePlayScreen::HandleLeavingGameEvent, this, std::placeholders::_1));
}

GamePlayScreen::~GamePlayScreen()
{
	Managers::Get<GameEventManager>()->RemoveEvent(GameEventType::LeavingGame, this);
}

void GamePlayScreen::HandleInput(float elapsedTime)
{
	InputManager* inputManager = Managers::Get<InputManager>();

	// Pass input along to the local ship
	std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
	std::shared_ptr<Ship> localShip = localPlayerState->GetShip();
	if (localShip->Active() && !g_game->IsGameWon())
	{
		localShip->Input = ShipInput(inputManager->CurrentGamePadState);
		localShip->Input.Add(ShipInput(inputManager->CurrentKeyboardState()));

		m_frameTime += elapsedTime;

		// Send ShipInput Message when the status of the ship changed,
		// and the send interval should be at least (elapsedTime * 2)
		if (m_frameTime >= elapsedTime * 2 &&
			(localShip->Velocity.LengthSquared() > 0.0f) ||
			(localShip->Input.LeftStick != SimpleMath::Vector2(0.0f, 0.0f) ||
				localShip->Input.RightStick != SimpleMath::Vector2(0.0f, 0.0f) ||
				localShip->Input.MineFired))
		{
			m_frameTime = 0.0f;
			Managers::Get<OnlineManager>()->SendGameMessage(
				GameMessage(
					GameMessageType::ShipInput,
					localShip->Serialize()
				)
			);
		}
	}
	else
	{
		localShip->Input = ShipInput();
	}

	if (g_game->IsGameWon())
	{
		if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::A) ||
			inputManager->IsNewKeyPress(Keyboard::Keys::Enter) ||
			inputManager->IsNewKeyPress(Keyboard::Keys::A))
		{
			ExitScreen();
			g_game->GetWorld()->ResetDefaults();
			Managers::Get<OnlineManager>()->SetVictoryCount();
		}
	}

	if (inputManager->IsNewButtonPress(InputManager::GamepadButtons::View) || inputManager->IsNewKeyPress(Keyboard::Keys::Escape))
	{
		std::string message = "Leaving game....";
		Managers::Get<GameEventManager>()->DispatchEvent(GameEventMessage{ GameEventType::LeavingGame, const_cast<char*>(message.c_str()) });
	}

	GameScreen::HandleInput(elapsedTime);
}

void GamePlayScreen::Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen)
{
	g_game->UpdateWorld(totalTime, elapsedTime);
	GameScreen::Update(totalTime, elapsedTime, otherScreenHasFocus, coveredByOtherScreen);
}

void GamePlayScreen::Draw(float totalTime, float elapsedTime)
{
	UNREFERENCED_PARAMETER(totalTime);

	g_game->DrawWorld(elapsedTime);
	DrawHud(elapsedTime);
}

void GamePlayScreen::ExitScreen(bool immediate)
{
	std::shared_ptr<PlayerState> localPlayerState = g_game->GetLocalPlayerState();
	localPlayerState->InGame = false;
	localPlayerState->InLobby = false;
	localPlayerState->LobbyReady = false;
	g_game->ClearPlayerScores();

	Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
	Managers::Get<OnlineManager>()->SendGameMessage(GameMessage{ GameMessageType::PlayerLeftGame, 0 });
	GameScreen::ExitScreen(immediate);
}

void GamePlayScreen::DrawHud(float elapsedTime)
{
	RenderManager* renderManager = Managers::Get<RenderManager>();
	std::unique_ptr<RenderContext> renderContext = renderManager->GetRenderContext();
	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
	XMFLOAT2 fontOrigin = XMFLOAT2(0, 0);
	float playerNameScale = .4f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
	float messageScale = 2.25f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

	renderContext->Begin();

	std::vector<std::shared_ptr<PlayerState>> playerStates = g_game->GetAllPlayerStates();
	std::vector<uint16_t> playerIds = std::vector<uint16_t>();
	size_t count = playerStates.size();

	Vector2 memberPositions[] =
	{
		Vector2(viewportWidth * 0.15f, viewportHeight * 0.065f),
		Vector2(viewportWidth * 0.30f, viewportHeight * 0.065f),
		Vector2(viewportWidth * 0.70f, viewportHeight * 0.065f),
		Vector2(viewportWidth * 0.85f, viewportHeight * 0.065f)
	};

	// Draw players 0 - 3 at the top of the screen
	for (uint32_t i = 0; i < std::min<size_t>(static_cast<size_t>(4), count); ++i)
	{
		std::shared_ptr<PlayerState> playerState = playerStates[i];
		if (playerState)
		{
			std::string memberName = DX::ChsToUtf8(playerState->DisplayName);
			XMVECTORF32 memberColor = (playerState->InGame) ? Ship::Colors[playerState->ShipColor()] : Colors::LightGray;
			float memberNameLen = (playerNameScale * Vector2(m_scoreFont->MeasureString(memberName.c_str())).x) / 2;

			Vector2 namePosition = memberPositions[i] - Vector2(memberNameLen / 2, 0);

			renderContext->DrawString(m_playerFont, memberName, namePosition, memberColor, 0, fontOrigin, playerNameScale);
			memberName = playerState->DisplayName;
			// Draw score and respawn counter centered underneath each name
			std::shared_ptr<Ship> ship = playerState->GetShip();
			std::string memberData = std::to_string(ship->Score);

			float scoreLen = (playerNameScale * Vector2(m_scoreFont->MeasureString(memberData.c_str())).x) / 2;
			Vector2 scorePosition = memberPositions[i] + Vector2((-scoreLen) / 2, (playerNameScale * m_scoreFont->GetLineSpacing()));
			renderContext->DrawString(m_scoreFont, memberData.c_str(), scorePosition, memberColor, 0, fontOrigin, 2 * playerNameScale);
		}
	}

	// Draw players 4 - 7 at the bottom of the screen
	for (uint32_t i = 4; i < std::min<size_t>(static_cast<size_t>(8), count); ++i)
	{
		memberPositions[i % 4].y = viewportHeight * 0.9f;

		std::shared_ptr<PlayerState> playerState = playerStates[i];
		if (playerState)
		{
			std::string memberName = playerState->DisplayName;
			XMVECTORF32 memberColor = (playerState->InGame) ? Ship::Colors[playerState->ShipColor()] : Colors::LightGray;

			renderContext->DrawString(m_playerFont, memberName, memberPositions[i % 4], memberColor, 0, fontOrigin, playerNameScale);
			memberName = playerState->DisplayName;
			// Draw score and respawn counter centered underneath each name
			std::shared_ptr<Ship> ship = playerState->GetShip();
			std::string memberData = std::to_string(ship->Score);
			if (!ship->Active() && ship->RespawnTimer > 0.0f)
			{
				memberData += "  (" + std::to_string(1 + static_cast<int>(ship->RespawnTimer)) + ")";
			}
			Vector2 memberNameLen = playerNameScale * Vector2(m_playerFont->MeasureString(memberName.c_str()));
			XMFLOAT2 scorePosition = XMFLOAT2(memberPositions[i % 4].x + (XMVectorGetX(memberNameLen) / 2.0f), memberPositions[i % 4].y + (playerNameScale * m_playerFont->GetLineSpacing()));
			renderContext->DrawString(m_scoreFont, memberData.c_str(), scorePosition, memberColor, 0, fontOrigin, 2 * playerNameScale);
		}
	}

	// Draw a spawn countdown text message for the local user when appropriate
	std::shared_ptr<Ship> localShip = g_game->GetLocalPlayerState()->GetShip();
	if (!g_game->IsGameWon() && !localShip->Active() && localShip->RespawnTimer > 0.0f)
	{
		std::string respawnMessage = "Spawning in " + std::to_string(1 + static_cast<int>(localShip->RespawnTimer));
		XMVECTOR respawnMessageLen = m_playerFont->MeasureString(respawnMessage.c_str());
		XMFLOAT2 respawnMessagePosition = XMFLOAT2(viewportWidth / 2.0f, viewportHeight / 2.0f);
		XMFLOAT2 respawnMessageOrigin = XMFLOAT2(XMVectorGetX(respawnMessageLen) / 2.0f, 0.0f);
		renderContext->DrawString(m_playerFont, respawnMessage.c_str(), respawnMessagePosition, Colors::White, 0.0f, respawnMessageOrigin, messageScale);
	}

	// If round has been won, draw win message
	if (g_game->IsGameWon())
	{
		std::string winMessageLine1, winMessageLine2;

		if (g_game->GetWinnerName() == "Insufficient Players")
		{
			winMessageLine1 = "Insufficient Players to continue the game.";
		}
		else
		{
			winMessageLine1 = g_game->GetWinnerName().data();
			winMessageLine1 += " has won the game!";
		}

		winMessageLine2 = "Press (A) to return to the main menu";

		XMFLOAT2 winMessagePosition = XMFLOAT2(viewportWidth / 2.0f, viewportHeight * 0.4f);

		XMVECTOR winMessageLen = m_playerFont->MeasureString(DX::ChsToUtf8(winMessageLine1).c_str());
		XMFLOAT2 winMessageOrigin = XMFLOAT2(XMVectorGetX(winMessageLen) / 2.0f, m_playerFont->GetLineSpacing());
		renderContext->DrawString(m_playerFont, winMessageLine1, winMessagePosition, g_game->GetWinningColor(), 0.0f, winMessageOrigin, playerNameScale);

		winMessageLen = m_playerFont->MeasureString(winMessageLine2.data());
		winMessageOrigin = XMFLOAT2(XMVectorGetX(winMessageLen) / 2.0f, 0.0f);
		renderContext->DrawString(m_playerFont, winMessageLine2.data(), winMessagePosition, g_game->GetWinningColor(), 0.0f, winMessageOrigin, playerNameScale);
	}

	if (m_leavingGame)
	{
		m_countdownTimer -= elapsedTime;
		if (m_countdownTimer < 0.5f)
		{
			m_leavingGame = false;
			ExitScreen();
		}
		else
		{
			XMFLOAT2 leavingGameMessagePosition = XMFLOAT2(viewportWidth / 2.0f, viewportHeight * 0.4f);

			XMVECTOR leavingGameMessageLen = m_playerFont->MeasureString(DX::ChsToUtf8(m_connectFailInGameMessage).c_str());
			XMFLOAT2 leavingGameMessageOrigin = XMFLOAT2(XMVectorGetX(leavingGameMessageLen) / 2.0f, m_playerFont->GetLineSpacing());
			renderContext->DrawString(m_playerFont, m_connectFailInGameMessage, leavingGameMessagePosition, g_game->GetWinningColor(), 0.0f, leavingGameMessageOrigin, playerNameScale);
		}
	}
	renderContext->End();
}

void GamePlayScreen::HandleLeavingGameEvent(GameEventMessage message)
{
	m_leavingGame = true;
	m_connectFailInGameMessage = static_cast<char*>(message.m_message);
}
