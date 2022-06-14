//--------------------------------------------------------------------------------------
// RenderManager.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RenderManager.h"
#include "RenderContext.h"

#include "DeviceResources.h"

#include "SpriteBatch.h"
#include "CommonStates.h"

namespace
{
	constexpr float c_backgroundColor[4] = { 0, 0, 16.0f / 255.0f, 1.0f };
}

namespace NetRumble
{
	RenderManager::RenderManager() noexcept
	{
		m_deviceResources = std::make_unique<DX::DeviceResources>();
		m_deviceResources->RegisterDeviceNotify(this);
	}

	RenderManager::~RenderManager() noexcept
	{

	}

	std::unique_ptr<RenderContext> RenderManager::GetRenderContext(BlendMode mode) const
	{
		std::shared_ptr<DirectX::SpriteBatch> spriteBatch;

		switch (mode)
		{
		case BlendMode::NonPremultiplied:
			spriteBatch = m_nonPreMultipliedspriteBatch;
			break;
		case BlendMode::Additive:
			spriteBatch = m_additiveSpriteBatch;
			break;
		default:
			spriteBatch = m_defaultSpriteBatch;
			break;
		};

		std::unique_ptr<RenderContext> context;
		context.reset(new RenderContext(spriteBatch, m_deviceResources->GetCommandList()));

		return context;

	}

	void RenderManager::Initialize(HWND window, int width, int height)
	{
		m_windowWidth = width;
		m_windowHeight = height;
		m_deviceResources->SetWindow(window, width, height);

		m_deviceResources->CreateDeviceResources();
		CreateDeviceDependentResources();

		m_deviceResources->CreateWindowSizeDependentResources();
	}

	bool RenderManager::OnWindowSizeChanged(int width, int height)
	{
		m_windowWidth = width;
		m_windowHeight = height;
		return m_deviceResources->WindowSizeChanged(width, height);
	}

	void RenderManager::Clear(ID3D12DescriptorHeap* descriptorHeap)
	{
		m_deviceResources->Prepare();
		auto commandList = m_deviceResources->GetCommandList();

		// Clear the views.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = m_deviceResources->GetRenderTargetView();
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = m_deviceResources->GetDepthStencilView();

		commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
		commandList->ClearRenderTargetView(rtvDescriptor, c_backgroundColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// Set the viewport and scissor rect.
		D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
		D3D12_RECT scissorRect = m_deviceResources->GetScissorRect();
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		if (nullptr != descriptorHeap)
		{
			ID3D12DescriptorHeap* heaps[] = { descriptorHeap };
			commandList->SetDescriptorHeaps(_countof(heaps), heaps);
		}
	}

	std::shared_ptr<DX::Texture> RenderManager::LoadTexture(const wchar_t* path, D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor)
	{
		DirectX::ResourceUploadBatch resourceUpload(m_deviceResources->GetD3DDevice());

		resourceUpload.Begin();
		std::shared_ptr<DX::Texture> texture = std::make_shared<DX::Texture>(m_deviceResources->GetD3DDevice(), resourceUpload, srvDescriptor, path);
		resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();

		return texture;
	}

	std::shared_ptr<DirectX::SpriteFont> RenderManager::LoadFont(const wchar_t* path, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor)
	{
		DirectX::ResourceUploadBatch resourceUpload(m_deviceResources->GetD3DDevice());

		resourceUpload.Begin();
		std::shared_ptr<DirectX::SpriteFont> font = std::make_shared<DirectX::SpriteFont>(m_deviceResources->GetD3DDevice(), resourceUpload, path, cpuDescriptor, gpuDescriptor);
		resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();

		return font;
	}

	std::shared_ptr<DirectX::DescriptorPile> RenderManager::CreateDescriptorPile(size_t descriptorCount)
	{
		return std::make_shared<DirectX::DescriptorPile>(m_deviceResources->GetD3DDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, descriptorCount);
	}

	void RenderManager::Suspend()
	{
		m_deviceResources->Suspend();
	}

	void RenderManager::Resume()
	{
		m_deviceResources->Resume();
	}

	void RenderManager::Present()
	{
		m_deviceResources->Present();
		m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
	}

	void RenderManager::CreateDeviceDependentResources()
	{
		auto device = m_deviceResources->GetD3DDevice();

		m_graphicsMemory = std::make_unique<DirectX::GraphicsMemory>(device);

		DirectX::RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

		DirectX::ResourceUploadBatch resourceUpload(m_deviceResources->GetD3DDevice());

		resourceUpload.Begin();

		DirectX::SpriteBatchPipelineStateDescription defPSD(rtState);
		m_defaultSpriteBatch = std::make_shared<DirectX::SpriteBatch>(device, resourceUpload, defPSD);

		DirectX::SpriteBatchPipelineStateDescription nonPrePSD(rtState, &DirectX::CommonStates::NonPremultiplied);
		m_nonPreMultipliedspriteBatch = std::make_shared<DirectX::SpriteBatch>(device, resourceUpload, nonPrePSD);

		DirectX::SpriteBatchPipelineStateDescription additivePSD(rtState, &DirectX::CommonStates::Additive);
		m_additiveSpriteBatch = std::make_shared<DirectX::SpriteBatch>(device, resourceUpload, additivePSD);

		resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();

		D3D12_VIEWPORT viewport = { 0.f, 0.f, static_cast<float>(m_windowWidth),static_cast<float>(m_windowHeight), 0.f, 0.f };
		m_defaultSpriteBatch->SetViewport(viewport);
		m_nonPreMultipliedspriteBatch->SetViewport(viewport);
		m_additiveSpriteBatch->SetViewport(viewport);
	}

	// Allocate all memory resources that change on a window SizeChanged event.
	void RenderManager::CreateWindowSizeDependentResources()
	{
		// TODO: Initialize windows-size dependent objects here.
	}

	void RenderManager::OnDeviceLost()
	{
		m_graphicsMemory.reset();
	}

	void RenderManager::OnDeviceRestored()
	{
		// TODO: ContentManager dump reload textures and fonts
	}
}
