#pragma once
#include <stdint.h>
#include <string.h>

#include "BaseLib/Utility.h"
#include <BaseLib/Platform.h>
//#include <DirectXPackedVector.h>
namespace NetEngine
{
    namespace Packets
    {
        namespace Front
        {
#pragma pack(push, 1)

            union FrontServerInfo
            {
                struct
                {
                    uint8_t serverId : 8;
                    uint8_t channel1 : 2;
                    uint8_t channel2 : 2;
                    uint8_t channel3 : 2;
                    uint8_t channel4 : 2;
                    uint8_t channel5 : 2;
                    uint8_t channel6 : 2;
                    uint8_t channel7 : 2;
                    uint8_t channel8 : 2;
                    uint8_t channel9 : 2;
                    uint8_t channel10 : 2;
                };
                uint32_t data;

                FrontServerInfo()
                {
                    std::memset(this, 0, sizeof(FrontServerInfo));
                }

                FrontServerInfo(uint32_t data)
                {
                    std::memset(this, 0, sizeof(FrontServerInfo));
                    this->data = data;
                }
            };

            struct FrontUserAccountInfo
            {
                uint32_t  level;
                uint32_t  experience;
                uint32_t  kills;
                uint32_t  deaths;
                uint32_t  assists;
                uint32_t  wins;
                uint32_t  losses;
                uint32_t  draws;
                char      nickname[16];
                uint16_t  clanLogoFront;
                uint16_t  clanLogoBack;
                char      clanName[16];
                uint32_t  unknown;

                FrontUserAccountInfo(
                    uint32_t level = 0,
                    uint32_t experience = 0,
                    uint32_t kills = 0,
                    uint32_t deaths = 0,
                    uint32_t assists = 0,
                    uint32_t wins = 0,
                    uint32_t losses = 0,
                    uint32_t draws = 0,
                    const char* nickname = "",
                    uint16_t clanLogoFront = 0,
                    uint16_t clanLogoBack = 0,
                    const char* clanName = "",
                    uint32_t unknown = 0
                ) :
                    level(level),
                    experience(experience),
                    kills(kills),
                    deaths(deaths),
                    assists(assists),
                    wins(wins),
                    losses(losses),
                    draws(draws),
                    clanLogoFront(clanLogoFront),
                    clanLogoBack(clanLogoBack),
                    unknown(unknown)
                {
                    memset(this->nickname, 0, sizeof(this->nickname));
                    memset(this->clanName, 0, sizeof(this->clanName));
                    strcpy(this->nickname, nickname);
                    strcpy(this->clanName, clanName);
                }
            };

#pragma pack(pop)
        }

        namespace Main
        {
#pragma pack(push, 1)
            union MainServerInfo
            {
                struct
                {
                    uint8_t serverId : 8;
                    uint8_t channel1 : 2;
                    uint8_t channel2 : 2;
                    uint8_t channel3 : 2;
                    uint8_t channel4 : 2;
                    uint8_t channel5 : 2;
                    uint8_t channel6 : 2;
                    uint8_t channel7 : 2;
                    uint8_t channel8 : 2;
                    uint8_t channel9 : 2;
                    uint8_t channel10 : 2;
                };
                uint32_t data;

                MainServerInfo()
                {
                    std::memset(this, 0, sizeof(MainServerInfo));
                }

                MainServerInfo(uint32_t data)
                {
                    std::memset(this, 0, sizeof(MainServerInfo));
                    this->data = data;
                }
            };
            union GachaSaleDataId
            {
                struct
                {
                    uint32_t id : 6;
                    uint32_t sale_price : 15;
                    uint32_t unknown : 11;
                };
                uint32_t data;

                GachaSaleDataId(uint32_t data = 0)
                {
                    std::memset(this, 0, sizeof(data));
                    this->data = data;
                }
                GachaSaleDataId(uint32_t new_id, uint32_t new_sale_price, uint32_t new_unknown)
                {
                    std::memset(this, 0, sizeof(GachaSaleDataId));
                    this->id = new_id;
                    this->sale_price = new_sale_price;
                    this->unknown = new_unknown;
                }
            };
            struct MainGachaponSaleInfo
            {
                GachaSaleDataId data;
                uint32_t start_date;
                uint32_t end_date;
                MainGachaponSaleInfo(
                    uint32_t id = 0,
                    uint32_t sale_price = 0,
                    uint32_t start_date = 0,
                    uint32_t end_date = 0
                ) :
                    data(GachaSaleDataId(id, sale_price, 0)),
                    start_date(start_date),
                    end_date(end_date)
                {
                }
                MainGachaponSaleInfo()
                {
                    std::memset(this, 0, sizeof(MainGachaponSaleInfo));
                }
            };
            union ItemSerialInfo
            {
                struct
                {
                    uint32_t id : 20;
                    uint32_t sid : 4;
                    uint32_t unknown : 4;
                    uint32_t origin : 4;
                    uint32_t creation_date;
                };
                uint64_t data;

