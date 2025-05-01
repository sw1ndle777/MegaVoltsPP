#pragma once
namespace Game
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;

    namespace Handlers
    {
        inline void PlayerMovement(SCallbackData& callback, CCastServer* cast_server)
        {
            auto session = callback.session;
            auto message = callback.message;
            if (!session || !message) return;

            std::shared_lock lock(session->GetMutex());
            CServer* server = callback.server;
            auto option = message->GetOption();
            auto extra = message->GetExtra();
            auto mission = message->GetMission();
            auto self_session_id = session->GetSessionId();
            auto self_player = cast_server->GetPlayerCacheShared(self_session_id);
            auto in_room = self_player->in_room;
            auto in_plaza = self_player->in_plaza;
            auto room_id = self_player->room_id;
            auto plaza_id = self_player->plaza_id;
            auto is_dead = self_player->health == 0;
            self_player.unlock();
            auto data_size =message->GetDataSize();
            auto room = cast_server->GetRoomCacheShared(room_id);

            CMessage movementMsg = CMessage();
            movementMsg.SetSession(self_session_id);
            movementMsg.SetCommand(322, 0, 0, 1);

            ClientPlayerInfoBasic* player_info = (ClientPlayerInfoBasic*)message->GetData();

            if (data_size == 28)
            {
                ClientPlayerInfoBullet* bullets_info = (ClientPlayerInfoBullet*)message->GetData();

                PlayerInfoResponseWithBullets player_info_bullets;
                player_info_bullets.specificInfo.enableBullet = true;

                player_info_bullets.tick = player_info->matchTick;
                player_info_bullets.position = player_info->position;
                player_info_bullets.direction = player_info->direction;
                player_info_bullets.specificInfo.animation1 = player_info->animation1;
                player_info_bullets.specificInfo.animation2 = player_info->animation2;
                player_info_bullets.rotation1 = extra;
                player_info_bullets.rotation2 = option;
                player_info_bullets.rotation3 = player_info->rotation;
                player_info_bullets.specificInfo.sessionId = static_cast<uint32_t>(self_session_id);
                player_info_bullets.bullets = bullets_info->bulletStruct;
                player_info_bullets.currentWeapon = bullets_info->bulletStruct.bullet4;
                movementMsg.SetData(reinterpret_cast<uint8_t*>(&player_info_bullets), sizeof(player_info_bullets));
            }
            else if (data_size == 32)
            {
                ClientPlayerInfoComplete* player_info_complete = (ClientPlayerInfoComplete*)message->GetData();
                ClientPlayerInfoBullet* bullets_info = (ClientPlayerInfoBullet*)message->GetData();

                PlayerInfoResponseWithBullets player_info_bullets;
                player_info_bullets.specificInfo.enableBullet = true;

                player_info_bullets.tick = player_info->matchTick;
                player_info_bullets.position = player_info->position;
                player_info_bullets.direction = player_info->direction;
                player_info_bullets.specificInfo.animation1 = player_info->animation1;
                player_info_bullets.specificInfo.animation2 = player_info->animation2;
                player_info_bullets.rotation1 = extra;
                player_info_bullets.rotation2 = option;
                player_info_bullets.rotation3 = player_info->rotation;
                player_info_bullets.specificInfo.sessionId = static_cast<uint32_t>(self_session_id);
                player_info_bullets.bullets = bullets_info->bulletStruct;
                player_info_bullets.currentWeapon = bullets_info->bulletStruct.bullet4;

                PlayerInfoResponseComplete new_player_info_complete{ player_info_bullets };
                new_player_info_complete.playerInfoBasicResponse.specificInfo.enableJump = true;
                new_player_info_complete.jump = player_info_complete->jumpStruct;
                movementMsg.SetData(reinterpret_cast<uint8_t*>(&new_player_info_complete), sizeof(new_player_info_complete));
            }
            else
            {
                PlayerInfoBasicResponse player_info_basic;
                player_info_basic.tick = player_info->matchTick;
                player_info_basic.position = player_info->position;
                player_info_basic.direction = player_info->direction;
                player_info_basic.currentWeapon = player_info->weapon;
                player_info_basic.specificInfo.animation1 = player_info->animation1;
                player_info_basic.specificInfo.animation2 = player_info->animation2;
                player_info_basic.rotation1 = extra;
                player_info_basic.rotation2 = option;
                player_info_basic.rotation3 = player_info->rotation;
                player_info_basic.specificInfo.sessionId = static_cast<uint32_t>(self_session_id);

                if (data_size == 20)
                    movementMsg.SetData(reinterpret_cast<uint8_t*>(&player_info_basic), sizeof(player_info_basic));
                else if (data_size == 24)
                {
                    player_info_basic.specificInfo.enableJump = true;
                    PlayerInfoResponseWithJump player_info_jump{ player_info_basic };
                    ClientPlayerInfoJump* jump_info = (ClientPlayerInfoJump*)callback.message->GetData();
                    player_info_jump.jump = jump_info->jumpStruct;
                    movementMsg.SetData(reinterpret_cast<uint8_t*>(&player_info_jump), sizeof(player_info_jump));
                }
            }

            if (in_room)
            {
                auto room = cast_server->GetRoomCacheShared(room_id);
                auto& players = room->players_session_id;
                
                if (!is_dead)
                {
                    for (const auto& id : players)
                    {
                        movementMsg.SetEncryptMethod(SendOption::EncryptionMethod::None);
                        movementMsg.SetSession(id);
                        if (auto player_session = server->GetSessionById(id))
                            player_session->Send(movementMsg);
                        else
                            BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "couldn't broadcast packet to session id: ({})", id);
                    }
                }
                room.unlock();
            }
            else if (in_plaza)
            {
                auto plaza = cast_server->GetPlazaCacheShared(plaza_id);
                auto& players = plaza->players_session_id;
                
                for (const auto& id : players)
                {
                    if (id == self_session_id) continue;
                    movementMsg.SetEncryptMethod(SendOption::EncryptionMethod::None);
                    movementMsg.SetSession(id);
                    if (auto player_session = server->GetSessionById(id))
                        player_session->Send(movementMsg);
                    else
                        BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::red, "couldn't broadcast packet to session id: ({})", id);
                }
                plaza.unlock();
            }
        }
    }
}