#include "CMainServer.h"
#include "BaseLib/Utility.h"
#include "BaseLib/CDatabase.h"
#include "BaseLib/CDBData.h"

#include "handlers/BossBattleRespawn.h"
#include "handlers/BuyItem.h"
#include "handlers/ChangeTeam.h"
#include "handlers/ChannelsInfo.h"
#include "handlers/CreateRoom.h"
#include "handlers/DeleteItem.h"
#include "handlers/EndMatch.h"
#include "handlers/EquipItem.h"
#include "handlers/GachaponSpin.h"
#include "handlers/GameEventMessage.h"
#include "handlers/JoinRoom.h"
#include "handlers/JoinPlaza.h"
#include "handlers/LeaveMatch.h"
#include "handlers/LeaveRoom.h"
#include "handlers/LeavePlaza.h"
#include "handlers/LobbyUserDetails.h"
#include "handlers/LobbyUserList.h"
#include "handlers/NextRoundUpdateMatch.h"
#include "handlers/PackageOpen.h"
#include "handlers/PlayerAuthorize.h"
#include "handlers/PlayerChangeCharacter.h"
#include "handlers/PlayerChat.h"
#include "handlers/PlayerNameChange.h"
#include "handlers/PlayerPing.h"
#include "handlers/PlayerEnergy.h"
#include "handlers/PlayerSocials.h"
#include "handlers/PlayerMissions.h"
#include "handlers/RepairItem.h"
#include "handlers/SellItem.h"
#include "handlers/ServerIpcMessage.h"
#include "handlers/ServerConnect.h"
#include "handlers/ServerDisconnect.h"
#include "handlers/StartMatch.h"
#include "handlers/UpdateRoomList.h"
#include "handlers/UpdateRoomSettings.h"
#include "handlers/UpgradeItem.h"
#include "handlers/PlayerPickupDrop.h"
#include "handlers/PlayerMailbox.h"

#include "handlers/CreateParty.h"
#include "handlers/LeaveParty.h"

