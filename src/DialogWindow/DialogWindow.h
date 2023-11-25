#pragma once

#include <ToolboxUIPlugin.h>

namespace GW::Constants
{
enum class QuestID : uint32_t;
}

// namespace GW

class RawDialogs : public ToolboxUIPlugin
{
public:
    RawDialogs()
    {
        can_show_in_main_window = true;
        can_close = true;
    }

    const char *Name() const override
    {
        return "RawDialogs";
    }
    const char *Icon() const override
    {
        return ICON_FA_COMMENT_DOTS;
    }

    void Initialize(ImGuiContext *, ImGuiAllocFns, HMODULE) override;
    void SignalTerminate() override;
    void Draw(IDirect3DDevice9 *pDevice) override;
    void DrawSettings() override;
    bool HasSettings() const override
    {
        return true;
    }
    void LoadSettings(const wchar_t *folder) override;
    void SaveSettings(const wchar_t *folder) override;

private:
    static constexpr uint32_t QuestAcceptDialog(GW::Constants::QuestID quest)
    {
        return static_cast<int>(quest) << 8 | 0x800001;
    }
    static constexpr uint32_t QuestRewardDialog(GW::Constants::QuestID quest)
    {
        return static_cast<int>(quest) << 8 | 0x800007;
    }

    int fav_count = 0;
    std::vector<int> fav_index{};

    bool show_common = true;
    bool show_uwteles = true;
    bool show_favorites = true;
    bool show_custom = true;

    char customdialogbuf[11] = "";
};
