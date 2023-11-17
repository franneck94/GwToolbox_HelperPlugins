#pragma once

#include <cstdint>

#include <GWCA/GameEntities/Agent.h>

bool HelperActivationConditions(const bool need_party_loaded = true);

uint32_t QuestAcceptDialog(GW::Constants::QuestID quest);

uint32_t QuestRewardDialog(GW::Constants::QuestID quest);

void CancelMovement();

void AttackAgent(const GW::Agent *agent);
