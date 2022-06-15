//--------------------------------------------------------------------------------------
// DebugOverlayScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DebugOverlayScreen.h"
#include "Game.h"
#include "PlayerState.h"

using namespace NetRumble;
using namespace DirectX;

constexpr float c_UserInfoLeft = 50.0f;
constexpr float c_UserInfoTop = 50.0f;

DebugOverlayScreen::DebugOverlayScreen() : GameScreen()
{
	m_exitWhenHidden = false;
}

void DebugOverlayScreen::Draw(float totalTime, float elapsedTime)
{
	RenderManager* renderManager = Managers::Get<RenderManager>();
	std::unique_ptr<RenderContext> renderContext = renderManager->GetRenderContext(BlendMode::NonPremultiplied);
	std::shared_ptr<DirectX::SpriteFont> spriteFont = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\Consolas_32.spritefont");
	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
	float scale = 0.45f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

	renderContext->Begin();

	// Draw a status line for each player
	int count = 0;
	XMVECTOR lineWidth = spriteFont->MeasureString("X");
	std::vector<std::shared_ptr<PlayerState>> playerStates = g_game->GetAllPlayerStates();

	if (playerStates.size() > 0)
	{
		for (const auto& playerState : playerStates)
		{
			if (playerState != nullptr)
			{
				std::shared_ptr<Ship> ship = playerState->GetShip();
				char buffer[512]{};

				sprintf_s(
					buffer,
					512,
					"%-20s[%15llu]%4s%4s%4s%4sPos: %010.4f, %010.4f Vel: %010.4f, %010.4f LS: %010.4f, %010.4f RS %010.4f, %010.4f",
					playerState->DisplayName.empty() ? "[NONAMEYET]" : playerState->DisplayName.c_str(),
					playerState->PeerId,
					playerState->InGame ? "GME" : "",
					playerState->IsLocalPlayer ? "LCL" : "",
					playerState->LobbyReady ? "RDY" : "",
					playerState->IsInactive() ? "IDL" : "",
					ship ? ship->Position.x : 0.0f,
					ship ? ship->Position.y : 0.0f,
					ship ? ship->Velocity.x : 0.0f,
					ship ? ship->Velocity.y : 0.0f,
					ship ? ship->Input.LeftStick.x : 0.0f,
					ship ? ship->Input.LeftStick.y : 0.0f,
					ship ? ship->Input.RightStick.x : 0.0f,
					ship ? ship->Input.RightStick.y : 0.0f
				);

				renderContext->DrawString(
					spriteFont,
					buffer,
					XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
					Colors::Yellow,
					0,
					XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
					scale
				);

				count++;
			}
		}
	}

	std::string msgStr;

	msgStr = "CurrentlyQueuedSendMessages: ";

	count++;
	scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

	renderContext->DrawString(
		spriteFont,
		msgStr.c_str(),
		XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
		Colors::Yellow,
		0,
		XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
		scale
	);

	count++;

	msgStr = "AverageRelayServerRoundTripLatencyInMilliseconds: ";

	scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

	renderContext->DrawString(
		spriteFont,
		msgStr.c_str(),
		XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
		Colors::Yellow,
		0,
		XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
		scale
	);
	count++;

	msgStr = "DeathCount : " + std::to_string(Managers::Get<OnlineManager>()->GetDeathCount());
	scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
	renderContext->DrawString(
		spriteFont,
		msgStr.c_str(),
		XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
		Colors::Yellow,
		0,
		XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
		scale
	);
	count++;

	msgStr = "VictoryCount : " + std::to_string(Managers::Get<OnlineManager>()->GetVictoryCount());
	scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
	renderContext->DrawString(
		spriteFont,
		msgStr.c_str(),
		XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
		Colors::Yellow,
		0,
		XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
		scale
	);
	count++;

	msgStr = "StartGameCount : " + std::to_string(Managers::Get<OnlineManager>()->GetStartGameCount());
	scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
	renderContext->DrawString(
		spriteFont,
		msgStr.c_str(),
		XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
		Colors::Yellow,
		0,
		XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
		scale
	);
	count++;

	if (g_game->m_DebugLogMessageList.size() != 0)
	{
		msgStr.clear();
		for (size_t i = 0; i < g_game->m_DebugLogMessageList.size() && i < MAX_ERRORMESSAGE_COUNT; i++)
		{
			msgStr += "DebugLogMessage: " + g_game->m_DebugLogMessageList[i] + "\n";
		}

		scale = 0.50f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
		renderContext->DrawString(
			spriteFont,
			msgStr.c_str(),
			XMFLOAT2(c_UserInfoLeft, c_UserInfoTop + (count * (XMVectorGetY(lineWidth) * scale))),
			Colors::Yellow,
			0,
			XMFLOAT2(0.0f, spriteFont->GetLineSpacing() / 2.0f),
			scale
		);
		count++;
	}

	renderContext->End();

	GameScreen::Draw(totalTime, elapsedTime);
}
