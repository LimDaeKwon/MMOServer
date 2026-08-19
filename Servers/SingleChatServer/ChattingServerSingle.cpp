#include "ChattingServerSingle.h"
#include "ContentsCPacket.h"
#include "PacketDefine.h"

#include <cstdio>
#include <cwchar>
#include <process.h>
#include <Windows.h>

ChattingServerSingle::ChattingServerSingle() : playerPool_(0), messageDataFreeList_(0)
{
    messageEvent_ = CreateEvent(nullptr, TRUE, FALSE, L"WaitEvent");

    if (messageEvent_ == nullptr)
    {
        DebugBreak();
    }

    logicThreadHandle_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LogicThread, this, 0, nullptr));
}

ChattingServerSingle::~ChattingServerSingle() = default;

bool ChattingServerSingle::OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort)
{
    return false;
}

void ChattingServerSingle::OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId)
{
    MessageData* messageData = messageDataFreeList_.Alloc();
    messageData->sessionId_ = sessionId;
    messageData->contentsPacket_ = nullptr;
    messageData->type_ = MessageTypeAccept;

    messageQueue_.Enqueue(messageData);
    SetEvent(messageEvent_);
}

void ChattingServerSingle::OnRelease(__int64 sessionId)
{
    MessageData* messageData = messageDataFreeList_.Alloc();
    messageData->sessionId_ = sessionId;
    messageData->contentsPacket_ = nullptr;
    messageData->type_ = MessageTypeRelease;

    messageQueue_.Enqueue(messageData);
    SetEvent(messageEvent_);
}

void ChattingServerSingle::OnMessage(__int64 sessionId, ContentsCPacket* contentsPacket)
{
    MessageData* messageData = messageDataFreeList_.Alloc();
    messageData->sessionId_ = sessionId;
    messageData->contentsPacket_ = contentsPacket;
    messageData->type_ = MessageTypePacket;

    messageQueue_.Enqueue(messageData);
    SetEvent(messageEvent_);
}

void ChattingServerSingle::OnError(int errorCode, const wchar_t* errorLog)
{
}

void ChattingServerSingle::OnInitializeTPS()
{
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

void ChattingServerSingle::AcceptProc(MessageData* messageData)
{
    Player* newPlayer = playerPool_.Alloc();

    newPlayer->sessionId_ = messageData->sessionId_;
    newPlayer->lastRecvTime_ = 0;
    newPlayer->accountNo_ = 0;

    newPlayer->sectorPosition_.x_ = InitialSectorX;
    newPlayer->sectorPosition_.y_ = InitialSectorY;

    newPlayer->id_[0] = L'\0';
    newPlayer->nickname_[0] = L'\0';

    newPlayer->authFlag_ = false;
    newPlayer->duplicate_ = false;

    ++unloginPlayer_;

    playerMap_.insert(std::unordered_map<__int64, Player*>::value_type(newPlayer->sessionId_, newPlayer));
}

void ChattingServerSingle::MessageProc(MessageData* messageData)
{
    unsigned short messageType;

    ContentsCPacket contentsPacket = messageData->contentsPacket_;
    contentsPacket >> messageType;

    std::unordered_map<__int64, Player*>::iterator playerIterator = playerMap_.find(messageData->sessionId_);

    if (playerIterator == playerMap_.end())
    {
        DebugBreak();
        return;
    }

    Player* targetPlayer = playerIterator->second;

    if (!targetPlayer->authFlag_ && messageType != PacketCsChatReqLogin)
    {
        ++dcAuthFailed_;
        Disconnect(messageData->sessionId_);
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
            ++dcWrongPacket_;
            Disconnect(messageData->sessionId_);
            break;
        }

        contentsPacket >> accountNo;
        contentsPacket.GetData(reinterpret_cast<char*>(id), PlayerIdByteSize);
        contentsPacket.GetData(reinterpret_cast<char*>(nickname), PlayerNicknameByteSize);
        contentsPacket.GetData(sessionKey, SessionKeyLength);

        NetPacketProcLogin(targetPlayer, accountNo, id, nickname, sessionKey);
        ++loginCount_;

        break;
    }

    case PacketCsChatReqSectorMove:
    {
        __int64 accountNo;
        unsigned short sectorX;
        unsigned short sectorY;
        
        if (contentsPacket.GetDataSize() != PacketChatSectorMoveRequestDataSize)
        {
            ++dcWrongPacket_;
            Disconnect(messageData->sessionId_);
            break;
        }

        contentsPacket >> accountNo;
        contentsPacket >> sectorX;
        contentsPacket >> sectorY;

        if (!IsValidSector(sectorX, sectorY))
        {
            ++dcWrongPacket_;
            Disconnect(messageData->sessionId_);
            break;
        }

        NetPacketProcSectorMove(targetPlayer, accountNo, sectorX, sectorY);
        ++sectorMoveCount_;

        break;
    }

    case PacketCsChatReqMessage:
    {
        if (!IsValidSector(targetPlayer->sectorPosition_.x_, targetPlayer->sectorPosition_.y_))
        {
            ++dcWrongPacket_;
            Disconnect(messageData->sessionId_);
            break;
        }

        __int64 accountNo;
        unsigned short messageLength;
        wchar_t message[MaxChatSize];

        if (contentsPacket.GetDataSize() > PacketChatMessageRequestMaxDataSize || contentsPacket.GetDataSize() < PacketChatMessageRequestMinDataSize)
        {
            ++dcWrongPacket_;
            Disconnect(messageData->sessionId_);
            break;
        }

        contentsPacket >> accountNo;
        contentsPacket >> messageLength;

        if (messageLength > PacketChatMessageMaxByteSize)
        {
            ++dcWrongPacket_;
            Disconnect(messageData->sessionId_);
            break;
        }

        if(contentsPacket.GetData(reinterpret_cast<char*>(message), messageLength) != messageLength)
        {
            ++dcWrongPacket_;
            Disconnect(messageData->sessionId_);
            break;
        }
        message[messageLength / sizeof(wchar_t)] = L'\0';

        NetPacketProcMessage(targetPlayer, accountNo, messageLength, message);
        ++chatCount_;

        break;
    }

    case PacketCsChatReqHeartbeat:
    {
        NetPacketProcHeartbeat(targetPlayer);
        ++heartbeatCount_;

        break;
    }

    default:
    {
        ++dcWrongPacket_;
        Disconnect(messageData->sessionId_);

        break;
    }
    }

    ++logicCount_;
}

