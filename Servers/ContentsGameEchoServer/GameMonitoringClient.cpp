#include "GameMonitoringClient.h"
#include "CommonProtocol.h"

GameMonitoringClient::GameMonitoringClient()
{
}

GameMonitoringClient::~GameMonitoringClient()
{
}

void GameMonitoringClient::OnConnect()
{
    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();
    loginPacket.packet_buffer->InitLan();

    loginPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN << (int)dfMONITOR_SERVER_TYPE_GAME;

    SendPacket(loginPacket);
}

void GameMonitoringClient::OnRelease()
{



}


void GameMonitoringClient::OnMessage(ContentsCPacket* packet)
{
    //근데 모니터링서버로부터 받을게 뭐 있나? 
    //없지 않나? 

}

void GameMonitoringClient::OnError(int errorCode, const wchar_t* errorLog)
{
    
}

void GameMonitoringClient::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
    monitorValue.value_ = dataValue;
    monitorValue.timestamp_ = timeStamp;
}

void GameMonitoringClient::UpdateMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
    switch (dataType)
    {
    case dfMONITOR_DATA_TYPE_GAME_SERVER_RUN:
    {
        UpdateMonitorValue(gameServerData_.isRunning_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:
    {
        UpdateMonitorValue(gameServerData_.cpuUsage_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:
    {
        UpdateMonitorValue(gameServerData_.memoryMBytes_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_SESSION:
    {
        UpdateMonitorValue(gameServerData_.sessionCount_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:
    {
        UpdateMonitorValue(gameServerData_.authPlayerCount_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:
    {
        UpdateMonitorValue(gameServerData_.gamePlayerCount_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:
    {
        UpdateMonitorValue(gameServerData_.acceptTps_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:
    {
        UpdateMonitorValue(gameServerData_.packetRecvTps_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:
    {
        UpdateMonitorValue(gameServerData_.packetSendTps_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:
    {
        UpdateMonitorValue(gameServerData_.dbWriteTps_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:
    {
        UpdateMonitorValue(gameServerData_.dbWriteMessageQueueCount_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:
    {
        UpdateMonitorValue(gameServerData_.authThreadFps_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:
    {
        UpdateMonitorValue(gameServerData_.gameThreadFps_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:
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

void GameMonitoringClient::SendMonitorValue(BYTE dataType, const MonitorValue& monitorValue)
{
    ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
    packet.packet_buffer->InitLan();
    packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    packet << dataType;
    packet << monitorValue.value_;
    packet << monitorValue.timestamp_;

    SendPacket(packet);
}

void GameMonitoringClient::SendMonitorData()
{
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, gameServerData_.isRunning_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, gameServerData_.cpuUsage_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, gameServerData_.memoryMBytes_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_SESSION, gameServerData_.sessionCount_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, gameServerData_.authPlayerCount_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, gameServerData_.gamePlayerCount_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, gameServerData_.acceptTps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, gameServerData_.packetRecvTps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, gameServerData_.packetSendTps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, gameServerData_.dbWriteTps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, gameServerData_.dbWriteMessageQueueCount_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, gameServerData_.authThreadFps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, gameServerData_.gameThreadFps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, gameServerData_.packetPoolUsage_);
}