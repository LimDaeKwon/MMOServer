#pragma once

#include <WinSock2.h>
#include "Session.h"
#include <unordered_map>
#include <vector>
#include "ObjectFreeList.h"
#include "ServerStartConfig.h"

using SessionId = unsigned __int64;
class CPacket;



class SelectServerPQ
{
public:
    SelectServerPQ();
    virtual ~SelectServerPQ();

    bool Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int maxSessionCount, unsigned char packetCode, unsigned int frameMs);
    bool Start(const DKServerCore::SelectServerStartConfig& config);

    void Network();
    void SendPacket(SessionId sessionId, CPacket* packet);
    void Disconnect(SessionId sessionId);

protected:
    virtual void OnAccept(SessionId sessionId) = 0;
    virtual void OnMessage(SessionId sessionId, unsigned char packetType, CPacket* packet) = 0;
    virtual void OnRelease(SessionId sessionId) = 0;
    virtual void OnUpdate() = 0;
    void SetProfileEnabled();

private:
    void AcceptClient();
    void CommitAcceptedClients();
    void DeleteDisconnect();
    void TimeOut();
    void ProcessNetworkBatch(SessionPQ** sessionBatch, int sessionCount);


    void Receive(SessionPQ* target);
    void SendAll(SessionPQ* target);
    bool TryUpdate();
    void InitOldTick();



    static unsigned int WINAPI GameLoopThread(void* thisPointer);
    HANDLE gameLoopThread_;
    SOCKET listenSocket_;
    std::unordered_map<SessionId, SessionPQ*> sessions_;
    std::vector<SessionPQ*> pendingAcceptSessions_; 
    SessionId sessionId_;
    unsigned int frameMs_;
    unsigned int oldTick_;
    unsigned int maxSessionCount_;
    unsigned char packetCode_;



};
