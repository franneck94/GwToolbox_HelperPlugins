#pragma once

#include <cstdint>

namespace GW
{
namespace Packet
{
struct HeroUseSkill
{
    const uint32_t header = GAME_CMSG_HERO_USE_SKILL;
    uint32_t hero_id;
    uint32_t target_id;
    uint32_t skill_slot;
};
} // namespace Packet
} // namespace GW
