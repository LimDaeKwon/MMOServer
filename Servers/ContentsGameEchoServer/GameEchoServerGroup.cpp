#include "ContentsNetLibrary.h"
#include "GameEchoServerGroup.h"
#include "ContentsCPacket.h"
#include "GameDefine.h"
#include "PacketDefine.h"
#include "GameEchoServer.h"


AuthGroup::AuthGroup(GroupId groupId, DWORD frameMs)
{
    frameMS_ = frameMs;
    groupId_ = groupId;
}

AuthGroup::~AuthGroup()
{
}

void AuthGroup::OnEnter(SessionId sessionId)
{
    Player* newPlayer = static_cast<Player*>(contentsServer_->GetPlayerPointer(sessionId));

    if (newPlayer == nullptr)
    {
        return;
    }

    authPlayerMap_.insert(std::unordered_map<SessionId, Player*>::value_type(newPlayer->sessionId_, newPlayer));
}

void AuthGroup::OnLeave(SessionId sessionId)
{
    std::unordered_map<SessionId, Player*>::iterator authPlayerIter = authPlayerMap_.find(sessionId);

    if (authPlayerIter == authPlayerMap_.end())
    {
        DebugBreak();
        return;
    }

    authPlayerMap_.erase(authPlayerIter);

}

void AuthGroup::OnMessage(SessionId sessionId, ContentsCPacket* packet)
{
    std::unordered_map<SessionId, Player*>::iterator authPlayerIter = authPlayerMap_.find(sessionId);

    if (authPlayerIter == authPlayerMap_.end())
    {
        DebugBreak();
        return;
    }

    Player* target = authPlayerIter->second;
    unsigned short messageType;

    ContentsCPacket contentsPacket = packet;

    while (contentsPacket.GetDataSize() > 0)
    {
        int packetSize = 0;

        contentsPacket >> packetSize;
        contentsPacket >> messageType;

        if (messageType == PacketCsGameReqLogin)
        {
            if (packetSize != PacketGameReqLoginSize)
            {
                contentsServer_->Disconnect(sessionId);
                return;
            }

            __int64 accountNo;
            char sessionKey[SessionKeyLength];
            int version;

            contentsPacket >> accountNo;
            contentsPacket.GetData(sessionKey, SessionKeyLength);
            contentsPacket >> version;

            target->accountNo_ = accountNo;

            contentsServer_->MoveGroup(sessionId, EchoGroupId);
        }
        else
        {
            contentsServer_->Disconnect(sessionId);
        }
    }
}

void AuthGroup::OnRelease(SessionId sessionId)
{
    std::unordered_map<SessionId, Player*>::iterator authPlayerIter = authPlayerMap_.find(sessionId);

    if (authPlayerIter == authPlayerMap_.end())
    {
        return;
    }

    Player* target = authPlayerIter->second;

    authPlayerMap_.erase(authPlayerIter);

    GameEchoServer* gameEchoServer = static_cast<GameEchoServer*>(contentsServer_);
    gameEchoServer->FreePlayer(target);

}

void AuthGroup::OnUpdate()
{
    InterlockedIncrement(&updateCount_);
}

int AuthGroup::GetPlayerNum()
{
    return static_cast<int>(authPlayerMap_.size());
}

int AuthGroup::GetFPS()
{
    return updateTps_;
}

void AuthGroup::OnInitializeTPS()
{
    updateTps_ = updateCount_;
    InterlockedExchange(&updateCount_, 0);
}

EchoGroup::EchoGroup(GroupId groupId, DWORD frameMs)
{
    frameMS_ = frameMs;
    groupId_ = groupId;
}

EchoGroup::~EchoGroup()
{
}

void EchoGroup::OnEnter(SessionId sessionId)
{
    Player* target = static_cast<Player*>(contentsServer_->GetPlayerPointer(sessionId));

    if (target == nullptr)
    {
        return;
    }

    echoPlayerMap_.insert(std::unordered_map<SessionId, Player*>::value_type(target->sessionId_, target));

    ContentsCPacket loginPacket = ContentsCPacket::MakeContentsPacket();

    loginPacket << PacketScGameResLogin << GameLoginStatusOk << target->accountNo_;

    contentsServer_->SendPacket(target->sessionId_, loginPacket);
}

void EchoGroup::OnLeave(SessionId sessionId)
{
    std::unordered_map<SessionId, Player*>::iterator echoPlayerIter = echoPlayerMap_.find(sessionId);

    if (echoPlayerIter == echoPlayerMap_.end())
    {
        DebugBreak();
        return;
    }

    echoPlayerMap_.erase(echoPlayerIter);
}

void EchoGroup::OnMessage(SessionId sessionId, ContentsCPacket* packet)
{
    std::unordered_map<SessionId, Player*>::iterator echoPlayerIter = echoPlayerMap_.find(sessionId);

    if (echoPlayerIter == echoPlayerMap_.end())
    {
        DebugBreak();
        return;
    }

    Player* target = echoPlayerIter->second;
    unsigned short messageType;

    ContentsCPacket contentsPacket = packet;

    while (contentsPacket.GetDataSize() > 0)
    {
        int packetSize = 0;

        contentsPacket >> packetSize;
        contentsPacket >> messageType;

        if (messageType == PacketCsGameReqEcho)
        {
            if (packetSize != PacketGameReqEchoSize)
            {
                contentsServer_->Disconnect(sessionId);
                return;
            }

            __int64 accountNo;
            long long sendTick;

            contentsPacket >> accountNo;
            contentsPacket >> sendTick;

            if (target->accountNo_ != accountNo)
            {
                contentsServer_->Disconnect(sessionId);
                return;
            }

            ContentsCPacket echoPacket = ContentsCPacket::MakeContentsPacket();

            echoPacket << PacketScGameResEcho << target->accountNo_ << sendTick;

            contentsServer_->SendPacket(target->sessionId_, echoPacket);
        }
        else if (messageType == PacketCsGameReqHeartbeat)
        {
            if (packetSize != PacketGameReqHeartbeatSize)
            {
                contentsServer_->Disconnect(sessionId);
                return;
            }
        }
        else
        {
            contentsServer_->Disconnect(sessionId);
        }
    }
}

void EchoGroup::OnUpdate()
{
    InterlockedIncrement(&updateCount_);
}

void EchoGroup::OnRelease(SessionId sessionId)
{

    std::unordered_map<SessionId, Player*>::iterator echoPlayerIter = echoPlayerMap_.find(sessionId);

    if (echoPlayerIter == echoPlayerMap_.end())
    {
        return;
    }

    Player* target = echoPlayerIter->second;

    echoPlayerMap_.erase(echoPlayerIter);

    GameEchoServer* gameEchoServer = static_cast<GameEchoServer*>(contentsServer_);
    gameEchoServer->FreePlayer(target);
}


int EchoGroup::GetPlayerNum()
{
    return static_cast<int>(echoPlayerMap_.size());
}

int EchoGroup::GetFPS()
{
    return updateTps_;
}

void EchoGroup::OnInitializeTPS()
{
    updateTps_ = updateCount_;
    InterlockedExchange(&updateCount_, 0);
}