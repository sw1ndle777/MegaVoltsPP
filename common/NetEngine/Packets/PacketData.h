#pragma once
#include <stdint.h>
#include <string.h>

#include "BaseLib/Utility.h"
#include <DirectXPackedVector.h>
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
                    std::uint8_t serverId : 8;
                    std::uint8_t channel1 : 2;
                    std::uint8_t channel2 : 2;
                    std::uint8_t channel3 : 2;
                    std::uint8_t channel4 : 2;
                    std::uint8_t channel5 : 2;
                    std::uint8_t channel6 : 2;
                    std::uint8_t channel7 : 2;
                    std::uint8_t channel8 : 2;
                    std::uint8_t channel9 : 2;
                    std::uint8_t channel10 : 2;
                };
                std::uint32_t data;

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
                std::uint32_t  level;
                std::uint32_t  experience;
                std::uint32_t  kills;
                std::uint32_t  deaths;
                std::uint32_t  assists;
                std::uint32_t  wins;
                std::uint32_t  losses;
                std::uint32_t  draws;
                char      nickname[16];
                std::uint16_t  clanLogoFront;
                std::uint16_t  clanLogoBack;
                char      clanName[16];
                std::uint32_t  unknown;

                FrontUserAccountInfo(
                    std::uint32_t level = 0,
                    std::uint32_t experience = 0,
                    std::uint32_t kills = 0,
                    std::uint32_t deaths = 0,
                    std::uint32_t assists = 0,
                    std::uint32_t wins = 0,
                    std::uint32_t losses = 0,
                    std::uint32_t draws = 0,
                    const char* nickname = "",
                    std::uint16_t clanLogoFront = 0,
                    std::uint16_t clanLogoBack = 0,
                    const char* clanName = "",
                    std::uint32_t unknown = 0
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
                    std::uint8_t serverId : 8;
                    std::uint8_t channel1 : 2;
                    std::uint8_t channel2 : 2;
                    std::uint8_t channel3 : 2;
                    std::uint8_t channel4 : 2;
                    std::uint8_t channel5 : 2;
                    std::uint8_t channel6 : 2;
                    std::uint8_t channel7 : 2;
                    std::uint8_t channel8 : 2;
                    std::uint8_t channel9 : 2;
                    std::uint8_t channel10 : 2;
                };
                std::uint32_t data;

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
                    std::uint32_t id : 6;
                    std::uint32_t sale_price : 15;
                    std::uint32_t unknown : 11;
                };
                std::uint32_t data;

                GachaSaleDataId(std::uint32_t uint32_t = 0)
                {
                    std::memset(this, 0, sizeof(uint32_t));
                    this->data = data;
                }
                GachaSaleDataId(std::uint32_t new_id, std::uint32_t new_sale_price, std::uint32_t new_unknown)
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
                std::uint32_t start_date;
                std::uint32_t end_date;
                MainGachaponSaleInfo(
                    std::uint32_t id = 0,
                    std::uint32_t sale_price = 0,
                    std::uint32_t start_date = 0,
                    std::uint32_t end_date = 0
                ) :
                    start_date(start_date),
                    end_date(end_date)
                {
                    data = GachaSaleDataId(id, sale_price, 0);
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
                    std::uint32_t id : 20;
                    std::uint32_t sid : 4;
                    std::uint32_t unknown : 4;
                    std::uint32_t origin : 4;
                    std::uint32_t creation_date;
                };
                std::uint64_t data;

                ItemSerialInfo(std::uint64_t data = 0)
                {
                    std::memset(this, 0, sizeof(ItemSerialInfo));
                    this->data = data;
                }
                ItemSerialInfo(std::uint32_t id , std::uint32_t sid , std::uint32_t unknown, std::uint32_t origin, std::uint32_t creation_date)
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
                    std::uint32_t item_id : 23;
                    std::uint32_t stock : 9;
                };
                std::uint32_t data;

                InventoryItemNumber(std::uint32_t uint32_t = 0)
                {
                    std::memset(this, 0, sizeof(uint32_t));
                    this->data = data;
                }
                InventoryItemNumber(std::uint32_t new_item_id, std::uint32_t new_stock)
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
                    std::uint32_t item_type : 9;
                    std::uint32_t item_id : 23;
                   
                };
                std::uint32_t data;

                EquipItemNumber(std::uint32_t uint32_t = 0)
                {
                    std::memset(this, 0, sizeof(uint32_t));
                    this->data = data;
                }
                EquipItemNumber(std::uint32_t new_item_id, std::uint32_t new_type)
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
                    std::uint32_t gachapon_id : 8;
                    std::uint32_t item_id : 24;
                };
                std::uint32_t data;

                GachaponWonItemMsg(std::uint32_t data = 0)
                {
                    std::memset(this, 0, sizeof(GachaponWonItemMsg));
                    this->data = data;
                }
                GachaponWonItemMsg(std::uint32_t gachapon_id, std::uint32_t item_id)
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
                std::uint32_t expire_date;//0x04
                ItemSerialInfo serial_info;//0x08
                std::uint16_t repair;//0x10
                std::uint16_t energy;//0x12
                InventoryItemInfo(
                    const InventoryItemNumber& itemNumber = 0,
                    const std::uint32_t& expireDate = 0,
                    const ItemSerialInfo& serialInfo = ItemSerialInfo(),
                    const std::uint32_t& repairVal = 0,
                    const std::uint32_t& energyVal = 0)
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
                std::uint32_t expire_date;
                ItemSerialInfo serial_info;
                std::uint16_t repair;
                std::uint16_t energy;
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
                std::uint32_t expire_date;//0x04
                ItemSerialInfo serial_info;//0x08
                std::uint16_t repair;//0x10
                std::uint16_t energy;//0x12
                std::uint32_t is_sealed;//0x14
                std::uint32_t seal_level;
                std::uint32_t enhance_exp;
                std::uint32_t enhance_level;
                std::uint32_t item_type;
                InventoryItemInfo(
                    const InventoryItemNumber& itemNumber = 0,
                    const std::uint32_t& expireDate = 0,
                    const ItemSerialInfo& serialInfo = ItemSerialInfo(),
                    const std::uint32_t& repairVal = 0,
                    const std::uint32_t& energyVal = 0,
                    const std::uint32_t& isSealed = 0,
                    const std::uint32_t& sealLevel = 0,
                    const std::uint32_t& enhanceExp = 0,
                    const std::uint32_t& enhanceLevel = 0,
                    const std::uint32_t& itemType = 0)
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
                std::uint32_t expire_date;
                ItemSerialInfo serial_info;
                std::uint16_t repair;
                std::uint16_t energy;
                std::uint32_t is_sealed;
                std::uint32_t seal_level;
                std::uint32_t enhance_exp;
                std::uint32_t enhance_level;
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
                std::uint32_t expire_time;
                ShopSerialInfo(
                    const ItemSerialInfo& serialInfo = ItemSerialInfo(),
                    const std::uint32_t& expireDate = 0)
                    : serial_info(serialInfo),
                    expire_time(expireDate)
                {
                }
            };
            struct BossItem
            {
                std::uint32_t unique_id;
                std::uint32_t item_id;
            };
            struct ShopItem
            {
                InventoryItemNumber item_number;
                std::uint32_t expire_time;
                ItemSerialInfo serial_info;
                ShopItem(
                    const InventoryItemNumber& itemNumber = InventoryItemNumber(0),
                    const std::uint32_t& expireDate = 0,
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
                std::uint32_t mail_id;
                std::uint32_t mail_time;
                ShopItem item;
                MailboxGift(
                    const std::uint32_t& mailId = 0,
                    const std::uint32_t& mailTime = 0,
                    const ShopItem& item = ShopItem())
                    : mail_id(mailId),
                    mail_time(mailTime),
                    item(item)
                {
                };
            };
            struct PlayerFriendInfo
            {
                std::uint32_t unique_id;
                std::int32_t friend_id;
                char nickname[16];
                PlayerFriendInfo(const std::uint32_t& uniqueId = 0, const std::int32_t& friendId = 0, const char* newNickname = "")
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
                std::int32_t acc_id;
                char nickname[16];
                PlayerBlockedInfo(const std::int32_t& accId = 0, const char* newNickname = "")
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
                    std::uint64_t clanIdk : 4; // >> 0 (0 display clan, anythin else idk doesnt work)
                    std::uint64_t logo_front : 16; // >> 4
                    std::uint64_t logo_back : 14; // >> 20
                    std::uint64_t clanId : 27; // >> 34
                    std::uint64_t unknown2 : 3; // >> 61
                    
                };
                std::uint64_t data;
            };
            struct PlayerRoomClanListInfo
            {
                char clanName[16];
                PlayerClanInfoRoom info_room;

                PlayerRoomClanListInfo(std::uint32_t slotIndex = 0, std::string clanName = "", std::uint32_t logo_front = 0, std::uint32_t logo_back = 0, std::uint32_t clanId = 0, std::uint32_t unknown2 = 0)
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
                    std::uint32_t level : 7;
                    std::uint32_t unknown : 25;
                };
                std::uint32_t data;
            };
            struct PlayerClanInfo
            {
                char nickname[16];
                std::uint32_t unique_id;
                PlayerClanListInfo clanInfo;

                PlayerClanInfo(std::string nickname, std::uint32_t uniqueId, std::uint32_t level)
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
                    std::uint64_t logo_front : 16;
                    std::uint64_t logo_back : 14;
                    std::uint64_t level : 7;
                };
                std::uint64_t data;
            };
            struct PlayerAgoraInfo 
            {
                char nickname[16];
                std::uint32_t unique_id;
                PlayerClanInfoLobby clanInfo;

                PlayerAgoraInfo(std::string nickname, std::uint32_t uniqueId, std::uint32_t level, std::uint32_t clanIconFront, std::uint32_t clanIconBack)
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
                    std::uint32_t unused : 10;
                    std::uint32_t ping : 10;
                    std::uint32_t rest : 12;
                };
                std::uint32_t data;
            };
            union RoomUserPlayerInfo1
            {
            #if defined(RELEASE_1_0_3)
                struct
                {
                    std::uint32_t grade : 4;
                    std::uint32_t vip_level : 3;
                    std::uint32_t character : 4;
                    std::uint32_t team : 4;
                    std::uint32_t level : 7;
                    std::uint32_t ping : 10;
                };
            #else
                struct
                {
                    std::uint32_t grade : 4; //  << 0
                    std::uint32_t vip_level : 3; // << 4
                    std::uint32_t character : 5; // << 7
                    std::uint32_t team : 3; // << 12
                    std::uint32_t level : 7; // << 15
                    std::uint32_t ping : 10; // << 22
                };
            #endif
                std::uint32_t data;
            };
            union RoomUserPlayerInfo2
            {
                struct
                {
                    std::uint32_t fps_limit : 2;
                    std::uint32_t player_state : 4;
                    std::uint32_t ping : 10;
                    std::uint32_t unknown : 16;
                };
                std::uint32_t data;
            };
            union RoomUserPlayerInfo3
            {
                struct
                {
                    std::uint32_t unknown : 3;
                    std::uint32_t ping : 10;
                    std::uint32_t fps_limit : 2;
                };
                std::uint32_t data;
            };
            union RoomPlayerInfo1
            {
                struct
                {
                    std::uint32_t map_index : 7;
                    std::uint32_t mode_index : 5;
                    std::uint32_t max_players : 5;
                    std::uint32_t unknown : 1;
                    std::uint32_t has_password : 1; 
                    std::uint32_t allow_intruders : 1; 
                };
                std::uint32_t data;
            };
            union RoomSettingsModeInfo2
            {
                struct
                {
                    std::uint64_t time_limit : 5;
                    std::uint64_t score_limit : 5;
                    std::uint64_t allow_items : 1;
                    std::uint64_t unknown : 2;
                    std::uint64_t restriction : 4;
                };
                std::uint64_t data;
            };
            union TDM_ModeInfo
            {
                struct
                {
                    std::uint64_t redscore : 8;
                    std::uint64_t bluescore : 8;
                    std::uint64_t winrule : 8;
                    std::uint64_t state : 2;
                    std::uint64_t kitdrop : 1;
                    std::uint64_t timelimited : 5;
                    std::uint64_t weaponlimited : 4;
                };
                std::uint64_t data;
            };
            union Zombie_ModeInfo
            {
                struct
                {
                    std::uint64_t redscore : 8;
                    std::uint64_t bluescore : 8;
                    std::uint64_t winrule : 8;
                    std::uint64_t state : 2;
                    std::uint64_t kitdrop : 1;
                    std::uint64_t timelimited : 5;
                    std::uint64_t weaponlimited : 4;
                };
                std::uint64_t data;
            };
            union Elimination_ModeInfo
            {
                struct
                {
                    std::uint64_t redscore : 8;
                    std::uint64_t bluescore : 8;
                    std::uint64_t winrule : 8;
                    std::uint64_t state : 2;
                    std::uint64_t kitdrop : 1;
                    std::uint64_t timelimited : 5;
                    std::uint64_t weaponlimited : 4;
                };
                std::uint64_t data;
            };
            union CaptureTheBattery_ModeInfo
            {
                struct
                {
                    std::uint64_t redscore : 8;
                    std::uint64_t bluescore : 8;
                    std::uint64_t winrule : 8;
                    std::uint64_t state : 2;
                    std::uint64_t kitdrop : 1;
                    std::uint64_t timelimited : 5;
                    std::uint64_t weaponlimited : 4;
                };
                std::uint64_t data;
            };
            union FFA_ModeInfo
            {
                struct
                {
                    std::uint64_t timelimited : 5; // >> 0
                    std::uint64_t winrule : 5; // >> 5
                    std::uint64_t kitdrop : 1; // >> 10
                    std::uint64_t state : 2; // >> 11
                    std::uint64_t weaponlimited : 4; // >> 13
                };
                std::uint64_t data;
            };
            struct BossBattle_ModeInfo
            {
                std::uint32_t unknown1;//0
                std::uint32_t unknown2;//4
                std::uint32_t pos1 : 16;//8
                std::uint32_t pos2 : 16;//10
                std::uint32_t pos3 : 16;//12
                std::uint32_t pos4 : 16;//14
                std::uint32_t pos5 : 16;//16
                std::uint32_t pos6 : 16;//18
                std::uint64_t winrule : 8;//20
                std::uint64_t state : 2;
                std::uint64_t kitdrop : 1;
                std::uint64_t timelimited : 5;
                std::uint64_t weaponlimited : 4;
            };
            struct BombBattle_ModeInfo
            {
                std::uint32_t unknown1;//0
                std::uint32_t unknown2;//4
                std::uint32_t pos1 : 16;//8
                std::uint32_t pos2 : 16;//10
                std::uint32_t pos3 : 16;//12
                std::uint32_t pos4 : 16;//14
                std::uint32_t pos5 : 16;//16
                std::uint32_t pos6 : 16;//18
                std::uint32_t redscore : 8;//20
                std::uint32_t bluescore : 8;
                std::uint32_t winrule : 8;
                std::uint32_t state : 2;
                std::uint32_t kitdrop : 1;
                std::uint32_t timelimited : 5;
                std::uint32_t weaponlimited : 4;
            };
            union ArmsRace_ModeInfo
            {
                struct
                {
                    std::uint64_t timelimited : 5;
                    std::uint64_t winrule : 5;
                    std::uint64_t kitdrop : 1;
                    std::uint64_t state : 2;
                    std::uint64_t weaponlimited : 4;
                };
                std::uint64_t data;
            };

            union Scrimmage_ModeInfo
            {
                struct
                {
                    std::uint64_t redscore : 20; // >> 0
                    std::uint64_t bluescore : 20; // >> 20
                    std::uint64_t winrule : 5; // >> 40
                    std::uint64_t state : 2; // >> 45
                    std::uint64_t timelimited : 5; // >> 47
                    std::uint64_t weaponlimited : 4; // >> 52
                };
                std::uint64_t data;
            };
            union GameModeSettingsUpdateInfo
            {
                struct
                {
                    std::uint32_t max_players : 5;
                    std::uint32_t unknown1 : 11;
                    std::uint32_t map_index : 7;
                    std::uint32_t unknown2 : 9;
                };
                std::uint32_t data;
            };
            union RoomSettingsUpdateInfo
            {
                struct
                {
                    std::uint32_t max_players : 5;
                    std::uint32_t time : 5;
                    std::uint32_t restriction : 4;
                    std::uint32_t allow_items : 1;
                    std::uint32_t allow_intruders : 1;
                    std::uint32_t map_index : 6;
                    std::uint32_t unknown1 : 1;
                    std::uint32_t unknown2 : 1;
                    std::uint32_t score_limit : 5;
                    std::uint32_t team_balance : 2;
                    std::uint32_t unknown3 : 1;
                };
                std::uint32_t data;
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
                    std::uint32_t map_index : 7;
                    std::uint32_t mode_index : 5;
                    std::uint32_t max_players : 5; 
                    std::uint32_t has_matchStarted : 1; 
                    std::uint32_t has_password : 1;
                    std::uint32_t allow_intruders : 1; 
                    std::uint32_t restriction : 4;
                    std::uint32_t is_clan_room : 2;
                    std::uint32_t team_balance : 2;
                    std::uint32_t allow_observers : 1; 
                    std::uint32_t hide_password : 1;
                    std::uint32_t unknown1 : 1;
                    std::uint32_t unknown2 : 1; 
                };
            #else
                struct
                {
                    std::uint32_t map_index : 7;
                    std::uint32_t mode_index : 5;
                    std::uint32_t max_players : 5;
                    std::uint32_t has_matchStarted : 1;
                    std::uint32_t has_password : 1;
                    std::uint32_t allow_intruders : 1;
                    std::uint32_t restriction : 4;
                    std::uint32_t is_clan_room : 1;
                    std::uint32_t team_balance : 2;
                    std::uint32_t allow_observers : 1;
                    std::uint32_t hide_password : 1;
                    std::uint32_t unknown1 : 1;
                    std::uint32_t unknown2 : 1;
                    std::uint32_t unknown3 : 1;
                };
            #endif
                std::uint32_t data;
            };
            union RoomSettings
            {
                struct
                {
                    std::uint32_t time : 5;
                    std::uint32_t restriction : 4;
                    std::uint32_t allow_items : 1;
                    std::uint32_t mode_index : 4;
                    std::uint32_t unknown1 : 1;
                    std::uint32_t allow_intruders : 1;
                    std::uint32_t has_password : 1;
                    std::uint32_t unknown2 : 1;
                    std::uint32_t max_players : 4;
                    std::uint32_t map_index : 7;
                    std::uint32_t team_balance : 2;
                    std::uint32_t allow_observers : 1;
                    
                };
                std::uint32_t data;
            };
            struct RoomSettingsInfo
            {
                char title[32];
                char password[16];
                RoomSettings settings;
                RoomSettingsInfo(std::uint32_t settings_data = 0, const char* roomtitle = "", const char* roompass = "")
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
                    std::uint32_t room_id : 16;
                    //std::uint32_t room_id : 11;
                    //std::uint32_t main_id : 5;
                    std::uint32_t channel_id : 9; // >> 16
                    std::uint32_t map_index : 7; // >> 25
                    std::uint32_t mode_index : 5; // >> 32
                    std::uint32_t max_players : 5; // >> 37
                    std::uint32_t current_players : 5; // >> 42
                    std::uint32_t is_playing : 1; // >> 47
                    std::uint32_t has_password : 1; // >> 48
                    std::uint32_t is_observer_off : 1; // >> 49
                    std::uint32_t weapon_restriction : 4; // >> 50
                    std::uint32_t is_observer_enabled : 1; // >> 54
                    std::uint32_t host_ping : 9; // >> 55
                };
                std::uint64_t data;
            };
            struct MailboxMsgInfo
            {
                std::uint32_t mail_id;
                std::uint32_t date;
                std::uint32_t is_opened;
                char nickname[16];
                char msg[256];
            };
            struct GiftboxMsgInfo
            {
                std::uint32_t mail_id;
                std::uint32_t date;
                std::uint32_t item_id;
                std::uint32_t unknown1 = 1;
                std::uint32_t unknown2 = 0;
                char nickname[16];
                char msg[256];
            };
            struct RoomListInfo
            {
                char title[32];
                RoomDetails details;
                std::uint32_t unknown;

                RoomListInfo(
                    const std::string& room_title, 
                    const std::uint32_t& room_id, 
                    const std::uint32_t& channel_id, 
                    const std::uint32_t& map_index, 
                    const std::uint32_t& mode_index, 
                    const std::uint32_t& max_players, 
                    const std::uint32_t& current_players, 
                    const bool& is_playing, 
                    const bool& has_password, 
                    const bool& is_observer_off, 
                    const std::uint32_t& weapon_restriction,
                    const bool& is_observer_enabled, 
                    const std::uint32_t& host_ping)
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
                std::uint32_t red_score : 8;
                std::uint32_t blue_score : 8;
                std::uint32_t unknown1 : 8;
                std::uint32_t unknown2 : 8;
            };
            struct MainRoomEndMatchClientInfo
            {
                std::uint32_t melee_kills : 8;
                std::uint32_t rifle_kills : 8;
                std::uint32_t shotgun_kills : 8;
                std::uint32_t sniper_kills : 8;
                std::uint32_t gatling_kills : 8;
                std::uint32_t bazooka_kills : 8;
                std::uint32_t grenade_kills : 8;
                std::uint32_t killstreak : 8;
                std::uint32_t total_kills : 8;
                std::uint32_t deaths : 8;
                std::uint32_t headshots : 8;
                std::uint32_t assists : 8;
                std::uint32_t unknown1 : 8;
                std::uint32_t unknown2 : 8;
                std::uint32_t unknown3 : 8;
                std::uint32_t unknown4 : 8;
                std::uint32_t unique_id;
            };

            struct MainRoomEndMatchScoreClientBossBattleInfo
            {
                std::uint32_t player_count;
            };
            struct MainRoomEndMatchClientBossBattleInfo
            {
                
                std::uint32_t unique_id;
                std::uint32_t unknown[17];
            };


            // S2C
            struct MainRoomEndMatchResponse
            {
                std::uint32_t melee_kills : 8;
                std::uint32_t rifle_kills : 8;
                std::uint32_t shotgun_kills : 8;
                std::uint32_t sniper_kills : 8;
                std::uint32_t gatling_kills : 8;
                std::uint32_t bazooka_kills : 8;
                std::uint32_t grenade_kills : 8;
                std::uint32_t killstreak : 8;
                std::uint32_t total_kills : 8;
                std::uint32_t deaths : 8;
                std::uint32_t headshots : 8;
                std::uint32_t assists : 8;
                std::uint32_t unknown;
                std::uint32_t total_mp;
                std::uint32_t total_xp;
                std::uint32_t unknown2;
            };

