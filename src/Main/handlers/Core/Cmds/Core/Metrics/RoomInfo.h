#pragma once
namespace Game::Commands
{
    struct RoomInfoCommand
    {
        static constexpr std::string_view name = "roominfo";
        static constexpr uint8_t required_grade = Userlist::User::Grade::Tester;

        static void Run(std::span<const std::string_view> args, CommandContext& ctx)
        {

            if (!ctx.acc_cache->in_room)
            {
				ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] You are not in a room.");
				ctx.acc_cache.unlock();
                return;
            }
            if (!CRoom.contains(ctx.acc_cache->room_id))
            {
				ctx.server->SendServerMessage(ctx.callback.session, "[MegaVolts Online] Room not found.");
                ctx.acc_cache.unlock();
				return;
            }

            auto room = CRoom.get<shared_t>(ctx.acc_cache->room_id);
            ctx.acc_cache.unlock();
            auto ids = ctx.server->GetRoomSortedPlayerSessionIds(room);

            ctx.server->SendServerMessage(ctx.callback.session, std::format("[MegaVolts Online] Rooms Info: {} players, mode: {}", ids.size(), static_cast<uint8_t>(room->ModeIndex)).c_str());
            if (room->has_password)
                ctx.server->SendServerMessage(ctx.callback.session, std::format("RoomId: {} - Title: {} - Password: {}", room->room_id, room->title.c_str(), room->password.c_str()));
            else
                ctx.server->SendServerMessage(ctx.callback.session, std::format("RoomId: {} - Title: {}", room->room_id, room->title.c_str()).c_str());

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
                ctx.server->SendServerMessage(ctx.callback.session, msg.c_str());
            }
        };

        inline static CommandRegister<RoomInfoCommand> reg{};
    };
}