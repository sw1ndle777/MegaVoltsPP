#include "CMainServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CDatabase.h"
#include "BaseLib/CDBData.h"
#include "BaseLib/CCache.h"

#include "handlers/Core/Cmds.h"

#include "handlers/Core/Cmds/Core/Core/Help.h"
#include "handlers/Core/Cmds/Core/Core/Notice.h"

#include "handlers/Core/Cmds/Core/Metrics/CastInfo.h"
#include "handlers/Core/Cmds/Core/Metrics/MainInfo.h"
#include "handlers/Core/Cmds/Core/Metrics/Online.h"
#include "handlers/Core/Cmds/Core/Metrics/RoomInfo.h"
#include "handlers/Core/Cmds/Core/Metrics/Rooms.h"

#include "handlers/Core/Cmds/Core/System/Maintenance.h"
#include "handlers/Core/Cmds/Core/System/ReloadGachaSale.h"
#include "handlers/Core/Cmds/Core/System/TpToProj.h"

#include "handlers/Core/Cmds/Items/ClearInventory.h"
#include "handlers/Core/Cmds/Items/EffectCurr.h"
#include "handlers/Core/Cmds/Items/ExaItems.h"
#include "handlers/Core/Cmds/Items/GodItems.h"
#include "handlers/Core/Cmds/Items/Item.h"
#include "handlers/Core/Cmds/Items/Items.h"
#include "handlers/Core/Cmds/Items/ItemsByType.h"


#include "handlers/Core/Cmds/Moderation/Player/Register.h"
#include "handlers/Core/Cmds/Moderation/Player/Ban.h"
#include "handlers/Core/Cmds/Moderation/Player/Mute.h"
#include "handlers/Core/Cmds/Moderation/Player/RoomAids.h"

#include "handlers/Core/Cmds/Moderation/Room/Break.h"
#include "handlers/Core/Cmds/Moderation/Room/BreakAll.h"
#include "handlers/Core/Cmds/Moderation/Room/Kick.h"

#include "handlers/Core/Cmds/Player/Level.h"

#include "handlers/Core/Cmds/Economy/Cash.h"
#include "handlers/Core/Cmds/Economy/Point.h"
#include "handlers/Core/Cmds/Economy/Energy.h"
#include "handlers/Core/Cmds/Economy/Coupon.h"

#include "handlers/Core/Cmds/Staff/Invis.h"


#include "handlers/Socials/Blockeds/Add.h"
#include "handlers/Socials/Blockeds/Remove.h"
#include "handlers/Socials/Blockeds/View.h"

#include "handlers/Socials/Friends/Add.h"
#include "handlers/Socials/Friends/Remove.h"
#include "handlers/Socials/Friends/View.h"

#include "handlers/Socials/Trade/Trade.h"

#include "handlers/Socials/Clan/View.h"
#include "handlers/Socials/Party/View.h"

#include "handlers/Player/Core/BattlePass.h"
#include "handlers/Player/Core/Authorize.h"
#include "handlers/Player/Core/NameChange.h"
#include "handlers/Player/Core/Ping.h"
#include "handlers/Player/Core/Channels.h"
#include "handlers/Player/Core/InviteJoin.h"
#include "handlers/Player/Core/Automatch.h"
#include "handlers/Player/Core/Chat.h"

#include "handlers/Player/Character/CharacterChange.h"
#include "handlers/Player/Character/CurrencyUpdate.h"
#include "handlers/Player/Character/StateUpdate.h"
#include "handlers/Player/Character/VoiceUpdate.h"

#include "handlers/Player/Milestones/Achievements.h"
#include "handlers/Player/Milestones/Missions.h"

#include "handlers/Player/Gift/Delete.h"
#include "handlers/Player/Gift/Receive.h"
#include "handlers/Player/Gift/Send.h"
#include "handlers/Player/Gift/View.h"

#include "handlers/Player/Mail/Delete.h"
#include "handlers/Player/Mail/Read.h"
#include "handlers/Player/Mail/Send.h"
#include "handlers/Player/Mail/View.h"

#include "handlers/Lobby/Users/View.h"
#include "handlers/Lobby/Users/Profile.h"

#include "handlers/Items/Core/Delete.h"
#include "handlers/Items/Core/Equip.h"
#include "handlers/Items/Core/Repair.h"
#include "handlers/Items/Core/Sell.h"
#include "handlers/Items/Core/Upgrade.h"

#include "handlers/Items/Gamble/Package.h"
#include "handlers/Items/Gamble/Gachapon.h"
#include "handlers/Items/Gamble/GachaponPity.h"

#include "handlers/Items/Shop/Normal.h"
#include "handlers/Items/Shop/Coupon.h"

#include "handlers/Room/Gameplay/Battery.h"
#include "handlers/Room/Gameplay/Pickups.h"

#include "handlers/Room/Match/Rounds/End.h"
#include "handlers/Room/Match/Rounds/Start.h"

#include "handlers/Room/Match/PveRespawn.h"
#include "handlers/Room/Match/Start.h"
#include "handlers/Room/Match/End.h"
#include "handlers/Room/Match/Leave.h"

#include "handlers/Room/Core/Create.h"
#include "handlers/Room/Core/Join.h"
#include "handlers/Room/Core/Leave.h"
#include "handlers/Room/Core/View.h"
#include "handlers/Room/Core/TeamChange.h"

#include "handlers/Room/Moderation/Votekick/Start.h"
#include "handlers/Room/Moderation/Votekick/Agree.h"
#include "handlers/Room/Moderation/Votekick/Verify.h"

#include "handlers/Room/Moderation/Modinfo.h"
#include "handlers/Room/Moderation/ModinfoPassword.h"
#include "handlers/Room/Moderation/ModinfoTitle.h"
#include "handlers/Room/Moderation/ModinfoFull.h"

#include "handlers/Room/Moderation/AllowIntruders.h"
#include "handlers/Room/Moderation/AllowKitdrops.h"
#include "handlers/Room/Moderation/AllowObservers.h"
#include "handlers/Room/Moderation/HostChange.h"
#include "handlers/Room/Moderation/MapRule.h"
#include "handlers/Room/Moderation/PlayersRule.h"
#include "handlers/Room/Moderation/PointsRule.h"
#include "handlers/Room/Moderation/TimeRule.h"

#include "handlers/Plaza/Join.h"
#include "handlers/Plaza/Leave.h"

#include "handlers/Party/Core/Kick.h"
#include "handlers/Party/Core/Create.h"
#include "handlers/Party/Core/Leave.h"
#include "handlers/Party/Core/Join.h"

#include "handlers/Party/Clan/Leave.h"
#include "handlers/Party/Clan/OtherJoin.h"
#include "handlers/Party/Clan/Register.h"
#include "handlers/Party/Clan/View.h"
#include "handlers/Party/Core/Automatch.h"

#include "handlers/Party/Moderation/HostChange.h"
#include "handlers/Party/Moderation/Modinfo.h"
#include "handlers/Party/Moderation/PlayersRule.h"
#include "handlers/Party/Moderation/Password.h"

