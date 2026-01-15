#pragma once

#include "stl_includes.h"

#include <GWCA/GameEntities/Agent.h>

bool HelperActivationConditions(const bool need_party_loaded = true);

void CancelMovement();

void AttackAgent(const GW::Agent *agent);
