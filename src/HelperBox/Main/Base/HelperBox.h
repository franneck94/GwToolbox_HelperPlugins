#pragma once

#include <string>
#include <vector>

#include <GWCA/Managers/GameThreadMgr.h>

#include <Base/HelperBoxModule.h>
#include <Base/HelperBoxUIElement.h>
#include <DataLivings.h>
#include <DataPlayer.h>

#include <SimpleIni.h>
#include <d3d9.h>

DWORD __stdcall SafeThreadEntry(LPVOID mod);
DWORD __stdcall ThreadEntry(LPVOID);

LRESULT CALLBACK SafeWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) noexcept;
LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

class HelperBox
{
    HelperBox(){};
    ~HelperBox(){};

public:
    static HelperBox &Instance()
    {
        static HelperBox instance;
        return instance;
    }

    static HMODULE GetDLLModule();
    static void Draw(IDirect3DDevice9 *device);
    static void Update(GW::HookStatus *);

    void Initialize();
    void Terminate();

    void OpenSettingsFile();
    void LoadModuleSettings();
    void SaveSettings();
    static void FlashWindow();

    void StartSelfDestruct()
    {
        SaveSettings();
        must_self_destruct = true;
    }
    bool must_self_destruct = false;

    bool RegisterModule(HelperBoxModule *m)
    {
        if (std::find(modules.begin(), modules.end(), m) == modules.end())
            return modules.push_back(m), true;
        return false;
    }

    bool RegisterUIElement(HelperBoxUIElement *e)
    {
        if (std::find(uielements.begin(), uielements.end(), e) == uielements.end())
            return uielements.push_back(e), true;
        return false;
    }

    const std::vector<HelperBoxModule *> &GetModules() const
    {
        return modules;
    }

    const std::vector<HelperBoxModule *> &GetCoreModules() const
    {
        return core_modules;
    }

    const std::vector<HelperBoxUIElement *> &GetUIElements() const
    {
        return uielements;
    }

    bool right_mouse_down = false;

    AgentLivingData livings_data = {};

private:
    std::vector<HelperBoxModule *> modules;

    std::vector<HelperBoxModule *> core_modules;
    std::vector<HelperBoxModule *> optional_modules;
    std::vector<HelperBoxUIElement *> uielements;

    std::string imgui_inifile;
    CSimpleIni *inifile = nullptr;
    GW::HookEntry Update_Entry;
};