#include "handlers/Core/AnticheatHeartbeat.h"
#include "handlers/Core/Connect.h"
#include "handlers/Core/Disconnect.h"
#include "handlers/Core/Ipc/CastAuthorize.h"
#include "handlers/Core/Ipc/CastCombatStats.h"
#include "handlers/Core/Ipc/CastMatchEvent.h"
#include "handlers/Core/Ipc/CastMetrics.h"
#include "handlers/Core/Ipc/CastSid.h"
#include "handlers/Core/Ipc/FrontDisconnect.h"
#include "handlers/Core/Ipc/FrontAidOnline.h"
#include "handlers/Core/Ipc.h"






namespace Game
{
    /*
    namespace Commands
    {
        
        static void Help(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            const auto& cmds = Commands::ListCommands(acc_cache->acc_info.Grade);
            for(const auto& cmd : cmds)
                main_server->SendServerMessage(callback.session, cmd.c_str());
        }
        static void Items(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() <= 1)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /item item_id item_id2 (max 25 item ids)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            if (acc_cache->inventory_items.size() + args.size() - 1 > acc_cache->acc_info.MaximumItems)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, you can't spawn {} items because your inventory will be over it's capacity.", acc_cache->acc_info.Nickname.c_str(), args.size() - 1).c_str());
                return;
            }
            DatabaseUpdateCtx dctx{ .sid = callback.session->GetSessionId(), .aid = acc_cache->acc_info.Index};

            std::vector<uint32_t> spawn_ids;
            for (const auto& item_id_str : args)
            {
                if (!Utility::IsDigitsOnly(item_id_str)) continue;
                const auto& item_id = Utility::ExtractNumber(item_id_str.c_str());
                spawn_ids.push_back(item_id);
                
            }
            auto crafted_item = main_server->CraftInventoryItems(acc_cache, std::move(spawn_ids), NetEngine::Items::Origin::From_GM_Spawn);
            if(!crafted_item.has_value())
                DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
            else
                dctx.ops.push_back(crafted_item.value());

            auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
            if (!validated.has_value()) 
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = callback.session, 
                                                                         v = std::move(validated.value())
            ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value()) 
                {
                    DEBUGLOG(red,"ApplyDatabaseUpdates failed for [{}] [{}]: {}",new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(),static_cast<int>(applied.error()));
                    return;
                }

                if (!v.items_added.empty())
                {
                    std::vector<ShopItem> shop_items;
                    for (const auto& item : v.items_added)
                    {
                        auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                        ShopItem new_item = { {item.item_info.item_number.item_id , item_info->Stock} , ItemExpire::Type::Unused,  item.item_info.serial_info };
                        main_server->SendServerMessage(session, std::format("[MegaVolts Online] spawned ({}) item", item.item_info.item_number.item_id).c_str());
                        shop_items.push_back(new_item);
                    }

                    constexpr std::uint32_t max_packet_size = 1440;
                    constexpr std::uint32_t full_header_size = 8;
                    constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(ShopItem);
                    uint32_t total_fragments = (shop_items.size() + 1) <= split_size ? 1 : ((shop_items.size() + 1) / split_size) + 1;
                    if (!shop_items.empty())
                    {
                        for (auto i = 0; i < total_fragments; i++)
                        {
                            std::vector<ShopItem> items_batch;
                            const uint32_t start_index = i * split_size;
                            const uint32_t end_index = std::min(start_index + split_size, static_cast<uint32_t>(shop_items.size()));
                            for (auto i = start_index; i < end_index; i++)
                                items_batch.push_back(shop_items[i]);

                            if (!items_batch.empty())
                                session->SendMsg(99, 0, 37, static_cast<uint8_t>(items_batch.size()), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(ShopItem));
                        }
                    }   
                }
            });
        }
        static void AllItems(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() <= 1)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /allitems item_type", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            DatabaseUpdateCtx dctx{ .sid = callback.session->GetSessionId(), .aid = acc_cache->acc_info.Index};

            
			auto item_type = Utility::ExtractNumber(args[1].c_str());
			auto my_character = static_cast<Character::Type>(acc_cache->acc_info.SelectedCharacter);
            using enum Character::Type;
			auto ids = CItemsType.get<shared_t>(item_type);
            std::vector<uint32_t> spawn_ids;
            for (const auto& id : *ids)
            {
                auto item_info = CItemsInfo.get<shared_t>(id);
                if (!item_info->Id) continue;
				if (item_info->Type != item_type) continue;
				if (item_info->LimitedTime) continue;
                if (my_character == Naomi && !item_info->IsNaomiUsable) continue;
				if (my_character == Kai && !item_info->IsKaiUsable) continue;
				if (my_character == Pandora && !item_info->IsPandoraUsable) continue;
				if (my_character == CHIP && !item_info->IsChipUsable) continue;
				if (my_character == Knox && !item_info->IsKnoxUsable) continue;
                if (item_info->IsUpgradable)
                {
					auto is_weapon = item_info->Type == Items::WeaponItems::Type::Melee ||
                        item_info->Type == Items::WeaponItems::Type::Rifle ||
                        item_info->Type == Items::WeaponItems::Type::Shotgun ||
                        item_info->Type == Items::WeaponItems::Type::Sniper ||
                        item_info->Type == Items::WeaponItems::Type::Gatling ||
                        item_info->Type == Items::WeaponItems::Type::Bazooka ||
                        item_info->Type == Items::WeaponItems::Type::Grenade;

                    if (is_weapon)
                    {
                        auto upgrade_level = id % 10;
                        if (upgrade_level != 9) continue;
                        spawn_ids.push_back(id);
                    }
                    else
                        spawn_ids.push_back(id);
                }
                else
                    spawn_ids.push_back(id);
            }

            auto crafted_item = main_server->CraftInventoryItems(acc_cache, std::move(spawn_ids), NetEngine::Items::Origin::From_GM_Spawn);
            if(!crafted_item.has_value())
                DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
            else
                dctx.ops.push_back(crafted_item.value());

            auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx, true); // bypass inv limit check
            if (!validated.has_value()) 
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), 
                                                                         v = std::move(validated.value())
            ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value()) 
                {
                    DEBUGLOG(red,"ApplyDatabaseUpdates failed for [{}] [{}]: {}",new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(),static_cast<int>(applied.error()));
                    return;
                }

                if (!v.items_added.empty())
                {
                    std::vector<ShopItem> shop_items;
                    for (const auto& item : v.items_added)
                    {
                        auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                        ShopItem new_item = { {item.item_info.item_number.item_id , item_info->Stock} , ItemExpire::Type::Unused,  item.item_info.serial_info };
                        
                        shop_items.push_back(new_item);
                    }

                    constexpr std::uint32_t max_packet_size = 1440;
                    constexpr std::uint32_t full_header_size = 8;
                    constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(ShopItem);
                    uint32_t total_fragments = (shop_items.size() + 1) <= split_size ? 1 : ((shop_items.size() + 1) / split_size) + 1;
                    if (!shop_items.empty())
                    {
                        for (auto i = 0; i < total_fragments; i++)
                        {
                            std::vector<ShopItem> items_batch;
                            const uint32_t start_index = i * split_size;
                            const uint32_t end_index = std::min(start_index + split_size, static_cast<uint32_t>(shop_items.size()));
                            for (auto i = start_index; i < end_index; i++)
                                items_batch.push_back(shop_items[i]);

                            if (!items_batch.empty())
                                session->SendMsg(99, 0, 37, static_cast<uint8_t>(items_batch.size()), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(ShopItem));
                        }
                        main_server->SendServerMessage(session, std::format("[MegaVolts Online] spawned ({}) item(s)", shop_items.size()));
                    }   
                }
            });
        }
        inline constexpr std::array<uint32_t, 126> kExaVoltsWeapons = {
            3010059,3010069,3010079,3011659,3011669,3011679,
            3010259,3010269,3010279,3011759,3011769,3011779,
            3010359,3010369,3010379,3011859,3011869,3011879,

            3020159,3020169,3020179,3026159,3026169,3026179,
            3020259,3020269,3020279,3021959,3021969,3021979,
            3020359,3020369,3020379,3022059,3022069,3022079,

            3030159,3030169,3030179,3031159,3031169,3031179,
            3030259,3030269,3030279,3031259,3031269,3031279,
            3030359,3030369,3030379,3031359,3031369,3031379,

            3040159,3040169,3040179,3041059,3041069,3041079,
            3040259,3040269,3040279,3041159,3041169,3041179,
            3040359,3040369,3040379,3041259,3041269,3041279,

            3050159,3050169,3050179,3050859,3050869,3050879,
            3050259,3050269,3050279,3050959,3050969,3050979,
            3050359,3050369,3050379,3051059,3051069,3051079,

            3060159,3060169,3060179,3061059,3061069,3061079,
            3060259,3060269,3060279,3061159,3061169,3061179,
            3060359,3060369,3060379,3061259,3061269,3061279,

            3070159,3070169,3070179,3071459,3071469,3071479,
            3070259,3070269,3070279,3071559,3071569,3071579,
            3070359,3070369,3070379,3071659,3071669,3071679
        };
        static void ExavoltsItems(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            DatabaseUpdateCtx dctx{ .sid = callback.session->GetSessionId(), .aid = acc_cache->acc_info.Index };
            std::vector<uint32_t> spawn_ids(std::from_range, kExaVoltsWeapons);

            auto crafted_item = main_server->CraftInventoryItems(acc_cache, std::move(spawn_ids), NetEngine::Items::Origin::From_GM_Spawn);
            if (!crafted_item.has_value())
                DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
            else
                dctx.ops.push_back(crafted_item.value());

            auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx, true); // bypass inv limit check
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session),
                v = std::move(validated.value())
            ]() mutable
                {
                    if (!session) return;
                    ResultDbUpdateInfo dbres;
                    if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                    auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                    auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                    if (!applied.has_value())
                    {
                        DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                        return;
                    }

                    if (!v.items_added.empty())
                    {
                        std::vector<ShopItem> shop_items;
                        for (const auto& item : v.items_added)
                        {
                            auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                            ShopItem new_item = { {item.item_info.item_number.item_id , item_info->Stock} , ItemExpire::Type::Unused,  item.item_info.serial_info };
                            main_server->SendServerMessage(session, std::format("[MegaVolts Online] spawned ({}) item", item.item_info.item_number.item_id).c_str());
                            shop_items.push_back(new_item);
                        }

                        constexpr std::uint32_t max_packet_size = 1440;
                        constexpr std::uint32_t full_header_size = 8;
                        constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(ShopItem);
                        uint32_t total_fragments = (shop_items.size() + 1) <= split_size ? 1 : ((shop_items.size() + 1) / split_size) + 1;
                        if (!shop_items.empty())
                        {
                            for (auto i = 0; i < total_fragments; i++)
                            {
                                std::vector<ShopItem> items_batch;
                                const uint32_t start_index = i * split_size;
                                const uint32_t end_index = std::min(start_index + split_size, static_cast<uint32_t>(shop_items.size()));
                                for (auto i = start_index; i < end_index; i++)
                                    items_batch.push_back(shop_items[i]);

                                if (!items_batch.empty())
                                    session->SendMsg(99, 0, 37, static_cast<uint8_t>(items_batch.size()), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(ShopItem));
                            }
                        }
                    }
                });
        }

        inline constexpr std::array<uint32_t, 7> kGodWeapons = { 4626750,4626850,4626950,4627050,4627150,4627250,4627350 };
        static void GodsItems(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            DatabaseUpdateCtx dctx{ .sid = callback.session->GetSessionId(), .aid = acc_cache->acc_info.Index };
            std::vector<uint32_t> spawn_ids(std::from_range, kGodWeapons);

            auto crafted_item = main_server->CraftInventoryItems(acc_cache, std::move(spawn_ids), NetEngine::Items::Origin::From_GM_Spawn);
            if (!crafted_item.has_value())
                DEBUGLOG(red, "CraftInventoryItems failed for player [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(crafted_item.error()));
            else
                dctx.ops.push_back(crafted_item.value());

            auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx, true); // bypass inv limit check
            if (!validated.has_value())
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session),
                v = std::move(validated.value())
            ]() mutable
                {
                    if (!session) return;
                    ResultDbUpdateInfo dbres;
                    if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                    auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                    auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                    if (!applied.has_value())
                    {
                        DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                        return;
                    }

                    if (!v.items_added.empty())
                    {
                        std::vector<ShopItem> shop_items;
                        for (const auto& item : v.items_added)
                        {
                            auto item_info = CItemsInfo.get<shared_t>(item.item_info.item_number.item_id);
                            ShopItem new_item = { {item.item_info.item_number.item_id , item_info->Stock} , ItemExpire::Type::Unused,  item.item_info.serial_info };
                            main_server->SendServerMessage(session, fmt::format("[MegaVolts Online] spawned ({}) item", item.item_info.item_number.item_id).c_str());
                            shop_items.push_back(new_item);
                        }

                        constexpr std::uint32_t max_packet_size = 1440;
                        constexpr std::uint32_t full_header_size = 8;
                        constexpr std::uint32_t split_size = (max_packet_size - full_header_size) / sizeof(ShopItem);
                        uint32_t total_fragments = (shop_items.size() + 1) <= split_size ? 1 : ((shop_items.size() + 1) / split_size) + 1;
                        if (!shop_items.empty())
                        {
                            for (auto i = 0; i < total_fragments; i++)
                            {
                                std::vector<ShopItem> items_batch;
                                const uint32_t start_index = i * split_size;
                                const uint32_t end_index = std::min(start_index + split_size, static_cast<uint32_t>(shop_items.size()));
                                for (auto i = start_index; i < end_index; i++)
                                    items_batch.push_back(shop_items[i]);

                                if (!items_batch.empty())
                                    session->SendMsg(99, 0, 37, static_cast<uint8_t>(items_batch.size()), reinterpret_cast<uint8_t*>(items_batch.data()), items_batch.size() * sizeof(ShopItem));
                            }
                        }
                    }
                });
        }

        static void ClearInv(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {

            DatabaseUpdateCtx dctx{ .sid = callback.session->GetSessionId(), .aid = acc_cache->acc_info.Index};
			std::vector<ItemSerialInfo> items_to_delete;
            for (const auto& item : acc_cache->inventory_items)
            {
                items_to_delete.push_back(item.item_info.serial_info);
                dctx.ops.emplace_back(ItemDeleteCtx{ .serials = {item.item_info.serial_info} });
            }
                

            main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] gonna delete ({}) item(s) from ({})", items_to_delete.size(), acc_cache->acc_info.Nickname.c_str()).c_str());

            auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx); // bypass inv limit check
            if (!validated.has_value()) 
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, 
                                                                         session = callback.session, 
																		 items_to_delete = std::move(items_to_delete),
                                                                         v = std::move(validated.value())
            ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value()) 
                {
                    DEBUGLOG(red,"ApplyDatabaseUpdates failed for [{}] [{}]: {}",new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(),static_cast<int>(applied.error()));
                    return;
                }
                for (auto& item : items_to_delete)
                {
                    auto deleteItemData = MainDeleteItemAck({ item }).Serialize();
                    session->SendMsg(89, 0, 1, 0, reinterpret_cast<uint8_t*>(deleteItemData.data()), deleteItemData.size());
                }
                main_server->SendServerMessage(session, std::format("[MegaVolts Online] deleted ({}) item(s) from ({})", items_to_delete.size(), new_acc_cache->acc_info.Nickname.c_str()).c_str());
            });
        }

        static void Info(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (acc_cache->in_party && CParty.contains(acc_cache->party_id))
            {
				auto party = CParty.get<shared_t>(acc_cache->party_id);
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] Party Info: is_registered: ({}) is_queueing: ({}), has_password: ({}), password: ({}), is_clan: ({}), clan_id: ({}), is_playing: ({}), max_members: ({}), members.size(): ({})", party->is_registered, party->is_queueing, party->has_password, party->password.c_str(), party->is_clan, party->clan_id, party->is_playing, party->max_members, party->members.size()).c_str());
            }
            if (acc_cache->in_room && CRoom.contains(acc_cache->room_id))
            {
                auto room = CRoom.get<shared_t>(acc_cache->room_id);
                acc_cache.unlock();
                auto ids = main_server->GetRoomSortedPlayerSessionIds(room);

                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] Rooms Info: {} players, mode: {}", ids.size(), static_cast<uint8_t>(room->ModeIndex)).c_str());
                if (room->has_password)
                    main_server->SendServerMessage(callback.session, std::format("RoomId: {} - Title: {} - Password: {}", room->room_id, room->title.c_str(), room->password.c_str()));
                else
                    main_server->SendServerMessage(callback.session, std::format("RoomId: {} - Title: {}", room->room_id, room->title.c_str()).c_str());

                for (const auto& id : ids)
                {
                    auto player = CAccount.get<shared_t>(id);
					std::string msg = "";
                    if (player->acc_info.Index)
                    {
                        const auto& is_playing = player->playing ? "Yes" : "No";
                        const auto& state = player->state;
                        msg = fmt::format("({}) SessionID: {} - Grade: {}, Slot: {}, Playing: {}, State: {}",
                            player->acc_info.Nickname.c_str(), player->session_id, player->acc_info.Grade, player->slot_id, is_playing, state);
                        if (room->host_session_id == id)
                            msg = "(HOST) " + msg;
                    }
                    else
						msg = fmt::format("Unknown Cache Player SessionID: {}", id);

                    player.unlock();
					main_server->SendServerMessage(callback.session, msg.c_str());
                }
            }
            else
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] Your Info").c_str());
                main_server->SendServerMessage(callback.session, std::format("({}) SessionID: {} - Grade: {}", acc_cache->acc_info.Nickname.c_str(), acc_cache->session_id, acc_cache->acc_info.Grade).c_str());
            }
        }
        static void Online(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            acc_cache.unlock();
            CServer* server = callback.server;
            auto sessions_list = server->GetSessions();

            
            auto sidsOnline = CSid.get_all(shared);
            main_server->SendServerMessage(callback.session, fmt::format("[MegaVolts Online] Players Online: {}, Sessions Size: {}", sidsOnline->size(), sessions_list->size()).c_str());
            for (const auto& sid : *sidsOnline)
            {
				auto acc = CAccount.get<shared_t>(sid);
                const auto& is_playing = acc->playing ? "Yes" : "No";
                const auto& in_room = acc->in_room;
                const auto& state = acc->state;
                std::string msg = "";
				msg = fmt::format("({}) SessionID: {} - Grade: {}, Slot: {}, Playing: {}, State: {}, Ping: {}", 
                    acc->acc_info.Nickname.c_str(), sid, acc->acc_info.Grade, acc->slot_id, is_playing, acc->state, acc->ping);
				if (acc->in_room) msg += fmt::format(", roomId={}", acc->room_id);

				acc.unlock();
				main_server->SendServerMessage(callback.session, msg.c_str());
            }

            for (auto& sid : *sessions_list)
                main_server->SendServerMessage(callback.session, std::format("sid online: {}", sid.first).c_str());
        }
        static void Rooms(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
			auto roomIds = CRoomId.get_all(shared);
            for (const auto& id : *roomIds)
            {
				auto room = CRoom.get<shared_t>(id);
                std::string msg = "";
				const auto& neutral_size = room->neutralteam_session_ids.size();
				const auto& red_size = room->redteam_session_ids.size();
				const auto& blue_size = room->blueteam_session_ids.size();
                const auto& obs_size = room->observers_session_ids.size();
                if (room->has_password)
                    msg = fmt::format("({}) - Title: {} - Password: {} - plr count N: ({}), R: ({}), B: ({}), O: ({})", room->room_id, room->title.c_str(), room->password.c_str(), neutral_size, red_size, blue_size, obs_size);
                else
					msg = fmt::format("({}) - Title: {} - plr count N: ({}), R: ({}), B: ({}), O: ({})", room->room_id, room->title.c_str(), neutral_size, red_size, blue_size, obs_size);

				main_server->SendServerMessage(callback.session, msg.c_str());
            }
        }
        static void Disconnect(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {

            if (args.size() != 3)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /disc nickname id (0-255)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            auto disconnect_type = std::stoi(args[2].c_str());
            if (disconnect_type > 255 || disconnect_type < 0)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, disconnect id should be between (0-255)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            acc_cache.unlock();
            const auto& nickname = args[1];

            auto player = CAccount.get_by_filter<shared_t>([&](const auto&, auto& player) {
                return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(nickname);
                });

            auto player_session_id = player->session_id;
            auto player_auth_key = player->acc_info.AuthKey;
            player.unlock();
            main_server->DisconnectPlayer(player_session_id, disconnect_type);
            main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, disconnected auth key {}", acc_cache->acc_info.Nickname.c_str(), player_auth_key).c_str());

        }
        static void Announce(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            acc_cache.unlock();
            CServer* server = callback.server;
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /! msg (512 max chars)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            if (args[1].size() > 512 || args[1].size() < 1)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /! msg (512 max chars)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            auto msg = std::move(args[1]);
            auto sidsOnline = CSid.get_all(shared);
            for (const auto& sid : *sidsOnline)
                if (auto pss = main_server->GetSessionById(sid))
                    pss->SendMsg(402, 0, 10, 0, reinterpret_cast<uint8_t*>(msg.data()), msg.size());

        }
        static void Level(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /level new_level (0-100)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            if (!Utility::IsDigitsOnly(args[1]))
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /level new_level (0-100)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            auto lvl = std::stoi(args[1].c_str());

            DatabaseUpdateCtx dctx{ .sid = callback.session->GetSessionId(), .aid = acc_cache->acc_info.Index};

            auto gi = CGradesInfo.get<shared_t>(lvl + 2);
            if (gi->Grade)
            {
				dctx.ops.emplace_back(AccountInfoPatch{ .experience = gi->Exp, .level = lvl });
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, trying to change level to {}", acc_cache->acc_info.Nickname.c_str(), lvl).c_str());
            }
            else
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, invalid level", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }

            auto validated = main_server->ValidateDatabaseUpdates(acc_cache, dctx);
            if (!validated.has_value()) 
            {
                DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc_cache->acc_info.Index, acc_cache->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
                return;
            }

            acc_cache.unlock();

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session), 
                                                                         v = std::move(validated.value())
            ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc_cache = CAccount.get<unique_t>(session->GetSessionId());
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value()) 
                {
                    DEBUGLOG(red,"ApplyDatabaseUpdates failed for [{}] [{}]: {}",new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(),static_cast<int>(applied.error()));
                    return;
                }
            });
           
        }
        static void Kick(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /kick nickname (0-255)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            const auto& nickname = args[1];
            if (acc_cache->acc_info.Nickname == nickname)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, you can't kick yourself", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }

            acc_cache.unlock();
            

            auto player = CAccount.get_by_filter<shared_t>([&](const auto& , auto& player) {
                return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(nickname);
                });
            auto sid = player->session_id;
            auto roomId = player->room_id;
            auto teamId = player->team_id;
            if(!player->acc_info.Index) return;
            if (!player->in_room || !CRoom.contains(roomId))
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] player {} is not in any room.", player->acc_info.Nickname.c_str()).c_str());
                return;
            }
            player.unlock();

            auto room = CRoom.get<unique_t>(roomId);
            main_server->NewRemoveRoomPlayer(room, sid, teamId, NetEngine::Room::Leave::Ack::Result::KickedByGm, true);
        }

        static void Break(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            auto myself_in_room = acc_cache->in_room;
            auto myself_room_id = acc_cache->room_id;
           
            if(args.size() != 1 && args.size() != 2)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /break or /break room_id", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            uint16_t target_room_id = 0;

            if (args.size() == 1) 
            {
                if (!myself_in_room)
                {
                    main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, you are not in a room. Use /break room_id instead.", acc_cache->acc_info.Nickname.c_str()).c_str());
                    return;
                }
                target_room_id = myself_room_id;
            }
            else if (args.size() == 2) 
            {
                try
                {
                    target_room_id = static_cast<uint16_t>(std::stoi(args[1]));
                }
                catch (const std::exception&)
                {
                    main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, invalid room ID. Please provide a valid room ID.", acc_cache->acc_info.Nickname.c_str()).c_str());
                    return;
                }
            }

            if (!CRoom.contains(target_room_id))
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, the specified room ID does not exist.", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }

            main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, room ID {} has been successfully broken.", acc_cache->acc_info.Nickname.c_str(), target_room_id).c_str());
            acc_cache.unlock();

            

            auto room = CRoom.get<shared_t>(target_room_id);
            auto ids = main_server->GetRoomSortedPlayerSessionIds(room);
            for (const auto& id : ids)
            {
                auto acc = CAccount.get<unique_t>(id);
                if (!acc->acc_info.Index|| !acc->in_room || acc->room_id != room->room_id)
                {
                    acc.unlock();
                    continue;
                }
                else
                {
                    acc->in_room = false;
                    acc->slot_id = 0;
                    acc->playing = false;
                    acc->state = PlayerInfo::State::Waiting;
                    acc.unlock();

                    if (auto pss = main_server->GetSessionById(id))
                    {
                        pss->SendMsg(407, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // show gm break popup
                        pss->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // Leave room ack
                       
                    }  
                }
            }
            room.unlock();
			CRoom.erase(target_room_id);
			CRoomId.erase_value(target_room_id);
            main_server->SetRoomIdAvailable(target_room_id);
        }
        static void BreakAll(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {

            if (args.size() != 1)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /breakall", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, successfully broke all rooms.", acc_cache->acc_info.Nickname.c_str()).c_str());
            acc_cache.unlock();

            auto room_ids = CRoomId.get_all(shared);
            for (auto& room_id : *room_ids)
            {
                auto room = CRoom.get<shared_t>(room_id);
                auto ids = main_server->GetRoomSortedPlayerSessionIds(room);
                for (const auto& id : ids)
                {
                    auto acc = CAccount.get<unique_t>(id);
                    if (!acc->acc_info.Index || !acc->in_room || acc->room_id != room->room_id)
                    {
                        acc.unlock();
                        continue;
                    }
                    else
                    {
                        acc->in_room = false;
                        acc->slot_id = 0;
                        acc->playing = false;
                        acc->state = PlayerInfo::State::Waiting;
                        acc.unlock();

                        if (auto pss = main_server->GetSessionById(id))
                        {
                            pss->SendMsg(407, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // show gm break popup
                            pss->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // Leave room ack

                        }
                    }
                }
                room.unlock();
                CRoom.erase(room_id);
                CRoomId.erase_value(room_id);
                main_server->SetRoomIdAvailable(room_id);
            }
        }
        static void CreateClan(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /createclan name (15 chars max)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }

            if (!Utility::IsDigitsOnly(args[1]))
            {
                main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {}, command usage: /createclan name (15 chars max)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
        }
        static void ReloadGachaponSalesInfo(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            CGachaponSale.clear();
            CGachaponSaleInfo.clear();
            auto gachapon_sales = BaseLib::Database->GetGachaponSalesInfo();
            for (auto& sale : gachapon_sales)
            {
                if(CGachaponSaleInfo.contains(sale.gachapon_id))
					continue;

				CGachaponSaleInfo.insert(sale.gachapon_id, sale);
                CGachaponSale.emplace_back(sale.gachapon_id);
            }
            main_server->SendServerMessage(callback.session, std::format("[MegaVolts Online] {} gachapon sales info reloaded", gachapon_sales.size()).c_str());
        }
        static void CastProcessInfo(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            main_server->SendCastIpc(PacketIds::Ipc::MainToCastReqServerInfo, Utility::ToVector(acc_cache->acc_info.AuthKey));
        }
        static void MainProcessInfo(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
#ifdef _WIN32
            HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
            auto cpu_usage = Utility::GetCpuUsage(m_process_handle);
            auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(m_process_handle));
            CloseHandle(m_process_handle);
#else
            auto cpu_usage = Utility::GetCpuUsage(nullptr);
            auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(nullptr));
#endif
            auto sessions_count = static_cast<uint16_t>(main_server->GetSessions()->size());

            auto msg = std::format("[MegaVolts Online] Main Info: Sessions Online: {}, Memory Usage: {} MB, Cpu Usage: {:.2f}%",
                static_cast<uint16_t>(sessions_count),
                static_cast<uint32_t>(mem_usage),
                static_cast<double>(cpu_usage));

            main_server->SendServerMessage(callback.session, msg.c_str());
        }
        static void ShutdownPrepare(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            auto my_session_id = acc_cache->session_id;
            acc_cache.unlock();
            auto session_ids = main_server->GetSessions();
            std::vector<uint16_t> my_session_ids;
            for (auto& [id, session] : *session_ids)
                my_session_ids.push_back(id);
            session_ids.unlock();
            uint32_t kicked_cnt = 0;
            for (auto id : my_session_ids)
            {
                if (id == my_session_id) continue;
                DEBUGLOG(dark_cyan, "force command disconnect, sid=({})", id);
                main_server->DisconnectPlayer(id, Disconnect::Reason::Deny);
                kicked_cnt++;
            }
            
            auto msg = std::format("success. all {} player was kick for prepare maintanance.", kicked_cnt);

            main_server->SendServerMessage(callback.session, msg.c_str());
        }
        static void Init()
        {
            Commands::Register("shutdown_prepare", ShutdownPrepare, Userlist::User::Grade::GameMaster);
            Commands::Register("?", Help, Userlist::User::Grade::Tester);
            Commands::Register("!", Announce, Userlist::User::Grade::GameMaster);
            Commands::Register("item", Items, Userlist::User::Grade::Tester);
            Commands::Register("allitems", AllItems, Userlist::User::Grade::Tester);
            Commands::Register("exa", ExavoltsItems, Userlist::User::Grade::Tester);
            Commands::Register("gods", GodsItems, Userlist::User::Grade::Tester);
            Commands::Register("clearinv", ClearInv, Userlist::User::Grade::Tester);
            Commands::Register("info", Info, Userlist::User::Grade::Tester);
            Commands::Register("online", Online, Userlist::User::Grade::Tester);
            Commands::Register("rooms", Rooms, Userlist::User::Grade::Tester);
            Commands::Register("level", Level, Userlist::User::Grade::GameMaster);
            Commands::Register("kick", Kick, Userlist::User::Grade::GameMaster);
            Commands::Register("break", Break, Userlist::User::Grade::GameMaster);
            Commands::Register("breakall", BreakAll, Userlist::User::Grade::GameMaster);
            Commands::Register("reloadgachasale", ReloadGachaponSalesInfo, Userlist::User::Grade::GameMaster);
            Commands::Register("cast", CastProcessInfo, Userlist::User::Grade::Tester);
            Commands::Register("main", MainProcessInfo, Userlist::User::Grade::Tester);
        }
    }
   */
    /*
    std::shared_mutex items_ids_mutex;
    std::shared_mutex items_info_mutex;
    std::shared_mutex effect_info_mutex;
    std::shared_mutex collection_info_mutex;
    std::shared_mutex dailymission_info_mutex;
    std::shared_mutex setitems_info_mutex;
    std::shared_mutex vendors_info_mutex;
    std::shared_mutex upgrades_info_mutex;
    std::shared_mutex gachapons_info_mutex;
    std::shared_mutex packages_info_mutex;
    std::shared_mutex vendor_item_ids_mutex;
    std::shared_mutex dailymission_ids_mutex;
    std::shared_mutex roomoptionsinfo_cache_mutex;
    std::shared_mutex grades_info_mutex;
    std::shared_mutex rewards_info_mutex;
    std::shared_mutex friends_cache_mutex;
    std::shared_mutex blockeds_cache_mutex;
	std::shared_mutex socials_cache_mutex;
    std::shared_mutex accounts_cache_mutex;
    std::shared_mutex accounts_idx_cache_mutex;
	std::shared_mutex accounts_authkey_cache_mutex;
    std::shared_mutex online_sessions_cache_mutex;
    std::shared_mutex rooms_cache_mutex;
    std::shared_mutex plaza_cache_mutex;
    std::shared_mutex room_ids_mutex;
    std::shared_mutex party_ids_mutex;
    std::shared_mutex clan_cache_mutex;
    std::shared_mutex party_cache_mutex;
    std::shared_mutex mailbox_data_cache_mutex;
    std::shared_mutex mailbox_sent_cache_mutex;
    std::shared_mutex mailbox_recv_cache_mutex;
    std::shared_mutex giftbox_sent_cache_mutex;
    std::shared_mutex giftbox_recv_cache_mutex;
    std::shared_mutex gachapon_sale_cache_mutex;
    std::shared_mutex gachapon_ids_sale_cache_mutex;
    

    
    

    
	std::vector<uint32_t> items_ids; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::ItemInfo> items_info; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::EffectInfo> effect_info; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::CollectionInfo> collection_info; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::DailyMissionInfo> dailymission_info; //read only
    std::vector<uint32_t> dailymission_ids; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::SetItemInfo> setitems_info; //read only
    std::vector<BaseLib::VendorInfo> vendors_info; //read only
    boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<Items::Upgrade::Type, std::vector<BaseLib::UpgradeInfo>>> upgrades_info; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::GachaponInfo> gachapons_info; //read only
    boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::PackageInfo>>> packages_info; //read only
    std::vector<uint32_t> vendor_item_ids; //read only
    CCache<boost::unordered_flat_set<uint32_t>> CVendorItems;

    boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::RoomOptionInfo>>> roomoptionsinfo_cache; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::GradeInfo> grades_info; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::RewardInfo> rewards_info; //read only
    boost::unordered_flat_map<uint32_t, std::vector<BaseLib::FriendInfo>> friends_cache; //read & write
    boost::unordered_flat_map<uint32_t, std::vector<BaseLib::BlockedInfo>> blockeds_cache; //read & write
	boost::unordered_flat_map<uint16_t, std::vector<BaseLib::SocialInfo>> socials_cache; //read & write
    boost::unordered_flat_map<uint32_t, Player> accounts_cache; //read & write
	boost::unordered_flat_map<uint32_t, uint32_t> accounts_idx_cache; //read & write, key: acc index, value: sid
	boost::unordered_flat_map<uint64_t, uint32_t> accounts_authkey_cache; //read & write, key: auth key, value: sid
    boost::unordered_flat_map<uint32_t, Room> rooms_cache; //read & write
    boost::unordered_flat_map<uint32_t, Plaza> plaza_cache; //read & write
    std::vector<uint32_t> room_ids; //read & write 
    std::vector<uint32_t> party_ids; //read & write
    boost::unordered_flat_map<uint32_t, Clan> clan_cache; //read & write
    boost::unordered_flat_map<uint16_t, Party> party_cache; //read & write
    boost::unordered_flat_map<uint32_t, MailboxData> mailbox_data_cache; //read & write access by mail id
    boost::unordered_flat_map<uint32_t, std::vector<uint32_t>> mailbox_sent_cache; //read & write access by acc id, get vector of mail sent mail ids
    boost::unordered_flat_map<uint32_t, std::vector<uint32_t>> mailbox_recv_cache; //read & write access by acc id, get vector of mail recv mail ids
    boost::unordered_flat_map<uint32_t, std::vector<uint32_t>> giftbox_sent_cache; //read & write access by acc id, get vector of gift sent mail ids
    boost::unordered_flat_map<uint32_t, std::vector<uint32_t>> giftbox_recv_cache; //read & write access by acc id, get vector of gift recv mail ids
    boost::unordered_flat_map<uint32_t, BaseLib::GachaponSaleInfo> gachapon_sales_info;
    std::vector<uint32_t> gachapon_ids_sale;
	std::vector<uint16_t> online_sessions; //read & write
    */
    
    CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CItemsType;
    CCache<boost::unordered_flat_set<uint16_t>> g_tp_to_proj_sids;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::ItemInfo>> CItemsInfo;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::SetItemInfo>> CSetItemsInfo;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::EffectInfo>> CEffectInfo;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::BaseUnitInfo>> CBaseUnitInfo;
    CCache<boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<Items::Upgrade::Type, std::vector<BaseLib::UpgradeInfo>>>> CUpgradesInfo;

    CCache<boost::unordered_flat_map<uint32_t, BaseLib::CollectionInfo>> CCollectionInfo;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::DailyMissionInfo>> CDailyMissionInfo;

    CCache<boost::unordered_flat_map<uint32_t, BaseLib::GachaponInfo>> CGachaponsInfo;
    CCache<boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::PackageInfo>>>> CPackagesInfo;
    CCache<boost::unordered_flat_set<uint32_t>> CVendorItems;

    CCache<std::vector<uint32_t>> CDailyMissions;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::MapInfo>> CMapsInfo;
    CCache<boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::RoomOptionInfo>>>> CRoomOptionsInfo;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::GradeInfo>> CGradesInfo;
    CCache<boost::unordered_flat_map<uint32_t, BaseLib::RewardInfo>> CRewardsInfo;
    CCache<boost::unordered_flat_map<uint16_t, std::vector<BaseLib::SocialInfo>>> CSocial;

