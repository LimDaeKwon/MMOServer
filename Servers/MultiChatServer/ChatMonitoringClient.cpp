#include "ChatMonitoringClient.h"
#include "CommonProtocol.h"

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

    loginPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN << (int)dfMONITOR_SERVER_TYPE_CHAT;

    SendPacket(loginPacket);
}

void ChatMonitoringClient::OnRelease()
{



}


void ChatMonitoringClient::OnMessage(ContentsCPacket* packet)
{
    //근데 모니터링서버로부터 받을게 뭐 있나? 
    //없지 않나? 

}

void ChatMonitoringClient::OnError(int errorCode, const wchar_t* errorLog)
{
    
}

void ChatMonitoringClient::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
    monitorValue.value_ = dataValue;
    monitorValue.timestamp_ = timeStamp;
}

void ChatMonitoringClient::UpdateMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
    switch (dataType)
    {
    case dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN:
    {
        UpdateMonitorValue(chatServerData_.isRunning_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:
    {
        UpdateMonitorValue(chatServerData_.cpuUsage_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:
    {
        UpdateMonitorValue(chatServerData_.memoryMBytes_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_CHAT_SESSION:
    {
        UpdateMonitorValue(chatServerData_.sessionCount_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_CHAT_PLAYER:
    {
        UpdateMonitorValue(chatServerData_.playerCount_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:
    {
        UpdateMonitorValue(chatServerData_.updateTps_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:
    {
        UpdateMonitorValue(chatServerData_.packetPoolUsage_, dataValue, timeStamp);
        break;
    }
    case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:
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

void ChatMonitoringClient::SendMonitorValue(BYTE dataType, const MonitorValue& monitorValue)
{
    ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
    packet.packetBuffer_->InitLan();
    packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    packet << dataType;
    packet << monitorValue.value_;
    packet << monitorValue.timestamp_;

    SendPacket(packet);
}

void ChatMonitoringClient::SendMonitorData()
{
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, chatServerData_.isRunning_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, chatServerData_.cpuUsage_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, chatServerData_.memoryMBytes_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_SESSION, chatServerData_.sessionCount_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_PLAYER, chatServerData_.playerCount_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, chatServerData_.updateTps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, chatServerData_.packetPoolUsage_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, chatServerData_.updateMessagePoolUsage_);
}