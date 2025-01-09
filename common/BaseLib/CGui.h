#pragma once
#include <fstream>
#include <string>
#include <memory>
#include <cstdarg>
#include <filesystem>
#include <shared_mutex>
#include <source_location>
#include <ctime>
#include <chrono>
#include <fmt/format.h>
#include <fmt/color.h>
#include <regex>
#include <source_location>
#include <format>
#include "CThreadPool.h"

#pragma comment(lib, "dwmapi.lib")
#include "Dwmapi.h"
#include <d3d9.h>
#pragma comment(lib,"d3d9.lib")
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui_internal.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
namespace BaseLib
{
    extern HWND main_hwnd;

    extern LPDIRECT3DDEVICE9        g_pd3dDevice;
    extern D3DPRESENT_PARAMETERS    g_d3dpp;
    extern LPDIRECT3D9              g_pD3D;
    class CGui
    {
    public:
        using RenderCallback = std::function<void()>;
        void Initialize(RenderCallback renderCallback, std::uint32_t WINDOW_WIDTH, std::uint32_t WINDOW_HEIGHT);
        void CleanupDeviceD3D();
        bool CreateDeviceD3D(HWND hWnd, std::uint32_t WINDOW_WIDTH, std::uint32_t WINDOW_HEIGHT);
        void Style();
    private:

        std::jthread g_thread;
    };

    extern std::unique_ptr<CGui> Gui;
}

//#endif