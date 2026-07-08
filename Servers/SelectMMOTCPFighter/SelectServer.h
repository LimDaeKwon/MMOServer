#pragma once

#include <WinSock2.h>
#include "Session.h"
#include <unordered_map>
#include <vector>
#include "ObjectFreeList.h"
#include "ServerStartConfig.h"
#include "stack"
using SessionId = unsigned __int64;
class CPacket;

class SelectServer
{
public:
    SelectServer();
	virtual ~SelectServer();

	bool Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int maxSessionCount, unsigned char packetCode, unsigned int frameMs);
    bool Start(const DKServerCore::SelectServerStartConfig& config);

    void Network();
    void SendPacket(SessionId sessionId, CPacket* packet);
    void Disconnect(SessionId sessionId);

    unsigned int GetSessionCount() const;
    unsigned int GetAcceptTPS() const;
    unsigned int GetRecvPacketTPS() const;
    unsigned int GetSendPacketTPS() const;
    unsigned int GetSendCompleteTPS() const;
    unsigned int GetDisconnectTPS() const;
    unsigned int GetReleaseTPS() const;
    unsigned int GetFrameTPS() const;


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
    void ProcessNetworkBatch(Session** sessionBatch, int sessionCount);


    void Receive(Session* target);
    void SendAll(Session* target);
	bool TryUpdate();
    void InitOldTick();

   

    static unsigned int WINAPI GameLoopThread(void* thisPointer);
    HANDLE gameLoopThread_;
	SOCKET listenSocket_;
    std::unordered_map<SessionId, Session*> sessions_;
    ObjectFreeList<Session> sessionFreeList_;
   
	std::stack<Session*> freeSessionStack_;

    std::vector<Session*> pendingAcceptSessions_;
	SessionId sessionId_;
    unsigned int frameMs_;
	unsigned int oldTick_;
    unsigned int maxSessionCount_;
    unsigned char packetCode_;


    unsigned int acceptCount_ = 0;
    unsigned int recvPacketCount_ = 0;
    unsigned int sendPacketCount_ = 0;
    unsigned int sendCompleteCount_ = 0;
    unsigned int disconnectCount_ = 0;
    unsigned int releaseCount_ = 0;
    unsigned int frameCount_ = 0;

    unsigned int acceptTPS_ = 0;
    unsigned int recvPacketTPS_ = 0;
    unsigned int sendPacketTPS_ = 0;
    unsigned int sendCompleteTPS_ = 0;
    unsigned int disconnectTPS_ = 0;
    unsigned int releaseTPS_ = 0;
    unsigned int frameTPS_ = 0;

    DWORD lastMonitorTick_ = 0;


    

};