                ItemSerialInfo(uint64_t data = 0)
                {
                    std::memset(this, 0, sizeof(ItemSerialInfo));
                    this->data = data;
                }
                ItemSerialInfo(uint32_t id , uint32_t sid , uint32_t unknown, uint32_t origin, uint32_t creation_date)
                {
                    std::memset(this, 0, sizeof(ItemSerialInfo));
                    this->id = id;
                    this->sid = sid;
                    this->unknown = unknown;
                    this->origin = origin;
                    this->creation_date = creation_date;
                }

            };
            union InventoryItemNumber
            {
                struct
                {
                    uint32_t item_id : 23;
                    uint32_t stock : 9;
                };
                uint32_t data;

                InventoryItemNumber(uint32_t data = 0)
                {
                    std::memset(this, 0, sizeof(data));
                    this->data = data;
                }
                InventoryItemNumber(uint32_t new_item_id, uint32_t new_stock)
                {
                    std::memset(this, 0, sizeof(InventoryItemNumber));
                    this->item_id = new_item_id;
                    this->stock = new_stock;
                }
            };
            union EquipItemNumber
            {
                struct
                {
                    uint32_t item_type : 9;
                    uint32_t item_id : 23;
                   
                };
                uint32_t data;

                EquipItemNumber(uint32_t data = 0)
                {
                    std::memset(this, 0, sizeof(data));
                    this->data = data;
                }
                EquipItemNumber(uint32_t new_item_id, uint32_t new_type)
                {
                    std::memset(this, 0, sizeof(EquipItemNumber));
                    this->item_id = new_item_id;
                    this->item_type = new_type;
                }
            };
            union GachaponWonItemMsg
            {
                struct
                {
                    uint32_t gachapon_id : 8;
                    uint32_t item_id : 24;
                };
                uint32_t data;

                GachaponWonItemMsg(uint32_t data = 0)
                {
                    std::memset(this, 0, sizeof(GachaponWonItemMsg));
                    this->data = data;
                }
                GachaponWonItemMsg(uint32_t gachapon_id, uint32_t item_id)
                {
                    std::memset(this, 0, sizeof(GachaponWonItemMsg));
                    this->gachapon_id = gachapon_id;
                    this->item_id = item_id;
                }

            };

            struct GachaponAnnouncement
            {
                GachaponWonItemMsg won_item;
                char nickname[16];
                GachaponAnnouncement(const GachaponWonItemMsg& wonItem, const char* newNickname = "")
                {
                    std::memset(this, 0, sizeof(GachaponAnnouncement));
                    std::memset(this->nickname, 0, sizeof(nickname));
                    this->won_item.data = wonItem.data;
                    std::strcpy(this->nickname, newNickname);
                }
            };

        #if defined(RELEASE_1_0_3)
            struct InventoryItemInfo
            {
                
