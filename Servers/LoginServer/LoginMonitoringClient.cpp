#include "LoginMonitoringClient.h"
#include "CommonProtocol.h"

LoginMonitoringClient::LoginMonitoringClient()
{
}

LoginMonitoringClient::~LoginMonitoringClient()
{
}

void LoginMonitoringClient::OnConnect()
{
    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();
    loginPacket.packet_buffer->InitLan();

    loginPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN << (int)dfMONITOR_SERVER_TYPE_LOGIN;

    SendPacket(loginPacket);
}

void LoginMonitoringClient::OnRelease()
{



}


void LoginMonitoringClient::OnMessage(ContentsCPacket* packet)
{
    //근데 모니터링서버로부터 받을게 뭐 있나? 
    //없지 않나? 

}

void LoginMonitoringClient::OnError(int errorCode, const wchar_t* errorLog)
{
    
}

void LoginMonitoringClient::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
    monitorValue.value_ = dataValue;
    monitorValue.timestamp_ = timeStamp;
}

void LoginMonitoringClient::UpdateMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
    switch (dataType)
    {
        case dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN:
        {
            UpdateMonitorValue(loginServerData_.isRunning_, dataValue, timeStamp);
            break;
        }
        case dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:
        {
            UpdateMonitorValue(loginServerData_.cpuUsage_, dataValue, timeStamp);
            break;
        }
        case dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:
        {
            UpdateMonitorValue(loginServerData_.memoryMBytes_, dataValue, timeStamp);
            break;
        }
        case dfMONITOR_DATA_TYPE_LOGIN_SESSION:
        {
            UpdateMonitorValue(loginServerData_.sessionCount_, dataValue, timeStamp);
            break;
        }
        case dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:
        {
            UpdateMonitorValue(loginServerData_.authTps_, dataValue, timeStamp);
            break;
        }
        case dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:
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

void LoginMonitoringClient::SendMonitorValue(BYTE dataType, const MonitorValue& monitorValue)
{
    ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
    packet.packet_buffer->InitLan();
    packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    packet << dataType;
    packet << monitorValue.value_;
    packet << monitorValue.timestamp_;

    SendPacket(packet);
}

void LoginMonitoringClient::SendMonitorData()
{
    SendMonitorValue(dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, loginServerData_.isRunning_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, loginServerData_.cpuUsage_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, loginServerData_.memoryMBytes_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_LOGIN_SESSION, loginServerData_.sessionCount_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, loginServerData_.authTps_);
    SendMonitorValue(dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, loginServerData_.packetPoolUsage_);
}