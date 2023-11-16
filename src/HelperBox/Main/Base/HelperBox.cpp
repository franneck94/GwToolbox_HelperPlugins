#include <filesystem>
#include <fstream>

#include <GWCA/Context/CharContext.h>
#include <GWCA/Context/GameContext.h>
#include <GWCA/Context/PreGameContext.h>
#include <GWCA/GWCA.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/GameEntities/Party.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/MemoryMgr.h>
#include <GWCA/Managers/RenderMgr.h>
#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Managers/UIMgr.h>
#include <GWCA/Utilities/Hooker.h>
#include <GWCA/Utilities/Scanner.h>

#include <Base/HelperBox.h>
#include <Base/HelperBoxSettings.h>
#include <Base/HelperBoxTheme.h>
#include <Base/MainWindow.h>
#include "Logger.h"
#include "Utils.h"

#include <SimpleIni.h>
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <implot.h>

namespace
{
auto dllmodule = HMODULE{0};

auto OldWndProc = long{0};
auto hb_initialized = false;
auto hb_destroyed = false;
auto imgui_initialized = false;

auto drawing_world = false;
auto drawing_passes = int{0};
auto last_drawing_passes = int{0};

auto defer_close = false;

static auto gw_window_handle = HWND{0};
} // namespace

HMODULE HelperBox::GetDLLModule()
{
    return dllmodule;
}

DWORD __stdcall SafeThreadEntry(LPVOID module)
{
    dllmodule = (HMODULE)module;
    ThreadEntry(nullptr);

    return EXIT_SUCCESS;
}

DWORD __stdcall ThreadEntry(LPVOID)
{
    GW::HookBase::Initialize();
    if (!GW::Initialize())
    {
        if (MessageBoxA(0,
                        "Initialize Failed at finding all addresses, contact Developers about this.",
                        "API Error",
                        0) == IDOK)
        {
        }
        goto leave;
    }

    GW::Render::SetRenderCallback([](IDirect3DDevice9 *device) { HelperBox::Instance().Draw(device); });
    GW::Render::SetResetCallback([](IDirect3DDevice9 *) { ImGui_ImplDX9_InvalidateDeviceObjects(); });

    Log::Log("Installed dx hooks\n");
    Log::InitializeChat();
    Log::Log("Installed chat hooks\n");
    GW::HookBase::EnableHooks();
    Log::Log("Hooks Enabled!\n");
    GW::GameThread::Enqueue([]() { HelperBox::Instance().Initialize(); });

    while (!hb_destroyed)
        Sleep(100);
    while (GW::HookBase::GetInHookCount())
        Sleep(16);
    Sleep(16);

leave:
    GW::Terminate();
    FreeLibraryAndExitThread(dllmodule, EXIT_SUCCESS);
}