                InventoryItemNumber item_number;
                uint32_t expire_date;//0x04
                ItemSerialInfo serial_info;//0x08
                uint16_t repair;//0x10
                uint16_t energy;//0x12
                InventoryItemInfo(
                    const InventoryItemNumber& itemNumber = 0,
                    const uint32_t& expireDate = 0,
                    const ItemSerialInfo& serialInfo = ItemSerialInfo(),
                    const uint32_t& repairVal = 0,
                    const uint32_t& energyVal = 0)
                    : item_number(itemNumber),
                    expire_date(expireDate),
                    serial_info(serialInfo),
                    repair(repairVal),
                    energy(energyVal)
                {
                }
            };
            struct EquipItemInfo
            {
                EquipItemNumber item_number;
                uint32_t expire_date;
                ItemSerialInfo serial_info;
                uint16_t repair;
                uint16_t energy;
                EquipItemInfo(const InventoryItemInfo& inv_item_info)
                {
                    std::memset(this, 0, sizeof(EquipItemInfo));
                    item_number.item_id = inv_item_info.item_number.item_id;
                    expire_date = inv_item_info.expire_date;
                    serial_info.data = inv_item_info.serial_info.data;
                    repair = inv_item_info.repair;
                    energy = inv_item_info.energy;
                }
            };
        #else
            struct InventoryItemInfo
            {
                InventoryItemNumber item_number;
                uint32_t expire_date;//0x04
                ItemSerialInfo serial_info;//0x08
                uint16_t repair;//0x10
                uint16_t energy;//0x12
                uint32_t is_sealed;//0x14
                uint32_t seal_level;
                uint32_t enhance_exp;
                uint32_t enhance_level;
                uint32_t item_type;
                InventoryItemInfo(
                    const InventoryItemNumber& itemNumber = 0,
                    const uint32_t& expireDate = 0,
                    const ItemSerialInfo& serialInfo = ItemSerialInfo(),
                    const uint32_t& repairVal = 0,
                    const uint32_t& energyVal = 0,
                    const uint32_t& isSealed = 0,
                    const uint32_t& sealLevel = 0,
                    const uint32_t& enhanceExp = 0,
                    const uint32_t& enhanceLevel = 0,
                    const uint32_t& itemType = 0)
                    : item_number(itemNumber),
                    expire_date(expireDate),
                    serial_info(serialInfo),
                    repair(repairVal),
                    energy(energyVal),
                    is_sealed(isSealed),
                    seal_level(sealLevel),
                    enhance_exp(enhanceExp),
                    enhance_level(enhanceLevel),
                    item_type(itemType)
                {
                }
            };
            struct EquipItemInfo
            {
                EquipItemNumber item_number;
                uint32_t expire_date;
                ItemSerialInfo serial_info;
                uint16_t repair;
                uint16_t energy;
                uint32_t is_sealed;
                uint32_t seal_level;
                uint32_t enhance_exp;
                uint32_t enhance_level;
                EquipItemInfo(const InventoryItemInfo& inv_item_info)
                {
                    std::memset(this, 0, sizeof(EquipItemInfo));
                    item_number.item_id = inv_item_info.item_number.item_id;
                    expire_date = inv_item_info.expire_date;
                    serial_info.data = inv_item_info.serial_info.data;
                    repair = inv_item_info.repair;
                    energy = inv_item_info.energy;
                    is_sealed = inv_item_info.is_sealed;
                    seal_level = inv_item_info.seal_level;
                    enhance_exp = inv_item_info.enhance_exp;
                    enhance_level = inv_item_info.enhance_level;
                }
            };
        #endif

            struct ShopSerialInfo
            {
                ItemSerialInfo serial_info;
                uint32_t expire_time;
                ShopSerialInfo(
                    const ItemSerialInfo& serialInfo = ItemSerialInfo(),
                    const uint32_t& expireDate = 0)
                    : serial_info(serialInfo),
                    expire_time(expireDate)
                {
                }
            };
            struct BossItem
            {
                uint32_t unique_id;
                uint32_t item_id;
            };
            struct ShopItem
            {
                InventoryItemNumber item_number;
                uint32_t expire_time;
                ItemSerialInfo serial_info;
                ShopItem(
                    const InventoryItemNumber& itemNumber = InventoryItemNumber(0),
                    const uint32_t& expireDate = 0,
                    const ItemSerialInfo& serialInfo = ItemSerialInfo())
                    : item_number(itemNumber),
                    expire_time(expireDate),
                    serial_info(serialInfo)
                {
                    item_number.data = itemNumber.data;
                }
            };
            struct MailboxGift
            {
                uint32_t mail_id;
                uint32_t mail_time;
                ShopItem item;
                MailboxGift(
                    const uint32_t& mailId = 0,
                    const uint32_t& mailTime = 0,
                    const ShopItem& item = ShopItem())
                    : mail_id(mailId),
                    mail_time(mailTime),
                    item(item)
                {
                };
            };
            struct PlayerFriendInfo
            {
                uint32_t unique_id;
                int32_t friend_id;
                char nickname[16];
                PlayerFriendInfo(const uint32_t& uniqueId = 0, const int32_t& friendId = 0, const char* newNickname = "")
                {
                    std::memset(this, 0, sizeof(PlayerFriendInfo));
                    std::memset(this->nickname, 0, sizeof(nickname));
                    this->unique_id = uniqueId;
                    this->friend_id = friendId;
                    std::strcpy(this->nickname, newNickname);
                }
            };
            struct PlayerBlockedInfo
            {
                int32_t acc_id;
                char nickname[16];
                PlayerBlockedInfo(const int32_t& accId = 0, const char* newNickname = "")
                {
                    std::memset(this, 0, sizeof(PlayerBlockedInfo));
                    std::memset(this->nickname, 0, sizeof(nickname));
                    this->acc_id = accId;
                    std::strcpy(this->nickname, newNickname);
                }
            };
            union PlayerClanInfoRoom
            {
                struct
                {
                    uint64_t clanIdk : 4; // >> 0 (0 display clan, anythin else idk doesnt work)
                    uint64_t logo_front : 16; // >> 4
                    uint64_t logo_back : 14; // >> 20
                    uint64_t clanId : 27; // >> 34
                    uint64_t unknown2 : 3; // >> 61
                    
                };
                uint64_t data;
            };
            struct PlayerRoomClanListInfo
            {
                char clanName[16];
                PlayerClanInfoRoom info_room;

