//--------------------------------------------------------------------------------------
// ServerConfig.h
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{
#define USE_GS_AUTH_API 

	// Current game server version
#define NETRUMBLE_SERVER_VERSION "1.0.0.0"

	// This PlayFab title ID will need to be set, before building and running the game.
	// In the final code we will need to remove the hard - coded app specific information.PlatFabTitleId
	static constexpr std::string_view NETRUMBLE_PLAYFAB_TITLE_ID{ "00000" };
}
