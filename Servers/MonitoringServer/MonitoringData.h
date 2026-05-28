#pragma once

struct MonitorValue
{
    int value_{ 0 };
    int timestamp_{ 0 };

    long long sum_{ 0 };
    int min_{ 0 };
    int max_{ 0 };
    int sampleCount_{ 0 };

};

struct LoginServerMonitorData
{
    MonitorValue isRunning_;
    MonitorValue cpuUsage_;
    MonitorValue memoryMBytes_;
    MonitorValue sessionCount_;
    MonitorValue authTps_;
    MonitorValue packetPoolUsage_;
};

struct GameServerMonitorData
{
    MonitorValue isRunning_;
    MonitorValue cpuUsage_;
    MonitorValue memoryMBytes_;
    MonitorValue sessionCount_;
    MonitorValue authPlayerCount_;
    MonitorValue gamePlayerCount_;
    MonitorValue acceptTps_;
    MonitorValue packetRecvTps_;
    MonitorValue packetSendTps_;
    MonitorValue dbWriteTps_;
    MonitorValue dbWriteMessageQueueCount_;
    MonitorValue authThreadFps_;
    MonitorValue gameThreadFps_;
    MonitorValue packetPoolUsage_;
};

struct ChatServerMonitorData
{
    MonitorValue isRunning_;
    MonitorValue cpuUsage_;
    MonitorValue memoryMBytes_;
    MonitorValue sessionCount_;
    MonitorValue playerCount_;
    MonitorValue updateTps_;
    MonitorValue packetPoolUsage_;
    MonitorValue updateMessagePoolUsage_;
};

struct SystemMonitorData
{
    MonitorValue totalCpuUsage_;
    MonitorValue nonPagedMemoryMBytes_;
    MonitorValue networkRecvKBytes_;
    MonitorValue networkSendKBytes_;
    MonitorValue availableMemoryMBytes_;
};