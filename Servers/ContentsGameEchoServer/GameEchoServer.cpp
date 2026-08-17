#include "GameEchoServer.h"
#include "DKParser.h"
#include "GameDefine.h"
#include "GameEchoServerGroup.h"
#include "MonitoringDefine.h"

#include <ctime>
#include <string>

GameEchoServer::GameEchoServer()
    : playerPool_(0)
{
    InitializeSRWLock(&playerMapLock_);

    AuthGroup* authGroup = new AuthGroup(AuthGroupId, AuthGroupFrameMs);
    EchoGroup* echoGroup = new EchoGroup(EchoGroupId, EchoGroupFrameMs);

    authGroup->AttachServer(this);
    echoGroup->AttachServer(this);

    authGroup->Start();
    echoGroup->Start();

    groupManager_.AddGroup(authGroup, AuthGroupId);
    groupManager_.AddGroup(echoGroup, EchoGroupId);
}

GameEchoServer::~GameEchoServer()
{
}

bool GameEchoServer::Start(const DKServerCore::IocpServerStartConfig& config, DKParser& parser)
{
    unsigned int syncAsync;
    unsigned int sendThreadCount;

    if (!parser.GetUnsignedInt("GAMEECHO", "SYNCASYNC", &syncAsync))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMEECHO", "SENDTHREADS", &sendThreadCount))
    {
        return false;
    }

    if (!StartMonitoringClient(parser))
    {
        return false;
    }

    return ContentsNetLibrary::Start(config.ip.c_str(), config.port, config.workerThreadCount, config.concurrentThreadCount, config.nagle, config.maxSessionCount, config.headerSize, syncAsync, sendThreadCount, config.packetCode);
}

bool GameEchoServer::StartMonitoringClient(DKParser& parser)
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

bool GameEchoServer::OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort)
{
    return false;
}

void GameEchoServer::OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId)
{
    AcquireSRWLockExclusive(&playerMapLock_);

    Player* newPlayer = playerPool_.Alloc();
    newPlayer->sessionId_ = sessionId;

    playerMap_.insert(std::unordered_map<__int64, Player*>::value_type(newPlayer->sessionId_, newPlayer));

    ReleaseSRWLockExclusive(&playerMapLock_);
}

void GameEchoServer::OnRelease(__int64 sessionId)
{

}

void* GameEchoServer::GetPlayerPointer(__int64 sessionId)
{
    AcquireSRWLockShared(&playerMapLock_);

    std::unordered_map<__int64, Player*>::iterator playerIter = playerMap_.find(sessionId);

    if (playerIter == playerMap_.end())
    {
        ReleaseSRWLockShared(&playerMapLock_);
        return nullptr;
    }

    Player* target = playerIter->second;

    ReleaseSRWLockShared(&playerMapLock_);

    return target;
}

void GameEchoServer::FreePlayer(Player* target)
{
    if (target == nullptr)
    {
        return;
    }

    AcquireSRWLockExclusive(&playerMapLock_);

    std::unordered_map<SessionId, Player*>::iterator playerIter = playerMap_.find(target->sessionId_);

    if (playerIter == playerMap_.end() || playerIter->second != target)
    {
        ReleaseSRWLockExclusive(&playerMapLock_);
        DebugBreak();
        return;
    }

    playerMap_.erase(playerIter);

    ReleaseSRWLockExclusive(&playerMapLock_);

    playerPool_.Free(target);
}

void GameEchoServer::OnMessage(__int64 sessionId, ContentsCPacket* contentsPacket)
{
}

void GameEchoServer::OnError(int errorCode, const wchar_t* errorLog)
{
}

void GameEchoServer::OnInitializeTPS()
{
    groupManager_.GroupOnInitializeTPS();

    int timeStamp = static_cast<int>(time(nullptr));

    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameServerRun, 1, timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameServerCpu, static_cast<int>(processMonitoring_.ProcessTotal()), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameServerMemory, static_cast<int>(processMonitoring_.GetProcessUserMemoryMBytes()), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameSession, GetSessionNum(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameAuthPlayer, groupManager_.GetGroupPlayerSize(AuthGroupId), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameGamePlayer, groupManager_.GetGroupPlayerSize(EchoGroupId), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameAcceptTps, GetAcceptTPS(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGamePacketRecvTps, GetRecvMessageTPS(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGamePacketSendTps, GetSendMessageTPS(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameDbWriteTps, 0, timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameDbWriteMessage, 0, timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameAuthThreadFps, groupManager_.GetGroupFPS(AuthGroupId), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGameGameThreadFps, groupManager_.GetGroupFPS(EchoGroupId), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeGamePacketPool, CPacket::GetCapacity(), timeStamp);

    monitoringClient_.SendMonitorData();

    logicTps_ = logicCount_;
    loginTps_ = loginCount_;
    heartbeatTps_ = heartbeatCount_;

    logicCount_ = 0;
    loginCount_ = 0;
    heartbeatCount_ = 0;
}

DWORD GameEchoServer::GetLogicTPS()
{
    return logicTps_;
}

DWORD GameEchoServer::GetLoginTPS()
{
    return loginTps_;
}

DWORD GameEchoServer::GetHeartbeatTPS()
{
    return heartbeatTps_;
}

DWORD GameEchoServer::GetDCWrongPacket()
{
    return dcWrongPacket_;
}

DWORD GameEchoServer::GetDCAuthFailed()
{
    return dcAuthFailed_;
}

DWORD GameEchoServer::GetDCDuplicateLogin()
{
    return dcDuplicateLogin_;
}

DWORD GameEchoServer::GetLoginPlayer()
{
    return loginPlayer_;
}

DWORD GameEchoServer::GetUnloginPlayer()
{
    return unloginPlayer_;
}