void ChattingServerSingle::ReleaseProc(MessageData* messageData)
{
    std::unordered_map<__int64, Player*>::iterator playerIterator = playerMap_.find(messageData->sessionId_);

    if (playerIterator == playerMap_.end())
    {
        return;
    }

    Player* targetPlayer = playerIterator->second;

    if (targetPlayer->sectorPosition_.x_ != InitialSectorX)
    {
        std::list<Player*>::iterator sectorIterator = sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].begin();

        for (; sectorIterator != sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].end(); ++sectorIterator)
        {
            Player* target = *sectorIterator;

            if (target->sessionId_ == targetPlayer->sessionId_)
            {
                sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].erase(sectorIterator);
                break;
            }
        }
    }

    if (targetPlayer->authFlag_)
    {
        --loginPlayer_;
    }
    else
    {
        --unloginPlayer_;
    }

    std::unordered_map<__int64, Player*>::iterator accountIterator = accountMap_.find(targetPlayer->accountNo_);

    if (accountIterator != accountMap_.end())
    {
        if (targetPlayer->sessionId_ == accountIterator->second->sessionId_)
        {
            accountMap_.erase(targetPlayer->accountNo_);
        }
    }

    if (playerMap_.erase(targetPlayer->sessionId_) == 0)
    {
        DebugBreak();
    }

    playerPool_.Free(targetPlayer);
}

void ChattingServerSingle::NetPacketProcLogin(Player* targetPlayer, __int64 accountNo, wchar_t* id, wchar_t* nickname, char* sessionKey)
{
    targetPlayer->accountNo_ = accountNo;
    targetPlayer->lastRecvTime_ = GetTickCount64();

    wmemcpy_s(targetPlayer->id_, PlayerIdLength, id, PlayerIdLength);
    wmemcpy_s(targetPlayer->nickname_, PlayerNicknameLength, nickname, PlayerNicknameLength);

    std::unordered_map<__int64, Player*>::iterator accountIterator = accountMap_.find(accountNo);

    if (accountIterator != accountMap_.end())
    {
        Player* disconnectPlayer = accountIterator->second;

        Disconnect(disconnectPlayer->sessionId_);
        disconnectPlayer->duplicate_ = true;
        ++dcDuplicateLogin_;

        if (accountMap_.erase(accountNo) == 0)
        {
            DebugBreak();
        }
    }

    accountMap_.insert(std::unordered_map<__int64, Player*>::value_type(targetPlayer->accountNo_, targetPlayer));

    targetPlayer->authFlag_ = true;

    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();
    loginPacket << PacketScChatResLogin << static_cast<unsigned char>(TRUE) << targetPlayer->accountNo_;

    SendPacket(targetPlayer->sessionId_, loginPacket);

    --unloginPlayer_;
    ++loginPlayer_;
}

void ChattingServerSingle::NetPacketProcSectorMove(Player* targetPlayer, __int64 accountNo, unsigned short sectorX, unsigned short sectorY)
{
    if (targetPlayer->sectorPosition_.x_ == InitialSectorX)
    {
        targetPlayer->sectorPosition_.x_ = sectorX;
        targetPlayer->sectorPosition_.y_ = sectorY;

        sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].push_back(targetPlayer);
    }
    else
    {
        std::list<Player*>::iterator sectorIterator = sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].begin();

        for (; sectorIterator != sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].end(); ++sectorIterator)
        {
            Player* target = *sectorIterator;

            if (target->sessionId_ == targetPlayer->sessionId_)
            {
                sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].erase(sectorIterator);
                break;
            }
        }

        targetPlayer->sectorPosition_.x_ = sectorX;
        targetPlayer->sectorPosition_.y_ = sectorY;

        sectorList_[targetPlayer->sectorPosition_.y_][targetPlayer->sectorPosition_.x_].push_back(targetPlayer);
    }

    ContentsCPacket sectorMovePacket = ContentsCPacket::MakeContentsPacket();
    sectorMovePacket << PacketScChatResSectorMove << targetPlayer->accountNo_ << targetPlayer->sectorPosition_.x_ << targetPlayer->sectorPosition_.y_;

    SendPacket(targetPlayer->sessionId_, sectorMovePacket);
}

