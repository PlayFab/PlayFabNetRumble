﻿//--------------------------------------------------------------------------------------
// MenuScreen.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;
using namespace DirectX;

namespace {
	constexpr char TestLanguage[] = "english";
}

//
// MenuEntry
//
MenuEntry::MenuEntry(std::string_view label, MenuEntrySelectFn onSelect, MenuEntryAdjustFn onAdjust, std::string_view initialValue) :
	OnAdjust(onAdjust),
	OnSelect(onSelect),
	m_active(true),
	m_hasValue(onAdjust != nullptr),
	m_label(label),
	m_value(initialValue)
{
}

//
// MenuScreen
//
MenuScreen::MenuScreen() : GameScreen(),
m_animateSelected(true),
m_menuBounds{},
m_menuTextScale(0.5f),
m_showCurrentUser(true),
m_drawCentered(false),
m_menuOffset{},
m_selectedEntry(0),
m_transitionOnMultiplier(1),
m_transitionOffMultiplier(1)
{
	SetCenterJustified(true);
}

void MenuScreen::HandleInput(float elapsedTime)
{
	InputManager* input = Managers::Get<InputManager>();

	if (input->IsMenuUp())
	{
		// size_t doesn't go negative; it rolls over
		m_selectedEntry--;
		if (m_selectedEntry >= m_menuEntries.size())
			m_selectedEntry = m_menuEntries.size() - 1;

		Managers::Get<AudioManager>()->PlaySound(L"menu_scroll");
	}
	else if (input->IsMenuDown())
	{
		m_selectedEntry++;
		if (m_selectedEntry >= m_menuEntries.size())
			m_selectedEntry = 0;

		Managers::Get<AudioManager>()->PlaySound(L"menu_scroll");
	}
	else if (input->IsMenuSelect())
	{
		if (m_selectedEntry < m_menuEntries.size()
			&& m_menuEntries[m_selectedEntry].OnSelect != nullptr
			&& m_menuEntries[m_selectedEntry].m_active)
		{
			Managers::Get<AudioManager>()->PlaySound(L"menu_select");
			m_menuEntries[m_selectedEntry].OnSelect();
		}
	}
	else if (input->IsMenuLeft())
	{
		if (m_selectedEntry < m_menuEntries.size()
			&& m_menuEntries[m_selectedEntry].OnAdjust != nullptr
			&& m_menuEntries[m_selectedEntry].m_active)
		{
			Managers::Get<AudioManager>()->PlaySound(L"menu_scroll");
			m_menuEntries[m_selectedEntry].OnAdjust(true);
		}
	}
	else if (input->IsMenuRight())
	{
		if (m_selectedEntry < m_menuEntries.size()
			&& m_menuEntries[m_selectedEntry].OnAdjust != nullptr
			&& m_menuEntries[m_selectedEntry].m_active)
		{
			Managers::Get<AudioManager>()->PlaySound(L"menu_scroll");
			m_menuEntries[m_selectedEntry].OnAdjust(false);
		}
	}
	else if (input->IsMenuCancel())
	{
		OnCancel();
	}

	GameScreen::HandleInput(elapsedTime);
}

std::string UTF8_To_string(const std::string& str)
{
	int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

	wchar_t* pwBuf = new wchar_t[static_cast<size_t>(nwLen) + 1];
	memset(pwBuf, 0, static_cast<size_t>(nwLen) * 2 + 2);

	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), pwBuf, nwLen);

	int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, nullptr, 0, nullptr, nullptr);

	char* pBuf = new char[static_cast<size_t>(nLen) + 1];
	memset(pBuf, 0, static_cast<size_t>(nLen) + 1);

	WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, nullptr, nullptr);

	std::string retStr = pBuf;

	delete[]pBuf;
	delete[]pwBuf;

	pBuf = nullptr;
	pwBuf = nullptr;

	return retStr;
}

