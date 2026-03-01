#pragma once
#include <stdint.h>
#include <optional>
#include <string_view>
#include <memory>
#include <type_traits>
#include <utility>

using AuthKey32 = std::array<uint8_t, 32>;

namespace NetEngine
{



    template <typename T1, typename T2>
    struct LockedResource
    {
        T1 lockGuard;
        T2& resource;

        LockedResource(T1&& lockGuard, T2& resource)
            : lockGuard(std::move(lockGuard)), resource(resource) {}

        T2* operator->() {
            return &resource;
        }
        auto& operator*() {
            return resource;
        }
        void unlock() {
            lockGuard.unlock();
        }
        void lock() {
            lockGuard.lock();
        }
    };

    class CSession;
    class CMessage;
    class CServer;

    struct SCallbackData
    {
        CSession* session;
        CMessage* message;
        CServer* server;
    };

    template<class T, class U>
    concept EnumOf = std::is_enum_v<T> && std::same_as<std::underlying_type_t<T>, U>;

    template<class T>
    concept Any16 = EnumOf<T, uint16_t> || std::integral<T> || std::unsigned_integral<T>;

    template<class T>
    concept Any8 = EnumOf<T, uint8_t> || std::integral<T> || std::unsigned_integral<T>;

    template<class T>
    constexpr auto to_u(T v) noexcept
    {
        if constexpr (std::is_enum_v<std::remove_cvref_t<T>>)
            return std::to_underlying(v);
        else
            return v;
    }

    template <typename Out, typename T>
    constexpr Out field_cast(T v) 
    {
        static_assert(std::is_integral_v<Out>, "Out must be an integral type");
        if constexpr (std::is_enum_v<std::remove_cvref_t<T>>)
            return static_cast<Out>(std::to_underlying(v));
        else
            return static_cast<Out>(v);
    }

    template <typename T> constexpr uint8_t  u8_cast(T v) { return field_cast<uint8_t>(v); }
    template <typename T> constexpr uint16_t u16_cast(T v) { return field_cast<uint16_t>(v); }
    template <typename T> constexpr uint32_t u32_cast(T v) { return field_cast<uint32_t>(v); }


    namespace PacketId
    {
        namespace Front
        {
            enum class GsToCl : uint16_t
            {
                Authorize = 22,
                ChannelInfo = 23,
                EngineConnectionInit = 401
            };
            enum class ClToGs : uint16_t
            {
                Authorize = 22,
                ChannelInfo = 23,
                Reconnect = 25
            };
        }
    }

    namespace PacketIds
    {
        namespace Ipc
        {
            enum : uint32_t
            {
                MainToCastDisconnectPlayer = 1,
                MainToFrontDisconnectPlayer = 2,
                FrontToMainDisconnectPlayer = 3,
                MainToCastHostChange = 4,
                MainToCastReqServerInfo = 5,
                CastToMainAckServerInfo = 6,
                CastToMainPlayerAuthorizeInfo = 7,
                MainToCastSendPingAssure = 8,
                MainToCastSendPacket = 9,
                MainToCastAuthorizePlayer = 10,
				FrontToMainTryLoginPlayer = 11,
                MainToFrontAcknowledgeAidOnline = 12,
				MainToFrontAcknowledgeAidDisconnected = 13,
				CastToMainAcknowledgeAuthPlayer = 14
            };
        }

    }
    namespace NicknameChange
    {
        enum Errors : uint8_t
        {
            AVA_CREATE_OVERLAPPEDNAME = 5,
            ID_CREATE_PLAYER_FULL = 7,
            ID_CREATE_PLAYER_DONT_EXIST = 13,
            AVA_CREATE_SHORTNAME = 14,
            ID_CREATE_NO_PERMISSION = 16
        };
    }