void ChattingServerSingle::NetPacketProcMessage(Player* targetPlayer, __int64 accountNo, unsigned short messageLength, wchar_t* message)
{
    if (targetPlayer->duplicate_)
    {
        return;
    }

    ContentsCPacket chatPacket = ContentsCPacket::MakeContentsPacket();
    chatPacket << PacketScChatResMessage << targetPlayer->accountNo_;
    chatPacket.PutData(reinterpret_cast<char*>(targetPlayer->id_), PlayerIdByteSize);
    chatPacket.PutData(reinterpret_cast<char*>(targetPlayer->nickname_), PlayerNicknameByteSize);
    chatPacket << messageLength;
    chatPacket.PutData(reinterpret_cast<char*>(message), messageLength);

    SectorAround aroundSector;
    GetSectorAround(targetPlayer->sectorPosition_.x_, targetPlayer->sectorPosition_.y_, &aroundSector);

    for (unsigned int i = 0; i < aroundSector.count_; ++i)
    {
        unsigned int aroundX = aroundSector.around_[i].x_;
        unsigned int aroundY = aroundSector.around_[i].y_;

        std::list<Player*>::iterator sectorIterator;

        for (sectorIterator = sectorList_[aroundY][aroundX].begin(); sectorIterator != sectorList_[aroundY][aroundX].end(); ++sectorIterator)
        {
            Player* target = *sectorIterator;
            SendPacket(target->sessionId_, chatPacket);
        }
    }
}

void ChattingServerSingle::NetPacketProcHeartbeat(Player* targetPlayer)
{
    targetPlayer->lastRecvTime_ = GetTickCount64();
}

void ChattingServerSingle::GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
{
    aroundSector->count_ = 0;

    aroundSector->around_[aroundSector->count_].x_ = sectorX;
    aroundSector->around_[aroundSector->count_].y_ = sectorY;
    ++aroundSector->count_;

    if (sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        ++aroundSector->count_;
    }

    if (sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        ++aroundSector->count_;
    }

    if (sectorY + 1 < SectorMaxY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        ++aroundSector->count_;
    }

    if (sectorY - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        ++aroundSector->count_;
    }

    if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        ++aroundSector->count_;
    }

    if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        ++aroundSector->count_;
    }

    if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        ++aroundSector->count_;
    }

    if (sectorY - 1 >= 0 && sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        ++aroundSector->count_;
    }
}

bool ChattingServerSingle::IsValidSector(unsigned int sectorX, unsigned int sectorY)
{
    if (sectorX >= SectorMaxX || sectorY >= SectorMaxY)
    {
        return false;
    }

    return true;
}


unsigned int WINAPI ChattingServerSingle::LogicThread(LPVOID thisPtr)
{
    ChattingServerSingle* server = static_cast<ChattingServerSingle*>(thisPtr);

    while (true)
    {
        WaitForSingleObject(server->messageEvent_, INFINITE);

        while (true)
        {
            MessageData* messageData = nullptr;

            if (!server->messageQueue_.Dequeue(&messageData))
            {
                continue;
            }

            switch (messageData->type_)
            {
            case MessageTypeAccept:
            {
                server->AcceptProc(messageData);
                break;
            }

            case MessageTypePacket:
            {
                server->MessageProc(messageData);
                break;
            }

            case MessageTypeRelease:
            {
                server->ReleaseProc(messageData);
                break;
            }

            default:
            {
                break;
            }
            }

            server->messageDataFreeList_.Free(messageData);
        }
    }

    return 0;
}

DWORD ChattingServerSingle::GetLogicTPS()
{
    return logicTps_;
}

DWORD ChattingServerSingle::GetLoginTPS()
{
    return loginTps_;
}

DWORD ChattingServerSingle::GetSectorMoveTPS()
{
    return sectorMoveTps_;
}

DWORD ChattingServerSingle::GetChatTPS()
{
    return chatTps_;
}

DWORD ChattingServerSingle::GetHeartBeatTPS()
{
    return heartbeatTps_;
}

DWORD ChattingServerSingle::GetDCWrongPacket()
{
    return dcWrongPacket_;
}

DWORD ChattingServerSingle::GetDCAuthFailed()
{
    return dcAuthFailed_;
}

DWORD ChattingServerSingle::GetDCDuplicateLogin()
{
    return dcDuplicateLogin_;
}

DWORD ChattingServerSingle::GetLoginPlayer()
{
    return loginPlayer_;
}

DWORD ChattingServerSingle::GetUnloginPlayer()
{
    return unloginPlayer_;
}

DWORD ChattingServerSingle::GetLogicQueueSize()
{
    return messageQueue_.GetSize();
}