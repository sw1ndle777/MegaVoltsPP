#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <filesystem>
#include <string>
#include <random>
#include <chrono>
#include <map>

namespace Utility
{
   
    class Random {
    public:
        std::mt19937 rng;
        std::uniform_int_distribution<std::uint32_t> dist;

        Random();

        std::uint32_t Gen();
    };
    std::uint32_t GetUtcTimeNow();
    std::string GetReadableTime(std::uint32_t time, std::string time_zone);
    void PrintBytes(std::ostream& out, const char* title, const unsigned char* data, size_t dataLen, bool format = true);
}


#endif