                PlayerRoomClanListInfo(uint32_t slotIndex = 0, std::string clanName = "", uint32_t logo_front = 0, uint32_t logo_back = 0, uint32_t clanId = 0, uint32_t unknown2 = 0)
                {
                    std::memset(this, 0, sizeof(PlayerRoomClanListInfo));
                    std::strcpy(this->clanName, clanName.c_str());
                    this->info_room.clanIdk = slotIndex;
                    this->info_room.logo_front = logo_front;
                    this->info_room.logo_back = logo_back;
                    this->info_room.clanId = clanId;
                    this->info_room.unknown2 = unknown2;
                }
            };
           
            union PlayerClanListInfo
            {
                struct
                {
                    uint32_t level : 7;
                    uint32_t unknown : 25;
                };
                uint32_t data;
            };
            struct PlayerClanInfo
            {
                char nickname[16];
                uint32_t unique_id;
                PlayerClanListInfo clanInfo;

                PlayerClanInfo(std::string nickname, uint32_t uniqueId, uint32_t level)
                {
                    std::memset(this, 0, sizeof(PlayerClanInfo));
                    std::strcpy(this->nickname, nickname.c_str());
                    this->unique_id = uniqueId;
                    this->clanInfo.level = level;
                    this->clanInfo.unknown = 0;
                }
            };
            union PlayerClanInfoLobby
            {
                struct
                {
                    uint64_t logo_front : 16;
                    uint64_t logo_back : 14;
                    uint64_t level : 7;
                };
                uint64_t data;
            };
            struct PlayerAgoraInfo 
            {
                char nickname[16];
                uint32_t unique_id;
                PlayerClanInfoLobby clanInfo;

                PlayerAgoraInfo(std::string nickname, uint32_t uniqueId, uint32_t level, uint32_t clanIconFront, uint32_t clanIconBack)
                {
                    std::memset(this, 0, sizeof(PlayerAgoraInfo));
                    std::strcpy(this->nickname, nickname.c_str());
                    this->unique_id = uniqueId;
                    this->clanInfo.logo_front = clanIconFront;
                    this->clanInfo.logo_back = clanIconBack;
                    this->clanInfo.level = level;
                    /*
                    this->iconsAndLevel = level << 0x1E | level >> 32 - 0x1E;
                    this->additionalLevel = iconsAndLevel & 0x1F;
                    this->iconsAndLevel = this->iconsAndLevel >> 5 << 5; // bit clear
                    this->iconsAndLevel |= clanIconFront & 0xFFFF;
                    this->iconsAndLevel |= (clanIconBack & 0xFFFF) << 16;
                    */
                }
            };
          
