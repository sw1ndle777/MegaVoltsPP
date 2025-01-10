#pragma once
#include <iostream>
#include <boost/unordered/unordered_flat_map.hpp>
namespace BaseLib
{
    struct ItemInfo
    {
        std::uint32_t Id;
        std::string Name;
        std::string NameTime;
        std::string NameOption;
        bool IsNaomiUsable;
        bool IsKaiUsable;
        bool IsPandoraUsable;
        bool IsChipUsable;
        bool IsKnoxUsable;
        bool IsSimonUsable;
        bool IsAmeliaUsable;
        bool IsSharkillUsable;
        bool IsSophitiaUsable;
        std::uint32_t Type;
        std::uint32_t InventoryType;
        bool IsUpgradable;
        std::uint32_t LimitedTime;
        std::uint32_t Durability;
        std::uint32_t CouponPrice;
        std::uint32_t CashPrice;
        std::uint32_t PointPrice;
        std::uint32_t SellPointPrice;
        std::uint32_t Stock;
        ItemInfo()
        {
            this->Id = 0;
            this->Name = "";
            this->NameTime = "";
            this->NameOption = "";
            this->IsNaomiUsable = false;
            this->IsKaiUsable = false;
            this->IsPandoraUsable = false;
            this->IsChipUsable = false;
            this->IsKnoxUsable = false;
            this->IsSimonUsable = false;
            this->IsAmeliaUsable = false;
            this->IsSharkillUsable = false;
            this->IsSophitiaUsable = false;
            this->Type = -1;
            this->InventoryType = -1;
            this->IsUpgradable = false;
            this->LimitedTime = -1;
            this->Durability = 0;
            this->CouponPrice = 0;
            this->CashPrice = 0;
            this->PointPrice = 0;
            this->SellPointPrice = 0;
            this->Stock = -1;
        }
    };

    struct SetItemInfo
    {
        std::uint32_t Id;
        std::uint32_t Hair;
        std::uint32_t Face;
        std::uint32_t Upper;
        std::uint32_t Under;
        std::uint32_t Arms;
        std::uint32_t Pants;
        std::uint32_t Boots;
        std::uint32_t AccessoryA;
        std::uint32_t AccessoryB;
        std::uint32_t AccessoryC;
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
        std::uint32_t Id;
        std::uint32_t Category;
        std::uint32_t Type;
        std::uint32_t None;
        std::uint32_t New;
        std::uint32_t Hit;
        std::uint32_t ListType;
        std::uint32_t List01;
        std::uint32_t List01_a;
        std::uint32_t List01_b;
        std::uint32_t List01_c;
        std::uint32_t List01_d;
        std::uint32_t List02;
        std::uint32_t List02_a;
        std::uint32_t List02_b;
        std::uint32_t List02_c;
        std::uint32_t List02_d;
        std::uint32_t List03;
        std::uint32_t List03_a;
        std::uint32_t List03_b;
        std::uint32_t List03_c;
        std::uint32_t List03_d;
        std::uint32_t List04;
        std::uint32_t List04_a;
        std::uint32_t List04_b;
        std::uint32_t List04_c;
        std::uint32_t List04_d;
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
        std::uint32_t GroupId;
        std::uint32_t UpgradeType;
        std::uint32_t ItemId;
        std::uint32_t ItemParentId;
        std::uint32_t Probability;
        std::uint32_t AddedProbability;
        std::uint32_t HoldProbability;
        std::uint32_t BuyCash;
        std::uint32_t BuyPoint;
        std::uint32_t UseExp;
        std::uint32_t RestoreCash;
        std::uint32_t RestorePoint;
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
        std::uint32_t Id;
        std::uint32_t Group;
        std::uint32_t InfoId;
        std::uint32_t ItemType;
        std::uint32_t LuckyType;
        std::uint32_t Probability;
        std::uint32_t ItemId;
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
        GachaponPackageItem(std::uint32_t item_id)
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
        std::uint32_t Id;
        std::string Name;
        std::uint32_t Type;
        std::uint32_t InfoId;
        std::uint32_t LimitedGrade;
        std::uint32_t Price;
        std::uint32_t LuckyPoint;
        //std::unordered_map<std::uint32_t, std::vector<GachaponPackageItem>> Gachapons;
        boost::unordered_flat_map<std::uint32_t, std::vector<GachaponPackageItem>> Gachapons;
        GachaponInfo()
        {
            this->Id = -1;
            this->Name = "";
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
        std::uint32_t InfoId;
        std::uint32_t GroupId;
        std::uint32_t ItemId;
        std::uint32_t Type;
        std::uint32_t Probability;
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
        std::uint32_t Id;
        std::uint32_t Type;
        std::string Name;
        std::uint32_t Data;
        std::uint32_t Mode;
        std::uint32_t CombatType;
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
        std::uint32_t Id;
        std::string Name;
        std::uint32_t MaxUsers;
        std::int32_t DeathHeight;
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
        std::uint32_t Grade;
        std::uint32_t Exp;
        std::uint32_t RewardPoint;
        std::uint32_t RewardItem;
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
        std::uint32_t GameMode;
        std::uint32_t ExpBase;
        std::uint32_t ExpMax;
        std::uint32_t ExpKill;
        std::uint32_t ExpModeKill;
        std::uint32_t ExpDeath;
        std::uint32_t ExpAssist;
        std::uint32_t ExpMission;
        std::uint32_t ExpMissionWin;
        std::uint32_t PointBase;
        std::uint32_t PointMax;
        std::uint32_t PointKill;
        std::uint32_t PointModeKill;
        std::uint32_t PointDeath;
        std::uint32_t PointAssist;
        std::uint32_t PointMission;
        std::uint32_t PointMissionWin;
        std::uint32_t ModeLimitedTime;
        std::uint32_t PlayerLimitedTime;
        std::uint32_t PenaltyPoint;
        std::uint32_t ExpEvent;
        std::uint32_t PointEvent;
        std::uint32_t ClanExpBase;
        std::uint32_t ClanExpBnus;
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