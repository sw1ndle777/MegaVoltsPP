#define ASIO_STANDALONE
#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>
#include <cstdlib>

#include <chrono>
#include <BaseLib/CThreadPool.h>
#include "BaseLib/CLog.h"
#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CDatabase.h"
#include "BaseLib/CDBM.h"
#include "BaseLib/CDBData.h"

#include "NetEngine/CServer.h"
#include "CMainServer.h"
#include "BaseLib/Utility.h"
#include <monocypher.h>
#include "secure_channel.hpp"

std::ostream& outputStream = std::cout;

void GenerateSigningKeypair()
{
    using namespace Game::Anticheat;
    Utility::SecureRandomBlake2b::Generator rng;

    uint8_t seed[kKeySize];
    rng.NextBytes(seed, kKeySize);

    uint8_t secret[kSignSkSize]; // 64
    uint8_t pubkey[kKeySize];   // 32
    crypto_eddsa_key_pair(secret, pubkey, seed);
    crypto_wipe(seed, kKeySize);

    auto printArray = [](const char* name, const char* sizeConst,
                         const uint8_t* data, size_t len)
    {
        fmt::print("static const uint8_t {}[{}] = {{\n", name, sizeConst);
        for (size_t i = 0; i < len; ++i)
        {
            if (i % 8 == 0) fmt::print("    ");
            fmt::print("0x{:02x}", data[i]);
            if (i + 1 < len) fmt::print(", ");
            if (i % 8 == 7 || i + 1 == len) fmt::print("\n");
        }
        fmt::print("}};\n\n");
    };

    fmt::print("\n========== Ed25519 SIGNING KEYPAIR ==========\n\n");
    printArray("kServerSignSecret", "kSignSkSize", secret, kSignSkSize);
    printArray("kServerSignPubkey", "kKeySize", pubkey, kKeySize);
    fmt::print("==============================================\n\n");

    crypto_wipe(secret, kSignSkSize);
}

using namespace NetEngine::Packets::Main;
std::vector<uint8_t> loadFileCrossPlatform(
    std::source_location source_location,
    const std::string& relativePath)
{
    std::filesystem::path basePath;

#ifdef _WIN32
    // Keep your original behaviour on Windows
    basePath = "../cgd";
#else
    std::filesystem::path exePath = std::filesystem::canonical("/proc/self/exe");
    std::filesystem::path exeDir = exePath.parent_path();
    basePath = exeDir.parent_path() / "cgd";
#endif

    if (!std::filesystem::exists(basePath))
        std::filesystem::create_directories(basePath);

    std::filesystem::path filePath = basePath / relativePath;

    auto contents = Utility::load_file(source_location, filePath.string());

    if (contents.empty())
        BaseLib::EventLog->Debug(
            source_location,
            BaseLib::PacketDir::DEBUG,
            EOrder::NONE,
            fmt::color::red,
            "Error loading file ({}): {}",
            filePath.string(),
            "File not found");

    return contents;
}

#include <crashpad/client/crashpad_client.h>
#include <crashpad/client/crash_report_database.h>
#include <crashpad/client/settings.h>
#include <crashpad/client/crashpad_info.h>

#ifdef _WIN32
#include <Windows.h>
#endif