            union PlayerPingUpdateInfo
            {
                struct
                {
                    uint32_t unused : 10;
                    uint32_t ping : 10;
                    uint32_t rest : 12;
                };
                uint32_t data;
            };
            union RoomUserPlayerInfo1
            {
            #if defined(RELEASE_1_0_3)
                struct
                {
                    uint32_t grade : 4;
                    uint32_t vip_level : 3;
                    uint32_t character : 4;
                    uint32_t team : 4;
                    uint32_t level : 7;
                    uint32_t ping : 10;
                };
            #else
                struct
                {
                    uint32_t grade : 4; //  << 0
                    uint32_t vip_level : 3; // << 4
                    uint32_t character : 5; // << 7
                    uint32_t team : 3; // << 12
                    uint32_t level : 7; // << 15
                    uint32_t ping : 10; // << 22
                };
            #endif
                uint32_t data;
            };
            union RoomUserPlayerInfo2
            {
                struct
                {
                    uint32_t fps_limit : 2;
                    uint32_t player_state : 4;
                    uint32_t ping : 10;
                    uint32_t unknown : 16;
                };
                uint32_t data;
            };
            union RoomUserPlayerInfo3
            {
                struct
                {
                    uint32_t unknown : 3;
                    uint32_t ping : 10;
                    uint32_t fps_limit : 2;
                };
                uint32_t data;
            };
            union RoomPlayerInfo1
            {
                struct
                {
                    uint32_t map_index : 7;
                    uint32_t mode_index : 5;
                    uint32_t max_players : 5;
                    uint32_t unknown : 1;
                    uint32_t has_password : 1; 
                    uint32_t allow_intruders : 1; 
                };
                uint32_t data;
            };
            union RoomSettingsModeInfo2
            {
                struct
                {
                    uint64_t time_limit : 5;
                    uint64_t score_limit : 5;
                    uint64_t allow_items : 1;
                    uint64_t unknown : 2;
                    uint64_t restriction : 4;
                };
                uint64_t data;
            };
            union TDM_ModeInfo
            {
                struct
                {
                    uint64_t redscore : 8;
                    uint64_t bluescore : 8;
                    uint64_t winrule : 8;
                    uint64_t state : 2;
                    uint64_t kitdrop : 1;
                    uint64_t timelimited : 5;
                    uint64_t weaponlimited : 4;
                };
                uint64_t data;
            };
            union Zombie_ModeInfo
            {
                struct
                {
                    uint64_t redscore : 8;
                    uint64_t bluescore : 8;
                    uint64_t winrule : 8;
                    uint64_t state : 2;
                    uint64_t kitdrop : 1;
                    uint64_t timelimited : 5;
                    uint64_t weaponlimited : 4;
                };
                uint64_t data;
            };
            union Elimination_ModeInfo
            {
                struct
                {
                    uint64_t redscore : 8;
                    uint64_t bluescore : 8;
                    uint64_t winrule : 8;
                    uint64_t state : 2;
                    uint64_t kitdrop : 1;
                    uint64_t timelimited : 5;
                    uint64_t weaponlimited : 4;
                };
                uint64_t data;
            };
            union CaptureTheBattery_ModeInfo
            {
                struct
                {
                    uint64_t redscore : 8;
                    uint64_t bluescore : 8;
                    uint64_t winrule : 8;
                    uint64_t state : 2;
                    uint64_t kitdrop : 1;
                    uint64_t timelimited : 5;
                    uint64_t weaponlimited : 4;
                };
                uint64_t data;
            };
            union FFA_ModeInfo
            {
                struct
                {
                    uint64_t timelimited : 5; // >> 0
                    uint64_t winrule : 5; // >> 5
                    uint64_t kitdrop : 1; // >> 10
                    uint64_t state : 2; // >> 11
                    uint64_t weaponlimited : 4; // >> 13
                };
                uint64_t data;
            };
            struct BossBattle_ModeInfo
            {
                uint32_t unknown1;//0
                uint32_t unknown2;//4
                uint32_t pos1 : 16;//8
                uint32_t pos2 : 16;//10
                uint32_t pos3 : 16;//12
                uint32_t pos4 : 16;//14
                uint32_t pos5 : 16;//16
                uint32_t pos6 : 16;//18
                uint64_t winrule : 8;//20
                uint64_t state : 2;
                uint64_t kitdrop : 1;
                uint64_t timelimited : 5;
                uint64_t weaponlimited : 4;
            };
            struct BombBattle_ModeInfo
            {
                uint32_t unknown1;//0
                uint32_t unknown2;//4
                uint32_t pos1 : 16;//8
                uint32_t pos2 : 16;//10
                uint32_t pos3 : 16;//12
                uint32_t pos4 : 16;//14
                uint32_t pos5 : 16;//16
                uint32_t pos6 : 16;//18
                uint32_t redscore : 8;//20
                uint32_t bluescore : 8;
                uint32_t winrule : 8;
                uint32_t state : 2;
                uint32_t kitdrop : 1;
                uint32_t timelimited : 5;
                uint32_t weaponlimited : 4;
            };
            union ArmsRace_ModeInfo
            {
                struct
                {
                    uint64_t timelimited : 5;
                    uint64_t winrule : 5;
                    uint64_t kitdrop : 1;
                    uint64_t state : 2;
                    uint64_t weaponlimited : 4;
                };
                uint64_t data;
            };

