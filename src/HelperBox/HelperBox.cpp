
#include <cstdint>
#include <filesystem>

#include <GWCA/GWCA.h>

#include "HelperBox.h"

#include <imgui.h>


DLLAPI ToolboxPlugin *ToolboxPluginInstance()
{
    static HelperBox instance;
    return &instance;
}

void HelperBox::Initialize(ImGuiContext *ctx, const ImGuiAllocFns fns, const HMODULE toolbox_dll)
{
    ToolboxUIPlugin::Initialize(ctx, fns, toolbox_dll);
}

void HelperBox::SignalTerminate()
{
    ToolboxUIPlugin::SignalTerminate();
    GW::DisableHooks();
}

void HelperBox::Draw(IDirect3DDevice9 *)
{
}

void HelperBox::DrawSettings()
{
    ToolboxUIPlugin::DrawSettings();
}

void HelperBox::LoadSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::LoadSettings(folder);
    ini.LoadFile(GetSettingFile(folder).c_str());
}

void HelperBox::SaveSettings(const wchar_t *folder)
{
    ToolboxUIPlugin::SaveSettings(folder);

    PLUGIN_ASSERT(ini.SaveFile(GetSettingFile(folder).c_str()) == SI_OK);
}
