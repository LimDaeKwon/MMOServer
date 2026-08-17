#include "LoginServer.h"
#include "DKParser.h"
#include "GameDefine.h"
#include "MonitoringDefine.h"
#include "PacketDefine.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

LoginServer::LoginServer()
{
    tlsDbConnectionIndex_ = TlsAlloc();

    if (tlsDbConnectionIndex_ == TLS_OUT_OF_INDEXES)
    {
        DebugBreak();
    }

    InitializeSRWLock(&connectionLock_);

    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);

    connection_ = new cpp_redis::client;
}

LoginServer::~LoginServer()
{
}

bool LoginServer::Start(const DKServerCore::IocpServerStartConfig& config, DKParser& parser)
{
    if (!StartMonitoringClient(parser))
    {
        return false;
    }

    if (!ConnectRedis(parser))
    {
        return false;
    }

    if (!SetMySqlConfig(parser))
    {
        return false;
    }

    if (!SetTargetServerConfig(parser))
    {
        return false;
    }

    return NetLibrary::Start(config);
}

bool LoginServer::StartMonitoringClient(DKParser& parser)
{
    std::string monitoringIp;
    unsigned int monitoringPort;
    unsigned int monitoringNagle;
    unsigned int monitoringHeaderSize;

    if (!parser.GetString("MONITORING", "IP", &monitoringIp))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MONITORING", "PORT", &monitoringPort))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MONITORING", "NAGLE", &monitoringNagle))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MONITORING", "HEADERSIZE", &monitoringHeaderSize))
    {
        return false;
    }

    return monitoringClient_.Start(monitoringIp.c_str(), monitoringPort, monitoringNagle, monitoringHeaderSize);
}

bool LoginServer::ConnectRedis(DKParser& parser)
{
    std::string redisIp;
    unsigned int redisPort;

    if (!parser.GetString("REDIS", "IP", &redisIp))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("REDIS", "PORT", &redisPort))
    {
        return false;
    }

    connection_->connect(redisIp, redisPort);

    return true;
}

bool LoginServer::SetMySqlConfig(DKParser& parser)
{
    if (!parser.GetString("MYSQL", "IP", &mysqlIp_))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MYSQL", "PORT", &mysqlPort_))
    {
        return false;
    }

    if (!parser.GetString("MYSQL", "USER", &mysqlUser_))
    {
        return false;
    }

    if (!parser.GetString("MYSQL", "PASSWORD", &mysqlPassword_))
    {
        return false;
    }

    if (!parser.GetString("MYSQL", "DATABASE", &mysqlDatabase_))
    {
        return false;
    }

    return true;
}

bool LoginServer::SetTargetServerConfig(DKParser& parser)
{
    std::string gameServerIp;
    std::string chatServerIp;
    unsigned int gameServerPort;
    unsigned int chatServerPort;

    if (!parser.GetString("GAMESERVER", "IP", &gameServerIp))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMESERVER", "PORT", &gameServerPort))
    {
        return false;
    }

    if (!parser.GetString("CHATSERVER", "IP", &chatServerIp))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("CHATSERVER", "PORT", &chatServerPort))
    {
        return false;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, gameServerIp.c_str(), -1, gameServerIp_, ServerIpLength) == 0)
    {
        return false;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, chatServerIp.c_str(), -1, chatServerIp_, ServerIpLength) == 0)
    {
        return false;
    }

    gameServerPort_ = static_cast<unsigned short>(gameServerPort);
    chatServerPort_ = static_cast<unsigned short>(chatServerPort);

    return true;
}

bool LoginServer::OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort)
{
    return false;
}

void LoginServer::OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId)
{
}

void LoginServer::OnRelease(__int64 sessionId)
{
}

void LoginServer::OnMessage(__int64 sessionId, ContentsCPacket* contentsPacket)
{
    unsigned short messageType;

    ContentsCPacket receivedPacket = contentsPacket;
    receivedPacket >> messageType;

    if (messageType != PacketCsLoginReqLogin || receivedPacket.GetDataSize() != PacketLoginRequestDataSize)
    {
        InterlockedIncrement(&dcWrongPacket_);
        Disconnect(sessionId);
        return;
    }

    __int64 accountNo;
    char sessionKey[SessionKeyLength];

    receivedPacket >> accountNo;
    receivedPacket.GetData(sessionKey, SessionKeyLength);

    ProcessLogin(sessionId, accountNo, sessionKey);
}

