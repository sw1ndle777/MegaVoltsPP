#pragma once
#include <iostream>
#include <boost/unordered/unordered_flat_map.hpp>
namespace BaseLib
{
#pragma pack(push, 1)
    struct ItemInfo
    {
        uint32_t Id;
        std::string Name;
        //std::string NameTime;
        //std::string NameOption;
        bool IsNaomiUsable;
        bool IsKaiUsable;
        bool IsPandoraUsable;
        bool IsChipUsable;
        bool IsKnoxUsable;
        bool IsSimonUsable;
        //bool IsAmeliaUsable;
        //bool IsSharkillUsable;
        //bool IsSophitiaUsable;
        uint32_t Type;
        //uint32_t InventoryType;
        bool IsUpgradable;
        uint32_t LimitedTime;
        uint32_t Durability;
        uint32_t CouponPrice;
        uint32_t CashPrice;
        uint32_t PointPrice;
        uint32_t SellPointPrice;
        uint32_t Stock;
        uint32_t BonusEffectId;
        ItemInfo()
        {
            this->Id = 0;
            //this->Name = "";
            //this->NameTime = "";
            //this->NameOption = "";
            this->IsNaomiUsable = false;
            this->IsKaiUsable = false;
            this->IsPandoraUsable = false;
            this->IsChipUsable = false;
            this->IsKnoxUsable = false;
            this->IsSimonUsable = false;
            //this->IsAmeliaUsable = false;
            //this->IsSharkillUsable = false;
            //this->IsSophitiaUsable = false;
            this->Type = -1;
            //this->InventoryType = -1;
            this->IsUpgradable = false;
            this->LimitedTime = -1;
            this->Durability = 0;
            this->CouponPrice = 0;
            this->CashPrice = 0;
            this->PointPrice = 0;
            this->SellPointPrice = 0;
            this->Stock = -1;
            this->BonusEffectId = 0;
        }
    };
#pragma pack(pop)

    struct EffectInfo {
        uint32_t id;
        uint32_t key;
        uint32_t valueA;
        EffectInfo() {
            this->id = 0;
            this->key = 0;
            this->valueA = 0;
        }
    };

    struct CollectionInfo {
        uint32_t id;
        uint32_t setIndex;
        uint32_t rewardPoint;
        uint32_t rewardExp;
        uint32_t rewardItem;
        uint32_t missionType;
        CollectionInfo() {
            this->id = 0;
            this->setIndex = 0;
            this->rewardPoint = 0;
            this->rewardExp = 0;
            this->rewardItem = 0;
            this->missionType = 0;
        }
    };

    struct DailyMissionInfo {
        uint32_t id;
        uint32_t setIndex;
        uint32_t rewardPoint;
        uint32_t rewardExp;
        uint32_t rewardItem;
        uint32_t goal;
        DailyMissionInfo() {
            this->id = 0;
            this->setIndex = 0;
            this->rewardPoint = 0;
            this->rewardExp = 0;
            this->rewardItem = 0;
            this->goal = 0;
        }
    };