#pragma pack(pop)
        }

        namespace Cast
        {

#pragma pack(push, 1)
            struct PositionStruct
            {
                DirectX::PackedVector::HALF positionX{};//0
                DirectX::PackedVector::HALF positionY{};//2
                DirectX::PackedVector::HALF positionZ{};//4
            };
            struct DirectionStruct
            {
                DirectX::PackedVector::HALF directionX{};//6
                DirectX::PackedVector::HALF directionY{};//8
                DirectX::PackedVector::HALF directionZ{};//10
            };
            struct BulletsStruct
            {
                DirectX::PackedVector::HALF bullet1{};
                DirectX::PackedVector::HALF bullet2{};
                DirectX::PackedVector::HALF bullet3{};
                DirectX::PackedVector::HALF bullet4{};
            };
            struct JumpStruct
            {
                DirectX::PackedVector::HALF jump1;
                DirectX::PackedVector::HALF jump2;
            };
            struct ClientPlayerInfoBasic
            {
                PositionStruct position;//0
                DirectionStruct direction;//6
                std::uint32_t matchTick{};//12
            #if defined(RELEASE_1_0_3)
                std::uint32_t animation1 : 8 = 0;//16
            #else
                std::uint32_t animation1 : 7 = 0;
            #endif
                std::uint32_t animation2 : 6 = 0;
                std::uint32_t weapon : 4 = 0;
                std::uint32_t rotation : 9 = 0;
            #if defined(RELEASE_1_0_3)
                std::uint32_t unknown : 5 = 0;
            #else
                std::uint32_t unknown : 6 = 0;
            #endif
               
            };
            struct ClientPlayerInfoJump
            {
                ClientPlayerInfoBasic playerPositionBasic;
                JumpStruct jumpStruct{};
            };
            struct ClientPlayerInfoBullet
            {
                ClientPlayerInfoBasic playerPositionBasic;
                BulletsStruct bulletStruct{};
            };
            struct ClientPlayerInfoComplete
            {
                ClientPlayerInfoBasic playerPositionBasic;
                BulletsStruct bulletStruct{};
                JumpStruct jumpStruct{};
            };
            struct SpecificInfo
            {
                std::uint32_t sessionId : 14 = 0;
                std::uint32_t enableMovement : 1 = true;// >> 14
                std::uint32_t enableBullet : 1 = false;// >> 15
                std::uint32_t animation1 : 7 = 0;// >> 16
                std::uint32_t enableRotation : 1 = true;// >> 23
                std::uint32_t animation2 : 6 = 0;// >> 24
                std::uint32_t unknown : 1 = true; // >> 30
                std::uint32_t enableJump : 1 = false; // >> 31
            };
            struct PlayerInfoBasicResponse
            {
                std::uint32_t tick{};//0
                SpecificInfo specificInfo{};//4
                PositionStruct position;//+0
                DirectionStruct direction;//+6
                std::uint32_t rotation1 : 8 = 0;//+12
                std::uint32_t rotation2 : 8 = 0;//+13
                std::uint32_t rotation3 : 9 = 0;//+ 14
                std::uint32_t currentWeapon : 4 = 0;
            };
            struct PlayerInfoResponseWithJump
            {
                PlayerInfoBasicResponse playerInfoBasicResponse;
                JumpStruct jump{};
            };
            struct PlayerInfoResponseWithBullets
            {
                std::uint32_t tick{};
                SpecificInfo specificInfo{};
                PositionStruct position;
                DirectionStruct direction;
                BulletsStruct bullets{};
                std::uint32_t rotation1 : 8 = 0;
                std::uint32_t rotation2 : 8 = 0;
                std::uint32_t rotation3 : 9 = 0;
                std::uint32_t currentWeapon : 4 = 0; // + 22
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
                    std::uint32_t session : 16;
                    std::uint32_t server : 15;
                    std::uint32_t  unknown : 1;
                };
                std::uint32_t data;

                UniqueId()
                {
                    std::memset(this, 0, sizeof(UniqueId));
                }

                UniqueId(uint32_t data)
                {
                    std::memset(this, 0, sizeof(UniqueId));
                    this->data = data;
                }
                UniqueId(std::uint16_t session, std::uint16_t server)
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