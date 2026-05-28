#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace BaseLib
{
    // ==================== Chat Logging ====================
    namespace ChatLog
    {
        enum class Type : uint8_t
        {
            User = 0,
            Whisper = 1,
            Team = 2,
            Clan = 3,
            Command = 4,
            Party = 5
        };

        enum class Location : uint8_t
        {
            Lobby = 0,
            Room = 1,
            Plaza = 2
        };

        inline const char* TypeToString(Type t)
        {
            switch (t)
            {
                case Type::User:    return "User";
                case Type::Whisper: return "Whisper";
                case Type::Team:    return "Team";
                case Type::Clan:    return "Clan";
                case Type::Command: return "Command";
                case Type::Party:   return "Party";
                default:            return "User";
            }
        }

        inline const char* LocationToString(Location l)
        {
            switch (l)
            {
                case Location::Lobby: return "Lobby";
                case Location::Room:  return "Room";
                case Location::Plaza: return "Plaza";
                default:              return "Lobby";
            }
        }
    }

    struct ChatLogEntry
    {
        int32_t aid{ 0 };
        std::optional<int32_t> target_aid;
        ChatLog::Type chat_type{ ChatLog::Type::User };
        std::optional<ChatLog::Location> location;
        uint32_t server_id{ 0 };
        std::optional<uint32_t> room_id;
        std::optional<uint32_t> plaza_id;
        std::optional<uint32_t> clan_id;
        std::string message;
    };

    // ==================== Item Logging ====================
    namespace ItemLog
    {
        enum class ActionType : uint8_t
        {
            Added = 0,
            Deleted = 1,
            Sold = 2,
            Upgraded = 3,
            Reset = 4,
            Repaired = 5,
            EnergyInjected = 6,
            Gifted = 7,
            Received = 8
        };

        enum class OriginType : uint8_t
        {
            Shop = 0,
            ShopCoupon = 1,
            Gachapon = 2,
            Package = 3,
            BossBattle = 4,
            Tutorial = 5,
            Story = 6,
            LevelUp = 7,
            DailyMission = 8,
            MonthlyReward = 9,
            GiftSent = 10,
            GiftReceived = 11,
            GMSpawned = 12,
            Pickup = 13,
            Unknown = 14
        };

        enum class ItemType : uint8_t
        {
            Hair = 0,
            Face = 1,
            Upper = 2,
            Under = 3,
            Pants = 4,
            Hands = 5,
            Boots = 6,
            AccHead = 7,
            AccWaist = 8,
            AccBack = 9,
            Melee = 10,
            Rifle = 11,
            Shotgun = 12,
            Sniper = 13,
            Gatling = 14,
            Bazooka = 15,
            Grenade = 16,
            Set = 17,
            ShieldEnamel = 18,
            FlagBlue = 19,
            Gatcha = 20,
            Unknown1 = 21,
            Diorama1 = 22,
            Diorama2 = 23,
            Question1 = 24,
            MonsterFace = 25,
            Unknown3 = 26,
            Unknown4 = 27
        };

        inline const char* ActionTypeToString(ActionType t)
        {
            switch (t)
            {
                case ActionType::Added:          return "Added";
                case ActionType::Deleted:        return "Deleted";
                case ActionType::Sold:           return "Sold";
                case ActionType::Upgraded:       return "Upgraded";
                case ActionType::Reset:          return "Reset";
                case ActionType::Repaired:       return "Repaired";
                case ActionType::EnergyInjected: return "EnergyInjected";
                case ActionType::Gifted:         return "Gifted";
                case ActionType::Received:       return "Received";
                default:                         return "Added";
            }
        }

        inline const char* OriginTypeToString(OriginType t)
        {
            switch (t)
            {
                case OriginType::Shop:          return "Shop";
                case OriginType::ShopCoupon:    return "ShopCoupon";
                case OriginType::Gachapon:      return "Gachapon";
                case OriginType::Package:       return "Package";
                case OriginType::BossBattle:    return "BossBattle";
                case OriginType::Tutorial:      return "Tutorial";
                case OriginType::Story:         return "Story";
                case OriginType::LevelUp:       return "LevelUp";
                case OriginType::DailyMission:  return "DailyMission";
                case OriginType::MonthlyReward: return "MonthlyReward";
                case OriginType::GiftSent:      return "GiftSent";
                case OriginType::GiftReceived:  return "GiftReceived";
                case OriginType::GMSpawned:     return "GMSpawned";
                case OriginType::Pickup:        return "Pickup";
                default:                        return "Unknown";
            }
        }

        inline const char* ItemTypeToString(ItemType t)
        {
            switch (t)
            {
                case ItemType::Hair:        return "Hair";
                case ItemType::Face:        return "Face";
                case ItemType::Upper:       return "Upper";
                case ItemType::Under:       return "Under";
                case ItemType::Pants:       return "Pants";
                case ItemType::Hands:       return "Hands";
                case ItemType::Boots:       return "Boots";
                case ItemType::AccHead:     return "AccHead";
                case ItemType::AccWaist:    return "AccWaist";
                case ItemType::AccBack:     return "AccBack";
                case ItemType::Melee:       return "Melee";
                case ItemType::Rifle:       return "Rifle";
                case ItemType::Shotgun:     return "Shotgun";
                case ItemType::Sniper:      return "Sniper";
                case ItemType::Gatling:     return "Gatling";
                case ItemType::Bazooka:     return "Bazooka";
                case ItemType::Grenade:     return "Grenade";
                case ItemType::Set:         return "Set";
                case ItemType::ShieldEnamel:return "ShieldEnamel";
                case ItemType::FlagBlue:    return "FlagBlue";
                case ItemType::Gatcha:      return "Gatcha";
                case ItemType::Unknown1:    return "Unknown1";
                case ItemType::Diorama1:    return "Diorama1";
                case ItemType::Diorama2:    return "Diorama2";
                case ItemType::Question1:   return "Question1";
                case ItemType::MonsterFace: return "MonsterFace";
                case ItemType::Unknown3:    return "Unknown3";
                case ItemType::Unknown4:    return "Unknown4";
                default:                    return "Unknown1";
            }
        }
    }

    
    struct ItemLogEntry
    {
        int32_t aid{ 0 };
        std::optional<int32_t> related_aid; 
        ItemLog::ActionType action_type{ ItemLog::ActionType::Added };
        uint32_t item_id{ 0 };
        std::optional<ItemLog::ItemType> item_type;
        std::optional<uint64_t> serial_info;
        ItemLog::OriginType origin_type{ ItemLog::OriginType::Unknown };
        int32_t mp_delta{ 0 };
        int32_t rt_delta{ 0 };
        int32_t coupon_delta{ 0 };
        int32_t energy_delta{ 0 };
        std::optional<uint32_t> new_item_id;
        std::optional<uint16_t> new_repair;
    };

   
    namespace CurrencyLog
    {
        enum class Type : uint8_t
        {
            MP = 0,
            RT = 1,
            Coupons = 2,
            Energy = 3
        };

        enum class SourceType : uint8_t
        {
            Shop = 0,
            ShopCoupon = 1,
            Gachapon = 2,
            Package = 3,
            ItemSell = 4,
            ItemRepair = 5,
            ItemUpgrade = 6,
            BossBattle = 7,
            Tutorial = 8,
            LevelUp = 9,
            Achievement = 10,
            DailyMission = 11,
            MonthlyReward = 12,
            GiftSend = 13,
            VoteKick = 14,
            MatchReward = 15,
            Admin = 16,
            Unknown = 17
        };

        inline const char* TypeToString(Type t)
        {
            switch (t)
            {
                case Type::MP:      return "MP";
                case Type::RT:      return "RT";
                case Type::Coupons: return "Coupons";
                case Type::Energy:  return "Energy";
                default:            return "MP";
            }
        }

        inline const char* SourceTypeToString(SourceType t)
        {
            switch (t)
            {
                case SourceType::Shop:          return "Shop";
                case SourceType::ShopCoupon:    return "ShopCoupon";
                case SourceType::Gachapon:      return "Gachapon";
                case SourceType::Package:       return "Package";
                case SourceType::ItemSell:      return "ItemSell";
                case SourceType::ItemRepair:    return "ItemRepair";
                case SourceType::ItemUpgrade:   return "ItemUpgrade";
                case SourceType::BossBattle:    return "BossBattle";
                case SourceType::Tutorial:      return "Tutorial";
                case SourceType::LevelUp:       return "LevelUp";
                case SourceType::Achievement:   return "Achievement";
                case SourceType::DailyMission:  return "DailyMission";
                case SourceType::MonthlyReward: return "MonthlyReward";
                case SourceType::GiftSend:      return "GiftSend";
                case SourceType::VoteKick:      return "VoteKick";
                case SourceType::MatchReward:   return "MatchReward";
                case SourceType::Admin:         return "Admin";
                default:                        return "Unknown";
            }
        }
    }

    struct CurrencyLogEntry
    {
        int32_t aid{ 0 };
        CurrencyLog::Type currency_type{ CurrencyLog::Type::MP };
        int32_t amount{ 0 }; // positive = gain, negative = loss
        uint64_t before_value{ 0 };
        uint64_t after_value{ 0 };
        CurrencyLog::SourceType source_type{ CurrencyLog::SourceType::Unknown };
        std::optional<uint32_t> related_item_id;
    };

    // ==================== Room Logging ====================
    namespace RoomLog
    {
        enum class EventType : uint8_t
        {
            RoomCreated = 0,
            RoomJoined = 1,
            RoomLeft = 2,
            RoomKicked = 3,
            TeamChanged = 4,
            VoteKickStarted = 5,
            VoteKickAgreed = 6,
            VoteKickSucceeded = 7,
            VoteKickFailed = 8
        };

        inline const char* EventTypeToString(EventType t)
        {
            switch (t)
            {
                case EventType::RoomCreated:        return "RoomCreated";
                case EventType::RoomJoined:         return "RoomJoined";
                case EventType::RoomLeft:           return "RoomLeft";
                case EventType::RoomKicked:         return "RoomKicked";
                case EventType::TeamChanged:        return "TeamChanged";
                case EventType::VoteKickStarted:    return "VoteKickStarted";
                case EventType::VoteKickAgreed:     return "VoteKickAgreed";
                case EventType::VoteKickSucceeded:  return "VoteKickSucceeded";
                case EventType::VoteKickFailed:     return "VoteKickFailed";
                default:                            return "RoomJoined";
            }
        }
    }

    struct RoomLogEntry
    {
        int32_t aid{ 0 };
        std::optional<int32_t> target_aid;
        RoomLog::EventType event_type{ RoomLog::EventType::RoomJoined };
        uint32_t server_id{ 0 };
        uint32_t room_id{ 0 };
        std::optional<int32_t> host_aid;
        std::optional<uint8_t> team_id;
        std::optional<uint8_t> new_team_id;
        std::optional<uint8_t> votekick_reason;
    };

    // ==================== Anticheat Detection Log ====================
    namespace AcDetection
    {
        inline constexpr uint32_t kServerFlagBase = 0x40000000u;

        enum class Flag : uint32_t
        {
            None                     = 0,
            DebuggerPresent          = (1u << 0),
            DebugPort                = (1u << 1),
            TimingAnomaly            = (1u << 2),
            PebDebugFlag             = (1u << 3),
            InlineHook               = (1u << 4),
            IatHook                  = (1u << 5),
            HoneypotTriggered        = (1u << 6),
            IntegrityViolation       = (1u << 7),
            DllInjection             = (1u << 8),
            ManualMap                = (1u << 9),
            AnonymousThread          = (1u << 10),
            ProxyDll                 = (1u << 11),
            GlobalHookInjection      = (1u << 12),
            MappedImage              = (1u << 13),
            HookIntegrity            = (1u << 14),
            BlacklistedModule        = (1u << 15),
            UnsignedModule           = (1u << 16),
            DangerousHandle          = (1u << 17),
            VulnerableDriver         = (1u << 18),
            BlacklistedString        = (1u << 19),
            BlacklistedSignature     = (1u << 20),
            UnsignedDriver           = (1u << 21),
            DriverBlocklistDisabled  = (1u << 22),
            HvciDisabled             = (1u << 23),

            FileIntegrityFail          = kServerFlagBase + 1u,
            MatchKillMismatch         = kServerFlagBase + 2u,
            LoginSpam                 = kServerFlagBase + 3u,
            HeartbeatTimeout          = kServerFlagBase + 4u,
            InvalidResponse           = kServerFlagBase + 5u,
            UnknownFlag               = kServerFlagBase + 255u
        };

        inline constexpr Flag kClientFlags[] = {
            Flag::DebuggerPresent,
            Flag::DebugPort,
            Flag::TimingAnomaly,
            Flag::PebDebugFlag,
            Flag::InlineHook,
            Flag::IatHook,
            Flag::HoneypotTriggered,
            Flag::IntegrityViolation,
            Flag::DllInjection,
            Flag::ManualMap,
            Flag::AnonymousThread,
            Flag::ProxyDll,
            Flag::GlobalHookInjection,
            Flag::MappedImage,
            Flag::HookIntegrity,
            Flag::BlacklistedModule,
            Flag::UnsignedModule,
            Flag::DangerousHandle,
            Flag::VulnerableDriver,
            Flag::BlacklistedString,
            Flag::BlacklistedSignature,
            Flag::UnsignedDriver,
            Flag::DriverBlocklistDisabled,
            Flag::HvciDisabled,
        };

        inline constexpr uint32_t FlagToValue(Flag f)
        {
            return static_cast<uint32_t>(f);
        }

        inline constexpr Flag FlagFromValue(uint32_t value)
        {
            switch (value)
            {
                case FlagToValue(Flag::None):                    return Flag::None;
                case FlagToValue(Flag::DebuggerPresent):         return Flag::DebuggerPresent;
                case FlagToValue(Flag::DebugPort):               return Flag::DebugPort;
                case FlagToValue(Flag::TimingAnomaly):           return Flag::TimingAnomaly;
                case FlagToValue(Flag::PebDebugFlag):            return Flag::PebDebugFlag;
                case FlagToValue(Flag::InlineHook):              return Flag::InlineHook;
                case FlagToValue(Flag::IatHook):                 return Flag::IatHook;
                case FlagToValue(Flag::HoneypotTriggered):       return Flag::HoneypotTriggered;
                case FlagToValue(Flag::IntegrityViolation):      return Flag::IntegrityViolation;
                case FlagToValue(Flag::DllInjection):            return Flag::DllInjection;
                case FlagToValue(Flag::ManualMap):               return Flag::ManualMap;
                case FlagToValue(Flag::AnonymousThread):         return Flag::AnonymousThread;
                case FlagToValue(Flag::ProxyDll):                return Flag::ProxyDll;
                case FlagToValue(Flag::GlobalHookInjection):     return Flag::GlobalHookInjection;
                case FlagToValue(Flag::MappedImage):             return Flag::MappedImage;
                case FlagToValue(Flag::HookIntegrity):           return Flag::HookIntegrity;
                case FlagToValue(Flag::BlacklistedModule):       return Flag::BlacklistedModule;
                case FlagToValue(Flag::UnsignedModule):          return Flag::UnsignedModule;
                case FlagToValue(Flag::DangerousHandle):         return Flag::DangerousHandle;
                case FlagToValue(Flag::VulnerableDriver):        return Flag::VulnerableDriver;
                case FlagToValue(Flag::BlacklistedString):       return Flag::BlacklistedString;
                case FlagToValue(Flag::BlacklistedSignature):    return Flag::BlacklistedSignature;
                case FlagToValue(Flag::UnsignedDriver):          return Flag::UnsignedDriver;
                case FlagToValue(Flag::DriverBlocklistDisabled): return Flag::DriverBlocklistDisabled;
                case FlagToValue(Flag::HvciDisabled):            return Flag::HvciDisabled;
                case FlagToValue(Flag::FileIntegrityFail):       return Flag::FileIntegrityFail;
                case FlagToValue(Flag::MatchKillMismatch):       return Flag::MatchKillMismatch;
                case FlagToValue(Flag::LoginSpam):               return Flag::LoginSpam;
                case FlagToValue(Flag::HeartbeatTimeout):        return Flag::HeartbeatTimeout;
                case FlagToValue(Flag::InvalidResponse):         return Flag::InvalidResponse;
                default:                                         return Flag::UnknownFlag;
            }
        }

        inline std::vector<Flag> ExpandRawFlags(uint32_t value)
        {
            std::vector<Flag> flags;
            if (value == 0)
                return flags;

            if (const auto exact = FlagFromValue(value); exact != Flag::UnknownFlag && exact != Flag::None)
            {
                flags.push_back(exact);
                return flags;
            }

            uint32_t matched_mask = 0;
            for (const auto flag : kClientFlags)
            {
                const auto flag_value = FlagToValue(flag);
                if ((value & flag_value) == flag_value)
                {
                    flags.push_back(flag);
                    matched_mask |= flag_value;
                }
            }

            if (flags.empty() || matched_mask != value)
                flags.push_back(Flag::UnknownFlag);

            return flags;
        }

        inline const char* FlagToString(Flag f)
        {
            switch (f)
            {
                case Flag::None:                     return "None";
                case Flag::DebuggerPresent:          return "DebuggerPresent";
                case Flag::DebugPort:                return "DebugPort";
                case Flag::TimingAnomaly:            return "TimingAnomaly";
                case Flag::PebDebugFlag:             return "PebDebugFlag";
                case Flag::InlineHook:               return "InlineHook";
                case Flag::IatHook:                  return "IatHook";
                case Flag::HoneypotTriggered:        return "HoneypotTriggered";
                case Flag::IntegrityViolation:       return "IntegrityViolation";
                case Flag::DllInjection:             return "DllInjection";
                case Flag::ManualMap:                return "ManualMap";
                case Flag::AnonymousThread:          return "AnonymousThread";
                case Flag::ProxyDll:                 return "ProxyDll";
                case Flag::GlobalHookInjection:      return "GlobalHookInjection";
                case Flag::MappedImage:              return "MappedImage";
                case Flag::HookIntegrity:            return "HookIntegrity";
                case Flag::BlacklistedModule:        return "BlacklistedModule";
                case Flag::UnsignedModule:           return "UnsignedModule";
                case Flag::DangerousHandle:          return "DangerousHandle";
                case Flag::VulnerableDriver:         return "VulnerableDriver";
                case Flag::BlacklistedString:        return "BlacklistedString";
                case Flag::BlacklistedSignature:     return "BlacklistedSignature";
                case Flag::UnsignedDriver:           return "UnsignedDriver";
                case Flag::DriverBlocklistDisabled:  return "DriverBlocklistDisabled";
                case Flag::HvciDisabled:             return "HvciDisabled";
                case Flag::FileIntegrityFail:   return "FileIntegrityFail";
                case Flag::MatchKillMismatch:   return "MatchKillMismatch";
                case Flag::LoginSpam:           return "LoginSpam";
                case Flag::HeartbeatTimeout:    return "HeartbeatTimeout";
                case Flag::InvalidResponse:     return "InvalidResponse";
                default:                        return "UnknownFlag";
            }
        }
    }

    struct AcDetectionLogEntry
    {
        int32_t aid{ 0 };
        std::string ip;
        std::string hwid;
        AcDetection::Flag detection_flag{ AcDetection::Flag::UnknownFlag };
        uint32_t extra{ 0 };
        std::string details;
        uint32_t server_id{ 0 };
    };

    // ==================== Auth History Log ====================
    struct AuthHistoryLogEntry
    {
        int32_t aid{ 0 };
        std::string ip;
        std::string hwid;
        uint32_t server_id{ 0 };
    };

    // ==================== Aggregated Log Context ====================
    struct LogContext
    {
        std::vector<ChatLogEntry> chat_logs;
        std::vector<ItemLogEntry> item_logs;
        std::vector<CurrencyLogEntry> currency_logs;
        std::vector<RoomLogEntry> room_logs;
        std::vector<AcDetectionLogEntry> ac_detection_logs;

        void clear()
        {
            chat_logs.clear();
            item_logs.clear();
            currency_logs.clear();
            room_logs.clear();
            ac_detection_logs.clear();
        }

        bool empty() const
        {
            return chat_logs.empty() && item_logs.empty() && 
                   currency_logs.empty() && room_logs.empty() &&
                   ac_detection_logs.empty();
        }
    };
}