    struct SetItemInfo
    {
        uint32_t Id;
        uint32_t Hair;
        uint32_t Face;
        uint32_t Upper;
        uint32_t Under;
        uint32_t Arms;
        uint32_t Pants;
        uint32_t Boots;
        uint32_t AccessoryA;
        uint32_t AccessoryB;
        uint32_t AccessoryC;
        SetItemInfo()
        {
            this->Id = 0;
            this->Hair = 0;
            this->Face = 0;
            this->Upper = 0;
            this->Under = 0;
            this->Arms = 0;
            this->Pants = 0;
            this->Boots = 0;
            this->AccessoryA = 0;
            this->AccessoryB = 0;
            this->AccessoryC = 0;
        }
    };
    struct VendorInfo
    {
        uint32_t Id;
        uint32_t Category;
        uint32_t Type;
        uint32_t None;
        uint32_t New;
        uint32_t Hit;
        uint32_t ListType;
        uint32_t List01;
        uint32_t List01_a;
        uint32_t List01_b;
        uint32_t List01_c;
        uint32_t List01_d;
        uint32_t List02;
        uint32_t List02_a;
        uint32_t List02_b;
        uint32_t List02_c;
        uint32_t List02_d;
        uint32_t List03;
        uint32_t List03_a;
        uint32_t List03_b;
        uint32_t List03_c;
        uint32_t List03_d;
        uint32_t List04;
        uint32_t List04_a;
        uint32_t List04_b;
        uint32_t List04_c;
        uint32_t List04_d;
        bool IsGift;
        VendorInfo()
        {
            this->Id = -1;
            this->Category = -1;
            this->Type = -1;
            this->None = -1;
            this->New = -1;
            this->Hit = -1;
            this->ListType = -1;
            this->List01 = -1;
            this->List01_a = -1;
            this->List01_b = -1;
            this->List01_c = -1;
            this->List01_d = -1;
            this->List02 = -1;
            this->List02_a = -1;
            this->List02_b = -1;
            this->List02_c = -1;
            this->List02_d = -1;
            this->List03 = -1;
            this->List03_a = -1;
            this->List03_b = -1;
            this->List03_c = -1;
            this->List03_d = -1;
            this->List04 = -1;
            this->List04_a = -1;
            this->List04_b = -1;
            this->List04_c = -1;
            this->List04_d = -1;
            this->IsGift = false;
        }
    };

    struct UpgradeInfo
    {
        uint32_t GroupId;
        uint32_t UpgradeType;
        uint32_t ItemId;
        uint32_t ItemParentId;
        uint32_t Probability;
        uint32_t AddedProbability;
        uint32_t HoldProbability;
        uint32_t BuyCash;
        uint32_t BuyPoint;
        uint32_t UseExp;
        uint32_t RestoreCash;
        uint32_t RestorePoint;
        UpgradeInfo()
        {
            this->GroupId = -1;
            this->UpgradeType = -1;
            this->ItemId = -1;
            this->ItemParentId = -1;
            this->Probability = -1;
            this->AddedProbability = -1;
            this->HoldProbability = -1;
            this->BuyCash = -1;
            this->BuyPoint = -1;
            this->UseExp = -1;
            this->RestoreCash = -1;
            this->RestorePoint = -1;
        }
    };
    struct GachaponPackageItem
    {
        uint32_t Id;
        uint32_t Group;
        uint32_t InfoId;
        uint32_t ItemType;
        uint32_t LuckyType;
        uint32_t Probability;
        uint32_t ItemId;
        GachaponPackageItem()
        {
            this->Id = -1;
            this->Group = -1;
            this->InfoId = -1;
            this->ItemType = -1;
            this->LuckyType = -1;
            this->Probability = -1;
            this->ItemId = -1;
        }
        GachaponPackageItem(uint32_t item_id)
        {
            this->Id = -1;
            this->Group = -1;
            this->InfoId = -1;
            this->ItemType = -1;
            this->LuckyType = -1;
            this->Probability = -1;
            this->ItemId = item_id;
        }
    };
    struct GachaponInfo
    {
        uint32_t Id;
        //std::string Name;
        uint32_t Type;
        uint32_t InfoId;
        uint32_t LimitedGrade;
        uint32_t Price;
        uint32_t LuckyPoint;
        //std::unordered_map<uint32_t, std::vector<GachaponPackageItem>> Gachapons;
        boost::unordered_flat_map<uint32_t, std::vector<GachaponPackageItem>> Gachapons;
        GachaponInfo()
        {
            this->Id = -1;
            //this->Name = "";
            this->Type = -1;
            this->InfoId = -1;
            this->LimitedGrade = -1;
            this->Price = -1;
            this->LuckyPoint = -1;
            this->Gachapons.clear();
        }
    };
    struct PackageInfo
    {
        uint32_t InfoId;
        uint32_t GroupId;
        uint32_t ItemId;
        uint32_t Type;
        uint32_t Probability;
        PackageInfo()
        {
            this->InfoId = -1;
            this->GroupId = -1;
            this->ItemId = -1;
            this->Type = -1;
            this->Probability = -1;
        }
    };
    struct RoomOptionInfo
    {
        uint32_t Id;
        uint32_t Type;
        std::string Name;
        uint32_t Data;
        uint32_t Mode;
        uint32_t CombatType;
        RoomOptionInfo()
        {
            this->Id = -1;
            this->Type = -1;
            this->Name = "";
            this->Data = -1;
            this->Mode = -1;
            this->CombatType = -1;
        }
    };
    struct MapInfo
    {
        uint32_t Id;
        std::string Name;
        uint32_t MaxUsers;
        int32_t DeathHeight;
        bool tdm;
        bool ffa;
        bool itm;
        bool ctf;
        bool ctm;
        bool sab;
        bool cim;
        bool zsm;
        bool grm;
        bool mock;
        bool bmb;
        bool sni;
        bool nod;
        bool pve;
        bool bot;
        bool tut;
        bool clan_ctf;
        bool clan_sab;
        bool clan_tdm;
        bool clan_bmb;
        MapInfo()
        {
            this->Id = -1;
            this->Name = "";
            this->MaxUsers = -1;
            this->DeathHeight = -1;
            this->tdm = false;
            this->ffa = false;
            this->itm = false;
            this->ctf = false;
            this->ctm = false;
            this->sab = false;
            this->cim = false;
            this->zsm = false;
            this->grm = false;
            this->mock = false;
            this->bmb = false;
            this->sni = false;
            this->nod = false;
            this->pve = false;
            this->bot = false;
            this->tut = false;
            this->clan_ctf = false;
            this->clan_sab = false;
            this->clan_tdm = false;
            this->clan_bmb = false;
        }
    };

