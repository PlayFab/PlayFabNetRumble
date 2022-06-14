//--------------------------------------------------------------------------------------
// Game.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

extern void ExitGame();

using namespace NetRumble;
using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
	constexpr DWORD WaitTimeoutInMs = 500;
}

Game::~Game()
{
	CleanupUser();
	Managers::Shutdown();
	WaitForAndCleanupHandles();
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window)
{
	RECT rc;
	GetClientRect(window, &rc);

	m_outputWidth = rc.right - rc.left;
	m_outputHeight = rc.bottom - rc.top;

#ifdef _DEBUG
	DebugInit();
#endif

	// Lock the game to 60FPS
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);

	Managers::Initialize();

	Managers::Get<RenderManager>()->Initialize(window, m_outputWidth, m_outputHeight);
	m_descriptors = Managers::Get<RenderManager>()->CreateDescriptorPile(64);

	Managers::Get<ContentManager>()->Initialize(m_descriptors);
	Managers::Get<AudioManager>()->Initialize();
	Managers::Get<OnlineManager>()->Initialize();

	PrefetchContent();

	GameStateManager* gameStateMgr = Managers::Get<GameStateManager>();

	gameStateMgr->SwitchToState(GameState::StartMenu);
	m_world = std::make_unique<World>();

	Managers::Get<ScreenManager>()->AddBackgroundScreen(std::make_unique<StarfieldScreen>());
	Managers::Get<ScreenManager>()->AddForegroundScreen(std::make_unique<DebugOverlayScreen>());
	Managers::Get<OnlineManager>()->RegisterOnlineMessageHandler([this](std::string source, GameMessage* message)
		{
			Managers::Get<OnlineManager>()->ProcessGameNetworkMessage(source, message);
		});
}

// Executes the basic game loop.
void Game::Tick()
{
	m_timer.Tick([&]()
		{
			Update(m_timer);
		});

	Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	PlayFabClientAPI::Update();
	Managers::Get<AudioManager>()->Tick();
	Managers::Get<GameStateManager>()->Update();
	Managers::Get<InputManager>()->Update();
	Managers::Get<ParticleEffectManager>()->Update(static_cast<float_t>(timer.GetElapsedSeconds()));
	Managers::Get<ScreenManager>()->Update(timer);
	if (m_isLoggedIn)
	{
		Managers::Get<OnlineManager>()->Tick(static_cast<float_t>(timer.GetElapsedSeconds()));
		Managers::Get<OnlineManager>()->PlayfabPartyDoWork();
	}
}

// Draws the scene.
void Game::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	RenderManager* renderManager = Managers::Get<RenderManager>();
	renderManager->Clear(m_descriptors->Heap());

	Managers::Get<ScreenManager>()->Render(m_timer);

	renderManager->Present();
}

void Game::OnSuspending()
{
	Managers::Get<ScreenManager>()->Suspend();
	Managers::Get<RenderManager>()->Suspend();
	Managers::Get<AudioManager>()->Suspend();
	Managers::Get<InputManager>()->Suspend();

	Managers::Get<OnlineManager>()->XblCleanup();

	// Wait for all async completion handles
	WaitForAndCleanupHandles();
	Managers::Get<GameStateManager>()->SwitchToState(GameState::Suspended);
}

void Game::OnResuming()
{
	Managers::Get<InputManager>()->Resume();
	Managers::Get<AudioManager>()->Resume();
	Managers::Get<RenderManager>()->Resume();

	m_timer.ResetElapsedTime();

	// We need to go back to the start menu (this already waits for networking connectivity)
	ResumeGame();
}

void Game::OnWindowSizeChanged(int width, int height)
{
	RenderManager* renderMgr = Managers::Get<RenderManager>();

	if (!renderMgr->OnWindowSizeChanged(width, height))
		return;

	renderMgr->CreateWindowSizeDependentResources();
}

