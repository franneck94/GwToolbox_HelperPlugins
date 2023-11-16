#include <cmath>
#include <vector>

#include <GWCA/GameContainers/GamePos.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/GameEntities/Camera.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/CameraMgr.h>

#include "UtilsMath.h"

bool FloatCompare(const float a, const float b, const float epsilon)
{
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool GamePosCompare(const GW::GamePos &p1, const GW::GamePos &p2, const float epsilon)
{
    return (FloatCompare(p1.x, p2.x, epsilon) && FloatCompare(p1.y, p2.y, epsilon));
}

GW::GamePos MovePointAlongVector(const GW::GamePos &pos1, const GW::GamePos &pos2, const float move_amount)
{
    const auto dist = GW::GetNorm(GW::Vec2f{pos1.x - pos2.x, pos1.y - pos2.y});
    const auto d_t = dist + move_amount;
    const auto t = d_t / dist;

    const auto p_x = ((1.0F - t) * pos2.x + t * pos1.x);
    const auto p_y = ((1.0F - t) * pos2.y + t * pos1.y);

    return GW::GamePos{p_x, p_y, 0};
}

GameRectangle::GameRectangle(const GW::GamePos &p1, const GW::GamePos &p2, const float offset)
{
    const auto adj_p1 = MovePointAlongVector(p1, p2, offset * 0.00F); // Behind player_pos
    const auto adj_p2 = MovePointAlongVector(p2, p1, offset * 1.20F); // In front of Target

    const auto delta_x = adj_p1.x - adj_p2.x;
    const auto delta_y = adj_p1.y - adj_p2.y;
    const auto dist = GW::GetDistance(adj_p1, adj_p2);

    const auto half_offset = offset * 1.05F; // To the side

    v1 = GW::GamePos{adj_p1.x + ((-delta_y) / dist) * half_offset, adj_p1.y + (delta_x / dist) * half_offset, 0};
    v2 = GW::GamePos{adj_p1.x + ((-delta_y) / dist) * (-half_offset), adj_p1.y + (delta_x / dist) * (-half_offset), 0};
    v3 = GW::GamePos{adj_p2.x - (delta_y / dist) * half_offset, adj_p2.y + (delta_x / dist) * half_offset, 0};
    v4 = GW::GamePos{adj_p2.x + ((-delta_y) / dist) * (-half_offset), adj_p2.y + (delta_x / dist) * (-half_offset), 0};
}

float GameRectangle::Sign(const GW::GamePos &p1, const GW::GamePos &p2, const GW::GamePos &p3)
{
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool GameRectangle::PointInGameRectangle(const GW::GamePos &pt) const
{
    return PointInTriangle(pt, v1, v2, v3) || PointInTriangle(pt, v4, v2, v3);
}

bool GameRectangle::PointInTriangle(const GW::GamePos &pt,
                                    const GW::GamePos &v1,
                                    const GW::GamePos &v2,
                                    const GW::GamePos &v3)
{
    const auto b1 = Sign(pt, v1, v2) < 0.0F;
    const auto b2 = Sign(pt, v2, v3) < 0.0F;
    const auto b3 = Sign(pt, v3, v1) < 0.0F;

    return ((b1 == b2) && (b2 == b3));
}

GW::GamePos RotatePoint(const GW::GamePos &player_pos, GW::GamePos pos, const float theta, const bool swap)
{
    GW::GamePos v(pos.x, pos.y, 0);

    if (swap)
    {
        v.x = pos.x - player_pos.x;
        v.y = player_pos.y - pos.y;
    }

    const auto x1 = v.x * std::cos(theta) - v.y * std::sin(theta);
    const auto y1 = v.x * std::sin(theta) + v.y * std::cos(theta);
    v = GW::GamePos(x1, y1, 0);

    return v;
}

bool IsNearToGamePos(const GW::GamePos &player_pos, const GW::GamePos &pos, const float r)
{
    return GW::GetDistance(player_pos, pos) < r;
}

std::vector<GW::AgentLiving *> GetEnemiesInGameRectangle(const GameRectangle &rectangle,
                                                         const std::vector<GW::AgentLiving *> &living_agents)
{
    auto filtered_livings = std::vector<GW::AgentLiving *>{};

    for (const auto living : living_agents)
    {
        if (rectangle.PointInGameRectangle(living->pos))
            filtered_livings.push_back(living);
    }

    return filtered_livings;
}

std::pair<float, float> GetLineBasedOnPointAndAngle(const GW::GamePos &player_pos, const float theta)
{
    const auto orth_point = RotatePoint(player_pos, player_pos, theta + static_cast<float>(M_PI_2), false);
    const auto point2 = GW::GamePos{player_pos.x + orth_point.x, player_pos.x + orth_point.y, 0};

    const auto m = (point2.y - player_pos.y) / (point2.x - player_pos.x + FLT_EPSILON);
    const auto b = player_pos.y + m * player_pos.x;

    return std::make_pair(m, b);
}

bool PointIsBelowLine(const float slope, const float intercept, const GW::GamePos &point)
{
    const auto y_l = slope * point.x + intercept;
    const auto y_p = point.y;

    const auto y_delta = y_l - y_p;

    if (y_delta > 0.0F && slope > 0.0F)
        return true;
    if (y_delta > 0.0F && slope < 0.0F)
        return true;
    if (y_delta < 0.0F && slope > 0.0F)
        return false;
    if (y_delta < 0.0F && slope < 0.0F)
        return false;

    return false;
}
