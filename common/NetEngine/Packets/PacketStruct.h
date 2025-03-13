#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>

#include "PacketData.h"
namespace NetEngine
{
    namespace Packets
    {
        namespace Front
        {
#pragma pack(push, 1)

            // C2S

            struct FrontLoginAuthorizeReq
            {
                std::uint32_t  cryptoKey;
                char      password[40];
                std::uint32_t  serverTime;
                char      username[68];
                FrontLoginAuthorizeReq() : cryptoKey(0), serverTime(0)
                {
                    std::memset(password, 0, sizeof(password));
                    std::memset(username, 0, sizeof(username));
                }
                FrontLoginAuthorizeReq(std::uint32_t cryptoKey, char password[], std::uint32_t serverTime, char username[]) : cryptoKey(cryptoKey), serverTime(serverTime)
                {
                    std::memcpy(this->password, password, sizeof(this->password));
                    std::memcpy(this->username, username, sizeof(this->username));
                }
                FrontLoginAuthorizeReq(uint32_t cryptoKey, const char* password, std::uint32_t serverTime, const char* username) : cryptoKey(cryptoKey), serverTime(serverTime)
                {
                    std::memcpy(this->password, password, sizeof(this->password));
                    std::memcpy(this->username, username, sizeof(this->username));
                }
            };

            struct FrontLoginReconnectReq
            {
                std::uint64_t authKey;

                FrontLoginReconnectReq(std::uint64_t authKey) : authKey(authKey) {}
            };

            struct FrontServerInfoReq
            {
            };

            // S2C

            struct FrontEngineServerConnectionAck
            {
                std::int32_t cryptoKey;
                std::uint32_t serverTime;

                FrontEngineServerConnectionAck(std::int32_t cryptoKey, std::uint32_t serverTime) : cryptoKey(cryptoKey), serverTime(serverTime)
                {
                }
            };

            struct FrontLoginAuthorizeAck
            {
                std::uint64_t              authKey;
                FrontUserAccountInfo  accountInfo;

                FrontLoginAuthorizeAck(std::uint64_t authKey, FrontUserAccountInfo accountInfo) : authKey(authKey), accountInfo(accountInfo)
                {
                }
            };

            struct FrontLoginReconnectAck
            {
                std::uint64_t authKey;

                FrontLoginReconnectAck(std::uint64_t authKey) : authKey(authKey)
                {
                }
            };

            struct FrontServerInfoAck
            {
                FrontServerInfo serverInfos[];

                FrontServerInfoAck(std::vector<FrontServerInfo> serverInfos)
                {
                    std::memset(this, 0, serverInfos.size() * sizeof(FrontServerInfo));
                    std::copy(serverInfos.begin(), serverInfos.end(), this->serverInfos);
                }

                FrontServerInfoAck(std::uint8_t* data, std::size_t size)
                {
                    std::memset(this, 0, sizeof(FrontServerInfoAck));
                    std::size_t structSize = sizeof(FrontServerInfo);
                    std::size_t elementCount = size / sizeof(FrontServerInfo);
                    for (std::size_t i = 0; i < elementCount; i++) this->serverInfos[i] = *(FrontServerInfo*)(data + i * structSize);
                    
                }
            };

#pragma pack(pop)
        }

        namespace Main
        {
#pragma pack(push, 1)

            // C2S

            struct MainVersionCheckReq
            {
                std::uint64_t  authKey;
                std::uint32_t  versionCheck;
                std::uint8_t   NetVersion1;
                std::uint8_t   NetVersion2;
                std::uint8_t   NetVersion3;
                std::uint8_t   NetVersion4;
            };
            struct MainNicknameCreationReq
            {
                char Nickname[16];
            };
            struct MailboxUpdateInfo
            {
                std::uint32_t mail_id;
                std::uint32_t mail_time;
            };
            struct MailBoxUpdateReq
            {
                std::uint32_t mail_count;
                MailboxUpdateInfo mail_info[100];
            };
            struct MainBuyItemIdReq
            {
                std::uint32_t items[86];
            };
            struct MainBuyItemSerialInfoReq
            {
                ItemSerialInfo items[86];
            };
            struct MainSellItemSerialInfoReq
            {
                ItemSerialInfo item;
                std::uint32_t sell_price;
            };
            struct MainDeleteItemSerialInfoReq
            {
                std::uint32_t item_count;
                ItemSerialInfo items[86];
            };
            struct MainRepairItemSerialInfoReq
            {
                ItemSerialInfo items[86];
            };

            struct MainUpgradeEnergyInjectReq
            {
                ItemSerialInfo item;
                std::uint32_t energy;
            };
            struct MainUpgradeResetReq
            {
                ItemSerialInfo item;
                ItemSerialInfo upgrade_reset_item;
            };
            struct MainUpgradeItemReq
            {
                ItemSerialInfo item[4];
            };
        #if defined(RELEASE_1_0_3)
            struct MainGachaponSpinReq
            {
                union
                {
                    struct
                    {
                        std::uint32_t gachapon_id : 6;
                        std::uint32_t gachapon_price : 15;
                        std::uint32_t reserved : 11;
                    };
                    std::uint32_t gachaponData;//0x4C
                };
            };
        #else
            struct MainGachaponSpinReq
            {
                std::uint32_t gachapon_id;
                std::uint32_t gachapon_price;
            };
        #endif
            struct MainUsePackageItemHammerReq
            {
                ItemSerialInfo item;
                ItemSerialInfo mistery_capsule;
            };
            struct MainUsePackageItemNicknameReq
            {
                ItemSerialInfo item;
                char nickname[16];
            };
            struct MainGetEnergyInGameReq
            {
                std::uint32_t uniqueId;
            };
            struct MainPickupItemreq
            {
                std::uint16_t picked_up_drop;
                std::uint16_t total_drop_count;
                std::uint32_t drop_item_id;
            };
            struct MainUsePackageItemReq
            {
                ItemSerialInfo item;
                std::uint32_t unknown;
            };
            struct MainCharacterEquipUpdateReq
            {
                ItemSerialInfo item[84];
            };
            struct EquipSwitchItem
            {
                std::uint16_t character_id;
                std::uint32_t item_id;
            };
            struct MainCharacterEquipSwitchReq
            {
                EquipSwitchItem item[84];
            };
            struct MainPlayerFriendAddSendReq
            {
                char nickname[16];
            };
            struct MainPlayerFriendAddRecvReq
            {
                std::uint32_t unique_id;
                std::uint32_t player_id;
                char nickname[16];
            };
            struct MainPlayerFriendRemoveReq
            {
                std::uint32_t player_id;
            };
            struct MainPlayerBlockedAddReq
            {
                char nickname[16];
            };
            struct MainPlayerBlockedRemoveReq
            {
                std::uint32_t player_id;
            };
            struct MainPlayerDetailsInfoReq
            {
                std::uint32_t unique_id;
            };
            struct MainMailboxSendReq
            {
                char nickname[16];
                char msg[256];
            };
            struct MainChatWhisperReq
            {
                char nickname[16];
                char msg[256];
            };
            struct MainChatWhisperInRoomReq
            {
                Core::UniqueId unique_id;
                char msg[256];
            };
            struct MainChatReq
            {
                char msg[256];
            };
            struct MainCreateRoomReq
            {
                std::uint32_t settings_data;
                char title[32];
                char password[16];
            };
            struct MainLeaveRoomReq
            {
                std::uint32_t uniqueId;
            };
            struct MainJoinRoomReq
            {
                std::uint16_t room_id;
                std::uint16_t channel_id;
                char password[16];
            };

            struct MainJoinPlazaReq
            {
                std::uint16_t plaza_id;
                std::uint16_t channel_id;
            };

            struct MainCreatePartyReq
            {
                std::uint32_t unknown;
            };
            struct MainLeavePartyReq
            {
                std::uint32_t unknown;
            };
            struct SingleWaveEndReq
            {
                std::uint32_t type;
                std::uint32_t score;
                std::uint32_t stage;
            };
            struct MainCompleteMissionReq
            {
                std::uint32_t collection_id;
                std::uint32_t set_index;
                std::uint32_t idk1;
                std::uint32_t mission_type;//1 for guide mission and 4 for daily mission
            };
            struct MainVoteKickReq
            {
                std::uint32_t target_unique_id;
                std::uint32_t reason_id;
            };
            struct MainVoteKickAck
            {
                std::uint32_t target_unique_id;
                std::uint32_t voter_unique_id;
                std::uint32_t reason_id;
                std::uint32_t vote_time_tick;
            };
            struct MainEngineServerConnectionAck
            {
                std::int32_t cryptoKey;
                Core::UniqueId unique_id;