            union Scrimmage_ModeInfo
            {
                struct
                {
                    uint64_t redscore : 20; // >> 0
                    uint64_t bluescore : 20; // >> 20
                    uint64_t winrule : 5; // >> 40
                    uint64_t state : 2; // >> 45
                    uint64_t timelimited : 5; // >> 47
                    uint64_t weaponlimited : 4; // >> 52
                };
                uint64_t data;
            };
            union GameModeSettingsUpdateInfo
            {
                struct
                {
                    uint32_t max_players : 5;
                    uint32_t unknown1 : 11;
                    uint32_t map_index : 7;
                    uint32_t unknown2 : 9;
                };
                uint32_t data;
            };
            union RoomSettingsUpdateInfo
            {
                struct
                {
                    uint32_t max_players : 5;
                    uint32_t time : 5;
                    uint32_t restriction : 4;
                    uint32_t allow_items : 1;
                    uint32_t allow_intruders : 1;
                    uint32_t map_index : 6;
                    uint32_t unknown1 : 1;
                    uint32_t unknown2 : 1;
                    uint32_t score_limit : 5;
                    uint32_t team_balance : 2;
                    uint32_t unknown3 : 1;
                };
                uint32_t data;
            };
            struct RoomSettingsUpdateTitle
            {
                RoomSettingsUpdateInfo update_info;
                char title[30]{};
                char padding[2]{};
            };
            struct RoomSettingsUpdatePassword
            {
                RoomSettingsUpdateInfo update_info;
                char password[8]{};
                char padding[8]{};
            };
            struct RoomSettingsUpdateTitlePassword
            {
                RoomSettingsUpdateInfo update_info;
                char title[30]{};
                char padding[2]{};
                char password[8]{};
                char padding2[8]{};
            };
            union RoomSettingsInfo2
            {
            #if defined(RELEASE_1_0_3)
                struct
                {
                    uint32_t map_index : 7;
                    uint32_t mode_index : 5;
                    uint32_t max_players : 5; 
                    uint32_t has_matchStarted : 1; 
                    uint32_t has_password : 1;
                    uint32_t allow_intruders : 1; 
                    uint32_t restriction : 4;
                    uint32_t is_clan_room : 2;
                    uint32_t team_balance : 2;
                    uint32_t allow_observers : 1; 
                    uint32_t hide_password : 1;
                    uint32_t unknown1 : 1;
                    uint32_t unknown2 : 1; 
                };
            #else
                struct
                {
                    uint32_t map_index : 7;
                    uint32_t mode_index : 5;
                    uint32_t max_players : 5;
                    uint32_t has_matchStarted : 1;
                    uint32_t has_password : 1;
                    uint32_t allow_intruders : 1;
                    uint32_t restriction : 4;
                    uint32_t is_clan_room : 1;
                    uint32_t team_balance : 2;
                    uint32_t allow_observers : 1;
                    uint32_t hide_password : 1;
                    uint32_t unknown1 : 1;
                    uint32_t unknown2 : 1;
                    uint32_t unknown3 : 1;
                };
            #endif
                uint32_t data;
            };
            union RoomSettings
            {
                struct
                {
                    uint32_t time : 5;
                    uint32_t restriction : 4;
                    uint32_t allow_items : 1;
                    uint32_t mode_index : 4;
                    uint32_t unknown1 : 1;
                    uint32_t allow_intruders : 1;
                    uint32_t has_password : 1;
                    uint32_t unknown2 : 1;
                    uint32_t max_players : 4;
                    uint32_t map_index : 7;
                    uint32_t team_balance : 2;
                    uint32_t allow_observers : 1;
                    
                };
                uint32_t data;
            };
            struct RoomSettingsInfo
            {
                char title[32];
                char password[16];
                RoomSettings settings;
                RoomSettingsInfo(uint32_t settings_data = 0, const char* roomtitle = "", const char* roompass = "")
                {
                    std::memset(this, 0, sizeof(RoomSettingsInfo));
                    std::memset(this->title, 0, sizeof(this->title));
                    std::memset(this->password, 0, sizeof(this->password));
                    this->settings.data = settings_data;
                    std::strcpy(this->title, roomtitle);
                    std::strcpy(this->password, roompass);
                }
            };
            union RoomDetails
            {
                struct
                {
                    uint32_t room_id : 16;
                    //uint32_t room_id : 11;
                    //uint32_t main_id : 5;
                    uint32_t channel_id : 9; // >> 16
                    uint32_t map_index : 7; // >> 25
                    uint32_t mode_index : 5; // >> 32
                    uint32_t max_players : 5; // >> 37
                    uint32_t current_players : 5; // >> 42
                    uint32_t is_playing : 1; // >> 47
                    uint32_t has_password : 1; // >> 48
                    uint32_t is_observer_off : 1; // >> 49
                    uint32_t weapon_restriction : 4; // >> 50
                    uint32_t is_observer_enabled : 1; // >> 54
                    uint32_t host_ping : 9; // >> 55
                };
                uint64_t data;
            };
            struct MailboxMsgInfo
            {
                uint32_t mail_id;
                uint32_t date;
                uint32_t is_opened;
                char nickname[16];
                char msg[256];
            };
            struct GiftboxMsgInfo
            {
                uint32_t mail_id;
                uint32_t date;
                uint32_t item_id;
                uint32_t unknown1 = 1;
                uint32_t unknown2 = 0;
                char nickname[16];
                char msg[256];
            };
            struct RoomListInfo
            {
                char title[32];
                RoomDetails details;
                uint32_t unknown;

