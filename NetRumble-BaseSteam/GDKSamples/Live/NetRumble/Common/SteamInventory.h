#pragma once

namespace NetRumble
{

	class Inventory
	{
	public:
		Inventory();
		~Inventory();

		void CheckForItemDrops();
		void GetAllItems();

	private:
		SteamInventoryResult_t m_playtimeRequestResult;
		SteamInventoryResult_t m_hLastFullUpdate;
		STEAM_CALLBACK(Inventory, OnSteamInventoryResult, SteamInventoryResultReady_t, m_SteamInventoryResult);
		STEAM_CALLBACK(Inventory, OnSteamInventoryFullUpdate, SteamInventoryFullUpdate_t, m_SteamInventoryFullUpdate);
	};
}