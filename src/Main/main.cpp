#define ASIO_STANDALONE

#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>

#include <bitfileextractor.hpp>
#include <bitarchivereader.hpp>
#include <chrono>

#include "BaseLib/CLog.h"
#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CThreadPool.h"
#include "BaseLib/CDatabase.h"
#include "BaseLib/CDBM.h"
#include "BaseLib/CDBData.h"

#include "NetEngine/CServer.h"
#include "CMainServer.h"
#include "BaseLib/Utility.h"
#include <fmt/color.h>
#include "BaseLib/CCrashHandler.h"
std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Main;
using namespace bit7z;

int main()
{
    HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
    Utility::GetCpuUsage(m_process_handle);
    CloseHandle(m_process_handle);
    CrashHandler::Init("../crash_dumps/MegaVoltsPP_main.dmp");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);//enable colors
    std::srand(static_cast<std::uint32_t>(std::time(NULL)));

    BaseLib::EventLog->Initialize("../logs/MegaVoltsPP_main.log", false);
    BaseLib::DefaultSettings->LoadOptions();
    const auto& server_settings = BaseLib::DefaultSettings->GetServerSettings();
   
    //BaseLib::ThreadPool->Initialize(server_settings.main.pool_threads);
    BaseLib::Database->Initialize(server_settings.database.db_name.c_str(), server_settings.database.host.c_str(), server_settings.database.port, server_settings.database.user.c_str(), server_settings.database.password.c_str());

    Game::CMainServer* mainServer = new Game::CMainServer();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings(server_settings.main.host.c_str(), std::to_string(server_settings.main.port).c_str(), std::to_string(server_settings.main.ipc_port).c_str(), server_settings.main.debug, true, true, server_settings.main.watchguard, server_settings.main.asio_threads);
    auto start_time = std::chrono::system_clock::now();

    std::vector<std::uint8_t> buffer_iteminfo;
    std::vector<std::uint8_t> buffer_itemweaponsinfo;
    std::vector<std::uint8_t> buffer_setiteminfo;
    std::vector<std::uint8_t> buffer_vendorinfo;
    std::vector<std::uint8_t> buffer_upgradeinfo;
    std::vector<std::uint8_t> buffer_gachaponinfo;
    std::vector<std::uint8_t> buffer_gachaponpackageinfo;
    std::vector<std::uint8_t> buffer_itempackageinfo;
    std::vector<std::uint8_t> buffer_roomoptioninfo;
    std::vector<std::uint8_t> buffer_gradeinfo;
    std::vector<std::uint8_t> buffer_rewardinfo;
    try {

        Bit7zLibrary lib{ "7z.dll" };
        BitFileExtractor extractor{ lib, BitFormat::Zip };
        extractor.setPassword("!dptmzpdl@xmfkdlvhtm@goqm!");
        extractor.extractMatching("cgd.dip", "ENG\\iteminfo.cdb", buffer_iteminfo);
        extractor.extractMatching("cgd.dip", "ENG\\itemweaponsinfo.cdb", buffer_itemweaponsinfo);
        extractor.extractMatching("cgd.dip", "ENG\\setiteminfo.cdb", buffer_setiteminfo);
        extractor.extractMatching("cgd.dip", "ENG\\vendorinfo.cdb", buffer_vendorinfo);
        extractor.extractMatching("cgd.dip", "ENG\\upgradeinfo.cdb", buffer_upgradeinfo);
        extractor.extractMatching("cgd.dip", "ENG\\gachaponinfo.cdb", buffer_gachaponinfo);
        extractor.extractMatching("cgd.dip", "ENG\\gachaponpackageinfo.cdb", buffer_gachaponpackageinfo);
        extractor.extractMatching("cgd.dip", "ENG\\itempackageinfo.cdb", buffer_itempackageinfo);
        extractor.extractMatching("cgd.dip", "ENG\\roomoptionInfo.cdb", buffer_roomoptioninfo);
        extractor.extractMatching("cgd.dip", "ENG\\gradeinfo.cdb", buffer_gradeinfo);
        extractor.extractMatching("cgd.dip", "ENG\\rewardinfo.cdb", buffer_rewardinfo);
        //extractor.extractMatching("cgd.dip", "ENG\\mapinfo.cdb", buffer_mapinfo);
    }
    catch (const bit7z::BitException& ex) { std::printf("%s\n", ex.what()); BaseLib::EventLog->Info(ex.what());}
    CDBM iteminfo_cdb, itemweaponsinfo_cdb, setiteminfo_cdb, vendorinfo_cdb, upgradeinfo_cdb, gachaponinfo_cdb, gachaponpackageinfo_cdb, itempackageinfo_cdb, roomoptioninfo_cdb, gradeinfo_cdb, rewardinfo_cdb;
    iteminfo_cdb.LoadCDB(buffer_iteminfo);
    itemweaponsinfo_cdb.LoadCDB(buffer_itemweaponsinfo);
    
    auto iteminfo_data = iteminfo_cdb.GetDataRows();
    auto itemweaponsinfo_data = itemweaponsinfo_cdb.GetDataRows();
    for (std::uint32_t i = 0; i < iteminfo_data.size(); i++)
    {
        BaseLib::ItemInfo new_item_info;
        auto& data_fields = iteminfo_data[i];
        new_item_info.Id = data_fields.at("ii_id")->GetInt();
        new_item_info.Name = data_fields.at("ii_name")->GetString();
        new_item_info.NameTime = data_fields.at("ii_name_time")->GetString();
        new_item_info.NameOption = data_fields.at("ii_name_option")->GetString();
        new_item_info.IsNaomiUsable = data_fields.at("ii_class_a")->GetBool();
        new_item_info.IsKaiUsable = data_fields.at("ii_class_b")->GetBool();
        new_item_info.IsPandoraUsable = data_fields.at("ii_class_c")->GetBool();
        new_item_info.IsChipUsable = data_fields.at("ii_class_d")->GetBool();
        new_item_info.IsKnoxUsable = data_fields.at("ii_class_e")->GetBool();
        new_item_info.IsSimonUsable = data_fields.at("ii_class_f")->GetBool();
        new_item_info.IsAmeliaUsable = data_fields.at("ii_class_g")->GetBool();
        new_item_info.IsSharkillUsable = data_fields.at("ii_class_h")->GetBool();
        new_item_info.IsSophitiaUsable = data_fields.at("ii_class_i")->GetBool();
        new_item_info.Type = data_fields.at("ii_type")->GetInt();
        new_item_info.InventoryType = data_fields.at("ii_type_inven")->GetInt();
        new_item_info.IsUpgradable = data_fields.at("ii_upgradable")->GetBool();
        new_item_info.LimitedTime = data_fields.at("ii_limited_time")->GetInt();
        new_item_info.Durability = data_fields.at("ii_durable_value")->GetInt();
        new_item_info.CouponPrice = data_fields.at("ii_buy_coupon")->GetInt();
        new_item_info.CashPrice = data_fields.at("ii_buy_cash")->GetInt();
        new_item_info.PointPrice = data_fields.at("ii_buy_point")->GetInt();
        new_item_info.SellPointPrice = data_fields.at("ii_sell_point")->GetInt();
        new_item_info.Stock = data_fields.at("ii_stocks")->GetInt();
        mainServer->AddItemInfoCache(new_item_info.Id, new_item_info);
    }
    for (std::uint32_t i = 0; i < itemweaponsinfo_data.size(); i++)
    {
        BaseLib::ItemInfo new_item_info;
        auto& data_fields = itemweaponsinfo_data[i];
        new_item_info.Id = data_fields.at("ii_id")->GetInt();
        new_item_info.Name = data_fields.at("ii_name")->GetString();
        new_item_info.NameTime = data_fields.at("ii_name_time")->GetString();
        new_item_info.NameOption = data_fields.at("ii_name_option")->GetString();
        new_item_info.IsNaomiUsable = data_fields.at("ii_class_a")->GetBool();
        new_item_info.IsKaiUsable = data_fields.at("ii_class_b")->GetBool();
        new_item_info.IsPandoraUsable = data_fields.at("ii_class_c")->GetBool();
        new_item_info.IsChipUsable = data_fields.at("ii_class_d")->GetBool();
        new_item_info.IsKnoxUsable = data_fields.at("ii_class_e")->GetBool();
        new_item_info.IsSimonUsable = data_fields.at("ii_class_f")->GetBool();
        new_item_info.IsAmeliaUsable = data_fields.at("ii_class_g")->GetBool();
        new_item_info.IsSharkillUsable = data_fields.at("ii_class_h")->GetBool();
        new_item_info.IsSophitiaUsable = data_fields.at("ii_class_i")->GetBool();
        new_item_info.Type = data_fields.at("ii_type")->GetInt();
        new_item_info.InventoryType = data_fields.at("ii_type_inven")->GetInt();
        new_item_info.IsUpgradable = data_fields.at("ii_upgradable")->GetBool();
        new_item_info.LimitedTime = data_fields.at("ii_limited_time")->GetInt();
        new_item_info.Durability = data_fields.at("ii_durable_value")->GetInt();
        new_item_info.CouponPrice = data_fields.at("ii_buy_coupon")->GetInt();
        new_item_info.CashPrice = data_fields.at("ii_buy_cash")->GetInt();
        new_item_info.PointPrice = data_fields.at("ii_buy_point")->GetInt();
        new_item_info.SellPointPrice = data_fields.at("ii_sell_point")->GetInt();
        new_item_info.Stock = data_fields.at("ii_stocks")->GetInt();
        mainServer->AddItemInfoCache(new_item_info.Id, new_item_info);
    }
    auto end_time = std::chrono::system_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    auto elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) items in %s", mainServer->GetItemsInfoCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetItemsInfoCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") items in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());

    start_time = std::chrono::system_clock::now();
    setiteminfo_cdb.LoadCDB(buffer_setiteminfo);
    auto setiteminfo_data = setiteminfo_cdb.GetDataRows();
    for (std::uint32_t i = 0; i < setiteminfo_data.size(); i++)
    {
        BaseLib::SetItemInfo new_setitem_info;
        auto& data_fields = setiteminfo_data[i];
        new_setitem_info.Id = data_fields.at("si_id")->GetInt();
        new_setitem_info.Hair = data_fields.at("si_hair")->GetInt();
        new_setitem_info.Face = data_fields.at("si_face")->GetInt();
        new_setitem_info.Upper = data_fields.at("si_upper")->GetInt();
        new_setitem_info.Under = data_fields.at("si_under")->GetInt();
        new_setitem_info.Pants = data_fields.at("si_pants")->GetInt();
        new_setitem_info.Arms = data_fields.at("si_arms")->GetInt();
        new_setitem_info.Boots = data_fields.at("si_boots")->GetInt();
        new_setitem_info.AccessoryA = data_fields.at("si_acce_A")->GetInt();
        new_setitem_info.AccessoryB = data_fields.at("si_acce_B")->GetInt();
        new_setitem_info.AccessoryC = data_fields.at("si_acce_C")->GetInt();
        mainServer->AddSetItemInfoCache(new_setitem_info.Id, new_setitem_info);
    }
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) set items in %s", mainServer->GetSetItemsInfoCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetSetItemsInfoCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") set items in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());

    start_time = std::chrono::system_clock::now();
    vendorinfo_cdb.LoadCDB(buffer_vendorinfo);
    auto vendorinfo_data = vendorinfo_cdb.GetDataRows();
    for (std::uint32_t i = 0; i < vendorinfo_data.size(); i++)
    {
        BaseLib::VendorInfo new_vendorinfo;
        auto& data_fields = vendorinfo_data[i];
        new_vendorinfo.Id = data_fields.at("vi_id")->GetInt();
        new_vendorinfo.Category = data_fields.at("vi_category")->GetInt();
        new_vendorinfo.Type = data_fields.at("vi_type")->GetInt();
        new_vendorinfo.None = data_fields.at("vi_array_none")->GetInt();
        new_vendorinfo.New = data_fields.at("vi_array_new")->GetInt();
        new_vendorinfo.Hit = data_fields.at("vi_array_hit")->GetInt();
        new_vendorinfo.ListType = data_fields.at("vi_list_type")->GetInt();
        new_vendorinfo.List01 = data_fields.at("vi_list_01")->GetInt();
        new_vendorinfo.List01_a = data_fields.at("vi_list_01_a")->GetInt();
        new_vendorinfo.List01_b = data_fields.at("vi_list_01_b")->GetInt();
        new_vendorinfo.List01_c = data_fields.at("vi_list_01_c")->GetInt();
        new_vendorinfo.List01_d = data_fields.at("vi_list_01_d")->GetInt();
        new_vendorinfo.List02 = data_fields.at("vi_list_02")->GetInt();
        new_vendorinfo.List02_a = data_fields.at("vi_list_02_a")->GetInt();
        new_vendorinfo.List02_b = data_fields.at("vi_list_02_b")->GetInt();
        new_vendorinfo.List02_c = data_fields.at("vi_list_02_c")->GetInt();
        new_vendorinfo.List02_d = data_fields.at("vi_list_02_d")->GetInt();
        new_vendorinfo.List03 = data_fields.at("vi_list_03")->GetInt();
        new_vendorinfo.List03_a = data_fields.at("vi_list_03_a")->GetInt();
        new_vendorinfo.List03_b = data_fields.at("vi_list_03_b")->GetInt();
        new_vendorinfo.List03_c = data_fields.at("vi_list_03_c")->GetInt();
        new_vendorinfo.List03_d = data_fields.at("vi_list_03_d")->GetInt();
        new_vendorinfo.List04 = data_fields.at("vi_list_04")->GetInt();
        new_vendorinfo.List04_a = data_fields.at("vi_list_04_a")->GetInt();
        new_vendorinfo.List04_b = data_fields.at("vi_list_04_b")->GetInt();
        new_vendorinfo.List04_c = data_fields.at("vi_list_04_c")->GetInt();
        new_vendorinfo.List04_d = data_fields.at("vi_list_04_d")->GetInt();
        new_vendorinfo.IsGift = data_fields.at("vi_isgift")->GetBool();
        mainServer->AddVendorInfo(new_vendorinfo);
    }
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) vendor infos in %s", mainServer->GetVendorInfosCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetVendorInfosCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") vendor infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());

    start_time = std::chrono::system_clock::now();
    upgradeinfo_cdb.LoadCDB(buffer_upgradeinfo);
    auto upgradeinfo_data = upgradeinfo_cdb.GetDataRows();
    for (std::uint32_t i = 0; i < upgradeinfo_data.size(); i++)
    {
        BaseLib::UpgradeInfo new_upgradeinfo;
        auto& data_fields = upgradeinfo_data[i];
        new_upgradeinfo.GroupId = data_fields.at("ui_group")->GetInt();
        new_upgradeinfo.UpgradeType = data_fields.at("ui_type")->GetInt();
        new_upgradeinfo.ItemId = data_fields.at("ui_itemid")->GetInt();
        new_upgradeinfo.ItemParentId = data_fields.at("ui_parentid")->GetInt();
        new_upgradeinfo.Probability = data_fields.at("ui_prob")->GetInt();
        new_upgradeinfo.AddedProbability = data_fields.at("ui_added_prob")->GetInt();
        new_upgradeinfo.HoldProbability = data_fields.at("ui_hold_prob")->GetInt();
        new_upgradeinfo.BuyCash = data_fields.at("ui_buy_cash")->GetInt();
        new_upgradeinfo.BuyPoint = data_fields.at("ui_buy_point")->GetInt();
        new_upgradeinfo.UseExp = data_fields.at("ui_use_exp")->GetInt();
        new_upgradeinfo.RestoreCash = data_fields.at("ui_restore_cash")->GetInt();
        new_upgradeinfo.RestorePoint = data_fields.at("ui_restore_point")->GetInt();
        mainServer->AddUpgradeInfoCache(new_upgradeinfo.GroupId, static_cast<NetEngine::Items::Upgrade::Type>(new_upgradeinfo.UpgradeType), new_upgradeinfo);
    }
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) upgrade infos in %s", mainServer->GetUpgradeInfoCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetUpgradeInfoCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") upgrade infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());

    start_time = std::chrono::system_clock::now();
    gachaponinfo_cdb.LoadCDB(buffer_gachaponinfo);
    gachaponpackageinfo_cdb.LoadCDB(buffer_gachaponpackageinfo);
    auto gachaponinfo_data = gachaponinfo_cdb.GetDataRows();
    auto gachaponpackageinfo_data = gachaponpackageinfo_cdb.GetDataRows();
    //std::unordered_map<std::uint32_t, std::vector<BaseLib::GachaponPackageItem>> gachapon_package_items;
    boost::unordered_flat_map<std::uint32_t, std::vector<BaseLib::GachaponPackageItem>> gachapon_package_items;
    for (std::uint32_t i = 0; i < gachaponpackageinfo_data.size(); i++)
    {
        BaseLib::GachaponPackageItem new_gachaponpackageitem;
        auto& data_fields = gachaponpackageinfo_data[i];
        new_gachaponpackageitem.Id = data_fields.at("gi_id")->GetInt();
        new_gachaponpackageitem.Group = data_fields.at("gi_group")->GetInt();
        new_gachaponpackageitem.InfoId = data_fields.at("gi_infoid")->GetInt();
        new_gachaponpackageitem.ItemType = data_fields.at("gi_type")->GetInt();
        new_gachaponpackageitem.LuckyType = data_fields.at("gi_luckytype")->GetInt();
        new_gachaponpackageitem.Probability = data_fields.at("gi_prob")->GetInt();
        new_gachaponpackageitem.ItemId = data_fields.at("gi_itemid")->GetInt();
        gachapon_package_items[new_gachaponpackageitem.InfoId].push_back(new_gachaponpackageitem);
    }
    for (std::uint32_t i = 0; i < gachaponinfo_data.size(); i++)
    {
        BaseLib::GachaponInfo new_gachaponinfo;
        auto& data_fields = gachaponinfo_data[i];
        new_gachaponinfo.Id = data_fields.at("gi_id")->GetInt();
        new_gachaponinfo.Name = data_fields.at("gi_name")->GetString();
        new_gachaponinfo.Type = data_fields.at("gi_type")->GetInt();
        new_gachaponinfo.InfoId = data_fields.at("gi_infoid")->GetInt();
        new_gachaponinfo.LimitedGrade = data_fields.at("gi_limited_grade")->GetInt();
        new_gachaponinfo.Price = data_fields.at("gi_price")->GetInt();
        new_gachaponinfo.LuckyPoint = data_fields.at("gi_luckypoint")->GetInt();
        const auto& items = gachapon_package_items[new_gachaponinfo.InfoId];
        for (const auto& item : items)
            new_gachaponinfo.Gachapons[item.Group].push_back(item);

        mainServer->AddGachaponInfoCache(new_gachaponinfo.Id, new_gachaponinfo);
    }
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) gachapon infos in %s", mainServer->GetGachaponsCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetGachaponsCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") gachapon infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());

    start_time = std::chrono::system_clock::now();
    itempackageinfo_cdb.LoadCDB(buffer_itempackageinfo);
    auto itempackageinfo_data = itempackageinfo_cdb.GetDataRows();

    for (std::uint32_t i = 0; i < itempackageinfo_data.size(); i++)
    {
        BaseLib::PackageInfo new_packageinfo;
        auto& data_fields = itempackageinfo_data[i];
        new_packageinfo.InfoId = data_fields.at("ip_infoid")->GetInt();
        new_packageinfo.GroupId = data_fields.at("ip_group")->GetInt();
        new_packageinfo.ItemId = data_fields.at("ip_itemid")->GetInt();
        new_packageinfo.Type = data_fields.at("ip_type")->GetInt();
        new_packageinfo.Probability = data_fields.at("ip_prob")->GetInt();
        mainServer->AddPackageItemCache(new_packageinfo.InfoId, new_packageinfo.GroupId, new_packageinfo);
    }

    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) package infos in %s", mainServer->GetPackagesCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetPackagesCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") package infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());

    start_time = std::chrono::system_clock::now();
    roomoptioninfo_cdb.LoadCDB(buffer_roomoptioninfo);
    auto roomoptioninfo_data = roomoptioninfo_cdb.GetDataRows();

    for (std::uint32_t i = 0; i < roomoptioninfo_data.size(); i++)
    {
        BaseLib::RoomOptionInfo new_roomoptioninfo;
        auto& data_fields = roomoptioninfo_data[i];
        new_roomoptioninfo.Id = data_fields.at("ro_id")->GetInt();
        new_roomoptioninfo.Type = data_fields.at("ro_type")->GetInt();
        new_roomoptioninfo.Name = data_fields.at("ro_text")->GetString();
        new_roomoptioninfo.Data = data_fields.at("ro_data")->GetInt();
        new_roomoptioninfo.Mode = data_fields.at("ro_mod")->GetInt();
        new_roomoptioninfo.CombatType = data_fields.at("ro_combattype")->GetInt();

        mainServer->AddRoomOptionInfoCache(new_roomoptioninfo.Mode, new_roomoptioninfo);
    }

    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) room option infos in %s", mainServer->GetRoomOptionsInfoSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetRoomOptionsInfoSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") room option infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());


    start_time = std::chrono::system_clock::now();
    gradeinfo_cdb.LoadCDB(buffer_gradeinfo);
    auto gradeinfo_data = gradeinfo_cdb.GetDataRows();

    for(std::uint32_t i = 0; i < gradeinfo_data.size(); i++)
    {
        BaseLib::GradeInfo new_gradeinfo;
        auto& data_fields = gradeinfo_data[i];
        new_gradeinfo.Grade = data_fields.at("gi_grade")->GetInt();
        new_gradeinfo.Exp = data_fields.at("gi_exp")->GetInt();
        new_gradeinfo.RewardPoint = data_fields.at("gi_reward_point")->GetInt();
        new_gradeinfo.RewardItem = data_fields.at("gi_reward_item")->GetInt();
        mainServer->AddGradeInfoCache(new_gradeinfo.Grade, new_gradeinfo);
    }

    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) grade infos in %s", mainServer->GetGradesInfoCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetGradesInfoCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") grade infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());


    start_time = std::chrono::system_clock::now();
    rewardinfo_cdb.LoadCDB(buffer_rewardinfo);
    auto rewardinfo_data = rewardinfo_cdb.GetDataRows();

    for (std::uint32_t i = 0; i < rewardinfo_data.size(); i++)
    {
        BaseLib::RewardInfo new_rewardinfo;
        auto& data_fields = rewardinfo_data[i];
        new_rewardinfo.GameMode = data_fields.at("ri_mod")->GetInt();
        new_rewardinfo.ExpBase = data_fields.at("ri_exp_base")->GetInt();
        new_rewardinfo.ExpMax = data_fields.at("ri_exp_max")->GetInt();
        new_rewardinfo.ExpKill = data_fields.at("ri_exp_kill")->GetInt();
        new_rewardinfo.ExpModeKill = data_fields.at("ri_exp_mod_kill")->GetInt();
        new_rewardinfo.ExpDeath = data_fields.at("ri_exp_death")->GetInt();
        new_rewardinfo.ExpAssist = data_fields.at("ri_exp_assist")->GetInt();
        new_rewardinfo.ExpMission = data_fields.at("ri_exp_mission")->GetInt();
        new_rewardinfo.ExpMissionWin = data_fields.at("ri_exp_mission_win")->GetInt();
        new_rewardinfo.PointBase = data_fields.at("ri_poi_base")->GetInt();
        new_rewardinfo.PointMax = data_fields.at("ri_poi_max")->GetInt();
        new_rewardinfo.PointKill = data_fields.at("ri_poi_kill")->GetInt();
        new_rewardinfo.PointModeKill = data_fields.at("ri_poi_mod_kill")->GetInt();
        new_rewardinfo.PointDeath = data_fields.at("ri_poi_death")->GetInt();
        new_rewardinfo.PointAssist = data_fields.at("ri_poi_assist")->GetInt();
        new_rewardinfo.PointMission = data_fields.at("ri_poi_mission")->GetInt();
        new_rewardinfo.PointMissionWin = data_fields.at("ri_poi_mission_win")->GetInt();
        new_rewardinfo.ModeLimitedTime = data_fields.at("ri_mod_limited_time")->GetInt();
        new_rewardinfo.PlayerLimitedTime = data_fields.at("ri_player_limited_time")->GetInt();
        new_rewardinfo.PenaltyPoint = data_fields.at("ri_penalty_point")->GetInt();
        new_rewardinfo.ExpEvent = data_fields.at("ri_event_exp")->GetInt();
        new_rewardinfo.PointEvent = data_fields.at("ri_event_point")->GetInt();
        new_rewardinfo.ClanExpBase = data_fields.at("ri_clan_base_exp")->GetInt();
        new_rewardinfo.ClanExpBnus = data_fields.at("ri_clan_bonus_exp")->GetInt();
        mainServer->AddRewardInfoCache(new_rewardinfo.GameMode, new_rewardinfo);
    }

    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Info("CDBM::LoadCDB() - loaded (%d) reward infos in %s", mainServer->GetRewardsInfoCacheSize(), elapsed_time_str.c_str());
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{:d}", mainServer->GetRewardsInfoCacheSize());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") reward infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", elapsed_time_str.c_str());

    start_time = std::chrono::system_clock::now();
    auto gachapon_sales = BaseLib::Database->GetGachaponSalesInfo();
    mainServer->AddGachaponSaleCache(gachapon_sales);
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    BaseLib::EventLog->Debug(std::source_location::current(), fmt::color::dark_cyan, "loaded ({}) gachapon sales info in {}", gachapon_sales.size(), elapsed_time_str.c_str());
    mainServer->Setup(settings, server_settings);
    mainServer->Run();


    /* BaseLib::Database->Initialize("MegaVoltsPP", "127.0.0.1", 3307, "root", "ngiga123");
    auto started = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        auto task = BaseLib::ThreadPool->post([i]()
            {
                std::string username = "sw1ndle" + std::to_string(i);
                auto hash = Utility::Hash("god123");
                auto auth_key = Utility::GenerateAuthKey(username, "god123");
                BaseLib::Database->InsertFrontAccount(username, hash.first, hash.second, 2, auth_key);
                
            });
        task.wait();
        
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::printf("took %d ms to insert 100k accounts\n", std::chrono::duration_cast<std::chrono::milliseconds>(done - started).count());*/
    /*
    BaseLib::FrontAccount frontAccount;
    std::string username = "sw1ndle";
    Utility::ToLowercase(username);
    std::string password = "god123";
    auto started = std::chrono::high_resolution_clock::now();
    dp::thread_pool pool(std::jthread::hardware_concurrency());
    for (int i = 0; i < 1000; i++)
    {
        auto task = pool.enqueue([i, username, &frontAccount]
            {
                return BaseLib::Database->GetFrontAccount(username, &frontAccount);
           
            });
        auto AccountFound = task.get();
        pool.enqueue_detach([i, frontAccount, AccountFound]
            {
                if (AccountFound)
                {
                    //std::printf("Index:%d\n", frontAccount.Index);
                    //std::printf("Username:%s\n", frontAccount.Username.c_str());
                    //std::printf("PassHash:%s\n", frontAccount.Password.c_str());
                    //std::printf("Salt:%s\n", frontAccount.Salt.c_str());
                    //std::printf("Grade:%d\n", frontAccount.Grade);
                    //std::printf("(%d) AuthKey:%llu\n", i, frontAccount.AuthKey);
                }
                //else { std::printf("Account not found\n"); }
            });
        //
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::printf("took %d ms to check 1k accounts\n", std::chrono::duration_cast<std::chrono::milliseconds>(done - started).count());


    auto started2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++)
    {
        auto task = BaseLib::ThreadPool->post([i, username, &frontAccount]()
            {
                auto AccountFound = BaseLib::Database->GetFrontAccount(username, &frontAccount);
        BaseLib::ThreadPool->post([i, AccountFound, frontAccount]() {
            if (AccountFound)
            {
                //std::printf("Index:%d\n", frontAccount.Index);
                //std::printf("Username:%s\n", frontAccount.Username.c_str());
                //std::printf("PassHash:%s\n", frontAccount.Password.c_str());
                //std::printf("Salt:%s\n", frontAccount.Salt.c_str());
                //std::printf("Grade:%d\n", frontAccount.Grade);
                //std::printf("(%d) AuthKey:%llu\n", i, frontAccount.AuthKey);
            }
            else { std::printf("Account not found\n"); }
            });

            });
        task.wait();
    }
    auto done2 = std::chrono::high_resolution_clock::now();
    std::printf("took %d ms to check 1k accounts\n", std::chrono::duration_cast<std::chrono::milliseconds>(done2 - started2).count());
    
    */


/*
    if (BaseLib::Database->GetFrontAccount(username, &frontAccount))
    {
        std::printf("sw1ndle:god123 works: %s\n", Utility::IsPasswordValid(password, frontAccount.Password, frontAccount.Salt) ? "true" : "false");
    }
    else
    {
        
        auto hash = Utility::Hash(password);
        auto auth_key = Utility::GenerateAuthKey(username, password);
        BaseLib::Database->InsertFrontAccount(username, hash.first, hash.second, 2, auth_key);
    }

    */
    

    
    std::cin.ignore();
    return 0;
}