                RoomListInfo(
                    const std::string& room_title, 
                    const uint32_t& room_id, 
                    const uint32_t& channel_id, 
                    const uint32_t& map_index, 
                    const uint32_t& mode_index, 
                    const uint32_t& max_players, 
                    const uint32_t& current_players, 
                    const bool& is_playing, 
                    const bool& has_password, 
                    const bool& is_observer_off, 
                    const uint32_t& weapon_restriction,
                    const bool& is_observer_enabled, 
                    const uint32_t& host_ping)
                {
                    std::strcpy(this->title, room_title.c_str());
                    this->details.room_id = room_id;
                    this->details.channel_id = channel_id;
                    this->details.map_index = map_index;
                    this->details.mode_index = mode_index;
                    this->details.max_players = max_players;
                    this->details.current_players = current_players;
                    this->details.is_playing = is_playing;
                    this->details.has_password = has_password;
                    this->details.is_observer_off = is_observer_off;
                    this->details.weapon_restriction = weapon_restriction;
                    this->details.is_observer_enabled = is_observer_enabled;
                    this->details.host_ping = host_ping;
                    this->unknown = 0;
                }
            };
            struct MainRoomEndMatchScoreClientInfo
            {
                uint8_t red_score, blue_score, unknown1, unknown2;
            };
            struct MainRoomEndMatchClientInfo
            {
				uint8_t melee_kills, rifle_kills, shotgun_kills, sniper_kills, gatling_kills, bazooka_kills, grenade_kills;
                uint8_t killstreak, total_kills, deaths, headshots, assists, mission, missionWin, unknown3, unknown4;
                uint32_t unique_id;
            };
            struct MainRoomEndMatchClientBossBattleInfo
            {  
                uint32_t unique_id;
                uint32_t unk1;
                uint16_t pve007_state;
                uint16_t unk2;
                uint32_t unk3[16];
            };


            // S2C
            struct MainRoomEndMatchResponse
            {
                uint8_t melee_kills, rifle_kills, shotgun_kills, sniper_kills, gatling_kills, bazooka_kills, grenade_kills;
                uint8_t killstreak, total_kills, deaths, headshots, assists, mission, missionWin, unknown3, unknown4;
                uint32_t total_mp;
                uint32_t total_xp;
                uint32_t unique_id;
				MainRoomEndMatchResponse() :
					melee_kills(0),
					rifle_kills(0),
					shotgun_kills(0),
					sniper_kills(0),
					gatling_kills(0),
					bazooka_kills(0),
					grenade_kills(0),
					killstreak(0),
					total_kills(0),
					deaths(0),
					headshots(0),
					assists(0),
                    mission(0),
                    missionWin(0),
					unknown3(0),
					unknown4(0),
					total_mp(0),
					total_xp(0),
					unique_id(0)
				{}			
            };
            struct MainRoomEndMatchResponseBossBattle
            {
                uint32_t total_mp;
                uint32_t total_xp;
                uint32_t reward_item_id;
            };

            struct GuideMissions
            {
                union
                {
                    struct
                    {
                        uint32_t help : 1;
                        uint32_t personalinfo : 1;
                        uint32_t battleinfo : 1;
                        uint32_t options : 1;
                        uint32_t chat : 1;
                        uint32_t invasion : 1;
                        uint32_t otherinfo : 1;
                        uint32_t gachapon : 1;
                        uint32_t createroom : 1;
                        uint32_t roomlist : 1;
                        uint32_t createparty : 1;
                        uint32_t addfriend : 1;
                    };
                    uint32_t data;
                };

                GuideMissions() : data(0) {}
                void SetMissionStatus(uint32_t missionId, bool completed)
                {
                    if (missionId < 46 || missionId > 57) return;
                    auto bit_pos = static_cast<uint32_t>(missionId - 46);
                    completed ? this->data |= (1 << bit_pos) : this->data &= ~(1 << bit_pos);
                }
                bool RetrieveMissionStatus(int missionId) const
                {
                    if (missionId < 46 || missionId > 57) return false;
                    auto bit_pos = static_cast<uint32_t>(missionId - 46);
                    return (this->data & (1 << bit_pos)) != 0;
                }
                void Set(uint32_t data) { this->data = data; }
                uint32_t Get() const { return this->data; }
            };

#pragma pack(pop)
        }

