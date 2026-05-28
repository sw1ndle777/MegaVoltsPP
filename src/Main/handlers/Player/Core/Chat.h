#pragma once
#include <BaseLib/CLogging.h>
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void Chat(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc_cache = CAccount.get<shared_t>(session_id);

        //auto blockeds = main_server->GetBlockedsList(session_id); 
        //auto my_socials = CSocial.get<shared_t>(session_id);
		auto my_socials = CSocial.get<shared_t>(session_id);
        auto acc_index = acc_cache->acc_info.Index;

        auto message_length = message->GetOption();
        auto chat_type = message->GetExtra();
        auto order = message->GetOrder();
        if (!acc_index) return;
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
        auto my_unique_id = acc_cache->uid.data;
        auto server_id = acc_cache->server_id;
        auto muted_until = acc_cache->acc_info.MutedUntil;
        acc_cache.unlock();

        const auto now_utc = Utility::GetUtcTimeNow64();
        if (chat_type != Chat::Type::Command && muted_until > now_utc)
        {
            main_server->SendServerMessage(session,
                std::format("[MegaVolts Online] You are muted for {}.", Utility::FormatCompactDurationSeconds(muted_until - now_utc)));
            return;
        }

        ChatLogEntry chat_log;
        chat_log.aid = acc_index;
        chat_log.server_id = server_id;

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
            chat_log.chat_type = ChatLog::Type::User;
            chat_log.message = std::string(chatReq->msg, message_length);
            if (in_room)
            {
                chat_log.location = ChatLog::Location::Room;
                chat_log.room_id = room_id;
                if (CRoom.contains(room_id))
                {
                    auto room = CRoom.get<shared_t>(room_id);
                    
                    auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
                    for (const auto& id : player_ids)
                    {
                        if (id == session_id) continue;
                        auto room_player_cache = CAccount.get<shared_t>(id);
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
                        //auto room_player_blockeds = main_server->GetBlockedsList(id); 
                        auto room_player_socials = CSocial.get<shared_t>(id);
                        if (main_server->IsBlockedAlready(room_player_socials, acc_index)) continue;
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
                chat_log.location = ChatLog::Location::Plaza;
                chat_log.plaza_id = plaza_id;
                if (main_server->IsPlazaAlready(plaza_id))
                {
                    auto plaza = CPlaza.get<shared_t>(plaza_id);
                    if (main_server->IsPlazaBroadcastable(plaza))
                    {
                        
                        for (const auto& id : plaza->session_ids)
                        {
                            if (id == session_id) continue;
                            auto plaza_player_cache = CAccount.get<shared_t>(id);
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
                            //auto plaza_player_blockeds = main_server->GetBlockedsList(id);
                            auto plaza_player_socials = CSocial.get<shared_t>(id);
                            if (main_server->IsBlockedAlready(plaza_player_socials, acc_index)) continue;
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
                chat_log.location = ChatLog::Location::Lobby;
                auto sids = CSid->get_all(shared);
                for (const auto& id : *sids)
                {
                    auto lobby_player_acc = CAccount.get<shared_t>(id);
                    if (!lobby_player_acc->acc_info.Index ||
                        lobby_player_acc->in_room ||
                        lobby_player_acc->in_plaza ||
                        lobby_player_acc->acc_info.Index == acc_index)
                    {
                        lobby_player_acc.unlock();
                        continue;
                    }

                    auto lobby_player_socials = CSocial.get<shared_t>(id);
                    if (main_server->IsBlockedAlready(lobby_player_socials, acc_index))
                    {
                        lobby_player_acc.unlock();
                        continue;
                    }
                    lobby_player_acc.unlock();
                    if (auto pss = server->GetSessionById(id))
                    {
                        auto msgData = MainChatAck(my_nickname.c_str(), chatReq->msg, message_length).Serialize(chat_type, message_length);
                        pss->SendMsg(316, chat_color, Chat::Type::User, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                    }

                }
            }

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([chat_log]() mutable
                {
                    BaseLib::Database->PersistChatLogs({ chat_log });
                });
        }
        else if (chat_type == Chat::Type::Whisper)
        {
            auto in_room = order == 161;
            const auto& chatWhisperReq = reinterpret_cast<MainChatWhisperReq*>(message->GetData());
            const auto& chatWhisperInRoomReq = reinterpret_cast<MainChatWhisperInRoomReq*>(message->GetData());
            const auto& whisper_target_name = Utility::ReadMicrovoltsString(chatWhisperReq->nickname, 16);


            auto whisper_target_cache = in_room ?
                CAccount.get<shared_t>(chatWhisperInRoomReq->unique_id.session) :
                CAccount.get_by_filter<shared_t>([&](const auto& /*id*/, auto& player) {
                return Utility::ToLowercase(player.acc_info.Nickname) == Utility::ToLowercase(whisper_target_name);
                    });
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
            //auto my_blockeds = main_server->GetBlockedsList(session_id);
            auto my_socials = CSocial.get<shared_t>(session_id);
            if (main_server->IsBlockedAlready(my_socials, whisper_target_cache->acc_info.Index))
            {
                session->SendMsg(315, 1, Chat::WhisperResult::Failed, 0);
                return;
            }
            auto target_socials = CSocial.get<shared_t>(whisper_target_cache->session_id);
            //auto target_blockeds = main_server->GetBlockedsList(whisper_target_cache->session_id);
            if (main_server->IsBlockedAlready(target_socials, acc_index))
            {
                session->SendMsg(315, 1, Chat::WhisperResult::WhisperRefuse, 0);
                return;
            }
            auto msgData = MainChatAck(my_nickname.c_str(), in_room ? chatWhisperInRoomReq->msg : chatWhisperReq->msg, message_length).Serialize(chat_type, message_length);
            if (auto player_session = server->GetSessionById(whisper_target_cache->session_id))
                player_session->SendMsg(316, chat_color, Chat::Type::Whisper, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());


            session->SendMsg(316, chat_color, Chat::Type::Whisper, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());

            chat_log.chat_type = ChatLog::Type::Whisper;
            chat_log.target_aid = whisper_target_cache->acc_info.Index;
            chat_log.message = std::string(in_room ? chatWhisperInRoomReq->msg : chatWhisperReq->msg, message_length);

            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([chat_log]() mutable
                {
                    BaseLib::Database->PersistChatLogs({ chat_log });
                });

            whisper_target_cache.unlock();

        }
        else if (chat_type == Chat::Type::Team)
        {
            const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
            if (in_room)
            {
                if (CRoom.contains(room_id))
                {
                    chat_log.chat_type = ChatLog::Type::Team;
                    chat_log.location = ChatLog::Location::Room;
                    chat_log.room_id = room_id;
                    chat_log.message = std::string(chatReq->msg, message_length);

                    auto room = CRoom.get<shared_t>(room_id);
                    auto player_ids = main_server->GetRoomSortedPlayerSessionIds(room);
                    for (const auto& id : player_ids)
                    {
                        if (id == session_id) continue;
                        auto room_player_cache = CAccount.get<shared_t>(id);
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
                        //auto room_player_blockeds = main_server->GetBlockedsList(id); //blockeds_cache[id];
                        auto room_player_socials = CSocial.get<shared_t>(id);
                        if (main_server->IsBlockedAlready(room_player_socials, acc_index)) continue;
                        if (auto player_session = server->GetSessionById(id))
                        {
                            auto msgData = MainChatMatchAck(my_unique_id, chatReq->msg, message_length).Serialize(message_length);
                            player_session->SendMsg(315, chat_color, Chat::Type::Team, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                        }
                    }
                    [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([chat_log]() mutable
                        {
                            BaseLib::Database->PersistChatLogs({ chat_log });
                        });
                }
            }
        }
        else if (chat_type == Chat::Type::Clan)
        {
            const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
            if (clan_id)
            {
                if (CClan.contains(clan_id))
                {
                    chat_log.chat_type = ChatLog::Type::Clan;
                    chat_log.clan_id = clan_id;
                    chat_log.message = std::string(chatReq->msg, message_length);

                    auto clan_info = CClan.get<shared_t>(clan_id);
                    for (const auto& id : clan_info->online_members)
                    {
                        if (id == session_id) continue;
                        //auto room_player_blockeds = main_server->GetBlockedsList(id);
                        auto room_player_socials = CSocial.get<shared_t>(id);
                        if (main_server->IsBlockedAlready(room_player_socials, acc_index)) continue;
                        if (auto player_session = server->GetSessionById(id))
                        {
                            auto msgData = MainChatAck(my_nickname.c_str(), chatReq->msg, message_length).Serialize(chat_type, message_length);
                            player_session->SendMsg(316, chat_color, Chat::Type::Clan, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                        }
                    }
                    [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([chat_log]() mutable
                        {
                            BaseLib::Database->PersistChatLogs({ chat_log });
                        });
                    clan_info.unlock();
                }
            }
        }
        else if (chat_type == Chat::Type::Command)
        {
            const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
            const auto& args = Utility::SplitStrings(std::string_view(chatReq->msg, message_length), ' ');
            chat_log.chat_type = ChatLog::Type::Command;
            chat_log.message = std::string(chatReq->msg, message_length);
            if (!args.empty() && args[0].starts_with('/'))
            {
                

                auto cmd_acc_cache_unique = CAccount.get<unique_t>(session_id);
                Commands::Execute(args[0].substr(1), args, callback, cmd_acc_cache_unique, main_server);
            }
            [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([chat_log]() mutable
            {
                BaseLib::Database->PersistChatLogs({ chat_log });
            });
        }
        else if (chat_type == Chat::Type::Party)
        {
            const auto& chatReq = reinterpret_cast<MainChatReq*>(message->GetData());
            if (in_party)
            {
                if (CParty.contains(party_id))
                {
                    chat_log.chat_type = ChatLog::Type::Party;
                    chat_log.message = std::string(chatReq->msg, message_length);

                    auto party = CParty.get<shared_t>(party_id);
                    for (const auto& id : party->members)
                    {
                        if (id == session_id) continue;
                        auto room_player_cache = CAccount.get<shared_t>(id);
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
                        //auto room_player_blockeds = main_server->GetBlockedsList(id); //blockeds_cache[id];
                        auto room_player_socials = CSocial.get<shared_t>(id);
                        if (main_server->IsBlockedAlready(room_player_socials, acc_index)) continue;
                        if (auto player_session = server->GetSessionById(id))
                        {
                            auto msgData = MainChatAck(my_nickname.c_str(), chatReq->msg, message_length).Serialize(chat_type, message_length);
                            player_session->SendMsg(316, chat_color, Chat::Type::Team, message_length, reinterpret_cast<uint8_t*>(msgData.data()), msgData.size());
                        }
                    }
                    party.unlock();
                    [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([chat_log]() mutable
                        {
                            BaseLib::Database->PersistChatLogs({ chat_log });
                        });
                }
            }
        }
        else
        {
            main_server->SendServerMessage(session, std::format("chat type unknown {}", chat_type).c_str());
        }
            
    }
}

namespace NetEngine
{
    template <>
    struct PacketRateLimitPolicy<&Game::Handlers::Chat>
    {
        inline static const std::optional<RateLimit::Rule> value = RateLimit::Rule{
            .enabled = true,
            .bucket_scope = RateLimit::IdentityScope::Aid,
            .max_packets = 50,
            .window = std::chrono::seconds{ 50 },
            .max_packets_resolver = [](const SCallbackData& callback, const RateLimit::IdentitySnapshot&)
            {
                if (!callback.message)
                    return 50u;

                const auto length = callback.message->GetOption();
                if (length >= 160)
                    return 35u;
                if (length >= 96)
                    return 40u;
                return 50u;
            },
            .window_resolver = [](const SCallbackData& callback, const RateLimit::IdentitySnapshot&)
            {
                if (!callback.message)
                    return std::chrono::seconds{ 50 };

                const auto length = callback.message->GetOption();
                if (length >= 160)
                    return std::chrono::seconds{ 100 };
                if (length >= 96)
                    return std::chrono::seconds{ 75 };
                return std::chrono::seconds{ 50 };
            },
            .on_limit = [](RateLimit::ActionContext& ctx)
            {
                if (ctx.identity.aid <= 0)
                    return;

                const auto length = ctx.callback && ctx.callback->message ? ctx.callback->message->GetOption() : 0;
                const auto mute_duration = length >= 160 ? std::chrono::minutes{ 15 } : (length >= 96 ? std::chrono::minutes{ 10 } : std::chrono::minutes{ 5 });
                const auto muted_until = Utility::GetUtcTimeNow64() + static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(mute_duration).count());
                ctx.CooldownAid(std::chrono::duration_cast<std::chrono::milliseconds>(mute_duration));

                if (Game::CAidSid.contains(ctx.identity.aid))
                {
                    auto sid = Game::CAidSid.get<BaseLib::shared_t>(ctx.identity.aid);
                    if (sid && *sid)
                    {
                        auto acc = Game::CAccount.get<BaseLib::unique_t>(*sid);
                        if (acc && acc->acc_info.Index == ctx.identity.aid)
                            acc->acc_info.MutedUntil = muted_until;
                    }
                }

                [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([
                    aid = ctx.identity.aid,
                    muted_until
                ]() mutable
                {
                    BaseLib::Database->SetAccountMutedUntil(aid, muted_until);
                });
            },
        };
    };
}
