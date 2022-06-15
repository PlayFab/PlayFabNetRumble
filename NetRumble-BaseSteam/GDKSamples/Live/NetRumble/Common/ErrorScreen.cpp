//--------------------------------------------------------------------------------------
// ErrorScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ErrorScreen.h"

using namespace NetRumble;
using namespace DirectX;

ErrorScreen::ErrorScreen(std::string_view message, std::function<void(void)> callback) :
	_message{ message },
	_callback{ callback }
{
	m_isPopup = true;
}

void ErrorScreen::HandleInput(float elapsedTime)
{
	if (Managers::Get<InputManager>()->IsMenuSelect())
	{
		ExitScreen(true);
	}
}

void ErrorScreen::Draw(float totalTime, float elapsedTime)
{
	UNREFERENCED_PARAMETER(totalTime);
	UNREFERENCED_PARAMETER(elapsedTime);

	std::unique_ptr<RenderContext> renderContext = Managers::Get<RenderManager>()->GetRenderContext();

	ContentManager* contentManager = Managers::Get<ContentManager>();

	std::shared_ptr<DirectX::SpriteFont> spriteFont = contentManager->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");
	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());

	// Calculate position and size of error message
	SimpleMath::Vector2 errorMsgPosition = SimpleMath::Vector2(0, viewportHeight / 2.0f);
	XMVECTORF32 errorMsgColor = Colors::Yellow;
	const float scale = 0.5f * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
	SimpleMath::Vector2 origin = SimpleMath::Vector2(0, spriteFont->GetLineSpacing() / 2.0f);

	XMVECTOR size = spriteFont->MeasureString(_message.c_str());
	errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX(size) / 2.0f * scale;

	// Create a rectangle representing the screen dimensions of the error message background rectangle
	long rectangleWidth = long(std::min(std::max((XMVectorGetX(size) * scale) + 100.0f, 600.0f), viewportWidth));
	long rectangleHeight = long(spriteFont->GetLineSpacing() * scale * 6.0f);
	long rectangleLeft = long(viewportWidth / 2.0f) - (rectangleWidth / 2);
	long rectangleTop = long(errorMsgPosition.y + spriteFont->GetLineSpacing() * scale) - (rectangleHeight / 2);
	RECT backgroundRectangle = { rectangleLeft, rectangleTop, rectangleLeft + rectangleWidth, rectangleTop + rectangleHeight };

	renderContext->Begin();

	// Draw a background color for the rectangle
	renderContext->Draw(
		contentManager->LoadTexture(L"Assets\\Textures\\blank.png"),
		backgroundRectangle,
		Colors::DarkSlateGray,
		0.0f,
		TexturePosition::None
	);

	// Draw error message in the middle of the screen
	renderContext->DrawString(spriteFont, _message, errorMsgPosition, errorMsgColor, 0, origin, scale);
	// Draw continuation prompt
	errorMsgPosition.y += spriteFont->GetLineSpacing() * scale;
	size = spriteFont->MeasureString("Press (A) to Continue");
	errorMsgPosition.x = viewportWidth / 2.0f - XMVectorGetX(size) / 2.0f * scale;
	renderContext->DrawString(spriteFont, "Press (A) to Continue", errorMsgPosition, Colors::Yellow, 0, origin, scale);

	renderContext->End();
}

void ErrorScreen::ExitScreen(bool immediate)
{
	GameScreen::ExitScreen(immediate);

	if (_callback)
	{
		_callback();
	}
}
