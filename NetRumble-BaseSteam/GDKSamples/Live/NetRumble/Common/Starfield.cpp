//--------------------------------------------------------------------------------------
// Starfield.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

namespace
{

	constexpr float c_maximumMovementPerUpdate = 128.0f;
	constexpr size_t c_numberOfLayers = 8;
	constexpr int c_starSize = 2;

	constexpr XMVECTOR backgroundColor = { 0, 0, 16.0f / 255.0f, 1.0f };

	constexpr XMVECTOR LayerColor(float a)
	{
		return { 1.0f, 1.0f, 1.0f, a / 255.0f };
	}

	std::array<XMVECTOR, c_numberOfLayers> layerColors{
		LayerColor(255.0f),
		LayerColor(216.0f),
		LayerColor(192.0f),
		LayerColor(160.0f),
		LayerColor(128.0f),
		LayerColor(96.0f),
		LayerColor(64.0f),
		LayerColor(32.0f)
	};

	// See if this is really needed or used
	std::array<int, c_numberOfLayers> layerSizes;

	constexpr std::array<float, c_numberOfLayers> movementFactors = { 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f };
}

Starfield::Starfield(DirectX::SimpleMath::Vector2 position) :
	m_lastPosition(position),
	m_position(position)
{
	for (size_t i = 0; i < m_stars.size(); i++)
	{
		m_stars[i] = SimpleMath::Vector2(0, 0);
	}

	// Load the star texture
	m_starTexture = Managers::Get<ContentManager>()->LoadTexture(L"Assets\\Textures\\Blank.png");
}

Starfield::~Starfield()
{
}

void Starfield::Reset(DirectX::SimpleMath::Vector2 position)
{
	// Recreate the stars
	int viewportWidth = g_game->GetWindowWidth();
	int viewportHeight = g_game->GetWindowHeight();

	for (size_t i = 0; i < m_stars.size(); i++)
	{
		m_stars[i] = SimpleMath::Vector2(static_cast<float>(rand() % viewportWidth), static_cast<float>(rand() % viewportHeight));
	}

	// Reset the position
	m_lastPosition = m_position = position;
}

void Starfield::Draw(DirectX::SimpleMath::Vector2 position)
{
	std::unique_ptr<RenderContext> renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::NonPremultiplied);

	// Update the current position
	m_lastPosition = m_position;
	m_position = position;

	// Determine the movement vector of the stars
	// -- for the purposes of the parallax effect, 
	// this is the opposite direction as the position movement.
	SimpleMath::Vector2 movement = m_lastPosition - position;

	// Create a rectangle representing the screen dimensions of the starfield
	int viewportWidth = g_game->GetWindowWidth();
	int viewportHeight = g_game->GetWindowHeight();
	RECT starfieldRectangle = { 0, 0, viewportWidth, viewportHeight };

	// Draw a background color for the starfield
	renderContext->Begin();
	renderContext->Draw(m_starTexture, starfieldRectangle, backgroundColor);
	renderContext->End();


	// If we've moved too far, then reset, as the stars will be moving too fast
	if (movement.LengthSquared() > c_maximumMovementPerUpdate * c_maximumMovementPerUpdate)
	{
		Reset(position);
		return;
	}

	// Draw all of the stars
	renderContext->Begin();

	for (size_t i = 0; i < m_stars.size(); i++)
	{
		// Move the star based on the depth
		size_t depth = i % movementFactors.size();

		SimpleMath::Vector2 p = m_stars[i];
		p += movement * movementFactors[depth];

		// Wrap the stars around
		if (p.x < 0 - layerSizes[depth])
		{
			p.x = static_cast<float>(viewportWidth);
			p.y = static_cast<float>(rand() % viewportHeight);
		}
		if (p.x > viewportWidth)
		{
			p.x = static_cast<float>(0 - layerSizes[depth]);
			p.y = static_cast<float>(rand() % viewportHeight);
		}
		if (p.y < 0 - layerSizes[depth])
		{
			p.x = static_cast<float>(rand() % viewportWidth);
			p.y = static_cast<float>(viewportHeight);
		}
		if (p.y > viewportHeight)
		{
			p.x = static_cast<float>(rand() % viewportWidth);
			p.y = static_cast<float>(0 - layerSizes[depth]);
		}

		m_stars[i] = p;

		// Draw the star
		RECT b = { (int)p.x, (int)p.y, (int)p.x + c_starSize, (int)p.y + c_starSize };
		renderContext->Draw(m_starTexture, b, layerColors[depth]);
	}

	renderContext->End();
}