namespace Game
{
    namespace Commands
    {
        static void Help(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            const auto& cmds = Commands::ListCommands(acc_cache->acc_info.Grade);
            for(const auto& cmd : cmds)
                main_server->SendServerMessage(callback.session.get(), cmd.c_str());
        }
        static void Items(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() <= 1)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /item item_id item_id2 (max 25 item ids)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            if (acc_cache->inventory_items.size() + args.size() - 1 > acc_cache->acc_info.MaximumItems)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, you can't spawn {} items because your inventory will be over it's capacity.", acc_cache->acc_info.Nickname.c_str(), args.size() - 1).c_str());
                //return;
            }
            for (const auto& item_id_str : args)
            {
                if (!Utility::IsDigitsOnly(item_id_str)) continue;
                const auto& item_id = Utility::ExtractNumber(item_id_str.c_str());
                if (main_server->SendInventoryItem(callback.session.get(), acc_cache, { item_id }))
                    main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] spawned ({}) item", item_id).c_str());
                else
                    main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] Could not find item id ({})", item_id).c_str());
            }
        }
        static void Info(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (acc_cache->in_party)
            {
                if (main_server->IsPartyAlready(acc_cache->party_id))
                {
                    auto party = main_server->GetPartyCacheShared(acc_cache->party_id);
                    acc_cache.unlock();
                    main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] Party Info: is_registered: ({}) is_queueing: ({}), has_password: ({}), password: ({}), is_clan: ({}), clan_id: ({}), is_playing: ({}), max_members: ({}), members.size(): ({})", party->is_registered, party->is_queueing, party->has_password, party->password.c_str(), party->is_clan, party->clan_id, party->is_playing, party->max_members, party->members.size()).c_str());
                    acc_cache.lock();
                }
            }
            if (acc_cache->in_room)
            {
                if (main_server->IsRoomAlready(acc_cache->room_id))
                {
                    auto room = main_server->GetRoomCacheShared(acc_cache->room_id);
                    acc_cache.unlock();
                    auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
                  
                    main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] Rooms Info: {} players, mode: {}", player_ids.size(), static_cast<uint8_t>(room->ModeIndex)).c_str());
                    if (room->has_password)
                        main_server->SendServerMessage(callback.session.get(), fmt::format("RoomId: {} - Title: {} - Password: {}", room->room_id, room->title.c_str(), room->password.c_str()));
                    else
                        main_server->SendServerMessage(callback.session.get(), fmt::format("RoomId: {} - Title: {}", room->room_id, room->title.c_str()).c_str());

                    for (const auto& player_id : player_ids)
                    {
                        auto player_cache = main_server->GetAccCacheSharedBySessionId(player_id);
                        if (player_cache->acc_info.Index != -1)
                        {
                            const auto& is_playing = player_cache->playing ? "Yes" : "No";
                            const auto& state = player_cache->state;
                            if (room->host_session_id == player_id)
                                main_server->SendServerMessage(callback.session.get(), fmt::format("(HOST) ({}) SessionID: {} - Grade: {}, Slot: {}, Playing: {}, State: {}", player_cache->acc_info.Nickname.c_str(), player_cache->session_id, player_cache->acc_info.Grade, player_cache->slot_id, is_playing, state).c_str());
                            else
                                main_server->SendServerMessage(callback.session.get(), fmt::format("({}) SessionID: {} - Grade: {}, Slot: {}, Playing: {}, State: {}", player_cache->acc_info.Nickname.c_str(), player_cache->session_id, player_cache->acc_info.Grade, player_cache->slot_id, is_playing, state).c_str());
                        }
                        else
                            main_server->SendServerMessage(callback.session.get(), fmt::format("Unknown Cache Player SessionID: {}", player_id));

                        player_cache.unlock();

                    }
                }
            }
            else
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] Your Info").c_str());
                main_server->SendServerMessage(callback.session.get(), fmt::format("({}) SessionID: {} - Grade: {}", acc_cache->acc_info.Nickname.c_str(), acc_cache->session_id, acc_cache->acc_info.Grade).c_str());

            }
        }
        static void Online(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            acc_cache.unlock();
            CServer* server = callback.server;
            auto sessions_list = server->GetSessions();

            
            std::shared_lock lock(main_server->GetAccountsCacheMutex());
            main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] Players Online: {}, Sessions Size: {}", accounts_cache.size(), sessions_list->size()).c_str());
            for (const auto& acc : accounts_cache)
            {
                const auto& account = acc.second;
                const auto& is_playing = account.playing ? "Yes" : "No";
                const auto& in_room = account.in_room;
                const auto& state = account.state;
                if (in_room)
                    main_server->SendServerMessage(callback.session.get(), fmt::format("({}) SessionID: {} - Grade: {}, Slot: {}, Playing: {}, State: {}, Ping: {}, Room id: {}", account.acc_info.Nickname.c_str(), account.session_id, account.acc_info.Grade, account.slot_id, is_playing, state, account.ping, account.room_id).c_str());
                else
                    main_server->SendServerMessage(callback.session.get(), fmt::format("({}) SessionID: {} - Grade: {}, Slot: {}, Playing: {}, State: {}, Ping: {}", account.acc_info.Nickname.c_str(), account.session_id, account.acc_info.Grade, account.slot_id, is_playing, state, account.ping).c_str());
            }

            for (auto& sid : *sessions_list)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("sid online: {}", sid.first).c_str());
            }
        }
        static void Rooms(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            std::shared_lock lock(main_server->GetRoomsCacheMutex());
            main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] Rooms Online: {}", rooms_cache.size()).c_str());
            for (const auto& rooms : rooms_cache)
            {
                const auto& room = rooms.second;
                const auto& neutral_size = room.neutralteam_session_ids.size();
                const auto& red_size = room.redteam_session_ids.size();
                const auto& blue_size = room.blueteam_session_ids.size();
                const auto& obs_size = room.observers_session_ids.size();
                if (room.has_password)
                    main_server->SendServerMessage(callback.session.get(), fmt::format("  -> ({}) - Title: {} - Password: {} - plr count N: ({}), R: ({}), B: ({}), O: ({})", room.room_id, room.title.c_str(), room.password.c_str(), neutral_size, red_size, blue_size, obs_size).c_str());
                else
                    main_server->SendServerMessage(callback.session.get(), fmt::format("  -> ({}) - Title: {} - plr count N: ({}), R: ({}), B: ({}), O: ({})", room.room_id, room.title.c_str(), neutral_size, red_size, blue_size, obs_size).c_str());
            }
        }
        static void Disconnect(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {

            if (args.size() != 3)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /disc nickname id (0-255)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            auto disconnect_type = std::stoi(args[2].c_str());
            if (disconnect_type > 255 || disconnect_type < 0)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, disconnect id should be between (0-255)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            acc_cache.unlock();
            const auto& nickname = args[1];
           
            auto player = main_server->GetAccCacheSharedByNickname(nickname);
            auto player_session_id = player->session_id;
            auto player_auth_key = player->acc_info.AuthKey;
            player.unlock();
            main_server->DisconnectPlayer(main_server, player_session_id, player_auth_key, disconnect_type);
            main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, disconnected auth key {}", acc_cache->acc_info.Nickname.c_str(), player_auth_key).c_str());

        }
        static void Announce(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            acc_cache.unlock();
            CServer* server = callback.server;
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /! msg (512 max chars)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            if (args[1].size() > 512 || args[1].size() < 1)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /! msg (512 max chars)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            auto msg = args[1];
            std::shared_lock lock(main_server->GetAccountsCacheMutex());
            for (const auto& acc : accounts_cache)
            {
                auto session_id = acc.first;
                if (auto player_session = main_server->GetSessionById(session_id))
                {
                    player_session->SendMsg(402, 0, 10, 0, reinterpret_cast<uint8_t*>(msg.data()), msg.size());
                }
            }
        }
        static void Level(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /level new_level (0-100)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            
            if (!Utility::IsDigitsOnly(args[1]))
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /level new_level (0-100)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            auto lvl = std::stoi(args[1].c_str());
            auto gi = main_server->GetGradeInfoCache(lvl + 2);
            if (gi->Grade)
            {
                acc_cache->acc_info.Experience = gi->Exp;
                acc_cache->acc_info.Level = lvl;
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, changed level to {}", acc_cache->acc_info.Nickname.c_str(), lvl).c_str());
            }
            else
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, invalid level", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
           
        }
        static void Kick(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /kick nickname (0-255)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            const auto& nickname = args[1];
            if (acc_cache->acc_info.Nickname == nickname)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, you can't kick yourself", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }

            acc_cache.unlock();
            

            auto player = main_server->GetAccCacheUniqueByNickname(nickname);
            auto player_acc_index = player->acc_info.Index;
            auto player_session_id = player->session_id;
            auto player_unique_id = NetEngine::Packets::Core::UniqueId(player_session_id, 1);
            auto player_in_room = player->in_room;
            auto player_room_id = player->room_id;
            auto player_slot_id = player->slot_id;
            auto player_team_id = player->team_id;
            if(player_acc_index == -1) return;
            if (!player_in_room || !main_server->IsRoomAlready(player_room_id))
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] player {} is not in any room.", player->acc_info.Nickname.c_str()).c_str());
                return;
            }
            player.unlock();

            auto player_room = main_server->GetRoomCacheUnique(player_room_id);
            main_server->NewRemoveRoomPlayer(player_room, player_session_id, player_team_id, NetEngine::Room::Leave::Ack::Result::KickedByGm, true);
        }

        static void Break(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            auto myself_in_room = acc_cache->in_room;
            auto myself_room_id = acc_cache->room_id;
           
            if(args.size() != 1 && args.size() != 2)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /break or /break room_id", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            uint16_t target_room_id = 0;

            if (args.size() == 1) 
            {
                if (!myself_in_room)
                {
                    main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, you are not in a room. Use /break room_id instead.", acc_cache->acc_info.Nickname.c_str()).c_str());
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
                    main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, invalid room ID. Please provide a valid room ID.", acc_cache->acc_info.Nickname.c_str()).c_str());
                    return;
                }
            }

            if (!main_server->IsRoomAlready(target_room_id))
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, the specified room ID does not exist.", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }

            main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, room ID {} has been successfully broken.", acc_cache->acc_info.Nickname.c_str(), target_room_id).c_str());
            acc_cache.unlock();

            

            auto room_cache = main_server->GetRoomCacheUnique(target_room_id);
            auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
            for (const auto& player_id : players)
            {
                auto player_cache = main_server->GetAccCacheSharedBySessionId(player_id);
                if (player_cache->acc_info.Index == -1 || !player_cache->in_room || player_cache->room_id != room_cache->room_id)
                {
                    player_cache.unlock();
                    continue;
                }
                else
                {
                    player_cache->in_room = false;
                    player_cache->slot_id = 0xFF;
                    player_cache->playing = false;
                    player_cache->state = PlayerInfo::State::Waiting;
                    player_cache.unlock();

                    if (auto player_session = main_server->GetSessionById(player_id))
                    {
                        player_session->SendMsg(407, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // show gm break popup
                        player_session->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // Leave room ack
                       
                    }
                        
                }
            }
            main_server->RemoveRoomCache(target_room_id);
            main_server->SetRoomIdAvailable(target_room_id);
        }
        static void BreakAll(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {

            if (args.size() != 1)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /breakall", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
            main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, successfully broke all rooms.", acc_cache->acc_info.Nickname.c_str()).c_str());
            acc_cache.unlock();

            std::vector<uint16_t> room_ids;
            std::shared_lock room_cache_lock(main_server->GetRoomsCacheMutex());
            for (const auto& rooms : rooms_cache)
            {
                room_ids.push_back(rooms.first);
            }
            room_cache_lock.unlock();


            for (const auto& target_room_id : room_ids)
            {
                auto room_cache = main_server->GetRoomCacheUnique(target_room_id);
                auto players = main_server->GetRoomSortedPlayerSessionIds(room_cache);
                for (const auto& player_id : players)
                {
                    auto player_cache = main_server->GetAccCacheSharedBySessionId(player_id);
                    if (player_cache->acc_info.Index == -1 || !player_cache->in_room || player_cache->room_id != room_cache->room_id)
                    {
                        player_cache.unlock();
                        continue;
                    }
                    else
                    {
                        player_cache->in_room = false;
                        player_cache->slot_id = 0xFF;
                        player_cache->playing = false;
                        player_cache->state = PlayerInfo::State::Waiting;
                        player_cache.unlock();

                        if (auto player_session = main_server->GetSessionById(player_id))
                        {
                            player_session->SendMsg(407, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // show gm break popup
                            player_session->SendMsg(141, 0, NetEngine::Room::Leave::Ack::Result::ClosedByGm, 0); // Leave room ack
                        }
                            
                    }
                }
                main_server->RemoveRoomCache(target_room_id);
                main_server->SetRoomIdAvailable(target_room_id);
                room_cache.unlock();
            }
        }
        static void CreateClan(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            if (args.size() != 2)
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /createclan name (15 chars max)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }

            if (!Utility::IsDigitsOnly(args[1]))
            {
                main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {}, command usage: /createclan name (15 chars max)", acc_cache->acc_info.Nickname.c_str()).c_str());
                return;
            }
        }
        static void ReloadGachaponSalesInfo(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            main_server->ClearGachaponSaleCache();
            auto gachapon_sales = BaseLib::Database->GetGachaponSalesInfo();
            main_server->AddGachaponSaleCache(gachapon_sales);

            main_server->SendServerMessage(callback.session.get(), fmt::format("[MegaVolts Online] {} gachapon sales info reloaded", gachapon_sales.size()).c_str());
        }
        static void CastProcessInfo(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            main_server->SendCastIpc(PacketIds::Ipc::MainToCastReqServerInfo, Utility::ToVector(acc_cache->acc_info.AuthKey));
        }
        static void MainProcessInfo(const std::vector<std::string>& args, const SCallbackData& callback, AccCacheResource& acc_cache, CMainServer* main_server)
        {
            HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
            auto cpu_usage = Utility::GetCpuUsage(m_process_handle);
            auto mem_usage = static_cast<uint32_t>(Utility::GetMemoryUsage(m_process_handle));
            CloseHandle(m_process_handle);
            auto sessions_count = static_cast<uint16_t>(main_server->GetSessions()->size());

            auto msg = fmt::format("[MegaVolts Online] Main Info: Sessions Online: {}, Memory Usage: {} MB, Cpu Usage: {:.2f}%",
                static_cast<uint16_t>(sessions_count),
                static_cast<uint32_t>(mem_usage),
                static_cast<double>(cpu_usage));

            main_server->SendServerMessage(callback.session.get(), msg.c_str());
            //main_server->SendServerMessage(callback.session,
            //    std::format("[MegaVolts Online] Main Info: Sessions Online: {}, Memory Usage: {} MB, Cpu Usage: {.2f}",
            //        sessions_count, mem_usage, cpu_usage).c_str());
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
                BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "force command disconnect, session id: ({})", id);
                main_server->DisconnectPlayer(main_server, id, Disconnect::Reason::Deny);
                kicked_cnt++;
            }
            
            auto msg = fmt::format("success. all {} player was kick for prepare maintanance.", kicked_cnt);

            main_server->SendServerMessage(callback.session.get(), msg.c_str());
        }
        static void Init()
        {
            Commands::Register("shutdown_prepare", ShutdownPrepare, Userlist::User::Grade::GameMaster);
            Commands::Register("?", Help, Userlist::User::Grade::Tester);
            Commands::Register("!", Announce, Userlist::User::Grade::GameMaster);
            Commands::Register("item", Items, Userlist::User::Grade::Tester);
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
    std::shared_mutex accounts_cache_mutex;
    std::shared_mutex rooms_cache_mutex;
    std::shared_mutex plaza_cache_mutex;
    std::shared_mutex room_ids_mutex;
    std::shared_mutex party_ids_mutex;
    std::shared_mutex clan_cache_mutex;
    std::shared_mutex party_cache_mutex;
    std::shared_mutex mailbox_data_cache_mutex;
    std::shared_mutex mailbox_sent_cache_mutex;
    std::shared_mutex mailbox_recv_cache_mutex;
    std::shared_mutex giftbox_recv_cache_mutex;
    std::shared_mutex gachapon_sale_cache_mutex;
    std::shared_mutex gachapon_ids_sale_cache_mutex;
    /*
    std::unordered_map<uint32_t, BaseLib::ItemInfo> items_info; //read only
    std::unordered_map<uint32_t, BaseLib::SetItemInfo> setitems_info; //read only
    std::vector<BaseLib::VendorInfo> vendors_info; //read only
    std::unordered_map<uint32_t, std::unordered_map<Items::Upgrade::Type, std::vector<BaseLib::UpgradeInfo>>> upgrades_info; //read only
    std::unordered_map<uint32_t, BaseLib::GachaponInfo> gachapons_info; //read only
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<BaseLib::PackageInfo>>> packages_info; //read only
    std::vector<uint32_t> vendor_item_ids; //read only
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<BaseLib::RoomOptionInfo>>> roomoptionsinfo_cache; //read only
    std::unordered_map<uint32_t, BaseLib::GradeInfo> grades_info; //read only
    std::unordered_map<uint32_t, BaseLib::RewardInfo> rewards_info; //read only
    std::unordered_map<uint32_t, std::vector<BaseLib::FriendInfo>> friends_cache; //read & write
    std::unordered_map<uint32_t, std::vector<BaseLib::BlockedInfo>> blockeds_cache; //read & write
    std::unordered_map<uint32_t, Player> accounts_cache; //read & write
    std::unordered_map<uint32_t, Room> rooms_cache; //read & write
    std::unordered_map<uint32_t, Plaza> plaza_cache; //read & write
    std::vector<uint32_t> room_ids; //read & write
    */

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
    boost::unordered_flat_map<uint32_t, boost::unordered_flat_map<uint32_t, std::vector<BaseLib::RoomOptionInfo>>> roomoptionsinfo_cache; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::GradeInfo> grades_info; //read only
    boost::unordered_flat_map<uint32_t, BaseLib::RewardInfo> rewards_info; //read only
    boost::unordered_flat_map<uint32_t, std::vector<BaseLib::FriendInfo>> friends_cache; //read & write
    boost::unordered_flat_map<uint32_t, std::vector<BaseLib::BlockedInfo>> blockeds_cache; //read & write
    boost::unordered_flat_map<uint32_t, Player> accounts_cache; //read & write
    boost::unordered_flat_map<uint32_t, Room> rooms_cache; //read & write
    boost::unordered_flat_map<uint32_t, Plaza> plaza_cache; //read & write
    std::vector<uint32_t> room_ids; //read & write 
    std::vector<uint32_t> party_ids; //read & write
    boost::unordered_flat_map<uint32_t, Clan> clan_cache; //read & write
    boost::unordered_flat_map<uint16_t, Party> party_cache; //read & write
    boost::unordered_flat_map<uint32_t, MailboxData> mailbox_data_cache; //read & write access by mail id
    boost::unordered_flat_map<uint32_t, std::vector<uint32_t>> mailbox_sent_cache; //read & write access by acc id, get vector of mail sent mail ids
    boost::unordered_flat_map<uint32_t, std::vector<uint32_t>> mailbox_recv_cache; //read & write access by acc id, get vector of mail recv mail ids
    boost::unordered_flat_map<uint32_t, std::vector<uint32_t>> giftbox_recv_cache; //read & write access by acc id, get vector of mail recv mail ids
    boost::unordered_flat_map<uint32_t, BaseLib::GachaponSaleInfo> gachapon_sales_info;
    std::vector<uint32_t> gachapon_ids_sale;


    RECT rc = { 0 };

    CMainServer::CMainServer()
    {
        Commands::Init();

        this->OnNewSession(std::bind(&Game::Handlers::ServerConnect, std::placeholders::_1, this));
        this->OnSessionDisconnected(std::bind(&Game::Handlers::ServerDisconnect, std::placeholders::_1, this));
        this->OnIpcMessage(std::bind(&Game::Handlers::ServerIpcMessage, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this));
        this->On(52, std::bind(&Game::Handlers::PlayerSocials, std::placeholders::_1, this));//block add
        this->On(53, std::bind(&Game::Handlers::PlayerSocials, std::placeholders::_1, this));//block remove
        this->On(54, std::bind(&Game::Handlers::PlayerSocials, std::placeholders::_1, this));//block 
        this->On(57, std::bind(&Game::Handlers::PlayerSocials, std::placeholders::_1, this));//clan list
        this->On(58, std::bind(&Game::Handlers::PartyList, std::placeholders::_1, this));//new party clan implement
        this->On(59, std::bind(&Game::Handlers::PlayerCompleteAchievement, std::placeholders::_1, this));//achievement achivement mission completion
        this->On(61, std::bind(&Game::Handlers::PlayerSocials, std::placeholders::_1, this));//friend add
        this->On(62, std::bind(&Game::Handlers::PlayerSocials, std::placeholders::_1, this));//friend remove
        this->On(63, std::bind(&Game::Handlers::PlayerSocials, std::placeholders::_1, this));//friend list

        this->On(66, std::bind(&Game::Handlers::PlayerMailbox, std::placeholders::_1, this));//PlayerReceiveGiftbox
        this->On(67, std::bind(&Game::Handlers::PlayerMailbox, std::placeholders::_1, this));//PlayerOpenGiftbox

        this->On(68, std::bind(&Game::Handlers::PlayerAuthorize, std::placeholders::_1, this));//version check
        this->On(69, std::bind(&Game::Handlers::PlayerNameChange, std::placeholders::_1, this));//nickname creation
        this->On(71, std::bind(&Game::Handlers::PlayerPing, std::placeholders::_1, this));//player ping
        this->On(74, std::bind(&Game::Handlers::PlayerChangeCharacter, std::placeholders::_1, this));//character select
        this->On(82, std::bind(&Game::Handlers::ChannelsInfo, std::placeholders::_1, this));//channels info
        this->On(84, std::bind(&Game::Handlers::LobbyUserList, std::placeholders::_1, this));//lobby user list
        this->On(85, std::bind(&Game::Handlers::LobbyUserDetails, std::placeholders::_1, this));//lobby user details
        this->On(86, std::bind(&Game::Handlers::PlayerEnergy, std::placeholders::_1, this));//player energy
        this->On(87, std::bind(&Game::Handlers::NormalShop, std::placeholders::_1, this));//shop buy item
        this->On(88, std::bind(&Game::Handlers::EquipItem, std::placeholders::_1, this));//character equip update
        this->On(89, std::bind(&Game::Handlers::DeleteItem, std::placeholders::_1, this));//delete item
        this->On(91, std::bind(&Game::Handlers::CouponShop, std::placeholders::_1, this));//shop coupon buy item
        this->On(92, std::bind(&Game::Handlers::GachaponSpin, std::placeholders::_1, this));//gachapon spin
        this->On(96, std::bind(&Game::Handlers::PlayerPickupDrop, std::placeholders::_1, this));//gachapon spin
        this->On(98, std::bind(&Game::Handlers::RepairItem, std::placeholders::_1, this));//repair item
        this->On(100, std::bind(&Game::Handlers::SellItem, std::placeholders::_1, this));//sell item
        this->On(101, std::bind(&Game::Handlers::UpgradeItem, std::placeholders::_1, this));//upgrade item
        this->On(102, std::bind(&Game::Handlers::PackageOpen, std::placeholders::_1, this));//package open
        this->On(103, std::bind(&Game::Handlers::PlayerMailbox, std::placeholders::_1, this));//delete mailbox
        this->On(104, std::bind(&Game::Handlers::PlayerMailbox, std::placeholders::_1, this));//send mailbox
        this->On(105, std::bind(&Game::Handlers::PlayerMailbox, std::placeholders::_1, this));//update mailbox
        this->On(106, std::bind(&Game::Handlers::PlayerMailbox, std::placeholders::_1, this));//open mailbox
        this->On(107, std::bind(&Game::Handlers::StartMatch, std::placeholders::_1, this));//start match room
        this->On(108, std::bind(&Game::Handlers::NextRoundUpdateMatch, std::placeholders::_1, this));//start elimination next round
        this->On(109, std::bind(&Game::Handlers::CreateParty, std::placeholders::_1, this));//create party
        this->On(110, std::bind(&Game::Handlers::PartyJoin, std::placeholders::_1, this));//create party
        this->On(111, std::bind(&Game::Handlers::LeaveParty, std::placeholders::_1, this));//leave party
        this->On(112, std::bind(&Game::Handlers::GetActiveClanList, std::placeholders::_1, this));//clan active list
        this->On(113, std::bind(&Game::Handlers::GetActiveClanList, std::placeholders::_1, this));//clan active list
        this->On(114, std::bind(&Game::Handlers::PartyChangeLeader, std::placeholders::_1, this));//party change host
        this->On(115, std::bind(&Game::Handlers::ClanRegister, std::placeholders::_1, this));//clan register
        this->On(116, std::bind(&Game::Handlers::PartySettings, std::placeholders::_1, this));//party settings
        this->On(117, std::bind(&Game::Handlers::PartySettings, std::placeholders::_1, this));//party settings
        this->On(119, std::bind(&Game::Handlers::PartyRegister, std::placeholders::_1, this));//party register
        this->On(120, std::bind(&Game::Handlers::ClanRegister, std::placeholders::_1, this));//clan register
        this->On(121, std::bind(&Game::Handlers::ClanOtherJoin, std::placeholders::_1, this));
        this->On(122, std::bind(&Game::Handlers::ClanRoomLeave, std::placeholders::_1, this));
        this->On(124, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change objects state room
        this->On(125, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change settings room
        this->On(126, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change settings room
        this->On(127, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change intruders state room
        this->On(128, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change leader room
        this->On(129, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change settings room
        this->On(130, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change settings room
        this->On(131, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change map room
        this->On(132, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change player limit room
        this->On(133, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change observers state room
        this->On(134, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change points room
        this->On(135, std::bind(&Game::Handlers::UpdateRoomSettings, std::placeholders::_1, this));//change time limit room
        this->On(138, std::bind(&Game::Handlers::CreateRoom, std::placeholders::_1, this));//create room
        this->On(140, std::bind(&Game::Handlers::JoinRoom, std::placeholders::_1, this));//join room
        this->On(141, std::bind(&Game::Handlers::LeaveRoom, std::placeholders::_1, this));//leave room
        this->On(142, std::bind(&Game::Handlers::UpdateRoomList, std::placeholders::_1, this));//rooms info
        this->On(158, std::bind(&Game::Handlers::GameEventMessage, std::placeholders::_1, this));//game event message
        this->On(159, std::bind(&Game::Handlers::ChangeTeam, std::placeholders::_1, this));//change team room
        this->On(160, std::bind(&Game::Handlers::SelectVoiceType, std::placeholders::_1, this));//select voicetype
        this->On(161, std::bind(&Game::Handlers::PlayerChat, std::placeholders::_1, this));//chat message
        this->On(162, std::bind(&Game::Handlers::PlayerChat, std::placeholders::_1, this));//chat message
        this->On(163, std::bind(&Game::Handlers::PlayerInviteJoin, std::placeholders::_1, this));//invite and join
        this->On(168, std::bind(&Game::Handlers::PlayerCompleteGuideMission, std::placeholders::_1, this));//guide mission completion
        this->On(169, std::bind(&Game::Handlers::PlayerAutomatchLobby, std::placeholders::_1, this));//automatch
        this->On(177, std::bind(&Game::Handlers::PartyKickMember, std::placeholders::_1, this));//force kick a party member

#if defined(RELEASE_1_0_3)
        this->On(173, std::bind(&Game::Handlers::JoinPlaza, std::placeholders::_1, this));
        this->On(174, std::bind(&Game::Handlers::LeavePlaza, std::placeholders::_1, this));
#endif

        this->On(254, std::bind(&Game::Handlers::EndMatch, std::placeholders::_1, this));//end match
        this->On(256, std::bind(&Game::Handlers::LeaveMatch, std::placeholders::_1, this));//leave match
        this->On(259, std::bind(&Game::Handlers::NextRoundUpdateMatch, std::placeholders::_1, this));//start elimination next round
        this->On(329, std::bind(&Game::Handlers::BossBattleRespawn, std::placeholders::_1, this));//boss battle respawn

    }
    CMainServer::~CMainServer() {}
}