                MainEngineServerConnectionAck(std::int32_t crypto_key, std::uint16_t session_id, std::uint16_t server_id)
                {
                    cryptoKey = crypto_key;
                    unique_id.session = session_id;
                    unique_id.server = server_id;
                }
            };

            struct MainVersionCheckAck
            {
            };

            struct MainAccountInfoAck 
            {
                std::uint64_t Diorama;//0x00
                std::uint32_t Kills;//0x08
                std::uint32_t Deaths;//0x0C
                std::uint32_t Assists;//0x10
                std::uint32_t Wins;//0x14
                std::uint32_t Loses;//0x18
                std::uint32_t Draws;//0x1C
                std::uint32_t Melee;//0x20
                std::uint32_t Rifle;//0x24
                std::uint32_t Shotgun;//0x28
                std::uint32_t Sniper;//0x2C
                std::uint32_t Gatling;//0x30
                std::uint32_t Bazooka;//0x34
                std::uint32_t Grenade;//0x38
                union {
                    struct {
                        std::uint64_t unused : 27;
                        std::uint64_t Headshots : 29;
                        std::uint64_t HighestKillStreak : 8;
                    };
                    std::uint64_t HeadshotsAndHighestKillStreak;//0x3C
                };
                std::uint32_t Unknown2;//0x44
                std::uint32_t PlayTime;//0x48
                union {
                    struct {
                        std::uint32_t ClanPadding : 3;
                        std::uint32_t ClanId : 29;
                    };
                    std::uint32_t ClanInfo;//0x4C
                };
                std::array<std::uint64_t, 4> Achievements;//0x50
                std::uint32_t ZombieKillPoints;//0x70
                std::uint32_t Infections;//0x74
                std::uint32_t Unknown3;//0x78
                char Nickname[16];//0x7C
                std::uint64_t ServerTime;//0x8C
                std::uint32_t UniqueId;//0x94
            #if defined(RELEASE_1_0_3)
                union {
                    struct {
                        std::uint64_t Grade : 5; // >> 0
                        std::uint64_t SelectedCharacter : 4; // >> 5
                        std::uint64_t OwnedCharacters : 8; // >> 9
                        std::uint64_t Level : 7; // >> 17
                        std::uint64_t Energy : 14; // >> 24
                        std::uint64_t Energy2 : 14; // >> 38
                        std::uint64_t LuckyPoints : 12; // >> 52
                    };
                    std::uint64_t GradeCharacterInfoCurrencyInfo;//0x98
                };
            #else
                union
                {
                    struct
                    {
                        std::uint32_t Grade : 5; // >> 0
                        std::uint32_t SelectedCharacter : 4; // >> 5

                        std::uint32_t OwnedCharacters : 16; // >> 9
                        std::uint32_t Level : 7; // >> 25
                    };
                    std::uint32_t GradeCharacterInfo;//0x98
                };
                union
                {
                    struct
                    {
                        std::uint32_t Coins : 7; // >> 32
                        std::uint32_t Energy : 14; // >> 39
                        std::uint32_t LuckyPoints : 11; // >> 53
                    };
                    std::uint32_t CurrencyInfo;//0x9C
            };
            #endif

      
                std::uint32_t Experience;//0xA0
                union {
                    struct {
                        std::uint64_t MicroPoints : 31;
                        std::uint64_t RockTokens : 30;
                        std::uint64_t GoldenMode : 3;
                    };
                    std::uint64_t MicroPointsAndRockTokens;//0xA4
                };

                union {
                    struct {
                        std::uint32_t Tutorial : 22;
                        std::uint32_t MaximumItems : 10;
                    };
                    std::uint32_t TutorialAndItems;//0xAC
                };

                union {
                    struct {
                        std::uint32_t MaximumEnergy : 13;
                        std::uint32_t DailyAttempts : 9;
                        std::uint32_t HighestWave : 10;
                    };
                    std::uint32_t EnergyAndWaveInfo;//0xB0
                };
                std::uint32_t SinglewaveHighscore;//0xB4
                std::uint32_t Unknown4;//0xB8
                std::uint32_t Story;//0xBC
            #if defined(RELEASE_1_1_1)
                std::uint32_t VIPLevel;//0xC0
            #endif
                //std::uint32_t AccountId;//0xC4
                std::uint64_t AccountAuthkey;//0xC8
                char Unused[8];//0xCC
                char ClanName[16];//0xD4

                union {
                    struct {
                        std::uint64_t ClanLogoFront : 16;
                        std::uint64_t ClanLogoBack : 14;
                        std::uint64_t ClanContribution : 34;
                    };
                    std::uint64_t ClanLogoAndContribution;
                };

                union {
                    struct {
                        std::uint64_t ClanWins : 23;
                        std::uint64_t ClanLoses : 23;
                        std::uint64_t ClanDraws : 18;
                    };
                    std::uint64_t ClanMatchResults;
                };

                std::uint32_t ClanKills;
                std::uint32_t ClanDeaths;
                std::uint32_t ClanAssists;
                MainAccountInfoAck()
                {
                    std::memset(this, 0, sizeof(MainAccountInfoAck));
                    std::memset(Nickname, 0, sizeof(Nickname));
                    std::memset(Unused, 0, sizeof(Unused));
                    std::memset(ClanName, 0, sizeof(ClanName));
                    for (auto& achievement : Achievements) achievement = 0;
                }
            };
            struct MainNicknameCreationAck
            {
            };
           
            struct MainSellItemAck
            {
                ItemSerialInfo serial_info;
                std::uint32_t sell_price;
                MainSellItemAck(ItemSerialInfo serialInfo, std::uint32_t sellPrice)
                {
                    std::memset(this, 0, sizeof(MainSellItemAck));
                    serial_info.data = serialInfo.data;
                    sell_price = sellPrice;
                }
            };
            
            struct MainCurrencyUpdateAck
            {
                std::uint32_t RockTokens;
                std::uint32_t MicroPoints;
                std::uint32_t Coins;
                MainCurrencyUpdateAck(std::uint32_t RockTokens, std::uint32_t MicroPoints, std::uint32_t Coins) : RockTokens(RockTokens), MicroPoints(MicroPoints), Coins(Coins) {}
            };

           
            struct MainShopBuyItemSerialInfoResultAck
            {
                ShopSerialInfo shop_items[];

                MainShopBuyItemSerialInfoResultAck(std::vector<ShopSerialInfo> shopItems)
                {
                    std::memset(this, 0, shopItems.size() * sizeof(ShopSerialInfo));
                    std::copy(shopItems.begin(), shopItems.end(), this->shop_items);
                }

                MainShopBuyItemSerialInfoResultAck(std::uint8_t* data, std::size_t size)
                {
                    std::memset(this, 0, sizeof(MainShopBuyItemSerialInfoResultAck));
                    std::size_t structSize = sizeof(ShopSerialInfo);
                    std::size_t elementCount = size / sizeof(ShopSerialInfo);
                    for (std::size_t i = 0; i < elementCount; i++) this->shop_items[i] = *(ShopSerialInfo*)(data + i * structSize);
                }
            };

            struct MainShopBuyItemIdResultAck
            {
                ShopItem shop_items[];

                MainShopBuyItemIdResultAck(std::vector<ShopItem> shopItems)
                {
                    std::memset(this, 0, shopItems.size() * sizeof(ShopItem));
                    std::copy(shopItems.begin(), shopItems.end(), this->shop_items);
                }

                MainShopBuyItemIdResultAck(std::uint8_t* data, std::size_t size)
                {
                    std::memset(this, 0, sizeof(MainShopBuyItemIdResultAck));
                    std::size_t structSize = sizeof(ShopItem);
                    std::size_t elementCount = size / sizeof(ShopItem);
                    for (std::size_t i = 0; i < elementCount; i++) this->shop_items[i] = *(ShopItem*)(data + i * structSize);
                }
            };


            class MainGachaponSalesInfoAck
            {
            public:
                std::vector<MainGachaponSaleInfo> gachapon_sales_info;

