#include "ChatMonitoringClient.h"
#include "MonitoringDefine.h"

ChatMonitoringClient::ChatMonitoringClient()
{
}

ChatMonitoringClient::~ChatMonitoringClient()
{
}

void ChatMonitoringClient::OnConnect()
{
    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();
    loginPacket.packetBuffer_->InitLan();

    loginPacket << PacketSsMonitorLogin << MonitorServerTypeChat;

    SendPacket(loginPacket);
}

void ChatMonitoringClient::OnRelease()
{
}

void ChatMonitoringClient::OnMessage(ContentsCPacket* packet)
{
}

void ChatMonitoringClient::OnError(int errorCode, const wchar_t* errorLog)
{
}

void ChatMonitoringClient::UpdateMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
    switch (dataType)
    {
    case MonitorDataTypeChatServerRun:
    {
        UpdateMonitorValue(chatServerData_.isRunning_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeChatServerCpu:
    {
        UpdateMonitorValue(chatServerData_.cpuUsage_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeChatServerMemory:
    {
        UpdateMonitorValue(chatServerData_.memoryMBytes_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeChatSession:
    {
        UpdateMonitorValue(chatServerData_.sessionCount_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeChatPlayer:
    {
        UpdateMonitorValue(chatServerData_.playerCount_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeChatUpdateTps:
    {
        UpdateMonitorValue(chatServerData_.updateTps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeChatPacketPool:
    {
        UpdateMonitorValue(chatServerData_.packetPoolUsage_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeChatUpdateMessagePool:
    {
        UpdateMonitorValue(chatServerData_.updateMessagePoolUsage_, dataValue, timeStamp);
        break;
    }
    default:
    {
        break;
    }
    }
}

void ChatMonitoringClient::SendMonitorData()
{
    SendMonitorValue(MonitorDataTypeChatServerRun, chatServerData_.isRunning_);
    SendMonitorValue(MonitorDataTypeChatServerCpu, chatServerData_.cpuUsage_);
    SendMonitorValue(MonitorDataTypeChatServerMemory, chatServerData_.memoryMBytes_);
    SendMonitorValue(MonitorDataTypeChatSession, chatServerData_.sessionCount_);
    SendMonitorValue(MonitorDataTypeChatPlayer, chatServerData_.playerCount_);
    SendMonitorValue(MonitorDataTypeChatUpdateTps, chatServerData_.updateTps_);
    SendMonitorValue(MonitorDataTypeChatPacketPool, chatServerData_.packetPoolUsage_);
    SendMonitorValue(MonitorDataTypeChatUpdateMessagePool, chatServerData_.updateMessagePoolUsage_);
}

void ChatMonitoringClient::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
    monitorValue.value_ = dataValue;
    monitorValue.timestamp_ = timeStamp;
}

void ChatMonitoringClient::SendMonitorValue(BYTE dataType, const MonitorValue& monitorValue)
{
    ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
    packet.packetBuffer_->InitLan();

    packet << PacketSsMonitorDataUpdate;
    packet << dataType;
    packet << monitorValue.value_;
    packet << monitorValue.timestamp_;

    SendPacket(packet);
}