    CCache<boost::unordered_flat_map<uint16_t, Player>> CAccount;
    CCache<boost::unordered_flat_map<uint32_t, uint16_t>> CAidSid;
    CCache<boost::unordered_flat_map<uint64_t, uint16_t>> CAuthKey;
    CCache<std::vector<uint16_t>> CSid;

    CCache<boost::unordered_flat_map<uint32_t, Room>> CRoom;
    CCache<boost::unordered_flat_map<uint32_t, Plaza>> CPlaza;
    CCache<boost::unordered_flat_map<uint32_t, Clan>> CClan;
    CCache<boost::unordered_flat_map<uint16_t, Party>> CParty;
    CCache<std::vector<uint32_t>> CRoomId;
    CCache<std::vector<uint32_t>> CPartyId;

    CCache<boost::unordered_flat_map<uint32_t, MailboxData>> CMailboxData;
    CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CMailSent;
    CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CMailRecv;
    CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CGiftSent;
    CCache<boost::unordered_flat_map<uint32_t, std::vector<uint32_t>>> CGiftRecv;

    CCache<boost::unordered_flat_map<uint32_t, BaseLib::GachaponSaleInfo>> CGachaponSaleInfo;
    CCache<std::vector<uint32_t>> CGachaponSale;

    NetEngine::RateLimit::IdentitySnapshot CMainServer::BuildPacketRateLimitIdentitySnapshot(const SCallbackData& callback)
    {
        NetEngine::RateLimit::IdentitySnapshot snapshot{};
        if (!callback.session)
            return snapshot;

        snapshot.sid = callback.session->GetSessionId();
        snapshot.ip = callback.session->GetIpAddress();

        auto acc = CAccount.get<shared_t>(snapshot.sid);
        if (!acc)
            return snapshot;

        snapshot.aid = acc->acc_info.Index;
        snapshot.hwid = acc->hwid;
        return snapshot;
    }

