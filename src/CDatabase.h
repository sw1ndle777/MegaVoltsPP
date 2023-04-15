#pragma once
#ifndef CDATABASE_H
#define CDATABASE_H

#pragma comment(lib, "mariadb/lib/mariadbcpp.lib")
#include <conncpp.hpp>
#include <iostream>
#include "CLog.h"
#include "CThreadPool.h"
#include "Utility.h"
namespace BaseLib
{
    struct FrontAccount
    {
        std::uint32_t Index;
        std::string Username;
        std::string Password;
        std::string Salt;
        std::uint8_t Grade;
        std::uint64_t AuthKey;
        FrontAccount()
        {
            this->Index = -1;
            this->Username = "";
            this->Password = "";
            this->Salt = "";
            this->Grade = -1;
            this->AuthKey = -1;
        }
        FrontAccount(const std::uint32_t& index, const std::string& username, const std::string& password, const std::string& salt, const std::uint8_t& grade, const std::uint64_t& auth_key)
        {
            this->Index = index;
            this->Username = username;
            this->Password = password;
            this->Salt = salt;
            this->Grade = grade;
            this->AuthKey = auth_key;
        }
    };
    class CDatabase
    {

    public:
        void Initialize(const std::string& database, const std::string& host, const std::uint16_t& port, const std::string& user, const std::string& password);
        bool CreateTable(const std::string& table_name, const std::string& data_collumns);
        bool CreateDatabase(const std::string& name);
        bool RegisterAccount(const std::string& username, const std::string& password, const std::uint8_t& grade, const std::uint32_t& mp, const std::uint32_t& rt, const std::uint32_t& coupons = 0, const std::uint32_t& coins = 0, const std::uint32_t& energy = 0, const std::uint32_t& max_items = 1000, const std::uint32_t& max_battery = 5000, const std::string& nickname = "");
        bool InsertFrontAccount(const std::string& username, const std::string& password, const std::string& salt, std::uint8_t grade, std::uint64_t auth_key);
        bool InsertPlayers(const std::string& nickname, const std::uint32_t& level, const std::uint32_t& experience, const bool& tutorial, const std::uint32_t& story, const std::uint32_t& vip_experience, const std::uint32_t& max_items, const std::uint32_t& max_energy, const std::uint32_t& selected_character, std::uint32_t& outPlayerID);
        bool GetFrontAccount(const std::string& username, FrontAccount* outFrontAccount);
        std::future<bool> GetFrontAccountAsync(const std::string& username, FrontAccount* outFrontAccount);
        std::string GetDatabaseName();
        CDatabase() {};
        ~CDatabase() {};

    private:
        std::string database_name;
        sql::Driver* driver = nullptr;
        sql::Connection* conn = nullptr;
    };

    extern std::unique_ptr<CDatabase> Database;
}

#endif