void MenuScreen::Draw(float totalTime, float elapsedTime)
{
	UNREFERENCED_PARAMETER(elapsedTime);

	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());

	SimpleMath::Vector2 position = ComputeDrawStartPosition(viewportWidth, viewportHeight);
	float menuTextScale = m_menuTextScale * GetScaleMultiplierForViewport(viewportWidth, viewportHeight);

	// Make the menu slide into place during transitions, using a
	// power curve to make things look more interesting (this makes
	// the movement slow down as it nears the end).
	float transitionOffset = powf(TransitionPosition(), 2);

	if (State() == ScreenStateType::TransitionOn)
		position.y += transitionOffset * (256 * m_transitionOnMultiplier);
	else
		position.y += transitionOffset * (512 * m_transitionOffMultiplier);

	// Draw each menu entry in turn.
	std::unique_ptr<RenderContext> renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::NonPremultiplied);
	std::shared_ptr<DirectX::SpriteFont> font = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");

	renderContext->Begin();

	// Advance starting position halfway down the height of the first line, since this is where the draw origin is.
	position.y += font->GetLineSpacing() / 2.0f * menuTextScale;

	for (size_t i = 0; i < m_menuEntries.size(); i++)
	{
		if (m_menuEntries[i].m_label.empty())
			continue;

		XMVECTORF32 color = m_menuEntries[i].m_active ? Colors::White : Colors::Gray;
		float adjustedScale = menuTextScale;

		if (IsActive() && (i == m_selectedEntry))
		{
			// The selected entry is orange (or gray), and has an animating size (if enabled).
			if (m_menuEntries[i].m_active)
			{
				color = Colors::Orange;

				if (m_animateSelected)
				{
					float pulsate = sin(totalTime * 6.0f) + 1.0f;
					adjustedScale += pulsate * 0.05f;
				}
			}
			else
			{
				color = Colors::DarkGray;
			}
		}

		// Modify the alpha to fade text out during transitions.
		color.f[3] = TransitionAlpha();

		// Draw text, centered on the middle of each line.
		std::string menuLabel = m_menuEntries[i].m_label;
		if (m_menuEntries[i].m_hasValue)
		{
			menuLabel.append(" ");
			menuLabel += m_menuEntries[i].m_value;
		}

		menuLabel = DX::ChsToUtf8(menuLabel);
		XMVECTOR size = font->MeasureString(menuLabel.c_str());
		SimpleMath::Vector2 origin = SimpleMath::Vector2(m_drawCentered ? XMVectorGetX(size) / 2.0f : 0, font->GetLineSpacing() / 2.0f);
		renderContext->DrawString(font, menuLabel, position, color, 0, origin, adjustedScale);

		position.y += font->GetLineSpacing() * menuTextScale;
	}

	renderContext->End();
}

void MenuScreen::ComputeMenuBounds(float viewportWidth, float viewportHeight)
{
	m_menuBounds = XMFLOAT4(0, viewportHeight * 0.55f, viewportWidth, viewportHeight * 0.45f);
}

void MenuScreen::ConfigureAsPopUpMenu()
{
	m_isPopup = true;
	SetCenterJustified(false);
	m_animateSelected = false;
	m_showCurrentUser = false;
}

SimpleMath::Vector2 MenuScreen::ComputeDrawStartPosition(float viewportWidth, float viewportHeight)
{
	ComputeMenuBounds(viewportWidth, viewportHeight);

	if (m_drawCentered)
	{
		return SimpleMath::Vector2(m_menuOffset.x + m_menuBounds.z / 2.0f, m_menuOffset.y + m_menuBounds.y);
	}
	else
	{
		return SimpleMath::Vector2(m_menuOffset.x + m_menuBounds.x, m_menuOffset.y + m_menuBounds.y);
	}
}

void MenuScreen::DrawCurrentUser()
{
	float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
	float viewportHeight = static_cast<float>(g_game->GetWindowHeight());

	// Draw current user at bottom right.
	float scale = GetScaleMultiplierForViewport(viewportWidth, viewportHeight);
	std::string localName = g_game->GetLocalPlayerName().data();

	if (localName.empty())
	{
		localName = "Player One";
	}

	std::unique_ptr<RenderContext> renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::NonPremultiplied);
	std::shared_ptr<DirectX::SpriteFont> font = Managers::Get<ContentManager>()->LoadFont(L"Assets\\Fonts\\SegoeUI_64.spritefont");

	SimpleMath::Vector2 position = SimpleMath::Vector2(viewportWidth - 200 * scale, viewportHeight - 150 * scale);

	localName = DX::ChsToUtf8(localName);
	float userNameWidth = XMVectorGetX(font->MeasureString(localName.data()));
	SimpleMath::Vector2 origin = SimpleMath::Vector2(userNameWidth, font->GetLineSpacing() / 2.0f);

	renderContext->Begin();

	renderContext->DrawString(
		font,
		localName.data(),
		position,
		Colors::White,
		0,
		origin,
		scale * 0.35f
	);

	position.y += ((font->GetLineSpacing() / 2.5f) * scale) / 2;
	position.y += (font->GetLineSpacing() / 2.5f) * scale;

	renderContext->DrawString(
		font,
		TestLanguage,
		position,
		Colors::White,
		0,
		origin,
		scale * 0.35f
	);

	position.y += (font->GetLineSpacing() / 2.5f) * scale;

	renderContext->DrawString(
		font,
		TestLanguage,
		position,
		Colors::White,
		0,
		origin,
		scale * 0.35f
	);

	renderContext->End();
}
