#pragma once
#include <stdint.h>
#include <optional>
#include <string_view>
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

    namespace PacketIds
    {
        namespace Ipc
        {
            enum : std::uint32_t
            {
                MainToCastDisconnectPlayer = 1,
                MainToFrontDisconnectPlayer = 2,
                FrontToMainDisconnectPlayer = 3,
                MainToCastHostChange = 4,
                MainToCastReqServerInfo = 5,
                CastToMainAckServerInfo = 6,
                CastToMainPlayerAuthorizeInfo = 7,
                MainToCastSendPingAssure = 8,
                MainToCastSendPacket = 9
            };
        }

    }

    namespace Cryptography
    {
        enum class EncryptionType
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
        enum Type : std::uint8_t
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
        enum Gacha : std::uint8_t
        {
            LuckyNotice = 0x00,
            RareNotice = 0x01
        };
        namespace Chat
        {
            enum Type : std::uint8_t
            {
                Announce = 0x0A,
                Unknown = 0x0C,
                GameMessage = 0x01
            };
        }
        
    }
    namespace Items
    {
        enum Origin : std::uint32_t
        {
            From_Game = 0,
            From_Event = 4,
            From_Dev_Tool = 5,
            From_Web_Shop = 6,
            From_GM_Spawn = 8
        };

        namespace WeaponItems
        {
            enum Type : std::uint32_t
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
            enum Type : std::uint32_t
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
            enum Type : std::uint32_t
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
            enum Type : std::uint32_t
            {
                Footing = 22,
                Object = 23
            };
        }

        namespace Upgrade
        {
            enum Type : std::uint32_t
            {
                NoUpgrade = 0,
                UpgradeType1 = 1,
                UpgradeType2 = 2,
                UpgradeType3 = 3
            };
            enum Result : std::uint8_t
            {
                UpgradeSuccess = 0x01,
                UpgradeFailHigh = 0x02,
                UpgradeFailLow = 0x06,
                EnergyInjection = 0x20,
                UpgradeReset = 0x35,
                NotEnoughPoints = 0x0E,
                RepairItem = 0x08
            };
            enum FailType : std::uint8_t
            {
                Destroy = 0x00,
                NoChange = 0x01
            };

        }

        namespace Gachapon
        {
            namespace Spin
            {
                enum Result : std::uint8_t
                {
                    SpinSuccess = 0x01,
                    InventoryFull = 0x07,
                    MoneyError = 0x0E,
                    Stuck = 0x08
                };
                enum Type : std::uint8_t
                {
                    LuckySpin = 0x00,
                    NormalSpin = 0x01,
                    NormalSpinSale = 0x02
                };
            }
            enum Type : std::uint32_t
            {
                Coin = 0,
                RT = 1,
                MP = 2
            };
            enum Error : std::uint8_t
            {
                NoRT = 0x01,
                NoMP = 0x02,
                NoCoin = 0x03
            };
            enum Rarity : std::uint32_t
            {
                Normal = 0,
                Rare = 1,
                Etc = 2
            };
            enum LuckyType : std::uint32_t
            {
                NoLucky = 0,
                GoldLucky = 1,
                SilverLucky = 2,
                CopperLucky = 3
            };
        }

        namespace Package
        {
            enum Result : std::uint8_t
            {
                Package = 0x00,
                Capsule = 0x1A,
                StaticItems = 0xFF,

                ChangeNicknameFail = 0x04,
                ChangeNicknameSuccess = 0x35,

                DailyGiftError = 0x02,
                BoxInventoryFull = 0x07,
                CoinMax = 0x13,
                Unknown1 = 0x08,
                Unknown2 = 0x23,
                VoiceUnlock = 0x9F
            };
        }

    }

    namespace EquipUpdate
    {
        enum Type : std::uint8_t
        {
            Multiple = 0x00,
            Sigle = 0x01
        };
    }


    namespace Userlist
    {
        namespace User
        {
            enum Grade : std::uint8_t
            {
                NormalPlayer = 2,
                Moderator = 3,
                Tester = 4,
                GameMaster = 9
            };
        }

        enum ListResult : std::uint8_t
        {
            NoUsers = 0x06,
            Users = 0x25,
            Users2 = 0x00
        };
        enum FriendsState : std::uint8_t
        {
            Login = 0x2E,
            Logout = 0x2F
        };
        namespace Friends
        {
            enum DetailsType : std::uint8_t
            {
                WithoutClan = 0x00,
                WithClan = 0x01,
                FriendState = 0x35
            };
            enum State : std::uint8_t
            {
                Accepted = 0,
                Pending = 1,
                Ignored = 2
            };
            enum RequestResult : std::uint8_t
            {
                RequestSend = 0x1C,
                RequestRecv = 0x1E
            };
            enum AddResult : std::uint8_t
            {
                SendSingle = 0x1C,
                SendPending = 0x25,
                FriendAccepted = 0x01,
                UpdateList = 0x1E,

                PlayerBlocked = 0x2A,
                PlayerNotFound = 0x06,
                ListFull = 0x07,
            };
            enum ListState : std::uint8_t
            {
                OtherListIsFull = 0x00,
                YourListIsFull = 0x01
            };
        }

        namespace Blocked
        {
            enum AddResult : std::uint8_t
            {
                Success = 0x01,
                Offline = 0x06
            };
            enum ListResult : std::uint8_t
            {
                NotUser = 0x06,
                UsersBlocked = 0x25
            };
        }
        namespace Clan
        {
            enum ListResult : std::uint8_t
            {
                NotUser = 0x06,
                UsersClan = 0x25
            };
        }
    }
    namespace Chat
    {
        enum WhisperResult : std::uint8_t
        {
            NoUser = 0x0D,
            DontMyself = 0x0F,
            WhisperRefuse = 0x23,
            Failed = 0x02
        };
        enum Type : std::uint8_t
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
        enum SendResult : std::uint8_t
        {
            UserNotFound = 4,
            FullGiftReceiver = 7,
            FullReceiver = 8,
            FullSender = 17,
            Blacklist = 42,
            NewMail = 46,
            Gift = 53
        };
        enum OpenResult : std::uint8_t
        {
            SendMails= 0,
            Empty = 6,
            SendMails2 = 37,
            Confirm = 51
        };
    }

    namespace Team
    {
        enum IdType : std::uint8_t
        {
            Neutral = 0,
            Red = 1,
            Blue = 2,
            Observer = 4,
            Zombie = 5
        };
        namespace Join
        {
            enum Error : std::uint8_t
            {
                AlreadyInTeam = 0,
                TeamFull = 1,
                Ok = 2,
            };
        }
        namespace Change
        {
            enum Result : std::uint8_t
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
            enum Result : std::uint8_t
            {
                ChannelFull = 0x07,
                SendRoom = 0x25,
                SendRoom2 = 0x00,
                NoRooms = 0x06
            };
        }
        namespace Join
        {
            enum Error : std::uint8_t
            {
                Ok = 0,
                AlreadyInRoom = 1,
                RoomFull = 2,
                KickedPreviously = 3,
                NoIntrusion = 4,
                Generic = 5
            };
            enum ReqResult : std::uint8_t
            {
                NoPassword = 0x00,
                Password = 0x2C
            };
            enum Result : std::uint8_t
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
                enum Result : std::uint8_t
                {
                    Leave = 0x00,
                    KickedByHost = 0x1C
                };
            }
            namespace Ack
            {
                enum Result : std::uint8_t
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
            enum Error : std::uint8_t
            {
                Ok,
                InvalidGameRule,
                InvalidSettings
            };
            enum Result : std::uint8_t
            {
                Failed = 0x00,
                Success = 0x01,
                Empty = 0x06,
                BotBattleOnlyBeginner = 0x3D
            };
        }
        namespace Start
        {
            enum Result : std::uint8_t
            {
                Start = 0x26,
                NoReady = 0x2A,
                CountTeamNotSame = 0x21
            };
        }
        namespace Match
        {
            enum Result : std::uint8_t
            {
                SingleWave = 0x06,
                Started = 0x26,
                Loaded = 0x29
            };
        }
        namespace ChangeHost
        {
            enum Result : std::uint8_t
            {
                Success = 0x01,
                Error = 0x02,
                NotInRoom = 0x0D,
                NotTheHost = 0x10
            };
        }
        namespace ChangeMap
        {
            enum Result : std::uint8_t
            {
                GenericError = 0x02,
                NotSupportedByServer = 0x06,
                PlayerExceedMaxPlayers = 0x07,
                ExceedDesiredAmount = 0x23,
            };
        }
        namespace ChangeTeam
        {
            enum Result : std::uint8_t
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
            enum Type : std::uint8_t
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
            enum Index : std::uint8_t
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
            enum Index : std::uint8_t
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
            enum Index : std::uint8_t
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
            enum Type : std::uint8_t
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
            enum State : std::uint8_t
            {
                Disabled = 0, //Disable balance - Disable balance2
                Enabled1 = 1, //Enable balance - Disable balance2
                Enabled2 = 2  //Enable balance - Enable balance2
            };
        }

        namespace Clan
        {
            enum IconUpdateMission : std::uint8_t
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