                MainGachaponSalesInfoAck(const std::vector<MainGachaponSaleInfo>& sales) : gachapon_sales_info(sales) {}

                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;
                    for (const auto& sale_info : gachapon_sales_info)
                    {
                        const auto* sales_bytes = reinterpret_cast<const uint8_t*>(&sale_info);
                        data.insert(data.end(), sales_bytes, sales_bytes + sizeof(MainGachaponSaleInfo));
                    }

                    return data;
                }
            };

            class MainDeleteItemAck
            {
            public:
                std::vector<ItemSerialInfo> deleted_items;

                MainDeleteItemAck(const std::vector<ItemSerialInfo>& deletedItems) : deleted_items(deletedItems) {}
                    
                std::vector<std::uint8_t> Serialize() const
                {
                    std::uint32_t items_count = static_cast<std::uint32_t>(deleted_items.size());
                    std::vector<std::uint8_t> data;
                    auto count_bytes = reinterpret_cast<const uint8_t*>(&items_count);
                    data.insert(data.end(), count_bytes, count_bytes + sizeof(items_count));

                    for (const auto& item : deleted_items)
                    {
                        const auto* item_bytes = reinterpret_cast<const uint8_t*>(&item);
                        data.insert(data.end(), item_bytes, item_bytes + sizeof(ItemSerialInfo));
                    }

                    return data;
                }
            };
            class MainRepairItemAck
            {
            public:
                std::uint32_t rt;
                std::uint32_t mp;
               
                std::vector<ItemSerialInfo> repair_items;

                MainRepairItemAck(const std::uint32_t& micropoints, const std::uint32_t& rocktokens, const std::vector<ItemSerialInfo>& repairItems)
                    : mp(micropoints), rt(rocktokens), repair_items(repairItems) {}

                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;
                    auto mp_bytes = reinterpret_cast<const uint8_t*>(&mp);
                    auto rt_bytes = reinterpret_cast<const uint8_t*>(&rt);
                    data.insert(data.end(), rt_bytes, rt_bytes + sizeof(rt));
                    data.insert(data.end(), mp_bytes, mp_bytes + sizeof(mp));
                    

                    for (const auto& item : repair_items)
                    {
                        const auto* item_bytes = reinterpret_cast<const uint8_t*>(&item);
                        data.insert(data.end(), item_bytes, item_bytes + sizeof(ItemSerialInfo));
                    }

                    return data;
                }
            };
            struct MainInjectEnergyAck
            {
                ItemSerialInfo item;
                std::uint32_t energy;
                MainInjectEnergyAck(const ItemSerialInfo& desireditem, const std::uint32_t& desiredenergy)
                {
                    item.data = desireditem.data;
                    energy = desiredenergy;
                }
            };
            struct MainResetUpgradeItemAck
            {
                ShopItem item_shop_info;
                ItemSerialInfo item_serial_info;
                ItemSerialInfo upgrade_reset_item;
                MainResetUpgradeItemAck(const ShopItem& shopInfo, const ItemSerialInfo& serialInfo, const ItemSerialInfo& upgradeResetItem)
                {
                    std::memset(this, 0, sizeof(MainResetUpgradeItemAck));
                    item_shop_info = shopInfo;
                    item_serial_info.data = serialInfo.data;
                    upgrade_reset_item.data = upgradeResetItem.data;
                }
            };
            class MainUpgradeItemAck
            {
            public:
                ShopItem shop_item_info;
                ItemSerialInfo new_serial_info;
                ItemSerialInfo booster_item;
                ItemSerialInfo energy_refund_item;
                ItemSerialInfo protection_item;
                ItemSerialInfo zero;

                MainUpgradeItemAck(const ShopItem& shopItemInfo = ShopItem(), const ItemSerialInfo& newSerialInfo = ItemSerialInfo(), const ItemSerialInfo& boosterItem = ItemSerialInfo(), const ItemSerialInfo& energyRefundItem = ItemSerialInfo(), const ItemSerialInfo& protectionItem = ItemSerialInfo())
                {
                    shop_item_info = shopItemInfo;
                    new_serial_info.data = newSerialInfo.data;
                    booster_item.data = boosterItem.data;
                    energy_refund_item.data = energyRefundItem.data;
                    protection_item.data = protectionItem.data;
                    zero.data = ItemSerialInfo().data;
                }

                std::vector<std::uint8_t> Serialize(const std::uint8_t& option, const std::uint8_t& extra) const
                {
                    std::vector<std::uint8_t> data;
                    
                    if (extra == 1 || extra == 2)
                    {
                        auto shop_info_bytes = reinterpret_cast<const uint8_t*>(&shop_item_info);
                        data.insert(data.end(), shop_info_bytes, shop_info_bytes + sizeof(shop_item_info));
                    }
                        
                    auto serial_info_bytes = reinterpret_cast<const uint8_t*>(&new_serial_info.data);
                    data.insert(data.end(), serial_info_bytes, serial_info_bytes + sizeof(new_serial_info.data));
                    if (option == 1)
                    {
                        auto booster_bytes = reinterpret_cast<const uint8_t*>(&booster_item.data);
                        data.insert(data.end(), booster_bytes, booster_bytes + sizeof(booster_item.data));
                    }
                    else if (option == 2)
                    {
                        auto energy_bytes = reinterpret_cast<const uint8_t*>(&energy_refund_item.data);
                        data.insert(data.end(), energy_bytes, energy_bytes + sizeof(energy_refund_item.data));
                    }
                    else if (option == 3)
                    {
                        auto protection_bytes = reinterpret_cast<const uint8_t*>(&protection_item.data);
                        data.insert(data.end(), protection_bytes, protection_bytes + sizeof(protection_item.data));
                    }
                    else if (option == 4)
                    {
                        auto booster_bytes = reinterpret_cast<const uint8_t*>(&booster_item.data);
                        data.insert(data.end(), booster_bytes, booster_bytes + sizeof(booster_item.data));

                        auto energy_bytes = reinterpret_cast<const uint8_t*>(&energy_refund_item.data);
                        data.insert(data.end(), energy_bytes, energy_bytes + sizeof(energy_refund_item.data));
                    }
                    else if (option == 5)
                    {
                        auto booster_bytes = reinterpret_cast<const uint8_t*>(&booster_item.data);
                        data.insert(data.end(), booster_bytes, booster_bytes + sizeof(booster_item.data));

                        auto protection_bytes = reinterpret_cast<const uint8_t*>(&protection_item.data);
                        data.insert(data.end(), protection_bytes, protection_bytes + sizeof(protection_item.data));
                    }
                    else if (option == 6)
                    {
                        auto energy_bytes = reinterpret_cast<const uint8_t*>(&energy_refund_item.data);
                        data.insert(data.end(), energy_bytes, energy_bytes + sizeof(energy_refund_item.data));

                        auto protection_bytes = reinterpret_cast<const uint8_t*>(&protection_item.data);
                        data.insert(data.end(), protection_bytes, protection_bytes + sizeof(protection_item.data));
                    }
                    else if (option == 7)
                    {
                        auto booster_bytes = reinterpret_cast<const uint8_t*>(&booster_item.data);
                        data.insert(data.end(), booster_bytes, booster_bytes + sizeof(booster_item.data));

                        auto energy_bytes = reinterpret_cast<const uint8_t*>(&energy_refund_item.data);
                        data.insert(data.end(), energy_bytes, energy_bytes + sizeof(energy_refund_item.data));
                    }
                    else
                    {
                        auto zero_bytes = reinterpret_cast<const uint8_t*>(&zero.data);
                        data.insert(data.end(), zero_bytes, zero_bytes + sizeof(zero.data));
                    }

                    return data;
                }
            };
            class MainUsePackageItemAck
            {
            public:
                ItemSerialInfo item;
                std::vector<ShopItem> package_items;
                char nickname[16];
                MainUsePackageItemAck(const ItemSerialInfo& new_item = ItemSerialInfo(), const std::string& new_nickname = "", const std::vector<ShopItem>& new_items = std::vector<ShopItem>())
                {
                    std::memset(this, 0, sizeof(MainUsePackageItemAck));
                    std::memset(nickname, 0, sizeof(nickname));
                    item.data = new_item.data;
                    std::strcpy(nickname, new_nickname.c_str());
                    package_items = new_items;
                }

