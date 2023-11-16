#pragma once

#include <cstdint>

#include <GWCA/GameEntities/Agent.h>

#include "ActionsBase.h"
#include <Base/HelperBoxWindow.h>
#include "DataLivings.h"
#include "DataPlayer.h"

#include <SimpleIni.h>
#include <imgui.h>

class MimicWindowAction : public ActionABC
{
public:
    MimicWindowAction(DataPlayer *p) : ActionABC(p, "Mimic")
    {
    }

    RoutineState Routine() override;
    void Update() override;

    const AgentLivingData *_livings_data;
};

class MimicWindow : public HelperBoxWindow
{
public:
    MimicWindow() : player_data({}), mimic(&player_data){};
    ~MimicWindow(){};

    static MimicWindow &Instance()
    {
        static MimicWindow instance;
        return instance;
    }

    const char *Name() const override
    {
        return "MimicWindow";
    }

    void Draw() override;
    void Update(float delta, const AgentLivingData &_livings_data) override;

private:
    DataPlayer player_data;
    MimicWindowAction mimic;
};
