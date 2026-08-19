#include "MultiChatServer.h"
#include "ContentsCPacket.h"
#include "DKParser.h"
#include "MonitoringDefine.h"
#include "PacketDefine.h"

#include <cstring>
#include <ctime>
#include <string>

MultiChatServer::MultiChatServer() : playerPool_(0)
{
    InitializeSRWLock(&playerMapLock_);
    InitializeSRWLock(&accountMapLock_);

    for (int sectorY = 0; sectorY < SectorMaxY; ++sectorY)
    {
        for (int sectorX = 0; sectorX < SectorMaxX; ++sectorX)
        {
            InitializeSRWLock(&sectorLock_[sectorY][sectorX]);
        }
    }

    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);

    connection_ = new cpp_redis::client;

    InitializeSRWLock(&connectionLock_);
}

MultiChatServer::~MultiChatServer()
{
}

bool MultiChatServer::Start(const DKServerCore::IocpServerStartConfig& config, DKParser& parser)
{
    if (!StartMonitoringClient(parser))
    {
        return false;
    }

    if (!ConnectRedis(parser))
    {
        return false;
    }

    return NetLibrary::Start(config);
}

bool MultiChatServer::StartMonitoringClient(DKParser& parser)
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

bool MultiChatServer::ConnectRedis(DKParser& parser)
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

bool MultiChatServer::OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort)
{
    return false;
}

void MultiChatServer::OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId)
{
    Player* newPlayer = playerPool_.Alloc();

    newPlayer->sessionId_ = sessionId;
    newPlayer->sectorPosition_.x_ = InitialSectorX;
    newPlayer->sectorPosition_.y_ = InitialSectorY;
    newPlayer->authFlag_ = false;
    newPlayer->duplicate_ = false;

    InterlockedIncrement(&unloginPlayer_);

    AcquireSRWLockExclusive(&playerMapLock_);
    playerMap_.insert({ newPlayer->sessionId_, newPlayer });
    ReleaseSRWLockExclusive(&playerMapLock_);
}

void MultiChatServer::OnRelease(__int64 sessionId)
{
    Player* targetPlayer;

    AcquireSRWLockExclusive(&playerMapLock_);

    std::unordered_map<__int64, Player*>::iterator playerIter = playerMap_.find(sessionId);

    if (playerIter == playerMap_.end())
    {
        DebugBreak();
    }

    targetPlayer = playerIter->second;
    playerMap_.erase(targetPlayer->sessionId_);

    ReleaseSRWLockExclusive(&playerMapLock_);

    if (targetPlayer->sectorPosition_.x_ != InitialSectorX)
    {
        unsigned int sectorX = targetPlayer->sectorPosition_.x_;
        unsigned int sectorY = targetPlayer->sectorPosition_.y_;

        AcquireSRWLockExclusive(&sectorLock_[sectorY][sectorX]);

        std::list<Player*>::iterator playerListIter = sectorList_[sectorY][sectorX].begin();

        for (; playerListIter != sectorList_[sectorY][sectorX].end(); ++playerListIter)
        {
            Player* sectorPlayer = *playerListIter;

            if (sectorPlayer->sessionId_ == targetPlayer->sessionId_)
            {
                sectorList_[sectorY][sectorX].erase(playerListIter);
                break;
            }
        }

        ReleaseSRWLockExclusive(&sectorLock_[sectorY][sectorX]);
    }

    if (targetPlayer->authFlag_)
    {
        InterlockedDecrement(&loginPlayer_);
    }
    else
    {
        InterlockedDecrement(&unloginPlayer_);
    }

    AcquireSRWLockExclusive(&accountMapLock_);

    std::unordered_map<__int64, Player*>::iterator accountIter = accountMap_.find(targetPlayer->accountNo_);

    if (accountIter != accountMap_.end())
    {
        if (targetPlayer->sessionId_ == accountIter->second->sessionId_)
        {
            accountMap_.erase(targetPlayer->accountNo_);
        }
    }

    ReleaseSRWLockExclusive(&accountMapLock_);

    playerPool_.Free(targetPlayer);
}

