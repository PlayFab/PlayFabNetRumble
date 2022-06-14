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

extern bool ParseCommandLine(const char* pchCmdLine, const char** ppchServerAddress, const char** ppchLobbyID);

namespace
{
	constexpr DWORD WaitTimeoutInMs = 500;
}

Game::Game() noexcept
{
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

	LocalPlayerInitialize();

	PrefetchContent();

	GameStateManager* gameStateMgr = Managers::Get<GameStateManager>();

	gameStateMgr->SwitchToState(GameState::MainMenu);
	m_world = std::make_unique<World>();

	Managers::Get<ScreenManager>()->AddBackgroundScreen(std::make_unique<StarfieldScreen>());
	Managers::Get<ScreenManager>()->AddForegroundScreen(std::make_unique<DebugOverlayScreen>());
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

void NetRumble::Game::ProcessGameNetworkMessage(uint64_t steamID, GameMessage* message)
{
	UNREFERENCED_PARAMETER(steamID);
	UNREFERENCED_PARAMETER(message);
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	Managers::Get<AsyncTaskManager>()->Tick();
	Managers::Get<ScreenManager>()->Update(timer);
	Managers::Get<InputManager>()->Update();
	Managers::Get<AudioManager>()->Tick();
	Managers::Get<ParticleEffectManager>()->Update(static_cast<float_t>(timer.GetElapsedSeconds()));
	Managers::Get<GameStateManager>()->Update();
	Managers::Get<OnlineManager>()->Tick(static_cast<float_t>(timer.GetElapsedSeconds()));
	if (GetGameServer()) {
		GetGameServer()->Tick();
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

// Message handlers
void Game::OnActivated()
{
	// TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
	// TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
	Managers::Get<ScreenManager>()->Suspend();
	Managers::Get<RenderManager>()->Suspend();
	Managers::Get<AudioManager>()->Suspend();
	Managers::Get<InputManager>()->Suspend();

	CleanupUser();

	// Wait for all async completion handles
	WaitForAndCleanupHandles();
}

void Game::OnResuming()
{
	Managers::Get<InputManager>()->Resume();
	Managers::Get<AudioManager>()->Resume();
	Managers::Get<RenderManager>()->Resume();
	Managers::Get<ScreenManager>()->Resume();

	m_timer.ResetElapsedTime();

	// We need to go back to the start menu (this already waits for networking connectivity)
	Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
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

std::shared_ptr<PlayerState> Game::GetPlayerState(uint64_t peer)
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
	for (const auto& peer : m_peers)
	{
		if (peer.second->IsLocalPlayer)
		{
			return peer.second;
		}
	}

	return nullptr;
}

void Game::SetPlayerState(uint64_t id, std::shared_ptr<PlayerState> state)
{
	std::shared_ptr<PlayerState> peer = GetPlayerState(id);

	if (peer != nullptr && state == nullptr)
	{
		peer->DeactivatePlayer();
		m_peers.erase(id);
	}
	else
	{
		m_peers[id] = state;
	}
}

void Game::ClearPlayerScores()
{
	for (auto& pair : m_peers)
	{
		pair.second->GetShip()->Score = 0;
	}
}

void NetRumble::Game::RemovePeers(uint64 playerId)
{
	if (m_peers[playerId] && m_peers[playerId]->InGame == false)
	{
		m_peers.erase(playerId);
	}
}

bool NetRumble::Game::RemovePlayerFromLobbyPeers(uint64 playerId)
{
	if (!m_peers[playerId])
	{
		DEBUGLOG("m_peers is nullptr");
		return false;
	}
	if (m_peers[playerId]->InGame == true)
	{
		DEBUGLOG("m_peers[%d] is leaving the lobby for the game, so cannot remove this peer", playerId);
		return false;
	}
	m_peers[playerId]->IsReturnedToMainMenu = true;
	return true;
}

bool NetRumble::Game::RemovePlayerFromGamePeers(uint64 playerId)
{
	if (m_peers.find(playerId) != m_peers.end())
	{
		if (!g_game->GetPlayerState(playerId)->IsLocalPlayer)
		{
			DEBUGLOG("m_peers[%d] has left the game\n", playerId);
			m_peers.erase(playerId);
			return true;
		}
		else
		{
			DEBUGLOG("m_peers[%d] is local player\n", playerId);
			return true;
		}
	}
	DEBUGLOG("The PlayerId[%d] is not in m_peers\n", playerId);
	return false;
}

bool NetRumble::Game::RemovePlayerFromGamePeers(std::shared_ptr<PlayerState> player)
{
	if (!player)
	{
		DEBUGLOG("player is nullptr\n");
		return false;
	}
	if (!m_peers[player->PeerId])
	{
		DEBUGLOG("player does not exist m_peers.\n");
		return false;
	}
	else
	{
		DEBUGLOG("m_peers[%d] has left the game\n", player->PeerId);
		m_peers.erase(player->PeerId);
		return true;
	}
}

void NetRumble::Game::AddPlayerToLobbyPeers(uint64 playerId)
{
	std::shared_ptr<PlayerState> player = std::make_shared<PlayerState>(SteamFriends()->GetPersonaName());
	player->IsLocalPlayer = false;
	player->InGame = false;
	player->InLobby = true;
	player->IsReturnedToMainMenu = false;
	player->ShipColor(0);
	player->ShipVariation(0);
	player->PeerId = playerId;

	m_peers[player->PeerId] = player;
}

void NetRumble::Game::AddPlayerToLobbyPeers(std::shared_ptr<PlayerState> player)
{
	if (player)
	{
		m_peers[player->PeerId] = player;
	}
}

void NetRumble::Game::DeleteOtherPlayerInPeersAndExitLobby()
{
	for (auto item  = m_peers.begin(); item != m_peers.end();)
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
			item->second->IsReturnedToMainMenu = false;
			item++;
		}
	}
}

void Game::ServerPrepareGameEnviroment()
{
	if (Managers::Get<OnlineManager>()->IsServer())
	{
		if (!m_world->IsInitialized())
		{
			m_world->GenerateWorld();
		}

		Managers::Get<OnlineManager>()->ServerSendMessageToAll(
			GameMessage(
				GameMessageType::GameStart,
				0
			),
			false,
			k_nSteamNetworkingSend_Reliable
		);

		std::map<uint64_t, std::shared_ptr<Ship>> ships;

		for (auto item : m_peers)
		{
			if (item.second && !item.second->IsInactive())
			{
				ships[item.first] = item.second->GetShip();
			}
		}

		Managers::Get<OnlineManager>()->ServerSendMessageToAll(
			GameMessage(
				GameMessageType::ServerWorldSetup,
				m_world->SerializeWorldSetup(ships)
			),
			false,
			k_nSteamNetworkingSend_Reliable
		);

		GameState gameState = Managers::Get<GameStateManager>()->GetState();
		if (gameState == GameState::Gaming)
		{
			// If the verification is not completed until after the game has started due to network reasons,
			// the game should be notified to start again.
			Managers::Get<OnlineManager>()->SendGameMessageWithSourceID(
				GameMessage(
					GameMessageType::PlayerJoined,
					GetLocalPlayerState()->SerializePlayerStateData()
				)
			);
		}
	}
}

void Game::StartGame()
{
	GameState gameState = Managers::Get<GameStateManager>()->GetState();

	if (gameState == GameState::GameConnecting || gameState == GameState::EvaluatingNetwork || gameState == GameState::Gaming)
	{
		Managers::Get<ScreenManager>()->AddGameScreen<GamePlayScreen>();
		Managers::Get<ScreenManager>()->SetBackgroundsVisible(false);
		Managers::Get<GameStateManager>()->SwitchToState(GameState::JoinGameFromLobby);
		m_world->SetGameInProgress(true);
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
			player->IsReturnedToMainMenu = false;
		}
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
	m_localPlayerName = SteamFriends()->GetPersonaName();

	std::shared_ptr<PlayerState> localPlayer = std::make_shared<PlayerState>(m_localPlayerName);
	localPlayer->IsLocalPlayer = true;
	localPlayer->InLobby = true;
	localPlayer->ShipColor(0);
	localPlayer->ShipVariation(0);
	localPlayer->PeerId = SteamUser()->GetSteamID().ConvertToUint64();
	Managers::Get<OnlineManager>()->SetLocalSteamID(SteamUser()->GetSteamID());

	m_peers[localPlayer->PeerId] = localPlayer;
}

void Game::CleanupUser()
{
	ResetGameplayData();

	// Stop matchmaking
	if (Managers::Get<OnlineManager>()->IsMatchmaking())
	{
		Managers::Get<OnlineManager>()->CancelMatchmaking();
	}
	// Stop playing and leave the session
	else if (Managers::Get<OnlineManager>()->IsConnected())
	{
		Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
	}

	// Clean up local user state
	if (0 != Managers::Get<OnlineManager>()->GetNetworkId())
	{
		m_peers.erase(Managers::Get<OnlineManager>()->GetNetworkId());
		m_localPlayerName.clear();
	}

	HANDLE shutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	AddHandleToWaitSet(shutdownEvent);
}
