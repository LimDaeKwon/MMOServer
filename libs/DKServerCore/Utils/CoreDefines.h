#pragma once

#include <cstdint>

namespace DKServerCore
{
    constexpr std::int64_t AddressMask = 0x00007fffffffffff;
    constexpr std::int64_t TagMask = 0xffff800000000000;
    constexpr int TagOffset = 47;

    constexpr int PacketDefaultBufferSize = 500;
    constexpr int PacketLibHeaderSize = 5;
    constexpr int PacketServerKey = 0x32;
}