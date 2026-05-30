#pragma once

#include <cstdint>

namespace DKServerCore
{
    inline constexpr std::int64_t AddressMask = 0x00007fffffffffff;
    inline constexpr std::int64_t TagMask = 0xffff800000000000;
    inline constexpr int TagOffset = 47;
}