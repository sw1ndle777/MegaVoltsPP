#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    namespace Handlers
    {

        inline void PlayerChat(SCallbackData& callback, CMainServer* main_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto session_id = session->GetSessionId();
            auto acc_cache = main_server->GetAccCacheSharedBySessionId(session_id);
            
            auto blockeds = main_server->GetBlockedsList(session_id); 
            auto acc_index = acc_cache->acc_info.Index;

            auto message_length = message->GetOption();
            auto chat_type = message->GetExtra();
            auto order = message->GetOrder();
            if (acc_index != -1)
            {
                auto my_nickname = acc_cache->acc_info.Nickname;
                auto in_room = acc_cache->in_room;
                auto in_plaza = acc_cache->in_plaza;
                auto in_party = acc_cache->in_party;
                auto plaza_id = acc_cache->plaza_id;
                auto playing = acc_cache->playing;
                auto team_id = acc_cache->team_id;
                auto room_id = acc_cache->room_id;
                auto party_id = acc_cache->party_id;
                auto current_player_grade = acc_cache->acc_info.Grade;
                auto clan_id = acc_cache->acc_info.ClanId;
                auto my_unique_id = NetEngine::Packets::Core::UniqueId(session_id, 1).data;
                acc_cache.unlock();

                uint8_t chat_color = 0;
                if (current_player_grade == Userlist::User::Grade::Moderator)
                    chat_color = 13;
                else if (current_player_grade == Userlist::User::Grade::Tester)
                    chat_color = 15;
                else if (current_player_grade > Userlist::User::Grade::Tester)
                    chat_color = 14;


                if (chat_type == Chat::Type::User)
                {
                    const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
                    if (in_room)
                    {
                        if (main_server->IsRoomAlready(room_id))
                        {
                            auto room = main_server->GetRoomCacheShared(room_id);
                            auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
                            for (const auto& id : player_ids)
                            {
                                if (id == session_id) continue;
                                auto room_player_cache = main_server->GetAccCacheSharedBySessionId(id);
                                if (room_player_cache->acc_info.Index == -1)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                if (room_player_cache->room_id != room_id)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                if (room_player_cache->playing != playing)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                if (playing && team_id == Team::IdType::Observer && room_player_cache->team_id != Team::IdType::Observer)
                                {
                                    if (room_player_cache->team_id != Team::IdType::Observer)
                                    {
                                        room_player_cache.unlock();
                                        continue;
                                    }
                                }
                                room_player_cache.unlock();
                                auto room_player_blockeds = main_server->GetBlockedsList(id); //blockeds_cache[id];
                                if (main_server->IsBlockedAlready(room_player_blockeds, acc_index)) continue;
                                if (auto player_session = server->GetSessionById(id))
                                {
                                    auto msgData = MainChatMatchAck(my_unique_id, chatReq->msg, message_length).Serialize(message_length);
                                    player_session->SendMsg(315, chat_color, Chat::Type::User, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                                }
                            }
                        }
                    }
                    else if (in_plaza)
                    {
                        if (main_server->IsPlazaAlready(plaza_id))
                        {
                            auto plaza = main_server->GetPlazaCacheShared(plaza_id);
                            if (main_server->IsPlazaBroadcastable(plaza))
                            {
                                for (const auto& id : plaza->session_ids)
                                {
                                    if (id == session_id) continue;
                                    auto plaza_player_cache = main_server->GetAccCacheSharedBySessionId(id);
                                    if (plaza_player_cache->acc_info.Index == -1)
                                    {
                                        plaza_player_cache.unlock();
                                        continue;
                                    }
                                    if (plaza_player_cache->plaza_id != plaza_id)
                                    {
                                        plaza_player_cache.unlock();
                                        continue;
                                    }
                                    if (!plaza_player_cache->in_plaza)
                                    {
                                        plaza_player_cache.unlock();
                                        continue;
                                    }
                                    plaza_player_cache.unlock();
                                    auto plaza_player_blockeds = main_server->GetBlockedsList(id);
                                    if (main_server->IsBlockedAlready(plaza_player_blockeds, acc_index)) continue;
                                    if (auto player_session = server->GetSessionById(id))
                                    {
                                        auto msgData = MainChatAck(my_nickname.c_str(), chatReq->msg, message_length).Serialize(chat_type, message_length);
                                        player_session->SendMsg(316, chat_color, Chat::Type::User, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        std::shared_lock lock(main_server->GetAccountsCacheMutex());
                        for (const auto& lobby_player : accounts_cache)
                        {
                            if (lobby_player.second.in_room) continue;
                            if (lobby_player.second.in_plaza) continue;
                            auto lobby_player_session_id = lobby_player.first;
                            auto lobby_player_acc_id = lobby_player.second.acc_info.Index;
                            if (lobby_player_acc_id == acc_index) continue;
                            auto lobby_player_blockeds = main_server->GetBlockedsList(lobby_player_session_id);
                            if (main_server->IsBlockedAlready(lobby_player_blockeds, acc_index)) continue;
                            if (auto player_session = server->GetSessionById(lobby_player_session_id))
                            {
                                auto msgData = MainChatAck(my_nickname.c_str(), chatReq->msg, message_length).Serialize(chat_type, message_length);
                                player_session->SendMsg(316, chat_color, Chat::Type::User, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                            }
                        }
                        lock.unlock();
                    }
                }
                else if (chat_type == Chat::Type::Whisper)
                {
                    auto in_room = order == 161;
                    const auto& chatWhisperReq = reinterpret_cast<MainChatWhisperReq*>(message->GetData());
                    const auto& chatWhisperInRoomReq = reinterpret_cast<MainChatWhisperInRoomReq*>(message->GetData());
                    const auto& whisper_target_name = Utility::ReadMicrovoltsString(chatWhisperReq->nickname, 16);

                    auto whisper_target_cache = in_room ? main_server->GetAccCacheSharedBySessionId(chatWhisperInRoomReq->unique_id.session) : main_server->GetAccCacheSharedByNickname(whisper_target_name.c_str());
                    if (whisper_target_cache->acc_info.Index == -1)
                    {
                        session->SendMsg(315, 1, Chat::WhisperResult::NoUser, 0);
                        return;
                    }
                    if (acc_index == whisper_target_cache->acc_info.Index)
                    {
                        session->SendMsg(315, 1, Chat::WhisperResult::DontMyself, 0);
                        return;
                    }
                    auto my_blockeds = main_server->GetBlockedsList(session_id);
                    if (main_server->IsBlockedAlready(my_blockeds, whisper_target_cache->acc_info.Index))
                    {
                        session->SendMsg(315, 1, Chat::WhisperResult::Failed, 0);
                        return;
                    }
                    auto target_blockeds = main_server->GetBlockedsList(whisper_target_cache->session_id);
                    if (main_server->IsBlockedAlready(target_blockeds, acc_index))
                    {
                        session->SendMsg(315, 1, Chat::WhisperResult::WhisperRefuse, 0);
                        return;
                    }
                    auto msgData = MainChatAck(my_nickname.c_str(), in_room ? chatWhisperInRoomReq->msg : chatWhisperReq->msg, message_length).Serialize(chat_type, message_length);
                    if (auto player_session = server->GetSessionById(whisper_target_cache->session_id))
                        player_session->SendMsg(316, chat_color, Chat::Type::Whisper, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());

                    session->SendMsg(316, chat_color, Chat::Type::Whisper, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                    whisper_target_cache.unlock();
                        
                }
                else if (chat_type == Chat::Type::Team)
                {
                    const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
                    if (in_room)
                    {
                        if (main_server->IsRoomAlready(room_id))
                        {
                            auto room = main_server->GetRoomCacheShared(room_id);
                            auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
                            for (const auto& id : player_ids)
                            {
                                if (id == session_id) continue;
                                auto room_player_cache = main_server->GetAccCacheSharedBySessionId(id);
                                if (room_player_cache->acc_info.Index == -1)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                if (room_player_cache->playing != playing)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                if (room_player_cache->team_id != team_id)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                room_player_cache.unlock();
                                auto room_player_blockeds = main_server->GetBlockedsList(id); //blockeds_cache[id];
                                if (main_server->IsBlockedAlready(room_player_blockeds, acc_index)) continue;
                                if (auto player_session = server->GetSessionById(id))
                                {
                                    auto msgData = MainChatMatchAck(my_unique_id, chatReq->msg, message_length).Serialize(message_length);
                                    player_session->SendMsg(315, chat_color, Chat::Type::Team, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                                }
                            }
                        }
                    }
                }
                else if (chat_type == Chat::Type::Clan)
                {
                    const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
                    if (clan_id)
                    {
                        if (main_server->IsClanAlready(clan_id))
                        {
                            auto clan_info = main_server->GetClanCacheShared(clan_id);
                            for (const auto& id : clan_info->online_members)
                            {
                                if (id == session_id) continue;
                                auto room_player_blockeds = main_server->GetBlockedsList(id);
                                if (main_server->IsBlockedAlready(room_player_blockeds, acc_index)) continue;
                                if (auto player_session = server->GetSessionById(id))
                                {
                                    auto msgData = MainChatAck(my_nickname.c_str(), chatReq->msg, message_length).Serialize(chat_type, message_length);
                                    player_session->SendMsg(316, chat_color, Chat::Type::Clan, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                                }
                            }
                            clan_info.unlock();
                        }
                    }
                }
                else if (chat_type == Chat::Type::Command)
                {
                    const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
                    const auto& args = Utility::SplitStrings(std::string_view(chatReq->msg, message_length), ' ');
                    if (!args.empty() && args[0].starts_with('/'))
                    {
                        auto cmd_acc_cache_unique = main_server->GetAccCacheUniqueBySessionId(session_id);
                        Commands::Execute(args[0].substr(1), args, callback, cmd_acc_cache_unique, main_server);
                    }
                        
                }
                else if (chat_type == Chat::Type::Party)
                {
                    const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
                    if (in_party)
                    {
                        const auto& args = Utility::SplitStrings(std::string_view(chatReq->msg, message_length), ' ');
                        if (!args.empty() && args[0].starts_with('/'))
                        {
                            auto cmd_acc_cache_unique = main_server->GetAccCacheUniqueBySessionId(session_id);
                            Commands::Execute(args[0].substr(1), args, callback, cmd_acc_cache_unique, main_server);
                        }
                        if (main_server->IsPartyAlready(party_id))
                        {
                            auto party = main_server->GetPartyCacheShared(party_id);
                            for (const auto& id : party->members)
                            {
                                if (id == session_id) continue;
                                auto room_player_cache = main_server->GetAccCacheSharedBySessionId(id);
                                if (room_player_cache->acc_info.Index == -1)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                if (room_player_cache->playing != playing)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                if (room_player_cache->team_id != team_id)
                                {
                                    room_player_cache.unlock();
                                    continue;
                                }
                                room_player_cache.unlock();
                                auto room_player_blockeds = main_server->GetBlockedsList(id); //blockeds_cache[id];
                                if (main_server->IsBlockedAlready(room_player_blockeds, acc_index)) continue;
                                if (auto player_session = server->GetSessionById(id))
                                {
                                    auto msgData = MainChatAck(my_nickname.c_str(), chatReq->msg, message_length).Serialize(chat_type, message_length);
                                    player_session->SendMsg(316, chat_color, Chat::Type::Team, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                                }
                            }
                            party.unlock();
                        }
                    }
                }
                else
                {
                    main_server->SendServerMessage(session, fmt::format("chat type unknown {}", chat_type).c_str());
                }
            }
        }
    }
}