#pragma once

#include <WinSock2.h>
#include "Session.h"
#include <unordered_map>
#include <list>
#include "ObjectFreeList.h"

using SessionId = unsigned int;
class CPacket;

class SelectServer
{
public:
    SelectServer();
	virtual ~SelectServer();

	bool Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int maxSessionCount, unsigned char packetCode, unsigned int frameMs);


    void Network();
    void SendPacket(SessionId sessionId, CPacket* packet);
    void Disconnect(SessionId sessionId);

protected:
    virtual void OnAccept(SessionId sessionId) = 0;
    virtual void OnMessage(SessionId sessionId, unsigned char packetType, CPacket* packet) = 0;
    virtual void OnRelease(SessionId sessionId) = 0;
    virtual void OnUpdate() = 0;

	CPacket* cPacketBuffer_;

private:
    void AcceptClient();
    void DeleteDisconnect();
    void TimeOut();

    void Receive(Session* target);
    void SendAll(Session* target);
	bool TryUpdate();
    void InitOldTick();

    static unsigned int WINAPI GameLoopThread(void* thisPointer);
    HANDLE gameLoopThread_;
	SOCKET listenSocket_;
    std::unordered_map<unsigned int, Session*> sessions_;
    std::list<unsigned int> deleteList_;
    ObjectFreeList<Session> sessionFreeList_;
	SessionId sessionId_;
    unsigned int frameMs_;
	unsigned int oldTick_;
    unsigned int maxSessionCount_;
    unsigned char packetCode_;
};