                std::vector<std::uint8_t> Serialize(std::uint8_t extra) const
                {
                    std::vector<std::uint8_t> data;
                    
                    if (extra == 0 || extra == 0x1A)
                    {
                        for (const auto& package_item : package_items)
                        {
                            const auto* package_item_bytes = reinterpret_cast<const uint8_t*>(&package_item);
                            data.insert(data.end(), package_item_bytes, package_item_bytes + sizeof(ShopItem));
                        }
                    }
                    else if (extra == 0x35)
                    {
                        auto item_bytes = reinterpret_cast<const uint8_t*>(&item.data);
                        auto nickname_bytes = reinterpret_cast<const uint8_t*>(&nickname);
                        data.insert(data.end(), item_bytes, item_bytes + sizeof(item.data));
                        data.insert(data.end(), nickname_bytes, nickname_bytes + sizeof(nickname));
                    }
                    else if (extra == 0xFF)
                    {
                        auto item_bytes = reinterpret_cast<const uint8_t*>(&item.data);
                        data.insert(data.end(), item_bytes, item_bytes + sizeof(item.data));
                    }
                    

                    return data;
                }
            };

            struct MainPlayerDetailsInfoUpdateAck
            {
                std::uint32_t account_id;
                MainPlayerDetailsInfoUpdateAck()
                {
                    std::memset(this, 0, sizeof(MainPlayerDetailsInfoUpdateAck));
                }
            };
            struct MainPlayerDetailsInfoAck
            {
                union {
                    struct {
                        std::uint64_t diorama1 : 23;
                        std::uint64_t diorama2 : 41;
                    };
                    std::uint64_t Diorama;//0
                };
                std::uint32_t Kills;//8
                std::uint32_t Deaths;//c
                std::uint32_t Assists;//10
                std::uint32_t Wins;//14
                std::uint32_t Loses;//18
                std::uint32_t Draws;//1c
                std::uint32_t Melee;//20
                std::uint32_t Rifle;//24
                std::uint32_t Shotgun;//28
                std::uint32_t Sniper;//2c
                std::uint32_t Gatling;//30
                std::uint32_t Bazooka;//34
                std::uint32_t Grenade;//38
                union {
                    struct {
                        std::uint64_t unused : 27;
                        std::uint64_t Headshots : 29;
                        std::uint64_t HighestKillStreak : 8;
                    };
                    std::uint64_t HeadshotsAndHighestKillStreak;//3c
                };
                std::uint32_t Unknown1;//44
                std::uint32_t PlayTime;//48
                std::uint32_t ClanId;//4c
            #if defined(RELEASE_1_1_1)
                std::array<std::uint64_t, 4> Achievements;//50
            #else
                std::array<std::uint64_t, 4> Achievements;//50
                //char achivements[28]{};
            #endif
               
                std::uint32_t ZombieKillPoints;//70
                std::uint32_t Infections;//74
                std::uint32_t Unknown2;//78

                std::uint32_t EquippedHairItemId;//7c
                std::uint32_t EquippedFaceItemId;//80
                std::uint32_t EquippedUpperItemId;//84
                std::uint32_t EquippedUnderItemId;//88
                std::uint32_t EquippedPantsItemId;//8c
                std::uint32_t EquippedShirtItemId;//90
                std::uint32_t EquippedBootsItemId;//94
                std::uint32_t EquippedGlassItemId;//98
                std::uint32_t EquippedAccessoryWaistItemId;//9c
                std::uint32_t EquippedAccessoryBackItemId;//A0
                std::uint32_t EquippedMeleeItemId;//a4
                std::uint32_t EquippedRifleItemId;//a8
                std::uint32_t EquippedShotgunItemId;//ac
                std::uint32_t EquippedSniperItemId;//b0
                std::uint32_t EquippedGatlingItemId;//b4
                std::uint32_t EquippedGrenadeItemId;//b8
                std::uint32_t EquippedBazookaItemId;//bc

                union {
                    struct {
                        std::uint32_t SelectedCharacter : 4; // >> 0
                        std::uint32_t Channel : 4; // >> 4
                        std::uint32_t Grade : 5; // >> 8
                        std::uint32_t Level : 7; // >> 13
                    };
                    std::uint32_t SelectedCharacterChannelLevel;//c0
                };
            #if defined(RELEASE_1_1_1)
                std::uint32_t VIPLevel;//c4
                std::uint32_t Unknown3;
                std::array<std::uint64_t, 8> Unknown4;
            #endif
                char ClanName[16];//196

                union {
                    struct {
                        std::uint64_t ClanLogoFront : 16;
                        std::uint64_t ClanLogoBack : 14;
                        std::uint64_t ClanContribution : 34;
                    };
                    std::uint64_t ClanLogoAndContribution;//212
                };

                union {
                    struct {
                        std::uint64_t ClanWins : 23;
                        std::uint64_t ClanLoses : 23;
                        std::uint64_t ClanDraws : 18;
                    };
                    std::uint64_t ClanMatchResults;
                };

                std::uint32_t ClanKills;
                std::uint32_t ClanDeaths;
                std::uint32_t ClanAssists;
                MainPlayerDetailsInfoAck()
                {
                    std::memset(this, 0, sizeof(MainPlayerDetailsInfoAck));
                    std::memset(ClanName, 0, sizeof(ClanName));
                #if defined(RELEASE_1_1_1)
                    for (auto& achievement : Achievements) achievement = 0;
                    for (auto& unknown : Unknown4) unknown = 0;
                #endif
                }
            };
            class MainPlayerBlockedAddAck
            {
            public:
                std::uint32_t account_id;
                char nickname[16];
                MainPlayerBlockedAddAck(const std::uint32_t& account_id, const std::string& new_nickname = "")
                {
                    std::memset(this, 0, sizeof(MainPlayerBlockedAddAck));
                    std::memset(this->nickname, 0, sizeof(this->nickname));
                    this->account_id = account_id;
                    std::strcpy(this->nickname, new_nickname.c_str());
                }
            };

            class MainChatAck
            {
            public:
                char nickname[16];
                char msg[256];
                MainChatAck(const std::string& new_nickname, const char* new_msg, const std::uint32_t& new_msg_size)
                {
                    std::memset(this, 0, sizeof(MainChatAck));
                    std::memset(nickname, 0, sizeof(nickname));
                    std::memset(msg, 0, sizeof(msg));
                    std::strncpy(nickname, new_nickname.c_str(), sizeof(nickname));
                    std::uint32_t size_to_copy = std::min(new_msg_size, static_cast<std::uint32_t>(sizeof(msg)));
                    std::strncpy(msg, new_msg, size_to_copy);
                    msg[size_to_copy] = '\0';
                }


                std::vector<std::uint8_t> Serialize(const std::uint8_t& extra, const std::uint32_t& msg_size, const std::uint32_t& unknown = 0) const
                {
                    std::vector<std::uint8_t> data;

                    if (extra == 2) // whisper
                    {
                        const auto* unknown_bytes = reinterpret_cast<const uint8_t*>(&unknown);
                        data.insert(data.end(), unknown_bytes, unknown_bytes + sizeof(unknown));
                    }
                    auto nickname_bytes = reinterpret_cast<const uint8_t*>(&nickname);
                    data.insert(data.end(), nickname_bytes, nickname_bytes + sizeof(nickname));

                    auto msg_bytes = reinterpret_cast<const uint8_t*>(msg);
                    data.insert(data.end(), msg_bytes, msg_bytes + msg_size);
                   

                    return data;
                }
            };
            struct MainRoomCreateAck
            {
                std::uint16_t room_id;
                std::uint16_t channel_id;
                MainRoomCreateAck(std::uint16_t new_room_id = 0, std::uint16_t new_channel_id = 0)
                {
                    std::memset(this, 0, sizeof(MainRoomCreateAck));
                    this->room_id = new_room_id;
                    this->channel_id = new_channel_id;
                }
            };
            class MainRoomListInfoAck
            {
            public:
                std::uint16_t room_count;
                std::uint16_t max_room_count;
                std::vector<RoomListInfo> rooms;
                MainRoomListInfoAck(const std::uint16_t& room_count, const std::uint16_t& max_room_count, const std::vector<RoomListInfo>& new_rooms)
                {
                    std::memset(this, 0, sizeof(MainRoomListInfoAck));
                    this->room_count = room_count;
                    this->max_room_count = max_room_count;
                    this->rooms = new_rooms;
                }

