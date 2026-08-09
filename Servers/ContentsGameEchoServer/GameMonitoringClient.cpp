#include "GameMonitoringClient.h"
#include "MonitoringDefine.h"

GameMonitoringClient::GameMonitoringClient()
{
}

GameMonitoringClient::~GameMonitoringClient()
{
}

void GameMonitoringClient::OnConnect()
{
    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();
    loginPacket.packetBuffer_->InitLan();

    loginPacket << PacketSsMonitorLogin << MonitorServerTypeGame;

    SendPacket(loginPacket);
}

void GameMonitoringClient::OnRelease()
{
}

void GameMonitoringClient::OnMessage(ContentsCPacket* packet)
{
}

void GameMonitoringClient::OnError(int errorCode, const wchar_t* errorLog)
{
}

void GameMonitoringClient::UpdateMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
    switch (dataType)
    {
    case MonitorDataTypeGameServerRun:
    {
        UpdateMonitorValue(gameServerData_.isRunning_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameServerCpu:
    {
        UpdateMonitorValue(gameServerData_.cpuUsage_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameServerMemory:
    {
        UpdateMonitorValue(gameServerData_.memoryMBytes_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameSession:
    {
        UpdateMonitorValue(gameServerData_.sessionCount_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameAuthPlayer:
    {
        UpdateMonitorValue(gameServerData_.authPlayerCount_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameGamePlayer:
    {
        UpdateMonitorValue(gameServerData_.gamePlayerCount_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameAcceptTps:
    {
        UpdateMonitorValue(gameServerData_.acceptTps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGamePacketRecvTps:
    {
        UpdateMonitorValue(gameServerData_.packetRecvTps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGamePacketSendTps:
    {
        UpdateMonitorValue(gameServerData_.packetSendTps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameDbWriteTps:
    {
        UpdateMonitorValue(gameServerData_.dbWriteTps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameDbWriteMessage:
    {
        UpdateMonitorValue(gameServerData_.dbWriteMessageQueueCount_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameAuthThreadFps:
    {
        UpdateMonitorValue(gameServerData_.authThreadFps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGameGameThreadFps:
    {
        UpdateMonitorValue(gameServerData_.gameThreadFps_, dataValue, timeStamp);
        break;
    }
    case MonitorDataTypeGamePacketPool:
    {
        UpdateMonitorValue(gameServerData_.packetPoolUsage_, dataValue, timeStamp);
        break;
    }
    default:
    {
        break;
    }
    }
}

void GameMonitoringClient::SendMonitorData()
{
    SendMonitorValue(MonitorDataTypeGameServerRun, gameServerData_.isRunning_);
    SendMonitorValue(MonitorDataTypeGameServerCpu, gameServerData_.cpuUsage_);
    SendMonitorValue(MonitorDataTypeGameServerMemory, gameServerData_.memoryMBytes_);
    SendMonitorValue(MonitorDataTypeGameSession, gameServerData_.sessionCount_);
    SendMonitorValue(MonitorDataTypeGameAuthPlayer, gameServerData_.authPlayerCount_);
    SendMonitorValue(MonitorDataTypeGameGamePlayer, gameServerData_.gamePlayerCount_);
    SendMonitorValue(MonitorDataTypeGameAcceptTps, gameServerData_.acceptTps_);
    SendMonitorValue(MonitorDataTypeGamePacketRecvTps, gameServerData_.packetRecvTps_);
    SendMonitorValue(MonitorDataTypeGamePacketSendTps, gameServerData_.packetSendTps_);
    SendMonitorValue(MonitorDataTypeGameDbWriteTps, gameServerData_.dbWriteTps_);
    SendMonitorValue(MonitorDataTypeGameDbWriteMessage, gameServerData_.dbWriteMessageQueueCount_);
    SendMonitorValue(MonitorDataTypeGameAuthThreadFps, gameServerData_.authThreadFps_);
    SendMonitorValue(MonitorDataTypeGameGameThreadFps, gameServerData_.gameThreadFps_);
    SendMonitorValue(MonitorDataTypeGamePacketPool, gameServerData_.packetPoolUsage_);
}

void GameMonitoringClient::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
    monitorValue.value_ = dataValue;
    monitorValue.timestamp_ = timeStamp;
}

void GameMonitoringClient::SendMonitorValue(BYTE dataType, const MonitorValue& monitorValue)
{
    ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
    packet.packetBuffer_->InitLan();

    packet << PacketSsMonitorDataUpdate;
    packet << dataType;
    packet << monitorValue.value_;
    packet << monitorValue.timestamp_;

    SendPacket(packet);
}