void MultiChatServer::OnMessage(__int64 sessionId, ContentsCPacket* contentsPacketPtr)
{
    unsigned short messageType;

    ContentsCPacket contentsPacket = contentsPacketPtr;
    contentsPacket >> messageType;

    Player* targetPlayer;

    AcquireSRWLockShared(&playerMapLock_);

    std::unordered_map<__int64, Player*>::iterator playerIter = playerMap_.find(sessionId);

    if (playerIter == playerMap_.end())
    {
        ReleaseSRWLockShared(&playerMapLock_);
        DebugBreak();
        return;
    }

    targetPlayer = playerIter->second;

    ReleaseSRWLockShared(&playerMapLock_);

    if (!targetPlayer->authFlag_ && messageType != PacketCsChatReqLogin)
    {
        InterlockedIncrement(&dcAuthFailed_);
        Disconnect(sessionId);
        return;
    }

    switch (messageType)
    {
    case PacketCsChatReqLogin:
    {
        __int64 accountNo;
        wchar_t id[PlayerIdLength];
        wchar_t nickname[PlayerNicknameLength];
        char sessionKey[SessionKeyLength];

        if (contentsPacket.GetDataSize() != PacketChatLoginRequestDataSize)
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        contentsPacket >> accountNo;
        contentsPacket.GetData(reinterpret_cast<char*>(id), PlayerIdByteSize);
        contentsPacket.GetData(reinterpret_cast<char*>(nickname), PlayerNicknameByteSize);
        contentsPacket.GetData(sessionKey, SessionKeyLength);

        NetPacketProcLogin(targetPlayer, accountNo, id, nickname, sessionKey);
        InterlockedIncrement(&loginCount_);

        break;
    }

    case PacketCsChatReqSectorMove:
    {
        __int64 accountNo;
        unsigned short sectorX;
        unsigned short sectorY;

        if (contentsPacket.GetDataSize() != PacketChatSectorMoveRequestDataSize)
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        contentsPacket >> accountNo;
        contentsPacket >> sectorX;
        contentsPacket >> sectorY;

        if (!IsValidSector(sectorX, sectorY))
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        NetPacketProcSectorMove(targetPlayer, accountNo, sectorX, sectorY);
        InterlockedIncrement(&sectorMoveCount_);

        break;
    }

    case PacketCsChatReqMessage:
    {
        if (!IsValidSector(targetPlayer->sectorPosition_.x_, targetPlayer->sectorPosition_.y_))
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        __int64 accountNo;
        unsigned short messageLength;
        wchar_t message[MaxChatSize];

        if (contentsPacket.GetDataSize() > PacketChatMessageRequestMaxDataSize || contentsPacket.GetDataSize() < PacketChatMessageRequestMinDataSize)
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        contentsPacket >> accountNo;
        contentsPacket >> messageLength;

        if (messageLength > PacketChatMessageMaxByteSize)
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        if(contentsPacket.GetData(reinterpret_cast<char*>(message), messageLength) != messageLength)
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        message[messageLength / sizeof(wchar_t)] = L'\0';

        NetPacketProcMessage(targetPlayer, accountNo, messageLength, message);
        InterlockedIncrement(&chatCount_);

        break;
    }

    case PacketCsChatReqHeartbeat:
    {
        if (contentsPacket.GetDataSize() != 0)
        {
            InterlockedIncrement(&dcWrongPacket_);
            Disconnect(sessionId);
            break;
        }

        NetPacketProcHeartbeat(targetPlayer);
        InterlockedIncrement(&heartbeatCount_);

        break;
    }

    default:
    {
        InterlockedIncrement(&dcWrongPacket_);
        Disconnect(sessionId);

        break;
    }
    }

    InterlockedIncrement(&logicCount_);
}

void MultiChatServer::OnError(int errorCode, const wchar_t* errorLog)
{
}