                std::vector<std::uint8_t> Serialize(const std::uint8_t& extra) const
                {
                    std::vector<std::uint8_t> data;

                    if (extra == 0 || extra == 0x25)
                    {
                        auto room_count_bytes = reinterpret_cast<const uint8_t*>(&room_count);
                        auto max_room_count_bytes = reinterpret_cast<const uint8_t*>(&max_room_count);
                        data.insert(data.end(), room_count_bytes, room_count_bytes + sizeof(room_count));
                        data.insert(data.end(), max_room_count_bytes, max_room_count_bytes + sizeof(max_room_count));

                        for (const auto& room_info : rooms)
                        {
                            const auto* room_data_bytes = reinterpret_cast<const uint8_t*>(&room_info);
                            data.insert(data.end(), room_data_bytes, room_data_bytes + sizeof(room_info));
                        }
                    }

                    return data;
                }
            };

            class MainRoomSettingsInfoAck
            {
            public:
                RoomSettingsInfo2 info;
                char password[14]{};
                std::uint8_t unknow4{};
                MainRoomSettingsInfoAck(const std::string& password, const RoomSettingsInfo2& settings_info)
                {
                    std::memset(this, 0, sizeof(MainRoomSettingsInfoAck));
                    std::memset(this->password, 0, sizeof(this->password));
                   
                    if (settings_info.has_password)
                        std::strcpy(this->password, password.c_str());

                    this->info.data = settings_info.data;

                }
                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;

                    auto info_bytes = reinterpret_cast<const uint8_t*>(&info.data);
                    data.insert(data.end(), info_bytes, info_bytes + sizeof(info.data));

                    auto password_bytes = reinterpret_cast<const uint8_t*>(password);
                    data.insert(data.end(), password_bytes, password_bytes + sizeof(password));

                    data.insert(data.end(), unknow4);