void LoginServer::ProcessLogin(__int64 sessionId, __int64 accountNo, const char* sessionKey)
{
    wchar_t id[PlayerIdLength];
    wchar_t nickname[PlayerNicknameLength];

    unsigned char authResult = ProcessAuth(accountNo, sessionKey, id, nickname);

    if (authResult == LoginStatusFail)
    {
        Disconnect(sessionId);
        InterlockedIncrement(&dcAuthFailed_);
        return;
    }

    InterlockedIncrement(&loginCount_);

    StoreSessionKey(accountNo, sessionKey);

    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();

    loginPacket << PacketScLoginResLogin << accountNo << authResult;
    loginPacket.PutData(reinterpret_cast<char*>(id), PlayerIdByteSize);
    loginPacket.PutData(reinterpret_cast<char*>(nickname), PlayerNicknameByteSize);
    loginPacket.PutData(reinterpret_cast<char*>(gameServerIp_), ServerIpByteSize);
    loginPacket << gameServerPort_;
    loginPacket.PutData(reinterpret_cast<char*>(chatServerIp_), ServerIpByteSize);
    loginPacket << chatServerPort_;

    SendPacket(sessionId, loginPacket);
}

unsigned char LoginServer::ProcessAuth(__int64 accountNo, const char* sessionKey, wchar_t* id, wchar_t* nickname)
{
    TLSDBConnection* dbConnection = static_cast<TLSDBConnection*>(TlsGetValue(tlsDbConnectionIndex_));

    if (dbConnection == nullptr)
    {
        dbConnection = new TLSDBConnection;

        mysql_init(&dbConnection->connectionInfo_);

        dbConnection->connection_ = mysql_real_connect(&dbConnection->connectionInfo_, mysqlIp_.c_str(), mysqlUser_.c_str(), mysqlPassword_.c_str(), mysqlDatabase_.c_str(), mysqlPort_, nullptr, 0);

        TlsSetValue(tlsDbConnectionIndex_, dbConnection);

        if (dbConnection->connection_ == nullptr)
        {
            DebugBreak();
        }

        mysql_set_server_option(dbConnection->connection_, MYSQL_OPTION_MULTI_STATEMENTS_ON);
    }

    char query[256];

    sprintf_s(query, "SELECT userid, usernick FROM %s.account WHERE accountno = %lld", mysqlDatabase_.c_str(), accountNo);

    if (mysql_query(dbConnection->connection_, query) != 0)
    {
    }

    MYSQL_RES* result = mysql_store_result(dbConnection->connection_);

    if (result == nullptr)
    {
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == nullptr)
    {
        return static_cast<unsigned char>(LoginStatusFail);
    }

    MultiByteToWideChar(CP_UTF8, 0, row[0], -1, id, PlayerIdLength);
    MultiByteToWideChar(CP_UTF8, 0, row[1], -1, nickname, PlayerNicknameLength);

    mysql_free_result(result);

    return static_cast<unsigned char>(LoginStatusOk);
}

void LoginServer::StoreSessionKey(__int64 accountNo, const char* sessionKey)
{
    AcquireSRWLockExclusive(&connectionLock_);

    std::string value(sessionKey, SessionKeyLength);

    connection_->set(std::to_string(accountNo), value);
    connection_->sync_commit();

    ReleaseSRWLockExclusive(&connectionLock_);
}

void LoginServer::OnError(int errorCode, const wchar_t* errorLog)
{
}

void LoginServer::OnInitializeTPS()
{
    int timeStamp = static_cast<int>(time(nullptr));

    monitoringClient_.UpdateMonitorData(MonitorDataTypeLoginServerRun, 1, timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeLoginServerCpu, static_cast<int>(processMonitoring_.ProcessTotal()), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeLoginServerMemory, static_cast<int>(processMonitoring_.GetProcessUserMemoryMBytes()), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeLoginSession, GetSessionNum(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeLoginAuthTps, loginTps_, timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeLoginPacketPool, CPacket::GetCapacity(), timeStamp);

    monitoringClient_.SendMonitorData();

    loginTps_ = loginCount_;
    InterlockedExchange(&loginCount_, 0);
}

DWORD LoginServer::GetLoginTPS()
{
    return loginTps_;
}

DWORD LoginServer::GetDCWrongPacket()
{
    return dcWrongPacket_;
}

DWORD LoginServer::GetDCAuthFailed()
{
    return dcAuthFailed_;
}

DWORD LoginServer::GetDCDuplicateLogin()
{
    return dcDuplicateLogin_;
}