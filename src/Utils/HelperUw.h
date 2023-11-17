#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/PartyMgr.h>

#include "ActionsBase.h"
#include "ActionsMove.h"
#include "ActionsUw.h"
#include "DataPlayer.h"
#include "Logger.h"
#include "UtilsMath.h"
#include "ActionsUw.h"

bool UwHelperActivationConditions(const bool need_party_loaded = true);

uint32_t GetTankId();

uint32_t GetEmoId();

uint32_t GetDhuumBitchId();

bool IsEmo(const DataPlayer &player_data);

bool IsDhuumBitch(const DataPlayer &player_data);

bool IsUwMesmer(const DataPlayer &player_data);

bool IsSpiker(const DataPlayer &player_data);

bool IsLT(const DataPlayer &player_data);

bool IsRangerTerra(const DataPlayer &player_data);

bool IsMesmerTerra(const DataPlayer &player_data);

const GW::Agent *GetDhuumAgent();

bool IsInDhuumFight(const GW::GamePos &player_pos);

void GetDhuumAgentData(const GW::Agent *dhuum_agent, float &dhuum_hp, uint32_t &dhuum_max_hp);

bool TankIsFullteamLT();

bool TankIsSoloLT();

bool TargetIsReaper(DataPlayer &player_data);

bool TargetReaper(DataPlayer &player_data, const std::vector<GW::AgentLiving *> &npcs);

bool TalkReaper(DataPlayer &player_data, const std::vector<GW::AgentLiving *> &npcs);

bool TargetClosestKeeper(DataPlayer &player_data, const std::vector<GW::AgentLiving *> enemies);

bool TakeChamber();

bool AcceptChamber();

bool TakeRestore();

bool TakeEscort();

bool TakeUWG();

bool TakePits();

bool TakePlanes();

template <uint32_t N>
void UpdateUwInfo(const std::map<std::string, uint32_t> &reaper_moves,
                  const DataPlayer &player_data,
                  const std::array<MoveABC *, N> &moves,
                  uint32_t &move_idx,
                  const bool first_call,
                  bool &move_ongoing)
{
    static auto last_pos = player_data.pos;

    if (move_idx >= moves.size() - 1)
        return;

    const auto curr_pos = player_data.pos;
    const auto port_detected = GW::GetDistance(last_pos, curr_pos) > GW::Constants::Range::Compass;
    const auto is_spawn = GW::GetDistance(GW::GamePos{1248.00F, 6965.51F, 0}, curr_pos) < GW::Constants::Range::Compass;

    const auto curr_move = moves[move_idx]->pos;
    const auto curr_move_oob = GW::GetDistance(curr_move, curr_pos) > GW::Constants::Range::Compass;

    const auto next_move = moves[move_idx + 1]->pos;
    const auto next_move_oob = GW::GetDistance(next_move, curr_pos) > GW::Constants::Range::Compass;

    if ((port_detected && next_move_oob) || (first_call && curr_move_oob && !is_spawn))
    {
        const auto lab_reaper = GW::GamePos{-5751.45F, 12746.52F, 0};
        const auto pits_reaper = GW::GamePos{8685.21F, 6344.59F, 0};
        const auto planes_reaper = GW::GamePos{11368.55F, -17974.64F, 0};
        const auto wastes_reaper = GW::GamePos{-235.05F, 18496.461F, 0};

        const auto ported_to_lab = GW::GetDistance(lab_reaper, player_data.pos) < 2000.0F;
        const auto ported_to_pits = GW::GetDistance(pits_reaper, player_data.pos) < 2000.0F;
        const auto ported_to_planes = GW::GetDistance(planes_reaper, player_data.pos) < 2000.0F;
        const auto ported_to_wastes = GW::GetDistance(wastes_reaper, player_data.pos) < 2000.0F;

        if (TankIsFullteamLT() && !ported_to_wastes)
        {
            move_ongoing = false;
            last_pos = curr_pos;
            return;
        }

        if (ported_to_lab && reaper_moves.contains("Lab"))
        {
            Log::Info("Ported to Lab!");
            move_idx = reaper_moves.at("Lab");
        }
        else if (ported_to_pits && reaper_moves.contains("Pits"))
        {
            Log::Info("Ported to Pits!");
            move_idx = reaper_moves.at("Pits");
        }
        else if (ported_to_planes && reaper_moves.contains("Planes"))
        {
            Log::Info("Ported to Planes!");
            move_idx = reaper_moves.at("Planes");
        }
        else if (ported_to_wastes && reaper_moves.contains("Wastes"))
        {
            Log::Info("Ported to Wastes!");
            move_idx = reaper_moves.at("Wastes");
        }

        move_ongoing = false;
    }
    else if (port_detected && !next_move_oob)
    {
        ++move_idx;
    }
    last_pos = curr_pos;
}

bool FoundKeeperAtPos(const std::vector<GW::AgentLiving *> &keeper_livings, const GW::GamePos &keeper_pos);

bool DhuumIsCastingJudgement(const GW::Agent *dhuum_agent);

bool CheckForAggroFree(const DataPlayer &player_data, const AgentLivingData *livings_data, const GW::GamePos &next_pos);

float GetProgressValue();

bool DhuumFightDone(const uint32_t num_objectives);

uint32_t GetUwTriggerRoleId(const TriggerRole role);

bool TargetTrigger(DataPlayer &player_data, const TriggerRole role);

bool LtIsBonded();