    void CMainServer::SendSessionAnnouncement(CSession* session, const std::string_view message)
    {
        if (!session || message.empty())
            return;

        const auto capped_size = static_cast<size_t>(std::min<uint32_t>(static_cast<uint32_t>(message.size()), 255u));
        std::string payload(message.substr(0, capped_size));
        session->SendMsg(402, 0, 10, 0, reinterpret_cast<uint8_t*>(payload.data()), static_cast<uint16_t>(payload.size()));
    }

    CMainServer::CMainServer()
    {
        //Commands::Init();
        using namespace Game::Handlers;
        using enum EOrder;

        this->OnNewSession(std::bind(&ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&ServerIpc, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));

        this->BindMainHandler<&BlockedsAdd>(BLOCKED_ADD);//block add
        this->BindMainHandler<&BlockedsRemove>(BLOCKED_DELETE);//block remove
        this->BindMainHandler<&BlockedsView>(BLOCKED_LIST);//block list
        
        this->BindMainHandler<&ClanView>(CLAN_LIST);//clan list
        this->BindMainHandler<&PartyView>(PARTY_LIST);//new party clan implement

        this->BindMainHandler<&Achievements>(INFO_COLLECTION);//achievement achivement mission completion

        this->BindMainHandler<&FriendsAdd>(FRIENDS_ADD);//friend add
        this->BindMainHandler<&FriendsRemove>(FRIENDS_DELETE);//friend remove
        this->BindMainHandler<&FriendsView>(FRIENDS_LIST);//friend list

        // TRADE DISABLED (matches client) — to be fixed later. Unregistered so no trade
        // order (190-199) is handled and the irreversible item-swap can't be triggered.
        // Re-enable together with the client trade handler.
        // this->BindMainHandler<&TradeRequest>(TRADE_REQUEST);//trade invite by name
        // this->BindMainHandler<&TradeAccept>(TRADE_ACCEPT);//trade accept invite
        // this->BindMainHandler<&TradeItemChange>(TRADE_ITEM_ADD);//trade offer item
        // this->BindMainHandler<&TradeItemChange>(TRADE_ITEM_REMOVE);//trade withdraw item
        // this->BindMainHandler<&TradeLock>(TRADE_LOCK);//trade lock offer
        // this->BindMainHandler<&TradeConfirm>(TRADE_CONFIRM);//trade confirm
        // this->BindMainHandler<&TradeCancel>(TRADE_CANCEL);//trade cancel/close

        this->BindMainHandler<&GiftSend>(GIFT_SEND);//PlayerSendGiftbox
        this->BindMainHandler<&GiftDelete>(GIFT_DELETE);//PlayerDeleteGiftbox
        this->BindMainHandler<&GiftReceive>(GIFT_RECEIVE);//PlayerReceiveGiftbox
        this->BindMainHandler<&GiftView>(GIFT_LIST);//PlayerOpenGiftbox

        this->BindMainHandler<&AcHeartbeat>(INFO_SECURITY_TOOLS);
        this->BindMainHandler<&Authorize>(ID_AUTHORIZE);//version check
        this->BindMainHandler<&BattlePassClaim>(BATTLEPASS_CLAIM);//battle pass claim
        this->BindMainHandler<&BattlePassReset>(BATTLEPASS_RESET);//battle pass mission reroll
        this->BindMainHandler<&NameChange>(ID_CREATE);//nickname creation
        // ID_CHARACTER_BUY 70
        this->BindMainHandler<&Ping>(ID_PING);//player ping
		this->BindMainHandler<&PlayerQuit>(ID_QUIT);//player quit client

        this->BindMainHandler<&CharacterChange>(ID_CHARACTER_SELECT);//character select
        this->BindMainHandler<&Channels>(INFO_CHANNEL);//channels info

        this->BindMainHandler<&UsersView>(INFO_USER_LIST);//lobby user list
        this->BindMainHandler<&UsersProfile>(INFO_USER_PROFILE);//lobby user details

        this->BindMainHandler<&Battery>(ITEM_BATTERY_GET);//player energy


        this->BindMainHandler<&ShopNormal>(ITEM_NORMALSHOP_BUY);//shop buy item

        this->BindMainHandler<&ItemEquip>(ITEM_EQUIP);//character equip update
        this->BindMainHandler<&ItemDelete>(ITEM_DELETE);//delete item

        this->BindMainHandler<&ShopCoupon>(ITEM_COUPONSHOP_BUY);//shop coupon buy item

        this->BindMainHandler<&GachaponSpin>(ITEM_GACHA_SPIN);//gachapon spin
		this->BindMainHandler<&GachaponPity>(ITEM_GACHA_PITY);//gachapon pity

        this->BindMainHandler<&Pickups>(ITEM_PICKUP);//player pickup drop

        this->BindMainHandler<&ItemRepair>(ITEM_REPAIR);//repair item
        // ITEM_RESTORE 98
        this->BindMainHandler<&ItemSell>(ITEM_SELL);//sell item
        this->BindMainHandler<&ItemUpgrade>(ITEM_UPGRADE);//upgrade item

        this->BindMainHandler<&PackageOpen>(ITEM_USE);//package open


        this->BindMainHandler<&MailDelete>(MAIL_DELETE);//delete mailbox
        this->BindMainHandler<&MailSend>(MAIL_SEND);//send mailbox
        this->BindMainHandler<&MailRead>(MAIL_READ);//update mailbox
        this->BindMainHandler<&MailView>(MAIL_LIST);//open mailbox


        this->BindMainHandler<&MatchStart>(MOD_START);//start match room
        this->BindMainHandler<&MatchRoundsStart>(MOD_ROUND_START);//start elimination next round

        this->BindMainHandler<&PartyCreate>(PARTY_CREATE);//create party
        this->BindMainHandler<&PartyJoin>(PARTY_JOIN);//create party
        this->BindMainHandler<&PartyLeave>(PARTY_QUIT);//leave party

        this->BindMainHandler<&PartyClanView>(PARTY_QUICK_LIST);//clan active list
        this->BindMainHandler<&PartyClanView>(PARTY_CLAN_LIST);//clan active list

        this->BindMainHandler<&PartyHostChange>(PARTY_CHANGE_HOST);//party change host
        this->BindMainHandler<&PartyPassword>(PARTY_PASS);//clan register
        this->BindMainHandler<&PartyModinfo>(PARTY_MODINFO);//party settings
        this->BindMainHandler<&PartyPlayersRule>(PARTY_RULE_MAXPLAYER);//party settings

        this->BindMainHandler<&PartyAutomatch>(PARTY_AUTOMATCH);//party register
        this->BindMainHandler<&PartyClanRegister>(PARTY_CLAN_QUEUESTATE);//clan register
		this->BindMainHandler<&PartyClanOtherJoin>(PARTY_CLAN_JOIN);//clan other join
		this->BindMainHandler<&PartyClanLeave>(PARTY_CLAN_LEAVE);//clan leave

        this->BindMainHandler<&AllowKitdrops>(ROOM_RULE_KITDROPS);//change objects state room

        this->BindMainHandler<&RoomModInfo>(ROOM_RULE_MOD);//change settings room
        this->BindMainHandler<&RoomModinfoFull>(ROOM_RULE_MOD_FULL);//change settings room

        this->BindMainHandler<&AllowIntruders>(ROOM_RULE_INTRUDERS);//change intruders state room
        this->BindMainHandler<&HostChange>(ROOM_CHANGE_HOST);//change leader room

        this->BindMainHandler<&RoomModinfoPassword>(ROOM_RULE_MOD_PASS);//change settings room
        this->BindMainHandler<&RoomModinfoTitle>(ROOM_RULE_MOD_TITLE);//change settings room

        this->BindMainHandler<&MapRule>(ROOM_RULE_MAP);//change map room
        this->BindMainHandler<&PlayersRule>(ROOM_RULE_MAX_PLAYER);//change player limit room
        this->BindMainHandler<&AllowObservers>(ROOM_RULE_OBSERVER);//change observers state room
        this->BindMainHandler<&PointsRule>(ROOM_RULE_MAX_POINTS);//change points room
        this->BindMainHandler<&TimeRule>(ROOM_RULE_TIME);//change time limit room

        this->BindMainHandler<&RoomCreate>(ROOM_CREATE);//create room
        this->BindMainHandler<&RoomJoin>(ROOM_JOIN);//join room
        this->BindMainHandler<&RoomLeave>(ROOM_LEAVE);//leave room
        this->BindMainHandler<&RoomView>(ROOM_LIST);//rooms list

        this->BindMainHandler<&StateUpdate>(USER_STATE);//game event message
        this->BindMainHandler<&TeamChange>(USER_CHANGE_TEAM);//change team room
        this->BindMainHandler<&VoiceUpdate>(USER_CHANGE_VOICE);//select voicetype

        this->BindMainHandler<&Chat>(USER_CHAT_GAME);//chat message
        this->BindMainHandler<&Chat>(USER_CHAT);//chat message

        this->BindMainHandler<&InviteJoin>(USER_INVITE_OR_JOIN);//invite and join
        this->BindMainHandler<&Missions>(USER_MISSION_EVENT);//guide mission, daily mission
        this->BindMainHandler<&Automatch>(USER_AUTOMATCH);//automatch

        this->BindMainHandler<&PartyKick>(PARTY_KICK);//force kick a party member

#if defined(RELEASE_1_0_3)
        this->BindMainHandler<&PlazaJoin>(PLAZA_JOIN);// join plaza
		this->BindMainHandler<&PlazaLeave>(PLAZA_LEAVE); // leave plaza
#endif

        this->BindMainHandler<&MatchEnd>(MOD_END);//end match
        this->BindMainHandler<&MatchLeave>(MOD_LEAVE);//leave match

        this->BindMainHandler<&MatchRoundsEnd>(MOD_ROUND_END);//start elimination next round

        this->BindMainHandler<&CurrencyUpdate>(CURRENCY_UPDATE);//gift box sends first request to update currency
        this->BindMainHandler<&PveRespawn>(INFO_PVE_RESPAWN);//boss battle respawn

    }
    CMainServer::~CMainServer() {}
}