void Game::PrefetchContent()
{
	ContentManager* contentManager = Managers::Get<ContentManager>();
	std::wstring texturePath = L"Assets\\Textures\\";

	// Ship
	contentManager->LoadTexture(texturePath + L"ship0.png");
	contentManager->LoadTexture(texturePath + L"ship1.png");
	contentManager->LoadTexture(texturePath + L"ship2.png");
	contentManager->LoadTexture(texturePath + L"ship3.png");
	contentManager->LoadTexture(texturePath + L"ship0Overlay.png");
	contentManager->LoadTexture(texturePath + L"ship1Overlay.png");
	contentManager->LoadTexture(texturePath + L"ship2Overlay.png");
	contentManager->LoadTexture(texturePath + L"ship3Overlay.png");
	contentManager->LoadTexture(texturePath + L"shipShields.png");

	// Asteroids
	contentManager->LoadTexture(texturePath + L"asteroid0.png");
	contentManager->LoadTexture(texturePath + L"asteroid1.png");
	contentManager->LoadTexture(texturePath + L"asteroid2.png");

	// Laser
	contentManager->LoadTexture(texturePath + L"laser.png");
	contentManager->LoadTexture(texturePath + L"powerupDoubleLaser.png");
	contentManager->LoadTexture(texturePath + L"powerupTripleLaser.png");

	// Mine
	contentManager->LoadTexture(texturePath + L"mine.png");

	// Rocket
	contentManager->LoadTexture(texturePath + L"rocket.png");
	contentManager->LoadTexture(texturePath + L"powerupRocket.png");

	AudioManager* audioManager = Managers::Get<AudioManager>();
	std::wstring audioPath = L"Assets\\Audio\\";

	audioManager->LoadSound(L"asteroid_touch", audioPath + L"asteroid_touch.wav");
	audioManager->LoadSound(L"explosion_large", audioPath + L"explosion_large.wav");
	audioManager->LoadSound(L"explosion_medium", audioPath + L"explosion_medium.wav");
	audioManager->LoadSound(L"explosion_shockwave", audioPath + L"explosion_shockwave.wav");
	audioManager->LoadSound(L"fire_laser1", audioPath + L"fire_laser1.wav");
	audioManager->LoadSound(L"fire_laser2", audioPath + L"fire_laser2.wav");
	audioManager->LoadSound(L"fire_laser3", audioPath + L"fire_laser3.wav");
	audioManager->LoadSound(L"fire_rocket1", audioPath + L"fire_rocket1.wav");
	audioManager->LoadSound(L"fire_rocket2", audioPath + L"fire_rocket2.wav");
	audioManager->LoadSound(L"menu_scroll", audioPath + L"menu_scroll.wav");
	audioManager->LoadSound(L"menu_select", audioPath + L"menu_select.wav");
	audioManager->LoadSound(L"player_spawn", audioPath + L"player_spawn.wav");
	audioManager->LoadSound(L"powerup_spawn", audioPath + L"powerup_spawn.wav");
	audioManager->LoadSound(L"powerup_touch", audioPath + L"powerup_touch.wav");
	audioManager->LoadSound(L"rocket", audioPath + L"rocket.wav");
}

std::shared_ptr<PlayerState> Game::GetPlayerState(const std::string& peer)
{
	auto itr = m_peers.find(peer);
	if (itr != m_peers.end())
	{
		return (*itr).second;
	}

	return nullptr;
}

std::shared_ptr<PlayerState> Game::GetLocalPlayerState()
{
	std::string localPlayerId = Managers::Get<OnlineManager>()->GetEntityKey().Id;
	auto itrLocalPlayer = m_peers.find(localPlayerId);
	return (*itrLocalPlayer).second;
}

void Game::ClearPlayerScores()
{
	for (auto& peer : m_peers)
	{
		peer.second->GetShip()->Score = 0;
	}
}

void Game::RemovePlayerFromLobbyPeers(std::string playerId)
{
	auto itr = m_peers.erase(playerId);
	if (itr == 0)
	{
		DEBUGLOG(" There is not player ID[%s] in peers\n", playerId.c_str());
	}
}

void Game::AddPlayerToLobbyPeers(std::shared_ptr<PlayerState> player)
{
	if (player)
	{
		m_peers[player->EntityId] = player;
	}
}

void Game::DeleteOtherPlayerInPeersAndExitLobby()
{
	for (auto item = m_peers.begin(); item != m_peers.end();)
	{
		if (!item->second->IsLocalPlayer)
		{
			item = m_peers.erase(item);
		}
		else
		{
			item->second->InLobby = false;
			item->second->InGame = false;
			item->second->LobbyReady = false;
			item->second->ShipColor(0);
			item->second->ShipVariation(0);
			item++;
		}
	}
}

bool Game::CheckAllPlayerReady()
{
	bool allIsReady = true;
	size_t lobbyMembersCount = m_peers.size();
	if (lobbyMembersCount > 0)
	{
		for (auto player : m_peers)
		{
			if (player.second->LobbyReady == false)
			{
				allIsReady = false;
			}
		}
	}
	else
	{
		allIsReady = false;
	}

	return allIsReady;
}

void Game::StartGame()
{
	GameState gameState = Managers::Get<GameStateManager>()->GetState();

	if (gameState == GameState::GameConnecting || gameState == GameState::Gaming)
	{
		std::shared_ptr<PlayerState> localPlayer = GetLocalPlayerState();
		localPlayer->InGame = true;

		Managers::Get<OnlineManager>()->SendGameMessage(
			GameMessage(
				GameMessageType::SynPlayerData,
				GetLocalPlayerState()->SerializePlayerStateData()
			)
		);

		m_world->SetGameInProgress(true);

		if (Managers::Get<OnlineManager>()->IsHost())
		{
			if (!m_world->IsInitialized())
			{
				m_world->GenerateWorld();
			}
			Managers::Get<OnlineManager>()->SendGameMessage(
				GameMessage(
					GameMessageType::GameStart,
					0
				)
			);

			std::map<std::string, std::shared_ptr<Ship>> ships;
			for (auto item : m_peers)
			{
				if (item.second && !item.second->IsInactive())
				{
					ships[item.first] = item.second->GetShip();
				}
			}

			Managers::Get<OnlineManager>()->SendGameMessage(
				GameMessage(
					GameMessageType::WorldSetup,
					m_world->SerializeWorldSetup(ships)
				)
			);
		}
	}
}

