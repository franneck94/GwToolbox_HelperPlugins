#pragma once

#include <cstdint>

#include <GWCA/GameEntities/Agent.h>

bool HelperActivationConditions(const bool need_party_loaded = true);

DWORD QuestAcceptDialog(const uint32_t quest);

DWORD QuestRewardDialog(const uint32_t quest);

void CancelMovement();

void AttackAgent(const GW::Agent *agent);
