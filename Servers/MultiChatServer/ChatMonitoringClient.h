#pragma once
#include "LanClient.h"

struct MonitorValue
{
    int value_{ 0 };
    int timestamp_{ 0 };
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

class ChatMonitoringClient : public LanClient
{
public:
    ChatMonitoringClient();
    virtual ~ChatMonitoringClient();

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
    //등록을 시키는 느낌으로 갈까? 
    ChatServerMonitorData chatServerData_;
};