void MultiChatServer::OnInitializeTPS()
{
    int timeStamp = static_cast<int>(time(nullptr));

    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatServerRun, 1, timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatServerCpu, static_cast<int>(processMonitoring_.ProcessTotal()), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatServerMemory, static_cast<int>(processMonitoring_.GetProcessUserMemoryMBytes()), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatSession, GetSessionNum(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatPlayer, GetLoginPlayer(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatUpdateTps, logicTps_, timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatPacketPool, CPacket::GetCapacity(), timeStamp);
    monitoringClient_.UpdateMonitorData(MonitorDataTypeChatUpdateMessagePool, 0, timeStamp);

    monitoringClient_.SendMonitorData();

    logicTps_ = logicCount_;
    loginTps_ = loginCount_;
    sectorMoveTps_ = sectorMoveCount_;
    chatTps_ = chatCount_;
    heartbeatTps_ = heartbeatCount_;

    logicCount_ = 0;
    loginCount_ = 0;
    sectorMoveCount_ = 0;
    chatCount_ = 0;
    heartbeatCount_ = 0;
}

bool MultiChatServer::AuthToken(__int64 accountNo, char* sessionKey)
{
    AcquireSRWLockExclusive(&connectionLock_);

    std::string key = std::to_string(accountNo);
    std::string value;

    connection_->get(key, [&value](cpp_redis::reply& reply)
        {
            if (reply.is_string())
            {
                value = reply.as_string();
            }
        });

    connection_->sync_commit();

    if (value.size() != SessionKeyLength)
    {
        ReleaseSRWLockExclusive(&connectionLock_);
        return false;
    }

    if (memcmp(value.c_str(), sessionKey, SessionKeyLength) != 0)
    {
        ReleaseSRWLockExclusive(&connectionLock_);
        return false;
    }

    connection_->del({ key });
    connection_->sync_commit();

    ReleaseSRWLockExclusive(&connectionLock_);

    return true;
}

void MultiChatServer::NetPacketProcLogin(Player* targetPlayer, __int64 accountNo, wchar_t* id, wchar_t* nickname, char* sessionKey)
{
    if (targetPlayer->authFlag_)
    {
        InterlockedIncrement(&dcLoginAgain_);
        Disconnect(targetPlayer->sessionId_);

        return;
    }

    targetPlayer->accountNo_ = accountNo;

    wmemcpy_s(targetPlayer->id_, PlayerIdLength, id, PlayerIdLength);
    wmemcpy_s(targetPlayer->nickname_, PlayerNicknameLength, nickname, PlayerNicknameLength);

    if (!AuthToken(accountNo, sessionKey))
    {
        InterlockedIncrement(&dcAuthFailed_);
        Disconnect(targetPlayer->sessionId_);

        return;
    }

    AcquireSRWLockExclusive(&accountMapLock_);

    std::unordered_map<__int64, Player*>::iterator accountIter = accountMap_.find(accountNo);

    if (accountIter != accountMap_.end())
    {
        Player* disconnectPlayer = accountIter->second;

        Disconnect(disconnectPlayer->sessionId_);
        disconnectPlayer->duplicate_ = true;

        InterlockedIncrement(&dcDuplicateLogin_);

        accountMap_.erase(accountNo);
    }

    accountMap_.insert({ targetPlayer->accountNo_, targetPlayer });

    ReleaseSRWLockExclusive(&accountMapLock_);

    targetPlayer->authFlag_ = true;

    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();

    loginPacket << PacketScChatResLogin;
    loginPacket << static_cast<unsigned char>(TRUE);
    loginPacket << targetPlayer->accountNo_;

    SendPacket(targetPlayer->sessionId_, loginPacket);

    InterlockedDecrement(&unloginPlayer_);
    InterlockedIncrement(&loginPlayer_);
}

void MultiChatServer::NetPacketProcSectorMove(Player* targetPlayer, __int64 accountNo, unsigned short sectorX, unsigned short sectorY)
{
    if (targetPlayer->sectorPosition_.x_ == InitialSectorX)
    {
        targetPlayer->sectorPosition_.x_ = sectorX;
        targetPlayer->sectorPosition_.y_ = sectorY;

        AcquireSRWLockExclusive(&sectorLock_[sectorY][sectorX]);
        sectorList_[sectorY][sectorX].push_back(targetPlayer);
        ReleaseSRWLockExclusive(&sectorLock_[sectorY][sectorX]);
    }
    else
    {
        unsigned short previousSectorX = static_cast<unsigned short>(targetPlayer->sectorPosition_.x_);
        unsigned short previousSectorY = static_cast<unsigned short>(targetPlayer->sectorPosition_.y_);

        targetPlayer->sectorPosition_.x_ = sectorX;
        targetPlayer->sectorPosition_.y_ = sectorY;

        if (previousSectorX != sectorX || previousSectorY != sectorY)
        {
            LockSectorMove(previousSectorX, previousSectorY, sectorX, sectorY);

            std::list<Player*>::iterator playerIter = sectorList_[previousSectorY][previousSectorX].begin();

            for (; playerIter != sectorList_[previousSectorY][previousSectorX].end(); ++playerIter)
            {
                Player* sectorPlayer = *playerIter;

                if (sectorPlayer->sessionId_ == targetPlayer->sessionId_)
                {
                    sectorList_[previousSectorY][previousSectorX].erase(playerIter);
                    break;
                }
            }

            sectorList_[sectorY][sectorX].push_back(targetPlayer);

            UnlockSectorMove(previousSectorX, previousSectorY, sectorX, sectorY);
        }
    }

    ContentsCPacket sectorMovePacket = ContentsCPacket::MakeContentsPacket();

    sectorMovePacket << PacketScChatResSectorMove;
    sectorMovePacket << targetPlayer->accountNo_;
    sectorMovePacket << static_cast<unsigned short>(targetPlayer->sectorPosition_.x_);
    sectorMovePacket << static_cast<unsigned short>(targetPlayer->sectorPosition_.y_);

    SendPacket(targetPlayer->sessionId_, sectorMovePacket);
}

void MultiChatServer::NetPacketProcMessage(Player* targetPlayer, __int64 accountNo, unsigned short messageLength, wchar_t* message)
{
    ContentsCPacket chatPacket = ContentsCPacket::MakeContentsPacket();

    if (targetPlayer->duplicate_)
    {
        return;
    }

    chatPacket << PacketScChatResMessage;
    chatPacket << targetPlayer->accountNo_;
    chatPacket.PutData(reinterpret_cast<char*>(targetPlayer->id_), PlayerIdByteSize);
    chatPacket.PutData(reinterpret_cast<char*>(targetPlayer->nickname_), PlayerNicknameByteSize);
    chatPacket << messageLength;
    chatPacket.PutData(reinterpret_cast<char*>(message), messageLength);

    SectorAround aroundSector;
    GetSectorAround(static_cast<int>(targetPlayer->sectorPosition_.x_), static_cast<int>(targetPlayer->sectorPosition_.y_), &aroundSector);

    for (unsigned int i = 0; i < aroundSector.count_; ++i)
    {
        unsigned int aroundX = aroundSector.around_[i].x_;
        unsigned int aroundY = aroundSector.around_[i].y_;

        AcquireSRWLockShared(&sectorLock_[aroundY][aroundX]);

        std::list<Player*>::iterator playerIter = sectorList_[aroundY][aroundX].begin();

        for (; playerIter != sectorList_[aroundY][aroundX].end(); ++playerIter)
        {
            Player* sectorPlayer = *playerIter;
            SendPacket(sectorPlayer->sessionId_, chatPacket);
        }
    }

    for (int i = static_cast<int>(aroundSector.count_) - 1; i >= 0; --i)
    {
        unsigned int aroundX = aroundSector.around_[i].x_;
        unsigned int aroundY = aroundSector.around_[i].y_;

        ReleaseSRWLockShared(&sectorLock_[aroundY][aroundX]);
    }
}

void MultiChatServer::NetPacketProcHeartbeat(Player* targetPlayer)
{
}

void MultiChatServer::GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
{
    aroundSector->count_ = 0;

    if (sectorY - 1 >= 0 && sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        ++aroundSector->count_;
    }

    if (sectorY - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        ++aroundSector->count_;
    }

    if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        ++aroundSector->count_;
    }

    if (sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        ++aroundSector->count_;
    }

    aroundSector->around_[aroundSector->count_].x_ = sectorX;
    aroundSector->around_[aroundSector->count_].y_ = sectorY;
    ++aroundSector->count_;

    if (sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        ++aroundSector->count_;
    }

    if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        ++aroundSector->count_;
    }

    if (sectorY + 1 < SectorMaxY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        ++aroundSector->count_;
    }

    if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        ++aroundSector->count_;
    }
}