    namespace Cryptography
    {
        enum EncryptionType : uint32_t
        {
            NO_ENCRYPTION = 0,
            DEFAULT_ENCRYPTION = 1,
            USER_ENCRYPTION = 2,
            DEFAULT_LARGE_ENCRYPTION = 3,
            USER_LARGE_ENCRYPTION = 4
        };
    }
    namespace Character
    {
        enum Type : uint8_t
        {
            Naomi = 0,
            Kai = 1,
            Pandora = 2,
            CHIP = 3,
            Knox = 4,
            Simon = 5,
            Amelia = 6,
            Sharkill = 7,
            Sophitia = 8
        };
    }
    namespace Announcement
    {
        enum Gacha : uint8_t
        {
            LuckyNotice = 0x00,
            RareNotice = 0x01
        };
        namespace Chat
        {
            enum Type : uint8_t
            {
                Announce = 0x0A,
                Unknown = 0x0C,
                GameMessage = 0x01
            };
        }
        
    }
    namespace Items
    {
        enum Origin : uint32_t // original origin used inside item serial by old devs
        {
            From_Game = 0,
            From_Event = 4,
            From_Dev_Tool = 5,
            From_Web_Shop = 6,
            From_GM_Spawn = 8
        };

        enum class OriginLog : uint8_t
        {
            DEV_TOOL = 0,
            WEB_SHOP = 1,
            WEB_SUPPORT = 2,
            GM_SPAWN = 3,
            GAME_SHOP = 4,
            GAME_GACHA = 5,
            GAME_PACKAGE = 6,
            TRADE = 7,
            EVENT = 8,
            TOURNAMENT = 9,
            GIFT = 10,
            MONTHLY_REWARD = 11,
            DAILY_MISSION = 12
        };

        namespace WeaponItems
        {
            enum Type : uint32_t
            {
                Melee = 10,
                Rifle = 11,
                Shotgun = 12,
                Sniper = 13,
                Gatling = 14,
                Bazooka = 15,
                Grenade = 16
            };
        }

        namespace CostumeItems
        {
            enum Type : uint32_t
            {
                Hair = 0,
                Face = 1,
                Upper = 2,
                Under = 3,
                Pants = 4,
                Shirt = 5,
                Boots = 6,
                Glasses = 7,
                AccBack1 = 8,
                AccBack2 = 9,
            };
        }

        namespace OtherItems
        {
            enum Type : uint32_t
            {
                Question = 17,
                ShieldEnamel = 18,
                FlagBlue = 19,
                BombDrop = 20,
                GatchaItem = 21,
                Question1 = 24,
                MonsterFace = 25,
                Undefined1 = 26,
                Undefined2 = 27
            };
        }

        namespace DioramaItems
        {
            enum Type : uint32_t
            {
                Footing = 22,
                Object = 23
            };
        }

        namespace Upgrade
        {
            enum Type : uint32_t
            {
                NoUpgrade = 0,
                UpgradeType1 = 1,
                UpgradeType2 = 2,
                UpgradeType3 = 3
            };
            enum Result : uint8_t
            {
                UpgradeSuccess = 0x01,
                UpgradeFailHigh = 0x02,
                UpgradeFailLow = 0x06,
                EnergyInjection = 0x20,
                UpgradeReset = 0x35,
                NotEnoughPoints = 0x0E,
                RepairItem = 0x08
            };
            enum FailType : uint8_t
            {
                Destroy = 0x00,
                NoChange = 0x01
            };

        }

        namespace Gachapon
        {
            namespace Spin
            {
                enum Result : uint8_t
                {
                    SpinSuccess = 0x01,
                    InventoryFull = 0x07,
                    MoneyError = 0x0E,
                    Stuck = 0x08
                };
                enum Type : uint8_t
                {
                    LuckySpin = 0x00,
                    NormalSpin = 0x01,
                    NormalSpinSale = 0x02
                };
            }
            enum Type : uint32_t
            {
                Coin = 0,
                RT = 1,
                MP = 2
            };
            enum Error : uint8_t
            {
                NoRT = 0x01,
                NoMP = 0x02,
                NoCoin = 0x03
            };
            enum Rarity : uint32_t
            {
                Normal = 0,
                Rare = 1,
                Etc = 2
            };
            enum LuckyType : uint32_t
            {
                NoLucky = 0,
                GoldLucky = 1,
                SilverLucky = 2,
                CopperLucky = 3
            };
        }

        namespace Package
        {
            enum Result : uint8_t
            {
                Package = 0x00,
                Capsule = 0x1A,
                StaticItems = 0xFF,

                MSG_NICKNAME_CHANGE_FAIL = 0x04,
                MSG_NICKNAME_CHANGE_SUCCESS = 0x35,