                    return data;
                }
            };
            class MainRoomPlayersEquipInfoUpdateRoomAck
            {
            public:
                Core::UniqueId unique_id;
                std::uint32_t EquippedHairItemId;
                std::uint32_t EquippedFaceItemId;
                std::uint32_t EquippedUpperItemId;
                std::uint32_t EquippedUnderItemId;
                std::uint32_t EquippedPantsItemId;
                std::uint32_t EquippedShirtItemId;
                std::uint32_t EquippedBootsItemId;
                std::uint32_t EquippedGlassItemId;
                std::uint32_t EquippedAccessoryWaistItemId;
                std::uint32_t EquippedAccessoryBackItemId;
                std::uint32_t EquippedMeleeItemId;
                std::uint32_t EquippedRifleItemId;
                std::uint32_t EquippedShotgunItemId;
                std::uint32_t EquippedSniperItemId;
                std::uint32_t EquippedGatlingItemId;
                std::uint32_t EquippedGrenadeItemId;
                std::uint32_t EquippedBazookaItemId;
                MainRoomPlayersEquipInfoUpdateRoomAck(const Core::UniqueId& uniqueId,
                    const std::uint32_t& hair, const std::uint32_t& face, const std::uint32_t& upper,
                    const std::uint32_t& under, const std::uint32_t& pants, const std::uint32_t& shirt,
                    const std::uint32_t& boots, const std::uint32_t& glass, const std::uint32_t& acc_waist,
                    const std::uint32_t& acc_back, const std::uint32_t& melee, const std::uint32_t& rifle,
                    const std::uint32_t& shotgun, const std::uint32_t& sniper, const std::uint32_t& gatling,
                    const std::uint32_t& grenade, const std::uint32_t& bazooka)
                {
                    std::memset(this, 0, sizeof(MainRoomPlayersEquipInfoUpdateRoomAck));
                    this->unique_id.data = uniqueId.data;
                    this->EquippedHairItemId = hair;
                    this->EquippedFaceItemId = face;
                    this->EquippedUpperItemId = upper;
                    this->EquippedUnderItemId = under;
                    this->EquippedPantsItemId = pants;
                    this->EquippedShirtItemId = shirt;
                    this->EquippedBootsItemId = boots;
                    this->EquippedGlassItemId = glass;
                    this->EquippedAccessoryWaistItemId = acc_waist;
                    this->EquippedAccessoryBackItemId = acc_back;
                    this->EquippedMeleeItemId = melee;
                    this->EquippedRifleItemId = rifle;
                    this->EquippedShotgunItemId = shotgun;
                    this->EquippedSniperItemId = sniper;
                    this->EquippedGatlingItemId = gatling;
                    this->EquippedGrenadeItemId = grenade;
                    this->EquippedBazookaItemId = bazooka;

                }
            };
            class MainRoomPlayersEquipInfoAck
            {
            public:
                
                std::uint32_t EquippedHairItemId;
                std::uint32_t EquippedFaceItemId;
                std::uint32_t EquippedUpperItemId;
                std::uint32_t EquippedUnderItemId;
                std::uint32_t EquippedPantsItemId;
                std::uint32_t EquippedShirtItemId;
                std::uint32_t EquippedBootsItemId;
                std::uint32_t EquippedGlassItemId;
                std::uint32_t EquippedAccessoryWaistItemId;
                std::uint32_t EquippedAccessoryBackItemId;
                std::uint32_t EquippedMeleeItemId;
                std::uint32_t EquippedRifleItemId;
                std::uint32_t EquippedShotgunItemId;
                std::uint32_t EquippedSniperItemId;
                std::uint32_t EquippedGatlingItemId;
                std::uint32_t EquippedGrenadeItemId;
                std::uint32_t EquippedBazookaItemId;
            #if defined(RELEASE_1_1_1)
                std::uint32_t EquippedHairItemId2 = 0;
                std::uint32_t EquippedFaceItemId2 = 0;
                std::uint32_t EquippedUpperItemId2 = 0;
                std::uint32_t EquippedUnderItemId2 = 0;
                std::uint32_t EquippedPantsItemId2 = 0;
                std::uint32_t EquippedShirtItemId2 = 0;
                std::uint32_t EquippedBootsItemId2 = 0;
                std::uint32_t EquippedGlassItemId2 = 0;
                std::uint32_t EquippedAccessoryWaistItemId2 = 0;
                std::uint32_t EquippedAccessoryBackItemId2 = 0;
                std::uint32_t EquippedMeleeItemId2 = 0;
                std::uint32_t EquippedRifleItemId2 = 0;
                std::uint32_t EquippedShotgunItemId2 = 0;
                std::uint32_t EquippedSniperItemId2 = 0;
                std::uint32_t EquippedGatlingItemId2 = 0;
                std::uint32_t EquippedGrenadeItemId2 = 0;
                std::uint32_t EquippedBazookaItemId2 = 0;
            #endif

                Core::UniqueId unique_id;

                MainRoomPlayersEquipInfoAck(const Core::UniqueId& uniqueId, 
                    const std::uint32_t& hair, const std::uint32_t& face, const std::uint32_t& upper, 
                    const std::uint32_t& under, const std::uint32_t& pants, const std::uint32_t& shirt,
                    const std::uint32_t& boots, const std::uint32_t& glass, const std::uint32_t& acc_waist,
                    const std::uint32_t& acc_back, const std::uint32_t& melee, const std::uint32_t& rifle, 
                    const std::uint32_t& shotgun, const std::uint32_t& sniper, const std::uint32_t& gatling,
                    const std::uint32_t& grenade, const std::uint32_t& bazooka)
                {
                    std::memset(this, 0, sizeof(MainRoomPlayersEquipInfoAck));
                    this->unique_id.data = uniqueId.data;
                    this->EquippedHairItemId = hair;
                    this->EquippedFaceItemId = face;
                    this->EquippedUpperItemId = upper;
                    this->EquippedUnderItemId = under;
                    this->EquippedPantsItemId = pants;
                    this->EquippedShirtItemId = shirt;
                    this->EquippedBootsItemId = boots;
                    this->EquippedGlassItemId = glass;
                    this->EquippedAccessoryWaistItemId = acc_waist;
                    this->EquippedAccessoryBackItemId = acc_back;
                    this->EquippedMeleeItemId = melee;
                    this->EquippedRifleItemId = rifle;
                    this->EquippedShotgunItemId = shotgun;
                    this->EquippedSniperItemId = sniper;
                    this->EquippedGatlingItemId = gatling;
                    this->EquippedGrenadeItemId = grenade;
                    this->EquippedBazookaItemId = bazooka;
                    
                }
            };
            class MainRoomPlayersUpdatePingInfoAck
            {
            public:
                PlayerPingUpdateInfo ping_info;
                Core::UniqueId unique_id;
                MainRoomPlayersUpdatePingInfoAck(const PlayerPingUpdateInfo& ping, const Core::UniqueId& uniqueId)
                {
                    std::memset(this, 0, sizeof(MainRoomPlayersUpdatePingInfoAck));
                    this->unique_id.data = uniqueId.data;
                    this->ping_info.data = ping.data;
  

                }
                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;

                    auto info1_bytes = reinterpret_cast<const uint8_t*>(&ping_info.data);
                    data.insert(data.end(), info1_bytes, info1_bytes + sizeof(ping_info.data));

                    auto uniqueid_bytes = reinterpret_cast<const uint8_t*>(&unique_id.data);
                    data.insert(data.end(), uniqueid_bytes, uniqueid_bytes + sizeof(unique_id.data));

                    
                    return data;
                }
            };
            class MainRoomPlayersInfoAck
            {
            public:
                Core::UniqueId unique_id;
                RoomUserPlayerInfo1 info;
                char nickname[16];
                RoomUserPlayerInfo2 info2;
            #if defined(RELEASE_1_1_1)
                std::uint32_t unknown2;
            #endif
                MainRoomPlayersInfoAck(const std::string& nickname, const Core::UniqueId& uniqueId, const RoomUserPlayerInfo1& info1, const RoomUserPlayerInfo2& info2)
                {
                    std::memset(this, 0, sizeof(MainRoomPlayersInfoAck));
                    std::memset(this->nickname, 0, sizeof(this->nickname));
                    std::strcpy(this->nickname, nickname.c_str());
                    this->unique_id.data = uniqueId.data;
                    this->info.data = info1.data;
                    this->info2.data = info2.data;
                #if defined(RELEASE_1_1_1)
                    this->unknown2 = 0;
                #endif

                }
                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;

                    auto uniqueid_bytes = reinterpret_cast<const uint8_t*>(&unique_id.data);
                    data.insert(data.end(), uniqueid_bytes, uniqueid_bytes + sizeof(unique_id.data));

                    auto info1_bytes = reinterpret_cast<const uint8_t*>(&info.data);
                    data.insert(data.end(), info1_bytes, info1_bytes + sizeof(info.data));

                    auto nickname_bytes = reinterpret_cast<const uint8_t*>(nickname);
                    data.insert(data.end(), nickname_bytes, nickname_bytes + sizeof(nickname));

                    auto info2_bytes = reinterpret_cast<const uint8_t*>(&info2.data);
                    data.insert(data.end(), info2_bytes, info2_bytes + sizeof(info2.data));

                #if defined(RELEASE_1_1_1)
                    auto unknown_bytes = reinterpret_cast<const uint8_t*>(&unknown2);
                    data.insert(data.end(), unknown_bytes, unknown_bytes + sizeof(unknown2));
                #endif

                    return data;
                }
            };

            class MainRoomPlayerEnterInfoAck
            {
            public:
                Core::UniqueId unique_id;//0
                RoomUserPlayerInfo1 info;//4
                std::uint32_t EquippedHairItemId;//8
                std::uint32_t EquippedFaceItemId;//c
                std::uint32_t EquippedUpperItemId;//10
                std::uint32_t EquippedUnderItemId;//14
                std::uint32_t EquippedPantsItemId;//18
                std::uint32_t EquippedShirtItemId;//1c
                std::uint32_t EquippedBootsItemId;//20
                std::uint32_t EquippedGlassItemId;//24
                std::uint32_t EquippedAccessoryWaistItemId;//28
                std::uint32_t EquippedAccessoryBackItemId;//2c
                std::uint32_t EquippedMeleeItemId;//30
                std::uint32_t EquippedRifleItemId;//34
                std::uint32_t EquippedShotgunItemId;//38
                std::uint32_t EquippedSniperItemId;//3c
                std::uint32_t EquippedGatlingItemId;//40
                std::uint32_t EquippedGrenadeItemId;//44
                std::uint32_t EquippedBazookaItemId;//48
            #if defined(RELEASE_1_1_1)
                std::uint32_t EquippedHairItemId2 = 0;
                std::uint32_t EquippedFaceItemId2 = 0;
                std::uint32_t EquippedUpperItemId2 = 0;
                std::uint32_t EquippedUnderItemId2 = 0;
                std::uint32_t EquippedPantsItemId2 = 0;
                std::uint32_t EquippedShirtItemId2 = 0;
                std::uint32_t EquippedBootsItemId2 = 0;
                std::uint32_t EquippedGlassItemId2 = 0;
                std::uint32_t EquippedAccessoryWaistItemId2 = 0;
                std::uint32_t EquippedAccessoryBackItemId2 = 0;
                std::uint32_t EquippedMeleeItemId2 = 0;
                std::uint32_t EquippedRifleItemId2 = 0;
                std::uint32_t EquippedShotgunItemId2 = 0;
                std::uint32_t EquippedSniperItemId2 = 0;
                std::uint32_t EquippedGatlingItemId2 = 0;
                std::uint32_t EquippedGrenadeItemId2 = 0;
                std::uint32_t EquippedBazookaItemId2 = 0;
            #endif
                char nickname[16];//4c
                RoomUserPlayerInfo2 info2;//5c
            #if defined(RELEASE_1_1_1)
                std::uint32_t unknown2;//60
            #endif
                MainRoomPlayerEnterInfoAck(const std::string& nickname, const Core::UniqueId& uniqueId, const RoomUserPlayerInfo1& info1, const RoomUserPlayerInfo2& info2,
                    const std::uint32_t& hair, const std::uint32_t& face, const std::uint32_t& upper,
                    const std::uint32_t& under, const std::uint32_t& pants, const std::uint32_t& shirt,
                    const std::uint32_t& boots, const std::uint32_t& glass, const std::uint32_t& acc_waist,
                    const std::uint32_t& acc_back, const std::uint32_t& melee, const std::uint32_t& rifle,
                    const std::uint32_t& shotgun, const std::uint32_t& sniper, const std::uint32_t& gatling,
                    const std::uint32_t& grenade, const std::uint32_t& bazooka)
                {
                    std::memset(this, 0, sizeof(MainRoomPlayerEnterInfoAck));
                    std::memset(this->nickname, 0, sizeof(this->nickname));
                    std::strcpy(this->nickname, nickname.c_str());
                    this->unique_id.data = uniqueId.data;
                    this->info.data = info1.data;
                    this->info2.data = info2.data;
                #if defined(RELEASE_1_1_1)
                    this->unknown2 = 0;
                #endif
                    this->EquippedHairItemId = hair;
                    this->EquippedFaceItemId = face;
                    this->EquippedUpperItemId = upper;
                    this->EquippedUnderItemId = under;
                    this->EquippedPantsItemId = pants;
                    this->EquippedShirtItemId = shirt;
                    this->EquippedBootsItemId = boots;
                    this->EquippedGlassItemId = glass;
                    this->EquippedAccessoryWaistItemId = acc_waist;
                    this->EquippedAccessoryBackItemId = acc_back;
                    this->EquippedMeleeItemId = melee;
                    this->EquippedRifleItemId = rifle;
                    this->EquippedShotgunItemId = shotgun;
                    this->EquippedSniperItemId = sniper;
                    this->EquippedGatlingItemId = gatling;
                    this->EquippedGrenadeItemId = grenade;
                    this->EquippedBazookaItemId = bazooka;
                }
                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;

                    auto uniqueid_bytes = reinterpret_cast<const uint8_t*>(&unique_id.data);
                    data.insert(data.end(), uniqueid_bytes, uniqueid_bytes + sizeof(unique_id.data));

                    auto info1_bytes = reinterpret_cast<const uint8_t*>(&info.data);
                    data.insert(data.end(), info1_bytes, info1_bytes + sizeof(info.data));

                    auto itemHair_bytes = reinterpret_cast<const uint8_t*>(&EquippedHairItemId);
                    data.insert(data.end(), itemHair_bytes, itemHair_bytes + sizeof(EquippedHairItemId));

                    auto itemFace_bytes = reinterpret_cast<const uint8_t*>(&EquippedFaceItemId);
                    data.insert(data.end(), itemFace_bytes, itemFace_bytes + sizeof(EquippedFaceItemId));

                    auto itemUpper_bytes = reinterpret_cast<const uint8_t*>(&EquippedUpperItemId);
                    data.insert(data.end(), itemUpper_bytes, itemUpper_bytes + sizeof(EquippedUpperItemId));

                    auto itemUnder_bytes = reinterpret_cast<const uint8_t*>(&EquippedUnderItemId);
                    data.insert(data.end(), itemUnder_bytes, itemUnder_bytes + sizeof(EquippedUnderItemId));

                    auto itemPants_bytes = reinterpret_cast<const uint8_t*>(&EquippedPantsItemId);
                    data.insert(data.end(), itemPants_bytes, itemPants_bytes + sizeof(EquippedPantsItemId));

                    auto itemShirt_bytes = reinterpret_cast<const uint8_t*>(&EquippedShirtItemId);
                    data.insert(data.end(), itemShirt_bytes, itemShirt_bytes + sizeof(EquippedShirtItemId));

                    auto itemBoots_bytes = reinterpret_cast<const uint8_t*>(&EquippedBootsItemId);
                    data.insert(data.end(), itemBoots_bytes, itemBoots_bytes + sizeof(EquippedBootsItemId));

                    auto itemGlass_bytes = reinterpret_cast<const uint8_t*>(&EquippedGlassItemId);
                    data.insert(data.end(), itemGlass_bytes, itemGlass_bytes + sizeof(EquippedGlassItemId));

                    auto itemAccessoryWaist_bytes = reinterpret_cast<const uint8_t*>(&EquippedAccessoryWaistItemId);
                    data.insert(data.end(), itemAccessoryWaist_bytes, itemAccessoryWaist_bytes + sizeof(EquippedAccessoryWaistItemId));

                    auto itemAccessoryBack_bytes = reinterpret_cast<const uint8_t*>(&EquippedAccessoryBackItemId);
                    data.insert(data.end(), itemAccessoryBack_bytes, itemAccessoryBack_bytes + sizeof(EquippedAccessoryBackItemId));

                    auto itemMelee_bytes = reinterpret_cast<const uint8_t*>(&EquippedMeleeItemId);
                    data.insert(data.end(), itemMelee_bytes, itemMelee_bytes + sizeof(EquippedMeleeItemId));

                    auto itemRifle_bytes = reinterpret_cast<const uint8_t*>(&EquippedRifleItemId);
                    data.insert(data.end(), itemRifle_bytes, itemRifle_bytes + sizeof(EquippedRifleItemId));

                    auto itemShotgun_bytes = reinterpret_cast<const uint8_t*>(&EquippedShotgunItemId);
                    data.insert(data.end(), itemShotgun_bytes, itemShotgun_bytes + sizeof(EquippedShotgunItemId));

                    auto itemSniper_bytes = reinterpret_cast<const uint8_t*>(&EquippedSniperItemId);
                    data.insert(data.end(), itemSniper_bytes, itemSniper_bytes + sizeof(EquippedSniperItemId));

                    auto itemGatling_bytes = reinterpret_cast<const uint8_t*>(&EquippedGatlingItemId);
                    data.insert(data.end(), itemGatling_bytes, itemGatling_bytes + sizeof(EquippedGatlingItemId));

                    auto itemGrenade_bytes = reinterpret_cast<const uint8_t*>(&EquippedGrenadeItemId);
                    data.insert(data.end(), itemGrenade_bytes, itemGrenade_bytes + sizeof(EquippedGrenadeItemId));

                    auto itemBazooka_bytes = reinterpret_cast<const uint8_t*>(&EquippedBazookaItemId);
                    data.insert(data.end(), itemBazooka_bytes, itemBazooka_bytes + sizeof(EquippedBazookaItemId));

                #if defined(RELEASE_1_1_1)
                    auto itemHair2_bytes = reinterpret_cast<const uint8_t*>(&EquippedHairItemId2);
                    data.insert(data.end(), itemHair2_bytes, itemHair2_bytes + sizeof(EquippedHairItemId2));

                    auto itemFace2_bytes = reinterpret_cast<const uint8_t*>(&EquippedFaceItemId2);
                    data.insert(data.end(), itemFace2_bytes, itemFace2_bytes + sizeof(EquippedFaceItemId2));

                    auto itemUpper2_bytes = reinterpret_cast<const uint8_t*>(&EquippedUpperItemId2);
                    data.insert(data.end(), itemUpper2_bytes, itemUpper2_bytes + sizeof(EquippedUpperItemId2));

                    auto itemUnder2_bytes = reinterpret_cast<const uint8_t*>(&EquippedUnderItemId2);
                    data.insert(data.end(), itemUnder2_bytes, itemUnder2_bytes + sizeof(EquippedUnderItemId2));

                    auto itemPants2_bytes = reinterpret_cast<const uint8_t*>(&EquippedPantsItemId2);
                    data.insert(data.end(), itemPants2_bytes, itemPants2_bytes + sizeof(EquippedPantsItemId2));

                    auto itemShirt2_bytes = reinterpret_cast<const uint8_t*>(&EquippedShirtItemId2);
                    data.insert(data.end(), itemShirt2_bytes, itemShirt2_bytes + sizeof(EquippedShirtItemId2));

                    auto itemBoots2_bytes = reinterpret_cast<const uint8_t*>(&EquippedBootsItemId2);
                    data.insert(data.end(), itemBoots2_bytes, itemBoots2_bytes + sizeof(EquippedBootsItemId2));

                    auto itemGlass2_bytes = reinterpret_cast<const uint8_t*>(&EquippedGlassItemId2);
                    data.insert(data.end(), itemGlass2_bytes, itemGlass2_bytes + sizeof(EquippedGlassItemId2));

                    auto itemAccessoryWaist2_bytes = reinterpret_cast<const uint8_t*>(&EquippedAccessoryWaistItemId2);
                    data.insert(data.end(), itemAccessoryWaist2_bytes, itemAccessoryWaist2_bytes + sizeof(EquippedAccessoryWaistItemId2));

                    auto itemAccessoryBack2_bytes = reinterpret_cast<const uint8_t*>(&EquippedAccessoryBackItemId2);
                    data.insert(data.end(), itemAccessoryBack2_bytes, itemAccessoryBack2_bytes + sizeof(EquippedAccessoryBackItemId2));

                    auto itemMelee2_bytes = reinterpret_cast<const uint8_t*>(&EquippedMeleeItemId2);
                    data.insert(data.end(), itemMelee2_bytes, itemMelee2_bytes + sizeof(EquippedMeleeItemId2));

                    auto itemRifle2_bytes = reinterpret_cast<const uint8_t*>(&EquippedRifleItemId2);
                    data.insert(data.end(), itemRifle2_bytes, itemRifle2_bytes + sizeof(EquippedRifleItemId2));

                    auto itemShotgun2_bytes = reinterpret_cast<const uint8_t*>(&EquippedShotgunItemId2);
                    data.insert(data.end(), itemShotgun2_bytes, itemShotgun2_bytes + sizeof(EquippedShotgunItemId2));

                    auto itemSniper2_bytes = reinterpret_cast<const uint8_t*>(&EquippedSniperItemId2);
                    data.insert(data.end(), itemSniper2_bytes, itemSniper2_bytes + sizeof(EquippedSniperItemId2));

                    auto itemGatling2_bytes = reinterpret_cast<const uint8_t*>(&EquippedGatlingItemId2);
                    data.insert(data.end(), itemGatling2_bytes, itemGatling2_bytes + sizeof(EquippedGatlingItemId2));

                    auto itemGrenade2_bytes = reinterpret_cast<const uint8_t*>(&EquippedGrenadeItemId2);
                    data.insert(data.end(), itemGrenade2_bytes, itemGrenade2_bytes + sizeof(EquippedGrenadeItemId2));

                    auto itemBazooka2_bytes = reinterpret_cast<const uint8_t*>(&EquippedBazookaItemId2);
                    data.insert(data.end(), itemBazooka2_bytes, itemBazooka2_bytes + sizeof(EquippedBazookaItemId2));
                #endif

                    auto nickname_bytes = reinterpret_cast<const uint8_t*>(nickname);
                    data.insert(data.end(), nickname_bytes, nickname_bytes + sizeof(nickname));

                    auto info2_bytes = reinterpret_cast<const uint8_t*>(&info2.data);
                    data.insert(data.end(), info2_bytes, info2_bytes + sizeof(info2.data));
                #if defined(RELEASE_1_1_1)
                    auto unknown_bytes = reinterpret_cast<const uint8_t*>(&unknown2);
                    data.insert(data.end(), unknown_bytes, unknown_bytes + sizeof(unknown2));
                #endif
                    return data;
                }
            };

            class MainBossBattleEndMatchResultAck
            {
            public:
                std::vector<BossItem> boss_items;

                MainBossBattleEndMatchResultAck(const std::vector<BossItem>& boss_items) : boss_items(boss_items) {}

                std::vector<std::uint8_t> Serialize() const
                {
                    std::uint32_t items_count = static_cast<std::uint32_t>(boss_items.size());
                    std::vector<std::uint8_t> data;
                    auto count_bytes = reinterpret_cast<const uint8_t*>(&items_count);
                    data.insert(data.end(), count_bytes, count_bytes + sizeof(items_count));

                    for (const auto& item : boss_items)
                    {
                        const auto* item_bytes = reinterpret_cast<const uint8_t*>(&item);
                        data.insert(data.end(), item_bytes, item_bytes + sizeof(BossItem));
                    }

                    return data;
                }
            };

            class MainMailboxAck
            {
            public:
                std::vector<MailboxMsgInfo> mails;
                MainMailboxAck(std::vector<MailboxMsgInfo>& mails)
                {
                    std::memset(this, 0, sizeof(MainMailboxAck));
                    this->mails = mails;
                  
                }

                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;

                    for (const auto& mail_info : mails)
                    {
                        const auto* mail_data_bytes = reinterpret_cast<const uint8_t*>(&mail_info);
                        data.insert(data.end(), mail_data_bytes, mail_data_bytes + sizeof(mail_info));
                    }

                    return data;
                }
            };

            class MainGiftboxAck
            {
            public:
                std::vector<GiftboxMsgInfo> mails;
                MainGiftboxAck(std::vector<GiftboxMsgInfo>& mails)
                {
                    std::memset(this, 0, sizeof(MainGiftboxAck));
                    this->mails = mails;

                }

                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;

                    for (const auto& mail_info : mails)
                    {
                        const auto* mail_data_bytes = reinterpret_cast<const uint8_t*>(&mail_info);
                        data.insert(data.end(), mail_data_bytes, mail_data_bytes + sizeof(mail_info));
                    }

                    return data;
                }
            };

            class MainMonthlyRewardAck
            {
            public:
                std::uint16_t month;
                std::uint16_t received;
                std::uint64_t unknown;
                std::array<std::uint32_t, 31> monthly_items;

                MainMonthlyRewardAck(const std::uint16_t& month, const std::uint16_t& received, const std::array<std::uint32_t, 31>& monthly_items) : month(month), received(received), unknown(0), monthly_items(monthly_items) {}

                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;
                    auto month_bytes = reinterpret_cast<const uint8_t*>(&month);
                    data.insert(data.end(), month_bytes, month_bytes + sizeof(month));

                    auto received_bytes = reinterpret_cast<const uint8_t*>(&received);
                    data.insert(data.end(), received_bytes, received_bytes + sizeof(received));

                    auto unknown_bytes = reinterpret_cast<const uint8_t*>(&unknown);
                    data.insert(data.end(), unknown_bytes, unknown_bytes + sizeof(unknown));

                    
                    for (const auto& item : monthly_items)
                    {
                        const auto* item_bytes = reinterpret_cast<const uint8_t*>(&item);
                        data.insert(data.end(), item_bytes, item_bytes + sizeof(std::uint32_t));
                    }

                    return data;
                }
            };

            class MainUserJoinConfirmAck
            {
            public:
                std::uint16_t server_id{};
                std::uint16_t room_id{};
                std::uint16_t channel_id{};
                MainUserJoinConfirmAck(std::uint16_t serverId, std::uint16_t roomId, std::uint16_t channelId)
                {
                    std::memset(this, 0, sizeof(MainUserJoinConfirmAck));
                    server_id = serverId;
                    room_id = roomId;
                    channel_id = channelId;

                }
                std::vector<std::uint8_t> Serialize() const
                {
                    std::vector<std::uint8_t> data;

                    auto server_id_bytes = reinterpret_cast<const uint8_t*>(&server_id);
                    data.insert(data.end(), server_id_bytes, server_id_bytes + sizeof(server_id));

                    auto room_id_bytes = reinterpret_cast<const uint8_t*>(&room_id);
                    data.insert(data.end(), room_id_bytes, room_id_bytes + sizeof(room_id));

                    auto channel_id_bytes = reinterpret_cast<const uint8_t*>(&channel_id);
                    data.insert(data.end(), room_id_bytes, channel_id_bytes + sizeof(channel_id));

                    return data;
                }
            };

            class MainUserInviteAck
            {
            public:
                std::uint32_t server_id{};
                char nickname[16]{};
                std::uint16_t room_id{};
                std::uint16_t channel_id{};
                char title[32];
                char password[14];
                MainUserInviteAck(std::uint32_t serverId, std::uint16_t roomId, std::uint16_t channelId, const std::string& nickname, const std::string& title = "", const std::string& pw = "")
                {
                    std::memset(this, 0, sizeof(MainUserInviteAck));
                    server_id = serverId;
                    room_id = roomId;
                    channel_id = channelId;
                    std::memset(this->nickname, 0, sizeof(this->nickname));
                    std::strcpy(this->nickname, nickname.c_str());
                    std::memset(this->title, 0, sizeof(this->title));
                    std::strcpy(this->title, title.c_str());
                    std::memset(this->password, 0, sizeof(this->password));
                    std::strcpy(this->password, pw.c_str());

                }
                std::vector<std::uint8_t> Serialize(bool bSendTitlePassword = false) const
                {
                    std::vector<std::uint8_t> data;

                    auto server_id_bytes = reinterpret_cast<const uint8_t*>(&server_id);
                    data.insert(data.end(), server_id_bytes, server_id_bytes + sizeof(server_id));

                    auto nickname_bytes = reinterpret_cast<const uint8_t*>(nickname);
                    data.insert(data.end(), nickname_bytes, nickname_bytes + sizeof(nickname));

                    auto room_id_bytes = reinterpret_cast<const uint8_t*>(&room_id);
                    data.insert(data.end(), room_id_bytes, room_id_bytes + sizeof(room_id));

                    auto channel_id_bytes = reinterpret_cast<const uint8_t*>(&channel_id);
                    data.insert(data.end(), channel_id_bytes, channel_id_bytes + sizeof(channel_id));

                    if (bSendTitlePassword)
                    {
                        auto title_bytes = reinterpret_cast<const uint8_t*>(title);
                        data.insert(data.end(), title_bytes, title_bytes + sizeof(title));
                        auto pw_bytes = reinterpret_cast<const uint8_t*>(password);
                        data.insert(data.end(), pw_bytes, pw_bytes + sizeof(password));
                    }
                    return data;
                }
            };

            class MainUserInvitePartyAck
            {
            public:
                std::uint32_t unk1;
                std::uint32_t unk2;
                char nickname[16];
                MainUserInvitePartyAck(const std::uint32_t& data1 = 0, const char* newNickname = "", const std::uint32_t& roomId = 0)
                {
                    std::memset(this, 0, sizeof(MainUserInvitePartyAck));
                    this->unk1 = roomId;
                    this->unk2 = 1;
                    std::memset(this->nickname, 0, sizeof(nickname));
                    std::strcpy(this->nickname, newNickname);

                }
            };

#pragma pack(pop)
        }

        namespace Cast
        {
#pragma pack(push, 1)

            // C2S

            struct CastConnectionReq
            {
                Core::UniqueId UniqueId;
                std::uint64_t Authkey;
            };

            struct CastPlayerSpawnReq
            {
                DirectX::PackedVector::XMHALF4 Position;
                Core::UniqueId UniqueId;
            };
            struct CastJoinPlazaReq
            {
                std::uint32_t plaza_id;
            };

            // S2C

            struct CastEngineServerConnectionAck
            {
                std::int32_t random;

                CastEngineServerConnectionAck(std::int32_t random) : random(random)
                {
                }
            };

            struct CastConnectionAck{};
#pragma pack(pop)
        }

        namespace Core
        {
#pragma pack(push, 1)

#pragma pack(pop)
        }
    }
}

//#endif