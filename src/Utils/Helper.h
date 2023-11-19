#pragma once

#include <cstdint>

#include <GWCA/GameEntities/Agent.h>

bool HelperActivationConditions(const bool need_party_loaded = true);

void CancelMovement();

void AttackAgent(const GW::Agent *agent);

void SendDialog(const wchar_t *, const int argc, const LPWSTR *argv);
