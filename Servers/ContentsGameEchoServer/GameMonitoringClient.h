#pragma once

#include "LanClient.h"

struct MonitorValue
{
    int value_{ 0 };
    int timestamp_{ 0 };
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

class GameMonitoringClient : public LanClient
{
public:
    GameMonitoringClient();
    virtual ~GameMonitoringClient() override;

    virtual void OnConnect() override;
    virtual void OnRelease() override;
    virtual void OnMessage(ContentsCPacket* packet) override;
    virtual void OnError(int errorCode, const wchar_t* errorLog) override;

    void UpdateMonitorData(BYTE dataType, int dataValue, int timeStamp);
    void SendMonitorData();

private:
    void UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp);
    void SendMonitorValue(BYTE dataType, const MonitorValue& monitorValue);

private:
    GameServerMonitorData gameServerData_;
};