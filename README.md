# NetRumble Demo Game

_(Please use Git client with Large File Storage (LFS) support to work with this repo)_

This is a simple multiplayer game intended to demonstrate how developers can use PlayFab Party and Multiplayer C++ SDKs and API in games initially created for Steam&reg; using Steamworks SDK. The main multiplayer features implemented in game are:

- Creating/joining lobbies for players
- Matchmaking
- Voice chat
- Data and message exchange between game clients
- Crossplay between Steam and Xbox Live (GamePass) game clients powered by PlayFab SDKs

It is expected that developers should be familiar with the basics of using PlayFab C++ SDKs and PlayFab API as well as using Steamworks SDK, creating, configuring and developing games for Steam and uploading them to Steam Storefront with a Steam developer account, before building this sample game.
Additionally, the Xbox Live variant of the game requires familiarity with Microsoft GDK and the process of creating and configuring Xbox Live game titles.

The source code is provided for 3 variants of game:

- `NetRumble-BaseSteam`: Base Steam (a game using Steamworks SDK only)
- `NetRumble-PlayFabSteam`: Integrated with PlayFab Party and Multiplayer C++ SDKs for Windows, and Steamworks SDK (the Base Steam variant with Steam multiplayer features replaced by PlayFab SDKs. It uses Steam player authentication like the Base Steam variant)
- `NetRumble-PlayFabXboxLiveGDK`: Integrated with PlayFab Party and Multiplayer C++ SDKs for GDK only (a variant with Steam multiplayer features replaced by PlayFab SDKs. This is a GDK game, it uses Xbox Live player authentication and requires Xbox Live title setup)

Each variant of the game comes with its own documentation file in their respecive folders. It contains detailed instructions and lists dependencies that need to be installed before configuring and building the app.

---
Steam&reg; and the Steam logo are trademarks and/or registered trademarks of Valve Corporation in the U.S. and/or other countries.