LRESULT CALLBACK SafeWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) noexcept
{
    __try
    {
        return WndProc(hWnd, Message, wParam, lParam);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return CallWindowProc((WNDPROC)OldWndProc, hWnd, Message, wParam, lParam);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static auto right_mouse_down = false;

    if (Message == WM_CLOSE)
    {
        HelperBox::Instance().StartSelfDestruct();
        defer_close = true;
        return 0;
    }

    if (!(!GW::PreGameContext::instance() && imgui_initialized && hb_initialized && !hb_destroyed))
        return CallWindowProc((WNDPROC)OldWndProc, hWnd, Message, wParam, lParam);

    if (Message == WM_RBUTTONUP)
        right_mouse_down = false;
    if (Message == WM_RBUTTONDOWN)
        right_mouse_down = true;
    if (Message == WM_RBUTTONDBLCLK)
        right_mouse_down = true;

    HelperBox::Instance().right_mouse_down = right_mouse_down;
    const auto skip_mouse_capture = right_mouse_down || GW::UI::GetIsWorldMapShowing();
    ImGuiIO &io = ImGui::GetIO();

    switch (Message)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        if (!skip_mouse_capture)
            io.MouseDown[0] = true;
        break;
    case WM_LBUTTONUP:
        io.MouseDown[0] = false;
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
        if (!skip_mouse_capture)
        {
            io.KeysDown[VK_MBUTTON] = true;
            io.MouseDown[2] = true;
        }
        break;
    case WM_MBUTTONUP:
        io.KeysDown[VK_MBUTTON] = false;
        io.MouseDown[2] = false;
        break;
    case WM_MOUSEWHEEL:
        if (!skip_mouse_capture)
            io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0F : -1.0F;
        break;
    case WM_MOUSEMOVE:
        if (!skip_mouse_capture)
        {
            io.MousePos.x = (float)GET_X_LPARAM(lParam);
            io.MousePos.y = (float)GET_Y_LPARAM(lParam);
        }
        break;
    case WM_XBUTTONDOWN:
        if (!skip_mouse_capture)
        {
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                io.KeysDown[VK_XBUTTON1] = true;
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
                io.KeysDown[VK_XBUTTON2] = true;
        }
        break;
    case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
            io.KeysDown[VK_XBUTTON1] = false;
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
            io.KeysDown[VK_XBUTTON2] = false;
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (wParam < 256)
            io.KeysDown[wParam] = true;
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        if (wParam < 256)
            io.KeysDown[wParam] = false;
        break;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacterUTF16((unsigned short)wParam);
        break;
    default:
        break;
    }

    auto &hb = HelperBox::Instance();
    switch (Message)
    {
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        for (auto *m : hb.GetModules())
        {
            m->WndProc(Message, wParam, lParam);
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    {
        if (io.WantCaptureMouse && !skip_mouse_capture)
            return true;
        auto captured = false;
        for (auto *m : hb.GetModules())
        {
            if (m->WndProc(Message, wParam, lParam))
                captured = true;
        }
        if (captured)
            return true;
        break;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (io.WantTextInput)
            break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_CHAR:
    case WM_SYSCHAR:
    case WM_IME_CHAR:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONUP:
    {
        if (io.WantTextInput)
            return true; // if imgui wants them, send just to imgui (above)

        // send to toolbox modules
        {
            bool captured = false;
            for (auto *m : hb.GetModules())
            {
                if (m->WndProc(Message, wParam, lParam))
                    captured = true;
            }
            if (captured)
                return true;
        }
        break;
    }
    case WM_SIZE:
    default:
    {
        if (Message >= 0xC000 && Message <= 0xFFFF)
        {
            for (auto *m : hb.GetModules())
            {
                m->WndProc(Message, wParam, lParam);
            }
        }
        break;
    }
    }

    return CallWindowProc((WNDPROC)OldWndProc, hWnd, Message, wParam, lParam);
}

void HelperBox::Initialize()
{
    if (hb_initialized || must_self_destruct)
        return;
    gw_window_handle = GW::MemoryMgr::GetGWWindowHandle();
    OldWndProc = SetWindowLongPtrW(gw_window_handle, GWL_WNDPROC, (long)SafeWndProc);
    Log::Log("Installed input event handler, oldwndproc = 0x%X\n", OldWndProc);

    const auto filename = std::wstring{L"imgui.ini"};
    const auto filepath_ws = GetPath(filename).wstring();
    imgui_inifile = std::string{filepath_ws.begin(), filepath_ws.end()};

    Log::Log("Creating HelperBox\n");
    GW::GameThread::RegisterGameThreadCallback(&Update_Entry, HelperBox::Update);
    OpenSettingsFile();

    core_modules.push_back(&HelperBoxTheme::Instance());
    core_modules.push_back(&HelperBoxSettings::Instance());
    core_modules.push_back(&MainWindow::Instance());

    for (auto module : core_modules)
    {
        module->LoadSettings(inifile);
        module->Initialize();
    }

    HelperBoxSettings::Instance().LoadModules(inifile);
    hb_initialized = true;
}

void HelperBox::OpenSettingsFile()
{
    Log::Info("Saving settings...");

    if (!inifile)
        inifile = new CSimpleIni(false, false, false);
    inifile->Reset();

    const auto filename = std::wstring{L"HelperBox.ini"};
    const auto settings_path = GetPath(filename);

    inifile->LoadFile(settings_path.wstring().data());
}

void HelperBox::LoadModuleSettings()
{
    for (auto module : modules)
        module->LoadSettings(inifile);
}

void HelperBox::SaveSettings()
{
    Log::Info("Saving settings...");
    for (auto module : modules)
        module->SaveSettings(inifile);

    if (inifile)
    {
        const auto filename = std::wstring{L"HelperBox.ini"};
        const auto inifile_path = GetPath(filename);

        if (!std::filesystem::exists(inifile_path))
        {
            std::ofstream ofs(inifile_path);
            ofs.close();
        }

        inifile->SaveFile(inifile_path.wstring().data());
    }
}

void HelperBox::FlashWindow()
{
    auto flashInfo = FLASHWINFO{0};
    flashInfo.cbSize = sizeof(FLASHWINFO);
    flashInfo.hwnd = GW::MemoryMgr::GetGWWindowHandle();
    flashInfo.dwFlags = FLASHW_TIMER | FLASHW_TRAY | FLASHW_TIMERNOFG;
    flashInfo.uCount = 0;
    flashInfo.dwTimeout = 0;
    FlashWindowEx(&flashInfo);
}

void HelperBox::Terminate()
{
    SaveSettings();

    if (inifile)
        inifile->Reset();

    GW::GameThread::RemoveGameThreadCallback(&Update_Entry);

    for (auto module : modules)
        module->Terminate();
}

void HelperBox::Draw(IDirect3DDevice9 *device)
{
    if (hb_initialized && HelperBox::Instance().must_self_destruct)
    {
        HelperBox::Instance().Terminate();
        if (imgui_initialized)
        {
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            imgui_initialized = false;
        }

        SetWindowLongPtr(gw_window_handle, GWL_WNDPROC, (long)OldWndProc);

        GW::DisableHooks();
        hb_initialized = false;
        hb_destroyed = true;
    }

    if (hb_initialized && !HelperBox::Instance().must_self_destruct && GW::Render::GetViewportWidth() > 0 &&
        GW::Render::GetViewportHeight() > 0)
    {
        if (!imgui_initialized)
        {
            ImGui::CreateContext();
            ImPlot::CreateContext();

            ImGui_ImplDX9_Init(device);
            ImGui_ImplWin32_Init(GW::MemoryMgr().GetGWWindowHandle());

            auto &io = ImGui::GetIO();
            io.MouseDrawCursor = false;
            io.IniFilename = HelperBox::Instance().imgui_inifile.c_str();

            imgui_initialized = true;
        }

        if (!GW::UI::GetIsUIDrawn())
            return;

        const auto world_map_showing = GW::UI::GetIsWorldMapShowing();

        if (GW::PreGameContext::instance() || GW::Map::GetIsInCinematic() ||
            IsIconic(GW::MemoryMgr::GetGWWindowHandle()))
            return;

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        ImGui::GetIO().KeysDown[VK_CONTROL] = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        ImGui::GetIO().KeysDown[VK_SHIFT] = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        ImGui::GetIO().KeysDown[VK_MENU] = (GetKeyState(VK_MENU) & 0x8000) != 0;

        for (auto uielement : HelperBox::Instance().uielements)
        {
            if (world_map_showing)
                continue;
            uielement->Draw();
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }


    if (hb_destroyed && defer_close)
        SendMessageW(gw_window_handle, WM_CLOSE, NULL, NULL);
}

void HelperBox::Update(GW::HookStatus *)
{
    static DWORD last_tick_count;
    if (last_tick_count == 0)
        last_tick_count = GetTickCount();

    auto &helper_box = HelperBox::Instance();
    if (!hb_initialized)
        HelperBox::Instance().Initialize();

    if (hb_initialized && imgui_initialized && !HelperBox::Instance().must_self_destruct)
    {
        const auto tick = GetTickCount();
        const auto delta = tick - last_tick_count;
        const auto delta_f = delta / 1000.f;

        HelperBox::Instance().livings_data.Update();

        for (auto module : helper_box.modules)
            module->Update(delta_f, HelperBox::Instance().livings_data);

        last_tick_count = tick;
    }
}