    struct GradeInfo
    {
        uint32_t Grade;
        uint32_t Exp;
        uint32_t RewardPoint;
        uint32_t RewardItem;
        GradeInfo()
        {
            this->Grade = 0;
            this->Exp = -1;
            this->RewardPoint = -1;
            this->RewardItem = -1;
        }
    };
    struct RewardInfo
    {
        uint32_t GameMode;
        uint32_t ExpBase;
        uint32_t ExpMax;
        uint32_t ExpKill;
        uint32_t ExpModeKill;
        uint32_t ExpDeath;
        uint32_t ExpAssist;
        uint32_t ExpMission;
        uint32_t ExpMissionWin;
        uint32_t PointBase;
        uint32_t PointMax;
        uint32_t PointKill;
        uint32_t PointModeKill;
        uint32_t PointDeath;
        uint32_t PointAssist;
        uint32_t PointMission;
        uint32_t PointMissionWin;
        uint32_t ModeLimitedTime;
        uint32_t PlayerLimitedTime;
        uint32_t PenaltyPoint;
        uint32_t ExpEvent;
        uint32_t PointEvent;
        uint32_t ClanExpBase;
        uint32_t ClanExpBnus;
        RewardInfo()
        {
            this->GameMode = -1;
            this->ExpBase = -1;
            this->ExpMax = -1;
            this->ExpKill = -1;
            this->ExpModeKill = -1;
            this->ExpDeath = -1;
            this->ExpAssist = -1;
            this->ExpMission = -1;
            this->ExpMissionWin = -1;
            this->PointBase = -1;
            this->PointMax = -1;
            this->PointKill = -1;
            this->PointModeKill = -1;
            this->PointDeath = -1;
            this->PointAssist = -1;
            this->PointMission = -1;
            this->PointMissionWin = -1;
            this->ModeLimitedTime = -1;
            this->PlayerLimitedTime = -1;
            this->PenaltyPoint = -1;
            this->ExpEvent = -1;
            this->PointEvent = -1;
            this->ClanExpBase = -1;
            this->ClanExpBnus = -1;
        }
    };

   
}
//#endif