                DailyGiftError = 0x02,
                PACKAGE_INVEN_FULL = 0x07,
                CoinMax = 0x13,
                Unknown1 = 0x08,
                Unknown2 = 0x23,
                INITIALIZE_WIN_LOSE_COMPLETE = 0x95,
                INITIALIZE_KILL_DEATH_COMPLETE = 0x96,
                MSG_ITEM_VOICE_OPEN = 0x9F
            };

            enum class CoinItemId : uint32_t 
            {
                COIN_1   = 4308001,
                COIN_2   = 4308002,
                COIN_3   = 4308003,
                COIN_4   = 4308004,
                COIN_5   = 4308005,
                COIN_6   = 4308006,
                COIN_7   = 4308007,
                COIN_8   = 4308008,
                COIN_9   = 4308009,
                COIN_10  = 4308010,
                COIN_20  = 4308011,
                COIN_30  = 4308012,
                COIN_40  = 4308013,
                COIN_50  = 4308014,
                COIN_60  = 4308015,
                COIN_70  = 4308016,
                COIN_80  = 4308017,
                COIN_90  = 4308018,
                COIN_100 = 4308019,
            };

            enum class CouponItemId : uint32_t 
            {
                COUPON_1    = 4305019,
                COUPON_5    = 4305020,
                COUPON_10   = 4305021,
                COUPON_15   = 4305022,
                COUPON_20   = 4305023,
                COUPON_25   = 4305024,
                COUPON_30   = 4305025,
                COUPON_1_1  = 4305026,
                COUPON_2    = 4305027,
                COUPON_3    = 4305028,
                COUPON_4    = 4305029,
                COUPON_6    = 4305030,
                COUPON_7    = 4305031,
                COUPON_8    = 4305032,
                COUPON_9    = 4305033,
                COUPON_40   = 4305034,
                COUPON_50   = 4305035,
                COUPON_100  = 4305036,
                COUPON_SLOT = 1000000
            };

            enum class MicroPointsItemId : uint32_t 
            {
                POINTS_100      = 4400001,
                POINTS_200      = 4400002,
                POINTS_300      = 4400003,
                POINTS_400      = 4400004,
                POINTS_500      = 4400005,
                POINTS_600      = 4400006,
                POINTS_700      = 4400007,
                POINTS_800      = 4400008,
                POINTS_900      = 4400009,
                POINTS_1000     = 4400010,
                POINTS_1100     = 4400011,
                POINTS_1200     = 4400012,
                POINTS_1300     = 4400013,
                POINTS_1400     = 4400014,
                POINTS_1500     = 4400015,
                POINTS_1600     = 4400016,
                POINTS_1700     = 4400017,
                POINTS_1800     = 4400018,
                POINTS_1900     = 4400019,
                POINTS_2000     = 4400020,
                POINTS_3000     = 4400030,
                POINTS_3500     = 4400035,
                POINTS_4000     = 4400040,
                POINTS_5000     = 4400050,
                POINTS_6000     = 4400060,
                POINTS_7000     = 4400070,
                POINTS_8000     = 4400080,
                POINTS_9000     = 4400090,
                POINTS_10000    = 4400100,
                POINTS_20000    = 4400200,
                POINTS_30000    = 4400300,
                POINTS_50000    = 4400500,
                POINTS_100000   = 4401000,
                POINTS_150000   = 4401500,
                POINTS_500000   = 4405000,
                POINTS_1000000  = 4410000,
            };

            enum class VoiceItemId : uint32_t 
            {
                NAOMI_A   = 4810000,
                NAOMI_B   = 4810001,
                NAOMI_C   = 4810002,
                NAOMI_D   = 4810003,
                KNOX_A    = 4810004,
                KNOX_B    = 4810005,
                KNOX_C    = 4810006,
                KNOX_D    = 4810007,
                PANDORA_A = 4810008,
                PANDORA_B = 4810009,
                PANDORA_C = 4810010,
                PANDORA_D = 4810011,
                CHIP_A    = 4810012,
                CHIP_B    = 4810013,
                CHIP_C    = 4810014,
                CHIP_D    = 4810015,
            };

            enum class ItemIds : uint32_t
            {
                WIN_LOSE_RESET        = 4302000,
				KILL_DEATH_RESET      = 4303000,
				INV_EXPAND_10         = 4305000,
                INV_EXPAND_20         = 4305001,
                INV_EXPAND_40         = 4305002,
                INV_EXPAND_80         = 4305003,
                BATTERY_RECHARGE_500  = 4305005,
                BATTERY_RECHARGE_1000 = 4305006,
				BATTERY_EXPAND        = 4305007,
                COUPON_1_PACKAGE      = 4306001
            };
        }
    }

