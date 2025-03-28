#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {
        inline void LobbyUserDetails(SCallbackData& callback, CMainServer* main_server)
        {
            auto send_msg = [&](CSession* session, uint16_t order, uint8_t mission, uint8_t extra, uint8_t option, uint8_t* data = nullptr, uint16_t data_size = 0)
            {
                CMessage message(session->GetEncryptionKey());
                message.SetSession(session->GetSessionId());
                message.SetCommand(order, mission, extra, option);
                if (data_size > 0 && data != nullptr) message.SetData(data, data_size);
                session->Send(message);
            };
            std::shared_lock lock(callback.session->GetMutex());
            CSession* session = callback.session;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);

            auto acc_index = acc_cache->acc_info.Index;
            auto request_type = callback.message->GetExtra();
            if (acc_index == -1) return;
            auto userInfoDetailsReq = reinterpret_cast<MainPlayerDetailsInfoReq*>(callback.message->GetData());
            auto user_uniqueid = NetEngine::Packets::Core::UniqueId(userInfoDetailsReq->unique_id);
            auto target_acc_cache = main_server->GetAccCacheSharedBySessionId(static_cast<uint16_t>(user_uniqueid.session));
            if (target_acc_cache->acc_info.Index == -1) return;
            std::vector<BaseLib::Item> equipped_items;
            for (auto& item : target_acc_cache->inventory_items)
                if (item.is_equipped == 1 && item.character_id == static_cast<uint8_t>(target_acc_cache->acc_info.SelectedCharacter))
                    equipped_items.push_back(item);

            MainPlayerDetailsInfoAck detailsInfo;
            detailsInfo.Kills = target_acc_cache->acc_info.Kills;
            detailsInfo.Deaths = target_acc_cache->acc_info.Deaths;
            detailsInfo.Assists = target_acc_cache->acc_info.Assists;
            detailsInfo.Wins = target_acc_cache->acc_info.Wins;
            detailsInfo.Loses = target_acc_cache->acc_info.Loses;
            detailsInfo.Draws = target_acc_cache->acc_info.Draws;
            detailsInfo.Melee = target_acc_cache->acc_info.MeleeKills;
            detailsInfo.Rifle = target_acc_cache->acc_info.RifleKills;
            detailsInfo.Shotgun = target_acc_cache->acc_info.ShotgunKills;
            detailsInfo.Sniper = target_acc_cache->acc_info.SniperKills;
            detailsInfo.Gatling = target_acc_cache->acc_info.GatlingKills;
            detailsInfo.Bazooka = target_acc_cache->acc_info.BazookaKills;
            detailsInfo.Grenade = target_acc_cache->acc_info.GrenadeKills;
            detailsInfo.Headshots = target_acc_cache->acc_info.Headshots;
            detailsInfo.HighestKillStreak = target_acc_cache->acc_info.HighestKillStreak;
            detailsInfo.PlayTime = static_cast<uint32_t>(target_acc_cache->acc_info.PlayTime);
            detailsInfo.Unknown1 = 0;
            detailsInfo.Unknown2 = 0;
        #if defined(RELEASE_1_1_1)
            detailsInfo.Unknown3 = 0;
        #endif
            detailsInfo.ClanId = target_acc_cache->acc_info.ClanId;
            if (detailsInfo.ClanId)
            {
                if (main_server->IsClanAlready(detailsInfo.ClanId))
                {
                    auto clan_cache = main_server->GetClanCacheShared(detailsInfo.ClanId);
                    detailsInfo.ClanLogoFront = clan_cache->logo_front;
                    detailsInfo.ClanLogoBack = clan_cache->logo_back;
                    std::strcpy(detailsInfo.ClanName, clan_cache->clan_name.c_str());
                    clan_cache.unlock();
                }
                detailsInfo.ClanContribution = target_acc_cache->acc_info.ClanContribution;
                detailsInfo.ClanWins = target_acc_cache->acc_info.ClanWins;
                detailsInfo.ClanLoses = target_acc_cache->acc_info.ClanLoses;
                detailsInfo.ClanDraws = target_acc_cache->acc_info.ClanDraws;
                detailsInfo.ClanKills = target_acc_cache->acc_info.ClanKills;
                detailsInfo.ClanDeaths = target_acc_cache->acc_info.ClanDeaths;
                detailsInfo.ClanAssists = target_acc_cache->acc_info.ClanAssists;
            }
            else
            {
                detailsInfo.ClanLogoFront = 0;
                detailsInfo.ClanLogoBack = 0;
                detailsInfo.ClanContribution = 0;
                detailsInfo.ClanWins = 0;
                detailsInfo.ClanLoses = 0;
                detailsInfo.ClanDraws = 0;
                detailsInfo.ClanKills = 0;
                detailsInfo.ClanDeaths = 0;
                detailsInfo.ClanAssists = 0;
            }
            
            detailsInfo.ZombieKillPoints = target_acc_cache->acc_info.ZombieKills;
            detailsInfo.Infections = target_acc_cache->acc_info.Infections;
            detailsInfo.SelectedCharacter = target_acc_cache->acc_info.SelectedCharacter;
        #if defined(RELEASE_1_1_1)
            detailsInfo.Channel = 0;
            detailsInfo.VIPLevel = 1;
        #endif
            detailsInfo.Level = target_acc_cache->acc_info.Level + 1;
            detailsInfo.Grade = target_acc_cache->acc_info.Grade;
            detailsInfo.Achievements[0] = target_acc_cache->acc_info.Achievement;
            detailsInfo.Channel = 1;

            detailsInfo.diorama1 = main_server->GetItemByType(equipped_items, 22).item_info.item_number.item_id;
            detailsInfo.diorama2 = main_server->GetItemByType(equipped_items, 23).item_info.item_number.item_id;
            auto set_item = main_server->GetItemByType(equipped_items, 25).item_info.item_number.item_id;
            auto setitem_info = main_server->GetSetItemInfoCache(set_item);
            auto assign_item = [&](int type, auto setitem_field)
            {
                auto item = main_server->GetItemByType(equipped_items, type).item_info.item_number.item_id;
                auto result = item ? item : (setitem_field != UINT32_MAX ? setitem_info->Id : 0);
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "for type ({}) setitem_field is ({}) result is ({})", type, setitem_field, result);
                return result;
            };
            detailsInfo.EquippedHairItemId = assign_item(0, setitem_info->Hair);
            detailsInfo.EquippedFaceItemId = assign_item(1, setitem_info->Face);
            detailsInfo.EquippedUpperItemId = assign_item(2, setitem_info->Upper);
            detailsInfo.EquippedUnderItemId = assign_item(3, setitem_info->Under);
            detailsInfo.EquippedPantsItemId = assign_item(4, setitem_info->Pants);
            detailsInfo.EquippedShirtItemId = assign_item(5, setitem_info->Arms);
            detailsInfo.EquippedBootsItemId = assign_item(6, setitem_info->Boots);
            detailsInfo.EquippedGlassItemId = assign_item(7, setitem_info->AccessoryA);
            detailsInfo.EquippedAccessoryWaistItemId = assign_item(8, setitem_info->AccessoryB);
            detailsInfo.EquippedAccessoryBackItemId = assign_item(9, setitem_info->AccessoryC);
            detailsInfo.EquippedMeleeItemId = main_server->GetItemByType(equipped_items, 10).item_info.item_number.item_id;
            detailsInfo.EquippedRifleItemId = main_server->GetItemByType(equipped_items, 11).item_info.item_number.item_id;
            detailsInfo.EquippedShotgunItemId = main_server->GetItemByType(equipped_items, 12).item_info.item_number.item_id;
            detailsInfo.EquippedSniperItemId = main_server->GetItemByType(equipped_items, 13).item_info.item_number.item_id;
            detailsInfo.EquippedGatlingItemId = main_server->GetItemByType(equipped_items, 14).item_info.item_number.item_id;
            detailsInfo.EquippedGrenadeItemId = main_server->GetItemByType(equipped_items, 15).item_info.item_number.item_id;
            detailsInfo.EquippedBazookaItemId = main_server->GetItemByType(equipped_items, 16).item_info.item_number.item_id;
            send_msg(session, 85, 0, detailsInfo.ClanId ? Userlist::Friends::DetailsType::WithClan : Userlist::Friends::DetailsType::WithoutClan, static_cast<uint8_t>(user_uniqueid.server), reinterpret_cast<uint8_t*>(&detailsInfo), sizeof(detailsInfo));
        }
    }
    
}