#include "pch.h"

using namespace NetRumble;

namespace {
	constexpr int32 SpaceWarItemTimedDropList = 11;
	constexpr int32 SteamInventoryResultInvalid = -1;
}

NetRumble::Inventory::Inventory() :
	m_playtimeRequestResult{ 0 },
	m_hLastFullUpdate{ 0 },
	m_SteamInventoryResult(this, &Inventory::OnSteamInventoryResult),
	m_SteamInventoryFullUpdate(this, &Inventory::OnSteamInventoryFullUpdate)
{
}

NetRumble::Inventory::~Inventory()
{
}

void NetRumble::Inventory::CheckForItemDrops()
{
	SteamInventory()->TriggerItemDrop(&m_playtimeRequestResult, SpaceWarItemTimedDropList);
}

void NetRumble::Inventory::GetAllItems()
{
	SteamInventory()->GetAllItems(nullptr);
}

void NetRumble::Inventory::OnSteamInventoryResult(SteamInventoryResultReady_t* callback)
{
	CSteamID localSteamID = Managers::Get<OnlineManager>()->GetLocalSteamID();
	if (callback->m_result == k_EResultOK && m_hLastFullUpdate != callback->m_handle &&
		SteamInventory()->CheckResultSteamID(callback->m_handle, localSteamID))
	{
		bool bGotResult = false;
		std::vector<SteamItemDetails_t> vecDetails;
		uint32 count = 0;
		if (SteamInventory()->GetResultItems(callback->m_handle, nullptr, &count))
		{
			vecDetails.resize(count);
			bGotResult = SteamInventory()->GetResultItems(callback->m_handle, vecDetails.data(), &count);
		}

		if (bGotResult)
		{
			// vecDetails is a list of items obtained from steam server
			std::map<int, int> tempItemId;	// Count inventory quantity
			for (size_t i = 0; i < vecDetails.size(); i++)
			{
				DEBUGLOG("vecDetails[%d] ItemID in game is %d\n", i, vecDetails[i].m_iDefinition);
				tempItemId[vecDetails[i].m_iDefinition]++;
				// This is where you can tell if items have been consumed
				if (vecDetails[i].m_unFlags & k_ESteamItemRemoved)
				{
					// TODO
				}
			}
			
			g_game->m_OutputMessage.clear();
			for (auto itemId : tempItemId)
			{
				g_game->m_OutputMessage += "item" + std::to_string(itemId.first) + "*" + std::to_string(itemId.second) + "; ";
			}
		}
	}

	if (callback->m_handle == m_playtimeRequestResult)
		m_playtimeRequestResult = SteamInventoryResultInvalid;
	if (callback->m_handle == m_hLastFullUpdate)
		m_hLastFullUpdate = SteamInventoryResultInvalid;

	// We're not hanging on the result after processing it.
	SteamInventory()->DestroyResult(callback->m_handle);
}

void NetRumble::Inventory::OnSteamInventoryFullUpdate(SteamInventoryFullUpdate_t* callback)
{
	bool bGotResult = false;
	std::vector<SteamItemDetails_t> vecDetails;
	uint32 count = 0;
	if (SteamInventory()->GetResultItems(callback->m_handle, nullptr, &count))
	{
		vecDetails.resize(count);
		bGotResult = SteamInventory()->GetResultItems(callback->m_handle, vecDetails.data(), &count);
	}

	if (bGotResult)
	{
		bool bFound = false;
		// vecDetails is a list of items obtained from steam server
		std::map<int, int> tempItemId;	// Count inventory quantity
		for (size_t i = 0; i < vecDetails.size(); i++)
		{
			DEBUGLOG("vecDetails[%d] ItemID in game is %d\n", i, vecDetails[i].m_iDefinition);
			tempItemId[vecDetails[i].m_iDefinition]++;
		}

		g_game->m_OutputMessage.clear();
		for (auto itemId: tempItemId)
		{
			g_game->m_OutputMessage += "item" + std::to_string(itemId.first) + "*" + std::to_string(itemId.second) + "; ";
		}
	}
	
	m_hLastFullUpdate = callback->m_handle;
}