    namespace EquipUpdate
    {
        enum Type : uint8_t
        {
            Multiple = 0x00,
            Sigle = 0x01
        };
    }

    namespace Socials
    {
        enum State : uint8_t
        {
            Accepted = 0,
            Pending = 1,
            Blocked = 2
        };
    }
    namespace Userlist
    {
        namespace User
        {
            enum Grade : uint8_t
            {
                NormalPlayer = 2,
                Moderator = 3,
                Tester = 4,
                GameMaster = 9
            };
        }

        enum ListResult : uint8_t
        {
            NoUsers = 0x06,
            Users = 0x25,
            Users2 = 0x00
        };
        enum FriendsState : uint8_t
        {
            Login = 0x2E,
            Logout = 0x2F
        };
        namespace Friends
        {
            enum DetailsType : uint8_t
            {
                WithoutClan = 0x00,
                WithClan = 0x01,
                FriendState = 0x35
            };
            enum State : uint8_t
            {
                Accepted = 0,
                Pending = 1,
                Ignored = 2
            };
            enum RequestResult : uint8_t
            {
                RequestSend = 0x1C,
                RequestRecv = 0x1E
            };
            enum AddResult : uint8_t
            {
                SendSingle = 28,
                SendPending = 37,
                FriendAccepted = 1,
                UpdateList = 30,

                PlayerBlocked = 42,
                PlayerNotFound = 6,
                ListFull = 7,
            };
            enum ListState : uint8_t
            {
                OtherListIsFull = 0x00,
                YourListIsFull = 0x01
            };
        }

        namespace Blocked
        {
            enum AddResult : uint8_t
            {
                Success = 0x01,
                Offline = 0x06
            };
            enum ListResult : uint8_t
            {
                NotUser = 0x06,
                UsersBlocked = 0x25
            };
        }
        namespace Clan
        {
            enum ListResult : uint8_t
            {
                NotUser = 0x06,
                UsersClan = 0x25
            };
        }
    }
    namespace Chat
    {
        enum WhisperResult : uint8_t
        {
            NoUser = 0x0D,
            DontMyself = 0x0F,
            WhisperRefuse = 0x23,
            Failed = 0x02
        };
        enum Type : uint8_t
        {
            User = 0,
            Server = 1,
            Whisper = 2,
            Command = 3,
            Team = 5,
            Clan = 7,
            Party = 8,
            Tip
        };
    }
    namespace Mailbox
    {
        enum SendResult : uint8_t
        {
            UserNotFound = 4,
            FullGiftReceiver = 7,
            FullReceiver = 8,
            FullSender = 17,
            Blacklist = 42,
            NewMail = 46,
            Gift = 53
        };
        enum OpenResult : uint8_t
        {
            SendMails= 0,
            Empty = 6,
            SendMails2 = 37,
            Confirm = 51
        };
		enum GiftResult : uint8_t
		{
            USER_OFFLINE = 4,
            MEMO_GIFT_FULL_RECIEVER = 8,
            NOT_ENOUGH_CASH = 14,
            MEMO_GIFT_FULL_SENDER = 17,
            PRESENT_SEND_SUCCESS = 37,
            BLACKLIST_ERROR_10 = 42
		};
    }

    namespace Team
    {
        enum IdType : uint8_t
        {
            Neutral = 0,
            Red = 1,
            Blue = 2,
            Observer = 4,
            Zombie = 5
        };
        namespace Join
        {
            enum Error : uint8_t
            {
                AlreadyInTeam = 0,
                TeamFull = 1,
                Ok = 2,
            };
        }
        namespace Change
        {
            enum Result : uint8_t
            {
                Success = 0x01,
                TeamFull = 0x07,
                MatchRunning = 0x23,
                NoBehavior = 0x0F,
                CantChange = 0x0C,
                DataError = 0x04,
                NoPermission = 0x10
            };
        }
    }
    namespace Room
    {
        namespace List
        {
            enum Result : uint8_t
            {
                ChannelFull = 0x07,
                SendRoom = 0x25,
                SendRoom2 = 0x00,
                NoRooms = 0x06
            };
        }
        namespace Join
        {
            enum Error : uint8_t
            {
                Ok = 0,
                AlreadyInRoom = 1,
                RoomFull = 2,
                KickedPreviously = 3,
                NoIntrusion = 4,
                Generic = 5
            };
            enum ReqResult : uint8_t
            {
                NoPassword = 0x00,
                Password = 0x2C
            };
            enum Result : uint8_t
            {
                JoinAsObserver = 0x00,
                JoinAsPlayer = 0x01,