void init_crash_handler()
{
    const base::FilePath exe_dir(FILE_PATH_LITERAL(".."));
#ifdef _WIN32
    base::FilePath handler = exe_dir.Append(FILE_PATH_LITERAL("crash_dumps")).Append(FILE_PATH_LITERAL("crashpad_handler.exe"));
    base::FilePath db_path = exe_dir.Append(FILE_PATH_LITERAL("crash_dumps")).Append(FILE_PATH_LITERAL("main"));
    base::FilePath metrics_path = db_path.Append(FILE_PATH_LITERAL("metrics"));
#else
    base::FilePath handler = exe_dir.Append("crash_dumps").Append("crashpad_handler");
    base::FilePath db_path = exe_dir.Append("crash_dumps").Append("main");
    base::FilePath metrics_path = db_path.Append("metrics");
#endif

    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";           // Required: Crashpad setting to save crash as a minidump
    annotations["database"] = "mvo_main_crash_db";             // Required: BugSplat appName
    annotations["product"] = "mvo_main_gs"; // Required: BugSplat appName
    annotations["version"] = "1.0.0";             // Required: BugSplat appVersion

    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit"); // optional

    std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(db_path);
    if (database == NULL) return;

    crashpad::CrashpadClient *client = new crashpad::CrashpadClient();
	client->StartHandler(handler, db_path, metrics_path, "", annotations, arguments, true, false);
}
using namespace BaseLib;
int main()
{
    init_crash_handler();

#ifdef _WIN32
    HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
    Utility::GetCpuUsage(m_process_handle);
    CloseHandle(m_process_handle);
#else
    Utility::GetCpuUsage(nullptr);
#endif
    //CrashHandler::Init("../crash_dumps/MegaVoltsPP_main.dmp");
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);//enable colors
#endif
    std::srand(static_cast<uint32_t>(std::time(NULL)));

    //GenerateSigningKeypair();

    BaseLib::DefaultSettings->LoadOptions();
    const auto& server_settings = BaseLib::DefaultSettings->GetServerSettings();
    BaseLib::LogPool = std::make_unique<BS::thread_pool<BS::tp::priority>>(server_settings.main.logger_threads);
	BaseLib::DbPool = std::make_unique<BS::thread_pool<BS::tp::priority>>(server_settings.main.database_threads);

    BaseLib::EventLog->Initialize("../logs/MegaVoltsPP_main.log", false);
    //BaseLib::ThreadPool->Initialize(server_settings.main.pool_threads);
    BaseLib::Database->Initialize(server_settings.database.db_name.c_str(), server_settings.database.host.c_str(), server_settings.database.port, server_settings.database.user.c_str(), server_settings.database.password.c_str());

    Game::CMainServer* mainServer = new Game::CMainServer();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings(server_settings.main.host.c_str(), std::to_string(server_settings.main.port).c_str(), std::to_string(server_settings.main.ipc_port).c_str(), server_settings.main.debug, true, true, server_settings.main.watchguard, server_settings.main.asio_threads, server_settings.main.database_threads, server_settings.main.logger_threads);
    settings.playtime_min_seconds = server_settings.main.playtime_min_seconds;
    auto start_time = std::chrono::system_clock::now();


    std::vector<uint8_t> buffer_iteminfo = loadFileCrossPlatform(std::source_location::current(), "iteminfo.cdb");
    std::vector<uint8_t> buffer_effectinfo = loadFileCrossPlatform(std::source_location::current(), "effectinfo.cdb");
    std::vector<uint8_t> buffer_collectioninfo = loadFileCrossPlatform(std::source_location::current(), "collectioninfo.cdb");
    std::vector<uint8_t> buffer_dailymissioninfo = loadFileCrossPlatform(std::source_location::current(), "dailymissioninfo.cdb");
    std::vector<uint8_t> buffer_itemweaponsinfo = loadFileCrossPlatform(std::source_location::current(), "itemweaponsinfo.cdb");
    std::vector<uint8_t> buffer_setiteminfo = loadFileCrossPlatform(std::source_location::current(), "setiteminfo.cdb");
    std::vector<uint8_t> buffer_vendorinfo = loadFileCrossPlatform(std::source_location::current(), "vendorinfo.cdb");
    std::vector<uint8_t> buffer_upgradeinfo = loadFileCrossPlatform(std::source_location::current(), "upgradeinfo.cdb");
    std::vector<uint8_t> buffer_gachaponinfo = loadFileCrossPlatform(std::source_location::current(), "gachaponinfo.cdb");
    std::vector<uint8_t> buffer_gachaponpackageinfo = loadFileCrossPlatform(std::source_location::current(), "gachaponpackageinfo.cdb");
    std::vector<uint8_t> buffer_itempackageinfo = loadFileCrossPlatform(std::source_location::current(), "itempackageinfo.cdb");
    std::vector<uint8_t> buffer_roomoptioninfo = loadFileCrossPlatform(std::source_location::current(), "roomoptioninfo.cdb");
    std::vector<uint8_t> buffer_gradeinfo = loadFileCrossPlatform(std::source_location::current(), "gradeinfo.cdb");
    std::vector<uint8_t> buffer_rewardinfo = loadFileCrossPlatform(std::source_location::current(), "rewardinfo.cdb");

  
    CDBM iteminfo_cdb, effectinfo_cdb, collectioninfo_cdb, dailymissioninfo_cdb, itemweaponsinfo_cdb, setiteminfo_cdb, vendorinfo_cdb, upgradeinfo_cdb, gachaponinfo_cdb, gachaponpackageinfo_cdb, itempackageinfo_cdb, roomoptioninfo_cdb, gradeinfo_cdb, rewardinfo_cdb;

    iteminfo_cdb.LoadCDB(buffer_iteminfo);
    itemweaponsinfo_cdb.LoadCDB(buffer_itemweaponsinfo);
    
    auto& iteminfo_data = iteminfo_cdb.GetDataRows();
    auto& itemweaponsinfo_data = itemweaponsinfo_cdb.GetDataRows();
    for (uint32_t i = 0; i < iteminfo_data.size(); i++)
    {
        BaseLib::ItemInfo new_item_info;
        auto& data_fields = iteminfo_data[i];
        new_item_info.Id = data_fields.at("ii_id").GetInt();
        new_item_info.Name = data_fields.at("ii_name").GetString();
        //new_item_info.NameTime = data_fields.at("ii_name_time").GetString();
        //new_item_info.NameOption = data_fields.at("ii_name_option").GetString();
        new_item_info.IsNaomiUsable = data_fields.at("ii_class_a").GetBool();
        new_item_info.IsKaiUsable = data_fields.at("ii_class_b").GetBool();
        new_item_info.IsPandoraUsable = data_fields.at("ii_class_c").GetBool();
        new_item_info.IsChipUsable = data_fields.at("ii_class_d").GetBool();
        new_item_info.IsKnoxUsable = data_fields.at("ii_class_e").GetBool();
//        new_item_info.IsSimonUsable = data_fields.at("ii_class_f").GetBool();
        //new_item_info.IsAmeliaUsable = data_fields.at("ii_class_g").GetBool();
        //new_item_info.IsSharkillUsable = data_fields.at("ii_class_h").GetBool();
        //new_item_info.IsSophitiaUsable = data_fields.at("ii_class_i").GetBool();
        new_item_info.Type = data_fields.at("ii_type").GetInt();
        //new_item_info.InventoryType = data_fields.at("ii_type_inven").GetInt();
        new_item_info.IsUpgradable = data_fields.at("ii_upgradable").GetBool();
        new_item_info.LimitedTime = data_fields.at("ii_limited_time").GetInt();
        new_item_info.Durability = data_fields.at("ii_durable_value").GetInt();
        new_item_info.DurabilityFactor = data_fields.at("ii_durable_factor").GetInt();
        new_item_info.CouponPrice = data_fields.at("ii_buy_coupon").GetInt();
        new_item_info.CashPrice = data_fields.at("ii_buy_cash").GetInt();
        new_item_info.PointPrice = data_fields.at("ii_buy_point").GetInt();
        new_item_info.SellPointPrice = data_fields.at("ii_sell_point").GetInt();
        new_item_info.Stock = data_fields.at("ii_stocks").GetInt();
        new_item_info.BonusEffectId = data_fields.at("ef_effect_2").GetInt();
		Game::CItemsInfo.insert(new_item_info.Id, new_item_info);
        //mainServer->AddItemInfoCache(new_item_info.Id, new_item_info);
    }
    for (uint32_t i = 0; i < itemweaponsinfo_data.size(); i++)
    {
        BaseLib::ItemInfo new_item_info;
        auto& data_fields = itemweaponsinfo_data[i];
        new_item_info.Id = data_fields.at("ii_id").GetInt();
        //new_item_info.Name = data_fields.at("ii_name").GetString();
        //new_item_info.NameTime = data_fields.at("ii_name_time").GetString();
        //new_item_info.NameOption = data_fields.at("ii_name_option").GetString();
        new_item_info.IsNaomiUsable = data_fields.at("ii_class_a").GetBool();
        new_item_info.IsKaiUsable = data_fields.at("ii_class_b").GetBool();
        new_item_info.IsPandoraUsable = data_fields.at("ii_class_c").GetBool();
        new_item_info.IsChipUsable = data_fields.at("ii_class_d").GetBool();
        new_item_info.IsKnoxUsable = data_fields.at("ii_class_e").GetBool();
        //new_item_info.IsSimonUsable = data_fields.at("ii_class_f").GetBool();
        //new_item_info.IsAmeliaUsable = data_fields.at("ii_class_g").GetBool();
        //new_item_info.IsSharkillUsable = data_fields.at("ii_class_h").GetBool();
        //new_item_info.IsSophitiaUsable = data_fields.at("ii_class_i").GetBool();
        new_item_info.Type = data_fields.at("ii_type").GetInt();
        //new_item_info.InventoryType = data_fields.at("ii_type_inven").GetInt();
        new_item_info.IsUpgradable = data_fields.at("ii_upgradable").GetBool();
        new_item_info.LimitedTime = data_fields.at("ii_limited_time").GetInt();
        new_item_info.Durability = data_fields.at("ii_durable_value").GetInt();
		new_item_info.DurabilityFactor = data_fields.at("ii_durable_factor").GetInt();
        new_item_info.CouponPrice = data_fields.at("ii_buy_coupon").GetInt();
        new_item_info.CashPrice = data_fields.at("ii_buy_cash").GetInt();
        new_item_info.PointPrice = data_fields.at("ii_buy_point").GetInt();
        new_item_info.SellPointPrice = data_fields.at("ii_sell_point").GetInt();
        new_item_info.Stock = data_fields.at("ii_stocks").GetInt();
        new_item_info.BonusEffectId = data_fields.at("ef_effect_2").GetInt();
        Game::CItemsInfo.insert(new_item_info.Id, new_item_info);
        //mainServer->AddItemInfoCache(new_item_info.Id, new_item_info);
    }

    iteminfo_data.clear();
    iteminfo_data.shrink_to_fit();
    itemweaponsinfo_data.clear();
    itemweaponsinfo_data.shrink_to_fit();
    iteminfo_cdb.Clear();
    itemweaponsinfo_cdb.Clear();
    buffer_iteminfo.clear();
    buffer_iteminfo.shrink_to_fit();
    buffer_itemweaponsinfo.clear();
    buffer_itemweaponsinfo.shrink_to_fit();
    auto end_time = std::chrono::system_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    auto elapsed_time_str = Utility::readable_time(elapsed_time.count());
   
    DEBUGLOG(dark_cyan, "loaded ({}) items in ({})", Game::CItemsInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) items in %s", Game::CItemsInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CItemsInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") items in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
    */
	
    start_time = std::chrono::system_clock::now();
    effectinfo_cdb.LoadCDB(buffer_effectinfo);
    auto& effectinfo_data = effectinfo_cdb.GetDataRows();
    for (uint32_t i = 0; i < effectinfo_data.size(); i++)
    {
        BaseLib::EffectInfo new_effectinfo;
        auto& data_fields = effectinfo_data[i];
        new_effectinfo.id = data_fields.at("ei_id").GetInt();
        new_effectinfo.key = data_fields.at("ei_key").GetInt();
        new_effectinfo.valueA = data_fields.at("ei_valueA").GetInt();
		Game::CEffectInfo.insert(new_effectinfo.id, new_effectinfo);
        //mainServer->AddEffectInfoCache(new_effectinfo.id, new_effectinfo);
    }
    effectinfo_data.clear();
    effectinfo_data.shrink_to_fit();
    effectinfo_cdb.Clear();
    buffer_effectinfo.clear();
    buffer_effectinfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) effect info in ({})", Game::CEffectInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) effect info in %s", Game::CEffectInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CEffectInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") effect info in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/

    start_time = std::chrono::system_clock::now();
    collectioninfo_cdb.LoadCDB(buffer_collectioninfo);
    auto& collectioninfo_data = collectioninfo_cdb.GetDataRows();
    for (uint32_t i = 0; i < collectioninfo_data.size(); i++)
    {
        BaseLib::CollectionInfo new_collectioninfo;
        auto& data_fields = collectioninfo_data[i];
        new_collectioninfo.id = data_fields.at("ci_id").GetInt();
        new_collectioninfo.rewardExp = data_fields.at("ci_rewardexp").GetInt();;
        new_collectioninfo.rewardItem = data_fields.at("ci_rewarditem").GetInt();;
        new_collectioninfo.rewardPoint = data_fields.at("ci_rewardpoint").GetInt();;
        new_collectioninfo.setIndex = data_fields.at("ci_set_index").GetInt();
        new_collectioninfo.missionType = data_fields.at("ci_mission_type").GetInt();;
		Game::CCollectionInfo.insert(new_collectioninfo.id, new_collectioninfo);
        //mainServer->AddCollectionInfoCache(new_collectioninfo.id, new_collectioninfo);
    }
    collectioninfo_data.clear();
    collectioninfo_data.shrink_to_fit();
    collectioninfo_cdb.Clear();
    buffer_collectioninfo.clear();
    buffer_collectioninfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) collection info in ({})", Game::CCollectionInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) collection info in %s", Game::CCollectionInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CCollectionInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") collection info in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/
    start_time = std::chrono::system_clock::now();
    dailymissioninfo_cdb.LoadCDB(buffer_dailymissioninfo);
    auto& dailymissioninfo_data = dailymissioninfo_cdb.GetDataRows();
    for (uint32_t i = 0; i < dailymissioninfo_data.size(); i++)
    {
        BaseLib::DailyMissionInfo new_dailymissioninfo;
        auto& data_fields = dailymissioninfo_data[i];
        new_dailymissioninfo.id = data_fields.at("di_id").GetInt();
        new_dailymissioninfo.rewardExp = data_fields.at("di_rewardexp").GetInt();;
        new_dailymissioninfo.rewardItem = data_fields.at("di_rewarditem").GetInt();;
        new_dailymissioninfo.rewardPoint = data_fields.at("di_rewardpoint").GetInt();;
        new_dailymissioninfo.setIndex = data_fields.at("di_set_index").GetInt();
        new_dailymissioninfo.goal = data_fields.at("di_goal").GetInt();
		Game::CDailyMissionInfo.insert(new_dailymissioninfo.id, new_dailymissioninfo);
		Game::CDailyMissions.emplace_back(new_dailymissioninfo.id);
        //mainServer->AddDailyMissionInfoCache(new_dailymissioninfo.id, new_dailymissioninfo);
    }
    dailymissioninfo_data.clear();
    dailymissioninfo_data.shrink_to_fit();
    dailymissioninfo_cdb.Clear();
    buffer_dailymissioninfo.clear();
    buffer_dailymissioninfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) daily mission info in ({})", Game::CDailyMissionInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) daily mission info in %s", Game::CDailyMissionInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CDailyMissionInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") dailymission info in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/
    start_time = std::chrono::system_clock::now();
    setiteminfo_cdb.LoadCDB(buffer_setiteminfo);
    auto& setiteminfo_data = setiteminfo_cdb.GetDataRows();
    for (uint32_t i = 0; i < setiteminfo_data.size(); i++)
    {
        BaseLib::SetItemInfo new_setitem_info;
        auto& data_fields = setiteminfo_data[i];
        new_setitem_info.Id = data_fields.at("si_id").GetInt();
        new_setitem_info.Hair = data_fields.at("si_hair").GetInt();
        new_setitem_info.Face = data_fields.at("si_face").GetInt();
        new_setitem_info.Upper = data_fields.at("si_upper").GetInt();
        new_setitem_info.Under = data_fields.at("si_under").GetInt();
        new_setitem_info.Pants = data_fields.at("si_pants").GetInt();
        new_setitem_info.Arms = data_fields.at("si_arms").GetInt();
        new_setitem_info.Boots = data_fields.at("si_boots").GetInt();
        new_setitem_info.AccessoryA = data_fields.at("si_acce_A").GetInt();
        new_setitem_info.AccessoryB = data_fields.at("si_acce_B").GetInt();
        new_setitem_info.AccessoryC = data_fields.at("si_acce_C").GetInt();
		Game::CSetItemsInfo.insert(new_setitem_info.Id, new_setitem_info);
        //mainServer->AddSetItemInfoCache(new_setitem_info.Id, new_setitem_info);
    }
    setiteminfo_data.clear();
    setiteminfo_data.shrink_to_fit();
    setiteminfo_cdb.Clear();
    buffer_setiteminfo.clear();
    buffer_setiteminfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    DEBUGLOG(dark_cyan, "loaded ({}) set items in ({})", Game::CSetItemsInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) set items in %s", Game::CSetItemsInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CSetItemsInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") set items in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/
    start_time = std::chrono::system_clock::now();
    vendorinfo_cdb.LoadCDB(buffer_vendorinfo);
    auto& vendorinfo_data = vendorinfo_cdb.GetDataRows();
    for (uint32_t i = 0; i < vendorinfo_data.size(); i++)
    {
        BaseLib::VendorInfo new_vendorinfo;
        auto& data_fields = vendorinfo_data[i];
        new_vendorinfo.Id = data_fields.at("vi_id").GetInt();
        new_vendorinfo.Category = data_fields.at("vi_category").GetInt();
        new_vendorinfo.Type = data_fields.at("vi_type").GetInt();
        new_vendorinfo.None = data_fields.at("vi_array_none").GetInt();
        new_vendorinfo.New = data_fields.at("vi_array_new").GetInt();
        new_vendorinfo.Hit = data_fields.at("vi_array_hit").GetInt();
        new_vendorinfo.ListType = data_fields.at("vi_list_type").GetInt();
        new_vendorinfo.List01 = data_fields.at("vi_list_01").GetInt();
        new_vendorinfo.List01_a = data_fields.at("vi_list_01_a").GetInt();
        new_vendorinfo.List01_b = data_fields.at("vi_list_01_b").GetInt();
        new_vendorinfo.List01_c = data_fields.at("vi_list_01_c").GetInt();
        new_vendorinfo.List01_d = data_fields.at("vi_list_01_d").GetInt();
        new_vendorinfo.List02 = data_fields.at("vi_list_02").GetInt();
        new_vendorinfo.List02_a = data_fields.at("vi_list_02_a").GetInt();
        new_vendorinfo.List02_b = data_fields.at("vi_list_02_b").GetInt();
        new_vendorinfo.List02_c = data_fields.at("vi_list_02_c").GetInt();
        new_vendorinfo.List02_d = data_fields.at("vi_list_02_d").GetInt();
        new_vendorinfo.List03 = data_fields.at("vi_list_03").GetInt();
        new_vendorinfo.List03_a = data_fields.at("vi_list_03_a").GetInt();
        new_vendorinfo.List03_b = data_fields.at("vi_list_03_b").GetInt();
        new_vendorinfo.List03_c = data_fields.at("vi_list_03_c").GetInt();
        new_vendorinfo.List03_d = data_fields.at("vi_list_03_d").GetInt();
        new_vendorinfo.List04 = data_fields.at("vi_list_04").GetInt();
        new_vendorinfo.List04_a = data_fields.at("vi_list_04_a").GetInt();
        new_vendorinfo.List04_b = data_fields.at("vi_list_04_b").GetInt();
        new_vendorinfo.List04_c = data_fields.at("vi_list_04_c").GetInt();
        new_vendorinfo.List04_d = data_fields.at("vi_list_04_d").GetInt();


		Game::CVendorItems.emplace_back(new_vendorinfo.List01);
		Game::CVendorItems.emplace_back(new_vendorinfo.List01_a);
		Game::CVendorItems.emplace_back(new_vendorinfo.List01_b);
		Game::CVendorItems.emplace_back(new_vendorinfo.List01_c);
		Game::CVendorItems.emplace_back(new_vendorinfo.List01_d);

		Game::CVendorItems.emplace_back(new_vendorinfo.List02);
		Game::CVendorItems.emplace_back(new_vendorinfo.List02_a);
		Game::CVendorItems.emplace_back(new_vendorinfo.List02_b);
		Game::CVendorItems.emplace_back(new_vendorinfo.List02_c);
		Game::CVendorItems.emplace_back(new_vendorinfo.List02_d);

		Game::CVendorItems.emplace_back(new_vendorinfo.List03);
		Game::CVendorItems.emplace_back(new_vendorinfo.List03_a);
		Game::CVendorItems.emplace_back(new_vendorinfo.List03_b);
		Game::CVendorItems.emplace_back(new_vendorinfo.List03_c);
		Game::CVendorItems.emplace_back(new_vendorinfo.List03_d);

		Game::CVendorItems.emplace_back(new_vendorinfo.List04);
		Game::CVendorItems.emplace_back(new_vendorinfo.List04_a);
		Game::CVendorItems.emplace_back(new_vendorinfo.List04_b);
		Game::CVendorItems.emplace_back(new_vendorinfo.List04_c);
		Game::CVendorItems.emplace_back(new_vendorinfo.List04_d);

        //new_vendorinfo.IsGift = data_fields.at("vi_isgift").GetBool();
        //mainServer->AddVendorInfo(new_vendorinfo);
    }
    vendorinfo_data.clear();
    vendorinfo_data.shrink_to_fit();
    vendorinfo_cdb.Clear();
    buffer_vendorinfo.clear();
    buffer_vendorinfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) vendor items in ({})", Game::CVendorItems.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) vendor infos in %s", Game::CVendorItems.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CVendorItems.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") vendor infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/
    start_time = std::chrono::system_clock::now();
    upgradeinfo_cdb.LoadCDB(buffer_upgradeinfo);
    auto& upgradeinfo_data = upgradeinfo_cdb.GetDataRows();
    auto upgrades = Game::CUpgradesInfo.get_all(BaseLib::unique);
    using namespace NetEngine::Items::Upgrade;
    for (uint32_t i = 0; i < upgradeinfo_data.size(); i++)
    {
		using enum NetEngine::Items::Upgrade::Type;
        BaseLib::UpgradeInfo new_upgradeinfo;
        auto& data_fields = upgradeinfo_data[i];
        new_upgradeinfo.GroupId = data_fields.at("ui_group").GetInt();
        new_upgradeinfo.UpgradeType = data_fields.at("ui_type").GetInt();
        new_upgradeinfo.ItemId = data_fields.at("ui_itemid").GetInt();
        new_upgradeinfo.ItemParentId = data_fields.at("ui_parentid").GetInt();
        new_upgradeinfo.Probability = data_fields.at("ui_prob").GetInt();
        new_upgradeinfo.AddedProbability = data_fields.at("ui_added_prob").GetInt();
        new_upgradeinfo.HoldProbability = data_fields.at("ui_hold_prob").GetInt();
        new_upgradeinfo.BuyCash = data_fields.at("ui_buy_cash").GetInt();
        new_upgradeinfo.BuyPoint = data_fields.at("ui_buy_point").GetInt();
        new_upgradeinfo.UseExp = data_fields.at("ui_use_exp").GetInt();
        new_upgradeinfo.RestoreCash = data_fields.at("ui_restore_cash").GetInt();
        new_upgradeinfo.RestorePoint = data_fields.at("ui_restore_point").GetInt();
        auto upgrade_type = static_cast<Type>(new_upgradeinfo.UpgradeType);
        
        auto group_it = upgrades->find(new_upgradeinfo.GroupId);
        if (group_it == upgrades->end()) group_it = upgrades->insert({ new_upgradeinfo.GroupId, {} }).first;

        auto& inner_map = group_it->second;
        if (upgrade_type != NoUpgrade)
        {
            auto upgrade_vec_it = inner_map.find(upgrade_type);
            if (upgrade_vec_it == inner_map.end()) upgrade_vec_it = inner_map.insert({ upgrade_type, {} }).first;
            auto& upgrade_vector = upgrade_vec_it->second;
            if (upgrade_vector.empty())
            {
                auto no_upgrade_it = inner_map.find(NoUpgrade);
                if (no_upgrade_it != inner_map.end() && !no_upgrade_it->second.empty()) upgrade_vector.push_back(no_upgrade_it->second.front());
            }
            upgrade_vector.push_back(new_upgradeinfo);
        }
        else
        {
            auto no_upgrade_it = inner_map.find(NoUpgrade);
            if (no_upgrade_it == inner_map.end())  no_upgrade_it = inner_map.insert({ NoUpgrade, {} }).first;
            no_upgrade_it->second.push_back(new_upgradeinfo);
        }
    }
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

    elapsed_time_str = Utility::readable_time(elapsed_time.count());
    DEBUGLOG(dark_cyan, "loaded ({}) upgrade info in ({})", upgradeinfo_data.size(), elapsed_time_str.c_str());

    upgrades.unlock();
    upgradeinfo_data.clear();
    upgradeinfo_data.shrink_to_fit();
    upgradeinfo_cdb.Clear();
    buffer_upgradeinfo.clear();
    buffer_upgradeinfo.shrink_to_fit();

   
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) upgrade infos in %s", upgradeinfo_data.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), upgradeinfo_data.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") upgrade infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/
    start_time = std::chrono::system_clock::now();
    gachaponinfo_cdb.LoadCDB(buffer_gachaponinfo);
    gachaponpackageinfo_cdb.LoadCDB(buffer_gachaponpackageinfo);
    auto& gachaponinfo_data = gachaponinfo_cdb.GetDataRows();
    auto& gachaponpackageinfo_data = gachaponpackageinfo_cdb.GetDataRows();

    boost::unordered_flat_map<uint32_t, std::vector<BaseLib::GachaponPackageItem>> gachapon_package_items;
    for (uint32_t i = 0; i < gachaponpackageinfo_data.size(); i++)
    {
        BaseLib::GachaponPackageItem new_gachaponpackageitem;
        auto& data_fields = gachaponpackageinfo_data[i];
        new_gachaponpackageitem.Id = data_fields.at("gi_id").GetInt();
        new_gachaponpackageitem.Group = data_fields.at("gi_group").GetInt();
        new_gachaponpackageitem.InfoId = data_fields.at("gi_infoid").GetInt();
        new_gachaponpackageitem.ItemType = data_fields.at("gi_type").GetInt();
        new_gachaponpackageitem.LuckyType = data_fields.at("gi_luckytype").GetInt();
        new_gachaponpackageitem.Probability = data_fields.at("gi_prob").GetInt();
        new_gachaponpackageitem.ItemId = data_fields.at("gi_itemid").GetInt();
        gachapon_package_items[new_gachaponpackageitem.InfoId].push_back(new_gachaponpackageitem);
    }
    
    for (uint32_t i = 0; i < gachaponinfo_data.size(); i++)
    {
        BaseLib::GachaponInfo new_gachaponinfo;
        auto& data_fields = gachaponinfo_data[i];
        new_gachaponinfo.Id = data_fields.at("gi_id").GetInt();
        //new_gachaponinfo.Name = data_fields.at("gi_name").GetString();
        new_gachaponinfo.Type = data_fields.at("gi_type").GetInt();
        new_gachaponinfo.InfoId = data_fields.at("gi_infoid").GetInt();
        new_gachaponinfo.LimitedGrade = data_fields.at("gi_limited_grade").GetInt();
        new_gachaponinfo.Price = data_fields.at("gi_price").GetInt();
        new_gachaponinfo.LuckyPoint = data_fields.at("gi_luckypoint").GetInt();
        const auto& items = gachapon_package_items[new_gachaponinfo.InfoId];
        for (const auto& item : items)
            new_gachaponinfo.Gachapons[item.Group].push_back(item);

		Game::CGachaponsInfo.insert(new_gachaponinfo.Id, new_gachaponinfo);
    }
    
    gachaponinfo_data.clear();
    gachaponinfo_data.shrink_to_fit();
    gachaponinfo_cdb.Clear();
    buffer_gachaponinfo.clear();
    buffer_gachaponinfo.shrink_to_fit();
    gachaponpackageinfo_data.clear();
    gachaponpackageinfo_data.shrink_to_fit();
    gachaponpackageinfo_cdb.Clear();
    buffer_gachaponpackageinfo.clear();
    buffer_gachaponpackageinfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) gachapon info in ({})", Game::CGachaponsInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) gachapon infos in %s", Game::CGachaponsInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CGachaponsInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") gachapon infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/
    start_time = std::chrono::system_clock::now();
    itempackageinfo_cdb.LoadCDB(buffer_itempackageinfo);
    auto& itempackageinfo_data = itempackageinfo_cdb.GetDataRows();
    auto packages = Game::CPackagesInfo.get_all(BaseLib::unique);
    for (uint32_t i = 0; i < itempackageinfo_data.size(); i++)
    {
        BaseLib::PackageInfo new_packageinfo;
        auto& data_fields = itempackageinfo_data[i];
        new_packageinfo.InfoId = data_fields.at("ip_infoid").GetInt();
        new_packageinfo.GroupId = data_fields.at("ip_group").GetInt();
        new_packageinfo.ItemId = data_fields.at("ip_itemid").GetInt();
        new_packageinfo.Type = data_fields.at("ip_type").GetInt();
        new_packageinfo.Probability = data_fields.at("ip_prob").GetInt();

		auto& inner_map = (*packages)[new_packageinfo.InfoId];
		inner_map[new_packageinfo.GroupId].push_back(new_packageinfo);
    }
    packages.unlock();
   
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

	DEBUGLOG(dark_cyan, "loaded ({}) package info in ({})", itempackageinfo_data.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) package infos in %s", itempackageinfo_data.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), itempackageinfo_data.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") package infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
    */
    itempackageinfo_data.clear();
    itempackageinfo_data.shrink_to_fit();
    itempackageinfo_cdb.Clear();
    buffer_itempackageinfo.clear();
    buffer_itempackageinfo.shrink_to_fit();
	
    start_time = std::chrono::system_clock::now();
    roomoptioninfo_cdb.LoadCDB(buffer_roomoptioninfo);
    auto& roomoptioninfo_data = roomoptioninfo_cdb.GetDataRows();
    auto roomOptionsInfo = Game::CRoomOptionsInfo.get_all(BaseLib::unique);
    for (uint32_t i = 0; i < roomoptioninfo_data.size(); i++)
    {
        BaseLib::RoomOptionInfo new_roomoptioninfo;
        auto& data_fields = roomoptioninfo_data[i];
        new_roomoptioninfo.Id = data_fields.at("ro_id").GetInt();
        new_roomoptioninfo.Type = data_fields.at("ro_type").GetInt();
        new_roomoptioninfo.Name = data_fields.at("ro_text").GetString();
        new_roomoptioninfo.Data = data_fields.at("ro_data").GetInt();
        new_roomoptioninfo.Mode = data_fields.at("ro_mod").GetInt();
        new_roomoptioninfo.CombatType = data_fields.at("ro_combattype").GetInt();
        auto& inner_map = (*roomOptionsInfo)[new_roomoptioninfo.Mode];
        inner_map[new_roomoptioninfo.Type].push_back(new_roomoptioninfo);
    }
   
    roomOptionsInfo.unlock();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) room option info in ({})", Game::CRoomOptionsInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) room option infos in %s", Game::CRoomOptionsInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CRoomOptionsInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") room option infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/
    roomoptioninfo_data.clear();
    roomoptioninfo_data.shrink_to_fit();
    roomoptioninfo_cdb.Clear();
    buffer_roomoptioninfo.clear();
    buffer_roomoptioninfo.shrink_to_fit();


    start_time = std::chrono::system_clock::now();
    gradeinfo_cdb.LoadCDB(buffer_gradeinfo);
    auto& gradeinfo_data = gradeinfo_cdb.GetDataRows();

    for(uint32_t i = 0; i < gradeinfo_data.size(); i++)
    {
        BaseLib::GradeInfo new_gradeinfo;
        auto& data_fields = gradeinfo_data[i];
        new_gradeinfo.Grade = data_fields.at("gi_grade").GetInt();
        new_gradeinfo.Exp = data_fields.at("gi_exp").GetInt();
        new_gradeinfo.RewardPoint = data_fields.at("gi_reward_point").GetInt();
        new_gradeinfo.RewardItem = data_fields.at("gi_reward_item").GetInt();
		Game::CGradesInfo.insert(new_gradeinfo.Grade, new_gradeinfo);
    }
    gradeinfo_data.clear();
    gradeinfo_data.shrink_to_fit();
    gradeinfo_cdb.Clear();
    buffer_gradeinfo.clear();
    buffer_gradeinfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) grade info in ({})", Game::CGradesInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) grade infos in %s", Game::CGradesInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CGradesInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") grade infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/

    start_time = std::chrono::system_clock::now();
    rewardinfo_cdb.LoadCDB(buffer_rewardinfo);
    auto& rewardinfo_data = rewardinfo_cdb.GetDataRows();

    for (uint32_t i = 0; i < rewardinfo_data.size(); i++)
    {
        BaseLib::RewardInfo new_rewardinfo;
        auto& data_fields = rewardinfo_data[i];
        new_rewardinfo.GameMode = data_fields.at("ri_mod").GetInt();
        new_rewardinfo.ExpBase = data_fields.at("ri_exp_base").GetInt();
        new_rewardinfo.ExpMax = data_fields.at("ri_exp_max").GetInt();
        new_rewardinfo.ExpKill = data_fields.at("ri_exp_kill").GetInt();
        new_rewardinfo.ExpModeKill = data_fields.at("ri_exp_mod_kill").GetInt();
        new_rewardinfo.ExpDeath = data_fields.at("ri_exp_death").GetInt();
        new_rewardinfo.ExpAssist = data_fields.at("ri_exp_assist").GetInt();
        new_rewardinfo.ExpMission = data_fields.at("ri_exp_mission").GetInt();
        new_rewardinfo.ExpMissionWin = data_fields.at("ri_exp_mission_win").GetInt();
        new_rewardinfo.PointBase = data_fields.at("ri_poi_base").GetInt();
        new_rewardinfo.PointMax = data_fields.at("ri_poi_max").GetInt();
        new_rewardinfo.PointKill = data_fields.at("ri_poi_kill").GetInt();
        new_rewardinfo.PointModeKill = data_fields.at("ri_poi_mod_kill").GetInt();
        new_rewardinfo.PointDeath = data_fields.at("ri_poi_death").GetInt();
        new_rewardinfo.PointAssist = data_fields.at("ri_poi_assist").GetInt();
        new_rewardinfo.PointMission = data_fields.at("ri_poi_mission").GetInt();
        new_rewardinfo.PointMissionWin = data_fields.at("ri_poi_mission_win").GetInt();
        new_rewardinfo.ModeLimitedTime = data_fields.at("ri_mod_limited_time").GetInt();
        new_rewardinfo.PlayerLimitedTime = data_fields.at("ri_player_limited_time").GetInt();
        new_rewardinfo.PenaltyPoint = data_fields.at("ri_penalty_point").GetInt();
        new_rewardinfo.ExpEvent = data_fields.at("ri_event_exp").GetInt();
        new_rewardinfo.PointEvent = data_fields.at("ri_event_point").GetInt();
        new_rewardinfo.ClanExpBase = data_fields.at("ri_clan_base_exp").GetInt();
        new_rewardinfo.ClanExpBnus = data_fields.at("ri_clan_bonus_exp").GetInt();
		Game::CRewardsInfo.insert(new_rewardinfo.GameMode, new_rewardinfo);
    }
    rewardinfo_data.clear();
    rewardinfo_data.shrink_to_fit();
    rewardinfo_cdb.Clear();
    buffer_rewardinfo.clear();
    buffer_rewardinfo.shrink_to_fit();
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

    DEBUGLOG(dark_cyan, "loaded ({}) reward info in ({})", Game::CRewardsInfo.size(), elapsed_time_str.c_str());
    /*
    BaseLib::EventLog->Add("CDBM::LoadCDB() - loaded (%d) reward infos in %s", Game::CRewardsInfo.size(), elapsed_time_str.c_str());
	
    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "CDBM::LoadCDB() ");
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, "- loaded (");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{:d}"), Game::CRewardsInfo.size());
    fmt::print(fg(fmt::color::dark_cyan) | fmt::emphasis::bold, ") reward infos in ");
    fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, fmt::runtime("{}\n"), elapsed_time_str.c_str());
	*/

    start_time = std::chrono::system_clock::now();
    auto gachapon_sales = BaseLib::Database->GetGachaponSalesInfo();
    for (auto& sale : gachapon_sales)
    {
        if (Game::CGachaponSaleInfo.contains(sale.gachapon_id))
            continue;

        Game::CGachaponSaleInfo.insert(sale.gachapon_id, sale);
        Game::CGachaponSale.emplace_back(sale.gachapon_id);
    }
    end_time = std::chrono::system_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    elapsed_time_str = Utility::readable_time(elapsed_time.count());

	DEBUGLOG(dark_cyan, "loaded ({}) gachapon sale info in ({})", Game::CGachaponSaleInfo.size(), elapsed_time_str.c_str());

	Game::Anticheat::g_fileIntegrityConfig = Game::Anticheat::FileIntegrityConfig::Load();

	mainServer->Setup(settings, server_settings);
	mainServer->Run();
    std::cin.get();
    delete mainServer;
    BaseLib::DbPool.reset();
    BaseLib::LogPool.reset();
    return 0;
}