void Game::DrawWorld(float elapsedTime)
{
	if (m_world->IsInitialized())
	{
		m_world->Draw(elapsedTime);
	}
}

void Game::UpdateWorld(float totalTime, float elapsedTime)
{
	if (m_world->IsInitialized())
	{
		m_world->Update(totalTime, elapsedTime);
	}
}

std::vector<std::shared_ptr<PlayerState>> Game::GetAllPlayerStates()
{
	std::vector<std::shared_ptr<PlayerState>> players;

	for (auto& [id, peer] : m_peers)
	{
		players.push_back(peer);
	}

	return players;
}

void Game::ResetGameplayData()
{
	// Reset any existing game world to its defaults
	if (m_world != nullptr)
	{
		m_world->ResetDefaults();
		m_world->SetGameInProgress(false);
	}

	for (auto& [id, player] : m_peers)
	{
		if (player)
		{
			player->LobbyReady = false;
			player->InGame = false;
			player->InLobby = false;
			player->ShipColor(0);
			player->ShipVariation(0);
		}
	}
}

void Game::ResumeGame()
{
	const PlayFabLoginType& loginType = Managers::Get<OnlineManager>()->GetLoginType();
	if (loginType == PlayFabLoginType::LoginWithCustomID)
	{
		Managers::Get<GameStateManager>()->SwitchToState(GameState::Login);
		Managers::Get<OnlineManager>()->LoginWithCustomID(
			[this](bool success, const std::string& userId)
			{
				UNREFERENCED_PARAMETER(userId);
				if (success)
				{
					Managers::Get<OnlineManager>()->InitializePlayfabParty();
					if (Managers::Get<OnlineManager>()->IsPartyInitialized()) {
						Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
					}
					else
					{
						DEBUGLOG("Unable to initialize party\n");
					}
				}
				else
				{
					DEBUGLOG("Unable to login with Custom ID");
				}
			});
	}
	else if (loginType == PlayFabLoginType::LoginWithXboxLive)
	{
		Managers::Get<GameStateManager>()->SwitchToState(GameState::Login);
		Managers::Get<OnlineManager>()->LoginWithXboxLive(
			[this](bool success, const std::string& errorMessage)
			{
				if (success)
				{
					Managers::Get<OnlineManager>()->InitializePlayfabParty();
					if (Managers::Get<OnlineManager>()->IsPartyInitialized()) {
						Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
					}
					else
					{
						DEBUGLOG("Unable to initialize party\n");
					}
				}
				else
				{
					DEBUGLOG(errorMessage.c_str());
					Managers::Get<ScreenManager>()->ShowError(errorMessage, []() {
						Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
						});
				}
			});
	}
	else if (loginType == PlayFabLoginType::Undefine)
	{
		Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
	}
}

void Game::AddHandleToWaitSet(HANDLE handle)
{
	HANDLE duplicate = nullptr;

	DuplicateHandle(
		GetCurrentProcess(),
		handle,
		GetCurrentProcess(),
		&duplicate,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);

	m_waitHandles.push_back(duplicate);
}

void Game::WaitForAndCleanupHandles()
{
	if (m_waitHandles.empty())
	{
		return;
	}

	WaitForMultipleObjects(DWORD(m_waitHandles.size()), m_waitHandles.data(), true, WaitTimeoutInMs);

	for (auto handle : m_waitHandles)
	{
		CloseHandle(handle);
	}
	m_waitHandles.clear();
}

void Game::LocalPlayerInitialize()
{
	Managers::Get<OnlineManager>()->GetUserName(m_localPlayerName);
	std::shared_ptr<PlayerState> localPlayer = std::make_shared<PlayerState>(m_localPlayerName);
	localPlayer->IsLocalPlayer = true;
	localPlayer->InLobby = true;
	localPlayer->ShipColor(0);
	localPlayer->ShipVariation(0);
	localPlayer->PeerId = Managers::Get<OnlineManager>()->GetLocalUserId();
	localPlayer->EntityId = Managers::Get<OnlineManager>()->GetLocalUserEntityId();
	m_peers[localPlayer->EntityId] = localPlayer;
}

void Game::CleanupUser()
{
	ResetGameplayData();

	// Stop matchmaking
	if (Managers::Get<OnlineManager>()->IsMatchmaking())
	{
		Managers::Get<OnlineManager>()->CancelMatchmaking();
	}

	// Clean up local user state
	if (!Managers::Get<OnlineManager>()->GetLocalEntityId().empty())
	{
		m_peers.erase(Managers::Get<OnlineManager>()->GetLocalEntityId());
		m_localPlayerName.clear();
	}

	HANDLE shutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	AddHandleToWaitSet(shutdownEvent);
}

void Game::ExitGame()
{
	Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
}

void Game::CleanupOnlineServices()
{
	if (GetPeers().size() != 0)
	{
		CleanupUser();
		Managers::Get<OnlineManager>()->CleanupOnlineServices();
	}
}