                NoObservers = 0x05,
                RoomDeleted = 0x06,
                LobbyFull = 0x07,
                PreviouslyKicked = 0x2A,
                InvalidPassword = 0x2C,
                LowLevelOnly = 0x15,

                ShowPasswordDLG = 0x0E,
                GenericError = 0x0F
            };
        }
        namespace Leave
        {
            namespace Req
            {
                enum Result : uint8_t
                {
                    Leave = 0x00,
                    KickedByHost = 0x1C
                };
            }
            namespace Ack
            {
                enum Result : uint8_t
                {
                    Error = 0,
                    Leave = 1,
                    Unknown2 = 4,
                    Unknown1 = 21,
                    ClosedByGm = 27,
                    KickedByGm = 35,
                    KickedByKickVote = 39,
                    KickedByHost = 42,
                    Offline = 47
                };
            }
        }
        namespace Create
        {
            enum Error : uint8_t
            {
                Ok,
                InvalidGameRule,
                InvalidSettings
            };
            enum Result : uint8_t
            {
                Failed = 0x00,
                Success = 0x01,
                Empty = 0x06,
                BotBattleOnlyBeginner = 0x3D
            };
        }
        namespace Start
        {
            enum Result : uint8_t
            {
                Start = 0x26,
                NoReady = 0x2A,
                CountTeamNotSame = 0x21
            };
        }
        namespace Match
        {
            enum Result : uint8_t
            {
                SingleWave = 0x06,
                Started = 0x26,
                Loaded = 0x29
            };
        }
        namespace ChangeHost
        {
            enum Result : uint8_t
            {
                Success = 0x01,
                Error = 0x02,
                NotInRoom = 0x0D,
                NotTheHost = 0x10
            };
        }
        namespace ChangeMap
        {
            enum Result : uint8_t
            {
                GenericError = 0x02,
                NotSupportedByServer = 0x06,
                PlayerExceedMaxPlayers = 0x07,
                ExceedDesiredAmount = 0x23,
            };
        }
        namespace ChangeTeam
        {
            enum Result : uint8_t
            {
                Success = 0x01,
                TeamFull = 0x07,
                MatchRunning = 0x23,
                NoBehavior = 0x0F,

