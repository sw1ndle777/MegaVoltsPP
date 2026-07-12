#pragma once
#include <atomic>
#include <chrono>
#include <cstring>
namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Cast;
    inline void UserMovement(SCallbackData& callback, CCastServer* server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        // --- DIAGNOSTIC: USER_MOVE server-receive rate (disabled) ---
        // Compare against the client's "[CustomTickrate] 128-tick send patches applied: N/21".
        //   ~128*players/sec here  => client IS sending at 128-tick; look downstream (rebroadcast).
        //   ~12*players/sec here   => client is sending at VANILLA rate (send patch not live) = the bug.
#if 0
        {
            static std::atomic<uint32_t> s_moveCount{ 0 };
            static std::atomic<int64_t>  s_lastLogMs{ 0 };
            s_moveCount.fetch_add(1, std::memory_order_relaxed);
            const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            auto last = s_lastLogMs.load(std::memory_order_relaxed);
            if (nowMs - last >= 1000 && s_lastLogMs.compare_exchange_strong(last, nowMs))
            {
                const auto cnt = s_moveCount.exchange(0, std::memory_order_relaxed);
                DEBUGLOG(dark_green, "[MoveRate] USER_MOVE received: ({}) packets/sec across all sessions", cnt);
            }
        }
#endif

        auto option = message->GetOption();
        auto extra = message->GetExtra();
        auto sid = session->GetSessionId();
        auto acc = CAccount.get<shared_t>(sid);
        auto in_room = acc->in_room;
        auto in_plaza = acc->in_plaza;
        auto room_id = acc->room_id;
        auto plaza_id = acc->plaza_id;
        auto is_dead = !acc->health;
        acc.unlock();
        auto data_size = message->GetDataSize();

        CMessage movementMsg = CMessage();
        movementMsg.SetSession(sid);
        movementMsg.SetCommand(322, 0, 0, 1);
        if (data_size == sizeof(ClientPlayerInfoBasic))
        {
			auto req = message->GetData<ClientPlayerInfoBasic*>();
            PlayerInfoBasicResponse response;
            response.tick = req->matchTick;
            response.position = req->position;
            response.direction = req->direction;
            response.currentWeapon = req->weapon;
            response.specificInfo.animation1 = req->animation1;
            response.specificInfo.animation2 = req->animation2;
            response.rotation1 = extra;
            response.rotation2 = option;
            response.rotation3 = req->rotation;
            response.specificInfo.sessionId = sid;

            movementMsg.SetData(reinterpret_cast<uint8_t*>(&response), sizeof(response));
        }
        else if (data_size == sizeof(ClientPlayerInfoJump))
        {
            auto req = message->GetData<ClientPlayerInfoJump*>();
            PlayerInfoBasicResponse info_response;
            info_response.tick = req->player.matchTick;
            info_response.position = req->player.position;
            info_response.direction = req->player.direction;
            info_response.currentWeapon = req->player.weapon;
            info_response.specificInfo.animation1 = req->player.animation1;
            info_response.specificInfo.animation2 = req->player.animation2;
            info_response.rotation1 = extra;
            info_response.rotation2 = option;
            info_response.rotation3 = req->player.rotation;
            info_response.specificInfo.sessionId = sid;
            info_response.specificInfo.enableJump = true;

            PlayerInfoResponseWithJump response{ info_response };
            response.jump = req->jumpStruct;

            movementMsg.SetData(reinterpret_cast<uint8_t*>(&response), sizeof(response));
        }
        else if (data_size == sizeof(ClientPlayerInfoBullet))
        {
            auto req = message->GetData<ClientPlayerInfoBullet*>();

            PlayerInfoResponseWithBullets response;
            response.specificInfo.enableBullet = true;
            response.tick = req->player.matchTick;
            response.position = req->player.position;
            response.direction = req->player.direction;
            response.specificInfo.animation1 = req->player.animation1;
            response.specificInfo.animation2 = req->player.animation2;
            response.rotation1 = extra;
            response.rotation2 = option;
            response.rotation3 = req->player.rotation;
            response.specificInfo.sessionId = sid;
            response.bullets = req->bulletStruct;
            response.currentWeapon = req->bulletStruct.bullet4;

            movementMsg.SetData(reinterpret_cast<uint8_t*>(&response), sizeof(response));
        }
        else if (data_size == sizeof(ClientPlayerInfoComplete))
        {
			auto req = message->GetData<ClientPlayerInfoComplete*>();

            PlayerInfoResponseWithBullets bullet_response;
            bullet_response.specificInfo.enableBullet = true;
            bullet_response.tick = req->player.matchTick;
            bullet_response.position = req->player.position;
            bullet_response.direction = req->player.direction;
            bullet_response.specificInfo.animation1 = req->player.animation1;
            bullet_response.specificInfo.animation2 = req->player.animation2;
            bullet_response.rotation1 = extra;
            bullet_response.rotation2 = option;
            bullet_response.rotation3 = req->player.rotation;
            bullet_response.specificInfo.sessionId = sid;
            bullet_response.bullets = req->bulletStruct;
            bullet_response.currentWeapon = req->bulletStruct.bullet4;

            PlayerInfoResponseComplete response{ bullet_response };
            response.playerInfoBasicResponse.specificInfo.enableJump = true;
            response.jump = req->jumpStruct;

            movementMsg.SetData(reinterpret_cast<uint8_t*>(&response), sizeof(response));
        }

        if (in_room && !is_dead)
        {
            // Batched path: queue this player's entry (the response body minus its
            // leading 4-byte tick) for the next fixed-rate flush, which emits one
            // multi-entry cmd-322 per room. See MovementBatcher.h.
            if (server->IsMoveBatchEnabled() && movementMsg.GetDataSize() >= sizeof(uint32_t))
            {
                auto* d = movementMsg.GetData();
                const auto dsz = movementMsg.GetDataSize();
                uint32_t tick = 0;
                std::memcpy(&tick, d, sizeof(uint32_t));
                server->SubmitMovement(room_id, sid, d + sizeof(uint32_t),
                    static_cast<uint8_t>(dsz - sizeof(uint32_t)), tick);
            }
            else
            {
                auto room = CRoom.get<shared_t>(room_id);
                if (!room) return;
                auto player_ids = room->players_session_id;
                room.unlock();
                server->Broadcast(player_ids, movementMsg);
            }
        }
        else if (in_plaza)
        {
            auto plaza = CPlaza.get<shared_t>(plaza_id);
            if (!plaza) return;
            auto player_ids = plaza->players_session_id;
            plaza.unlock();
            server->Broadcast(player_ids, movementMsg, sid);
        }

    }
}