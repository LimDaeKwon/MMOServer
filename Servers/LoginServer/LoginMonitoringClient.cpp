#include "LoginMonitoringClient.h"
#include "MonitoringDefine.h"

LoginMonitoringClient::LoginMonitoringClient()
{
}

LoginMonitoringClient::~LoginMonitoringClient()
{
}

void LoginMonitoringClient::OnConnect()
{
    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();
    loginPacket.packetBuffer_->InitLan();

    loginPacket << PacketSsMonitorLogin << MonitorServerTypeLogin;

    SendPacket(loginPacket);
}

void LoginMonitoringClient::OnRelease()
{
}

void LoginMonitoringClient::OnMessage(ContentsCPacket* packet)
{
}

void LoginMonitoringClient::OnError(int errorCode, const wchar_t* errorLog)
{
}

void LoginMonitoringClient::UpdateMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
    switch (dataType)
    {
    case MonitorDataTypeLoginServerRun:
    {
        UpdateMonitorValue(loginServerData_.isRunning_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeLoginServerCpu:
    {
        UpdateMonitorValue(loginServerData_.cpuUsage_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeLoginServerMemory:
    {
        UpdateMonitorValue(loginServerData_.memoryMBytes_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeLoginSession:
    {
        UpdateMonitorValue(loginServerData_.sessionCount_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeLoginAuthTps:
    {
        UpdateMonitorValue(loginServerData_.authTps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeLoginPacketPool:
    {
        UpdateMonitorValue(loginServerData_.packetPoolUsage_, dataValue, timeStamp);
        break;
    }
    default:
    {
        break;
    }
    }
}

void LoginMonitoringClient::SendMonitorData()
{
    SendMonitorValue(MonitorDataTypeLoginServerRun, loginServerData_.isRunning_);
    SendMonitorValue(MonitorDataTypeLoginServerCpu, loginServerData_.cpuUsage_);
    SendMonitorValue(MonitorDataTypeLoginServerMemory, loginServerData_.memoryMBytes_);
    SendMonitorValue(MonitorDataTypeLoginSession, loginServerData_.sessionCount_);
    SendMonitorValue(MonitorDataTypeLoginAuthTps, loginServerData_.authTps_);
    SendMonitorValue(MonitorDataTypeLoginPacketPool, loginServerData_.packetPoolUsage_);
}

void LoginMonitoringClient::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
    monitorValue.value_ = dataValue;
    monitorValue.timestamp_ = timeStamp;
}

void LoginMonitoringClient::SendMonitorValue(BYTE dataType, const MonitorValue& monitorValue)
{
    ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
    packet.packetBuffer_->InitLan();

    packet << PacketSsMonitorDataUpdate;
    packet << dataType;
    packet << monitorValue.value_;
    packet << monitorValue.timestamp_;

    SendPacket(packet);
}