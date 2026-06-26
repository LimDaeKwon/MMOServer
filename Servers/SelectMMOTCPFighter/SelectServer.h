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

    

};
