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

    constexpr int PacketQueueDefaultBufferSize = 2000;
    constexpr int MessageDataQueueDefaultBufferSize = 500000;
    constexpr long LockFreeQueueMaxSize = 50000;
    constexpr int LockFreeQueueCas2MaxSize = 10000;
    constexpr int RingBufferDefaultBufferSize = 5000;

    constexpr int ProfilerMaxIndex = 40;

    constexpr int RecvIoType = 10;
    constexpr int SendIoType = 20;

    constexpr int MaxBatchSize = 300;
    constexpr int MaxPacketPack = 20;

    constexpr long ReleaseFlag = static_cast<long>(0x80000000u);

    constexpr int LanPacketType = 0;
    constexpr int NetPacketType = 1;

    constexpr int LanNetworkMaxBatchSize = 250;
    constexpr unsigned char LibraryPacketCode = 0x89;

}