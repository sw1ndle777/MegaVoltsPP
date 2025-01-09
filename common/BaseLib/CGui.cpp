#include "CLog.h"
#include <format>
#include "CGui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace BaseLib
{

    HWND main_hwnd;

    LPDIRECT3DDEVICE9        g_pd3dDevice;
    D3DPRESENT_PARAMETERS    g_d3dpp;
    LPDIRECT3D9              g_pD3D;

    
    static void ResetDevice()
    {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
        if (hr == D3DERR_INVALIDCALL)
            IM_ASSERT(0);
        ImGui_ImplDX9_CreateDeviceObjects();
    }
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
            case WM_SIZE:
                if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
                {
                    g_d3dpp.BackBufferWidth = LOWORD(lParam);
                    g_d3dpp.BackBufferHeight = HIWORD(lParam);
                    ResetDevice();
                }
                return 0;
            case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                    return 0;
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    void CGui::CleanupDeviceD3D()
    {
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
        if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
    }
    bool CGui::CreateDeviceD3D(HWND hWnd, std::uint32_t WINDOW_WIDTH, std::uint32_t WINDOW_HEIGHT)
    {
        if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
            return false;

        // Create the D3DDevice
        ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
        g_d3dpp.Windowed = TRUE;
        g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        g_d3dpp.hDeviceWindow = hWnd;
        g_d3dpp.MultiSampleQuality = D3DMULTISAMPLE_NONE;
        g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
        g_d3dpp.BackBufferWidth = WINDOW_WIDTH;
        g_d3dpp.BackBufferHeight = WINDOW_HEIGHT;
        g_d3dpp.EnableAutoDepthStencil = TRUE;
        g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
        g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
        if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
            return false;
        return true;
    }
    void CGui::Style()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_WindowBg] = ImColor(18, 13, 26, 255);
        style.Colors[ImGuiCol_TitleBg] = ImColor(59, 59, 59, 255);
        style.Colors[ImGuiCol_TitleBgActive] = ImColor(59, 59, 59, 255);
        style.Colors[ImGuiCol_FrameBg] = ImColor(16, 12, 21, 255);
        style.Colors[ImGuiCol_Button] = ImColor(98, 98, 98, 102);
        style.Colors[ImGuiCol_ButtonHovered] = ImColor(98, 98, 98, 255);
        style.Colors[ImGuiCol_ButtonActive] = ImColor(81, 81, 81, 255);
        style.WindowBorderSize = 0.0f;
        style.WindowRounding = 0.0;
    }
   
    void CGui::Initialize(RenderCallback renderCallback, std::uint32_t WINDOW_WIDTH, std::uint32_t WINDOW_HEIGHT)
    {
        g_thread = std::jthread([this, renderCallback, WINDOW_WIDTH, WINDOW_HEIGHT]()
        {
            WNDCLASSEXW wc;
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_CLASSDC;
            wc.lpfnWndProc = WndProc;
            wc.cbClsExtra = NULL;
            wc.cbWndExtra = NULL;
            wc.hInstance = nullptr;
            wc.hIcon = LoadIcon(0, IDI_APPLICATION);
            wc.hCursor = LoadCursor(0, IDC_ARROW);
            wc.hbrBackground = nullptr;
            wc.lpszMenuName = L"megavolts";
            wc.lpszClassName = L"megavolts";
            wc.hIconSm = LoadIcon(0, IDI_APPLICATION);

            RegisterClassExW(&wc);
            main_hwnd = CreateWindowExW(NULL, wc.lpszClassName, L"megavolts", WS_POPUP | WS_EX_TRANSPARENT, (GetSystemMetrics(SM_CXSCREEN) / 2) - (WINDOW_WIDTH / 2), (GetSystemMetrics(SM_CYSCREEN) / 2) - (WINDOW_HEIGHT / 2), WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 0, 0);
            if (!CreateDeviceD3D(main_hwnd, WINDOW_WIDTH, WINDOW_HEIGHT))
            {
                CleanupDeviceD3D();
                UnregisterClassW(wc.lpszClassName, wc.hInstance);
                return;
            }
            ShowWindow(main_hwnd, SW_SHOW);
            MARGINS Margin = { -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(main_hwnd, &Margin);

            UpdateWindow(main_hwnd);

            ImGui::CreateContext();
            ImGui::StyleColorsDark();

            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;

            ImGui_ImplWin32_Init(main_hwnd);
            ImGui_ImplDX9_Init(g_pd3dDevice);
            Style();

            MSG msg;
            ZeroMemory(&msg, sizeof(msg));

            while (msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    continue;
                }

                ImGui_ImplDX9_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
                if (renderCallback) renderCallback(); //call render callback
                ImGui::EndFrame();

                g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
                g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
                g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
                if (g_pd3dDevice->BeginScene() >= 0)
                {
                    ImGui::Render();
                    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                    g_pd3dDevice->EndScene();
                }

                HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
                if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) ResetDevice();
            }

            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            CleanupDeviceD3D();
            DestroyWindow(main_hwnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
        });
    }
    std::unique_ptr<CGui> Gui = std::make_unique<CGui>();
    
}