        namespace Cast
        {

#pragma pack(push, 1)
            struct PositionStruct
            {
                uint16_t positionX{};//0
                uint16_t positionY{};//2
                uint16_t positionZ{};//4
            };
            struct DirectionStruct
            {
                uint16_t directionX{};//6
                uint16_t directionY{};//8
                uint16_t directionZ{};//10
            };
            struct BulletsStruct
            {
                uint16_t bullet1{};
                uint16_t bullet2{};
                uint16_t bullet3{};
                uint16_t bullet4{};
            };
            struct JumpStruct
            {
                uint16_t jump1;
                uint16_t jump2;
            };
            struct ClientPlayerInfoBasic
            {
                PositionStruct position;//0
                DirectionStruct direction;//6
                uint32_t matchTick{};//12
            #if defined(RELEASE_1_0_3)
                uint32_t animation1 : 8 = 0;//16
            #else
                uint32_t animation1 : 7 = 0;
            #endif
                uint32_t animation2 : 6 = 0;
                uint32_t weapon : 4 = 0;
                uint32_t rotation : 9 = 0;
            #if defined(RELEASE_1_0_3)
                uint32_t unknown : 5 = 0;
            #else
                uint32_t unknown : 6 = 0;
            #endif
               
            };
            struct ClientPlayerInfoJump
            {
                ClientPlayerInfoBasic player;
                JumpStruct jumpStruct{};
            };
            struct ClientPlayerInfoBullet
            {
                ClientPlayerInfoBasic player;
                BulletsStruct bulletStruct{};
            };
            struct ClientPlayerInfoComplete
            {
                ClientPlayerInfoBasic player;
                BulletsStruct bulletStruct{};
                JumpStruct jumpStruct{};
            };
            struct SpecificInfo
            {
                uint32_t sessionId : 14 = 0;
                uint32_t enableMovement : 1 = true;// >> 14
                uint32_t enableBullet : 1 = false;// >> 15
                uint32_t animation1 : 7 = 0;// >> 16
                uint32_t enableRotation : 1 = true;// >> 23
                uint32_t animation2 : 6 = 0;// >> 24
                uint32_t unknown : 1 = true; // >> 30
                uint32_t enableJump : 1 = false; // >> 31
            };
            struct PlayerInfoBasicResponse
            {
                uint32_t tick{};//0
                SpecificInfo specificInfo{};//4
                PositionStruct position;//+0
                DirectionStruct direction;//+6
                uint32_t rotation1 : 8 = 0;//+12
                uint32_t rotation2 : 8 = 0;//+13
                uint32_t rotation3 : 9 = 0;//+ 14
                uint32_t currentWeapon : 4 = 0;
            };
            struct PlayerInfoResponseWithJump
            {
                PlayerInfoBasicResponse playerInfoBasicResponse;
                JumpStruct jump{};
            };
            struct PlayerInfoResponseWithBullets
            {
                uint32_t tick{};
                SpecificInfo specificInfo{};
                PositionStruct position;
                DirectionStruct direction;
                BulletsStruct bullets{};
                uint32_t rotation1 : 8 = 0;
                uint32_t rotation2 : 8 = 0;
                uint32_t rotation3 : 9 = 0;
                uint32_t currentWeapon : 4 = 0; // + 22
            };
            struct PlayerInfoResponseComplete
            {
                PlayerInfoResponseWithBullets playerInfoBasicResponse;
                JumpStruct jump{};
            };
            struct PlayerNicknameRoomInMatchInfo
            {
                char nickname[16];
            };
#pragma pack(pop)

        }

        namespace Core
        {
#pragma pack(push, 1)

            union UniqueId
            {
                struct
                {
                    uint32_t session : 16;
                    uint32_t server : 15;
                    uint32_t unknown : 1;
                };
                uint32_t data;

                UniqueId()
                {
                    std::memset(this, 0, sizeof(UniqueId));
                }

                UniqueId(uint32_t data)
                {
                    std::memset(this, 0, sizeof(UniqueId));
                    this->data = data;
                }
                UniqueId(uint16_t session, uint16_t server)
                {
                    std::memset(this, 0, sizeof(UniqueId));
                    this->session = session;
                    this->server = server & 0x7FFF;
                    this->unknown = 0; 
                }
            };

#pragma pack(pop)
        }
    }
}

//#endif