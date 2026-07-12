#pragma once
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;
    inline void CharacterChange(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        //std::shared_lock lock(session->GetMutex());
        CServer* server = callback.server;
        auto session_id = session->GetSessionId();
        auto acc = CAccount.get<unique_t>(session_id);
        if (!acc->acc_info.Index) return;
        auto character = static_cast<Character::Type>(message->GetOption());
        if (acc->acc_info.SelectedCharacter == character)
        {
            session->SendMsg(74, 0, CharacterSelectInfo::Result::Ok, static_cast<uint8_t>(character));
            return;
        }
        DatabaseUpdateCtx dctx{ .sid = session_id,.aid = acc->acc_info.Index };
        dctx.ops.emplace_back(AccountInfoPatch{ .selected_character = static_cast<uint32_t>(character) });


        auto validated = main_server->ValidateDatabaseUpdates(acc, dctx);
        if (!validated.has_value())
        {
            DEBUGLOG(red, "ValidateDatabaseUpdates failed for [{}] [{}]: {}", acc->acc_info.Index, acc->acc_info.Nickname.c_str(), static_cast<int>(validated.error()));
            return;
        }
        acc.unlock();
        [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([main_server, session = std::move(callback.session),
            s_id = session_id,
            character = character,
            v = std::move(validated.value())
        ]() mutable
            {
                if (!session) return;
                ResultDbUpdateInfo dbres;
                if (!BaseLib::Database->UpdateAccount(v, dbres).has_value()) return;
                auto new_acc_cache = CAccount.get<unique_t>(s_id);
                auto applied = main_server->ApplyDatabaseUpdates(new_acc_cache, v);
                if (!applied.has_value())
                {
                    DEBUGLOG(red, "ApplyDatabaseUpdates failed for [{}] [{}]: {}", new_acc_cache->acc_info.Index, new_acc_cache->acc_info.Nickname.c_str(), static_cast<int>(applied.error()));
                    return;
                }
                main_server->RefreshPlayerHealthCache(*new_acc_cache, !new_acc_cache->playing);
                main_server->SendCastPlayerHealthSync(new_acc_cache->session_id, new_acc_cache->max_health, new_acc_cache->current_health);
                if (auto pss = main_server->GetSessionById(s_id))
                    pss->SendMsg(74, 0, CharacterSelectInfo::Result::Ok, static_cast<uint8_t>(character));

                // Equipment is per-character, so switching characters changes this player's whole
                // appearance. Broadcast the new character's equipped items to everyone in the same
                // room/plaza (order 414 EquipInfoAck + 314), mirroring ItemEquip and ToyBattles'
                // broadcastPlayerItems on character select. Without this, others keep seeing the old
                // character's items in the lobby and carried into the match.
                auto uid = new_acc_cache->uid.data;
                auto voice_id = new_acc_cache->voice_id;
                auto selected_character = new_acc_cache->acc_info.SelectedCharacter;
                if (new_acc_cache->in_room && CRoom.contains(new_acc_cache->room_id))
                {
                    auto room = CRoom.get<shared_t>(new_acc_cache->room_id);
                    auto equipped_items = main_server->GetEquippedItems(new_acc_cache);
                    auto equip_data = EquipInfoAck(new_acc_cache->uid, equipped_items);
                    new_acc_cache.unlock();
                    auto players_ids = main_server->GetRoomSortedPlayerSessionIds(room);
                    for (const auto& room_player_session_id : players_ids)
                    {
                        if (room_player_session_id == s_id) continue;
                        if (auto player_session = main_server->GetSessionById(room_player_session_id))
                        {
                            player_session->SendMsg(414, 0, selected_character, 17, reinterpret_cast<uint8_t*>(&equip_data), sizeof(EquipInfoAck));
                            player_session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                        }
                    }
                    new_acc_cache.lock();
                }
                else if (new_acc_cache->in_plaza && main_server->IsPlazaAlready(new_acc_cache->plaza_id))
                {
                    auto plaza = CPlaza.get<shared_t>(new_acc_cache->plaza_id);
                    auto equipped_items = main_server->GetEquippedItems(new_acc_cache);
                    auto equip_data = EquipInfoAck(new_acc_cache->uid, equipped_items);
                    new_acc_cache.unlock();
                    auto& players_ids = plaza->session_ids;
                    for (const auto& plaza_player_session_id : players_ids)
                    {
                        if (plaza_player_session_id == s_id) continue;
                        if (auto player_session = main_server->GetSessionById(plaza_player_session_id))
                        {
                            player_session->SendMsg(414, 0, selected_character, 17, reinterpret_cast<uint8_t*>(&equip_data), sizeof(EquipInfoAck));
                            player_session->SendMsg(314, 0, 0, voice_id, reinterpret_cast<uint8_t*>(&uid), sizeof(uid));
                        }
                    }
                    new_acc_cache.lock();
                }
            });
    }
}