                CantChange = 0x0C,
                DataError = 0x04,
                NoPermission = 0x10
            };
        }
        namespace Option
        {
            enum Type : uint8_t
            {
                KillInfo = 0,
                ModeInfo = 2,
                PlayerLimit = 3,
                TimeInfo = 4,
                WeaponLimit = 5
            };
        }
        namespace Mode
        {
        #if defined(RELEASE_1_0_3)
            enum Index : uint8_t
            {
                TeamDeathMatch = 0,
                FreeForAll = 1,
                ItemMatch = 2,
                CaptureTheBattery = 3,
                CloseCombat = 4,
                Elimination = 5,
                SuperItemMatch = 6,
                ZombieMode = 7,
                ArmsRace = 8,
                Scrimmage = 9,
                BombBattle = 10,
                BossBattle = 11,
                CLAN_CaptureTheBattery = 14,
                CLAN_TeamDeathMatch = 15,
                CLAN_Elimination = 16,
                CLAN_Random = 17
                
            };
            constexpr std::string_view modeNames[] =
            {
                "Team Death Match",
                "Free For All",
                "Item Match",
                "Capture The Battery",
                "Close Combat",
                "Sabotage",
                "Super Item Match",
                "Zombie Mode",
                "Arms Race",
                "Scrimmage",
                "Bomb Battle",
                "Boss Battle",
                "Clan Team Death Match"
                "Clan Capture The Battery",
                "Clan Elimination",
                "Unknown 15",
                "Unknown 16",
                "Clan Random"
            };
        #else
            enum Index : uint8_t
            {
                TeamDeathMatch = 0,
                FreeForAll = 1,
                ItemMatch = 2,
                CaptureTheBattery = 3,
                CloseCombat = 4,
                Elimination = 5,
                SuperItemMatch = 6,
                ZombieMode = 7,
                ArmsRace = 8,
                Scrimmage = 9,
                BombBattle = 10,
                SniperMode = 11,
                SquareMode = 12,
                BossBattle = 13,
                AiBattle = 14,
                CLAN_CaptureTheBattery = 15,
                CLAN_Elimination = 16,
                CLAN_TeamDeathMatch = 17
            };
            constexpr std::string_view modeNames[] =
            {
                "Team Death Match",
                "Free For All",
                "Item Match",
                "Capture The Battery",
                "Close Combat",
                "Sabotage",
                "Super Item Match",
                "Zombie Mode",
                "Arms Race",
                "Scrimmage",
                "Bomb Battle",
                "Sniper Mode",
                "Square Mode",
                "Boss Battle",
                "AI Battle",
                "Clan Capture The Battery",
                "Clan Elimination",
                "Clan Team Death Match"
            };
        #endif
        }
        namespace Map
        {
            enum Index : uint8_t
            {
                Random = 0,
                Chess = 1,
                ToyFleet = 2,
                TrackerFactory = 3,
                Beach = 4,
                BattleMine = 5,
                ToyGarden = 6,
                Neighborhood = 7,
                MagicPaperLand = 8,
                HobbyShop = 9,
                Academy = 10,
                TheStudio = 11,
                PvcTutiral = 12,
                PvcFactory = 13,
                HouseTop = 14,
                WildWest = 15,
                RumpusRoom = 16,
                Cargo = 17,
                PvcFactoryNight = 18,
                ForgottenJunkYard = 19,
                JunkYard = 20,
                GothicCastle = 21,
                Bitmap = 22,
                RockBand = 23,
                ModelShip = 27,
                MonsterAcademy = 28,
                Foosball = 29,
                Bbitmap2 = 30,
                ToyGarden2 = 31,
                RockBand2 = 32,
                RockBand3 = 33,
                CastleSeige = 34,
                GothicCastle2 = 35,
                BitmapPlant = 36,
                TheAftermath = 37,
                Invasion = 43,
                Invasion2 = 44,
                AcademyInvasion = 45,
                SquareMap1 = 46,
                SquareMap2 = 47
            };
            constexpr std::string_view mapNames[] =
            {
                "Random",
                "Chess",
                "Toy Fleet",
                "Tracker Factory",
                "Beach",
                "Battle Mine",
                "Toy Garden",
                "Neighborhood",
                "Magic Paper Land",
                "Hobby Shop",
                "Academy",
                "The Studio",
                "PVC Tutorial",
                "PVC Factory",
                "House Top",
                "Wild West",
                "Rumpus Room",
                "Cargo",
                "PVC Factory Night",
                "Forgotten Junk Yard",
                "Junk Yard",
                "Gothic Castle",
                "Bitmap",
                "Rock Band",
                "", "", "", "", "",
                "Model Ship",
                "Monster Academy",
                "Foosball",
                "Bitmap2",
                "Toy Garden 2",
                "Rock Band 2",
                "Rock Band 3",
                "Castle Siege",
                "Gothic Castle 2",
                "Bitmap Plant",
                "The Aftermath",
                "", "", "", "", "", "",
                "Invasion",
                "Invasion 2",
                "Academy Invasion",
                "Square Map 1",
                "Square Map 2"
            };


        }
        namespace Restriction
        {
            enum Type : uint8_t
            {
                MeleeOnly = 0,
                RifleOnly = 1,
                ShotgunOnly = 2,
                SniperOnly = 3,
                MinigunOnly = 4,
                BazookaOnly = 5,
                GrenadeOnly = 6,
                AllWeapons = 7
            };
        }
        namespace Balance
        {
            enum State : uint8_t
            {
                Disabled = 0, //Disable balance - Disable balance2
                Enabled1 = 1, //Enable balance - Disable balance2
                Enabled2 = 2  //Enable balance - Enable balance2
            };
        }

        namespace Clan
        {
            enum IconUpdateMission : uint8_t
            {
                RoomPlayers = 0,
                RoomObservers = 1,
                PartyMembers = 2,
                PlazaPlayers = 3
            };
        }
    }

}

//#endif