void MultiChatServer::LockSectorMove(unsigned short firstX, unsigned short firstY, unsigned short secondX, unsigned short secondY)
{
    if (firstY > secondY || firstY == secondY && firstX > secondX)
    {
        unsigned short temporaryX = firstX;
        unsigned short temporaryY = firstY;

        firstX = secondX;
        firstY = secondY;
        secondX = temporaryX;
        secondY = temporaryY;
    }

    AcquireSRWLockExclusive(&sectorLock_[firstY][firstX]);
    AcquireSRWLockExclusive(&sectorLock_[secondY][secondX]);
}

void MultiChatServer::UnlockSectorMove(unsigned short firstX, unsigned short firstY, unsigned short secondX, unsigned short secondY)
{
    if (firstY > secondY || firstY == secondY && firstX > secondX)
    {
        unsigned short temporaryX = firstX;
        unsigned short temporaryY = firstY;

        firstX = secondX;
        firstY = secondY;
        secondX = temporaryX;
        secondY = temporaryY;
    }

    ReleaseSRWLockExclusive(&sectorLock_[secondY][secondX]);
    ReleaseSRWLockExclusive(&sectorLock_[firstY][firstX]);
}

bool MultiChatServer::IsValidSector(unsigned int sectorX, unsigned int sectorY)
{
    if (sectorX >= SectorMaxX || sectorY >= SectorMaxY)
    {
        return false;
    }

    return true;
}

DWORD MultiChatServer::GetLogicTPS()
{
    return logicTps_;
}

DWORD MultiChatServer::GetLoginTPS()
{
    return loginTps_;
}

DWORD MultiChatServer::GetSectorMoveTPS()
{
    return sectorMoveTps_;
}

DWORD MultiChatServer::GetChatTPS()
{
    return chatTps_;
}

DWORD MultiChatServer::GetHeartBeatTPS()
{
    return heartbeatTps_;
}

DWORD MultiChatServer::GetDCWrongPacket()
{
    return dcWrongPacket_;
}

DWORD MultiChatServer::GetDCLoginAgain()
{
    return dcLoginAgain_;
}

DWORD MultiChatServer::GetDCAuthFailed()
{
    return dcAuthFailed_;
}

DWORD MultiChatServer::GetDCDuplicateLogin()
{
    return dcDuplicateLogin_;
}

DWORD MultiChatServer::GetLoginPlayer()
{
    return loginPlayer_;
}

DWORD MultiChatServer::GetUnloginPlayer()
{
    return unloginPlayer_;
}