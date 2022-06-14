//--------------------------------------------------------------------------------------
// ContentManager.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace NetRumble;

ContentManager::ContentManager() noexcept : m_lastIndex(0)
{
}

ContentManager::~ContentManager() noexcept
{
	m_textures.clear();
}

void ContentManager::Initialize(const std::shared_ptr<DirectX::DescriptorPile>& descriptorPile)
{
	m_descriptors = descriptorPile;
}

TextureHandle ContentManager::LoadTexture(const std::wstring& path)
{
	std::wstring lowerPath = path;
	// Lower case the path for our map
	std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](wchar_t wc) { return static_cast<wchar_t>(std::tolower(wc)); });

	// Look in our cache first
	auto itr = m_textures.find(path);
	if (itr != m_textures.end())
	{
		return TextureHandle{ itr->second.Resource, m_descriptors->GetGpuHandle(itr->second.Index) };
	}

	DirectX::DescriptorPile::IndexType index = m_lastIndex++;
	std::shared_ptr<DX::Texture> texture = Managers::Get<RenderManager>()->LoadTexture(path.c_str(), m_descriptors->GetCpuHandle(index));

	m_textures[path] = Storage<DX::Texture>{ texture, index };

	return TextureHandle{ texture, m_descriptors->GetGpuHandle(index) };
}

std::shared_ptr<DirectX::SpriteFont> ContentManager::LoadFont(const std::wstring& path)
{
	std::wstring lowerPath = path;
	// Lower case the path for our map
	std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](wchar_t wc) { return static_cast<wchar_t>(std::tolower(wc)); });

	// Look in our cache first
	auto itr = m_fonts.find(path);
	if (itr != m_fonts.end())
	{
		return itr->second.Resource;
	}

	// Otherwise use the WICTextureLoader APIs to load the texture into our cache
	DirectX::DescriptorPile::IndexType index = m_lastIndex++;
	std::shared_ptr<DirectX::SpriteFont> font = Managers::Get<RenderManager>()->LoadFont(path.c_str(), m_descriptors->GetCpuHandle(index), m_descriptors->GetGpuHandle(index));
	m_fonts[path] = Storage<DirectX::SpriteFont>{ font, index };

	return font;
}
