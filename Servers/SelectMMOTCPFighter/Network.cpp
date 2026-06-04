

#include "Network.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Character.h"
#include "CPacket.h"
#include "Contents.h"
#include "GameDefine.h"
#include "NetworkProxy.h"
#include "NetworkStub.h"
#include "ObjectFreeList.h"
#include "PacketDefine.h"
#include "RingBuffer.h"
#include "Sector.h"
#include "Session.h"



#include <conio.h>
#include <iostream>
#include <list>
#include <unordered_map>

#pragma comment(lib, "Ws2_32.lib")

unsigned int sessionId = 0;
SOCKET listenSocket = INVALID_SOCKET;
std::unordered_map<unsigned int, Session*> sessions;
ObjectFreeList<Session> sessionFreeList(10000);

extern std::list<unsigned int> deleteList;
extern std::unordered_map<unsigned int, Character*> characterMap;
extern std::list<Character*> sector[SectorMaxY][SectorMaxX];

CPacket* cPacketBuffer = CPacket::Alloc();

void Network()
{
    fd_set readSet;
    fd_set writeSet;

    timeval timeValue;
    timeValue.tv_sec = 0;
    timeValue.tv_usec = 0;

    int count = 0;

    unsigned int startId;

    if (!sessions.size())
    {
        AcceptClient();
    }

    std::unordered_map<unsigned int, Session*>::iterator iter;
    iter = sessions.begin();

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_SET(listenSocket, &readSet);

    Session* target;

    for (; iter != sessions.end(); ++iter)
    {
        target = iter->second;

        if (count == 0)
        {
            startId = target->sessionId_;
        }

        FD_SET(target->socket_, &readSet);

        if (target->sendQueue_.GetUseSize() > 0)
        {
            FD_SET(target->socket_, &writeSet);
        }

        ++count;

        if (count == 63)
        {
            timeval timeValue;
            timeValue.tv_sec = 0;
            timeValue.tv_usec = 0;

            int selectReturn = select(0, &readSet, &writeSet, nullptr, &timeValue);

            if (selectReturn == SOCKET_ERROR)
            {
                selectReturn = WSAGetLastError();
                wprintf(L"Select Error : %d", selectReturn);
                DebugBreak();
            }

            if (selectReturn > 0)
            {
                if (FD_ISSET(listenSocket, &readSet))
                {
                    AcceptClient();
                }

                std::unordered_map<unsigned int, Session*>::iterator iterSession = sessions.find(startId);

                for (; iterSession != sessions.end(); ++iterSession)
                {
                    if (selectReturn == 0)
                    {
                        break;
                    }

                    target = iterSession->second;

                    if (target->isDelete_ == 1)
                    {
                        continue;
                    }

                    if (FD_ISSET(target->socket_, &readSet))
                    {
                        Receive(target);
                        --selectReturn;
                    }

                    if (FD_ISSET(target->socket_, &writeSet))
                    {
                        SendAll(target);
                        --selectReturn;
                    }
                }
            }

            FD_ZERO(&readSet);
            FD_ZERO(&writeSet);
            FD_SET(listenSocket, &readSet);

            count = 0;
        }
    }

    if (count > 0)
    {
        timeval timeValue;
        timeValue.tv_sec = 0;
        timeValue.tv_usec = 0;

        int selectReturn = select(0, &readSet, &writeSet, nullptr, &timeValue);

        if (selectReturn == SOCKET_ERROR)
        {
            selectReturn = WSAGetLastError();
            wprintf(L"Select Error : %d", selectReturn);
            DebugBreak();
        }

        if (selectReturn > 0)
        {
            if (FD_ISSET(listenSocket, &readSet))
            {
                AcceptClient();
            }

            std::unordered_map<unsigned int, Session*>::iterator iterSession = sessions.find(startId);

            for (; iterSession != sessions.end(); ++iterSession)
            {
                if (selectReturn == 0)
                {
                    break;
                }

                target = iterSession->second;

                if (target->isDelete_ == 1)
                {
                    continue;
                }

                if (FD_ISSET(target->socket_, &readSet))
                {
                    Receive(target);
                    --selectReturn;
                }

                if (FD_ISSET(target->socket_, &writeSet))
                {
                    SendAll(target);
                    --selectReturn;
                }
            }
        }
    }
}

void AcceptClient()
{
    int acceptError;
    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);

    SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

    if (clientSocket == INVALID_SOCKET)
    {
        acceptError = WSAGetLastError();

        if (acceptError == WSAEWOULDBLOCK)
        {
            return;
        }

        wprintf(L"Accept Error : %d", acceptError);
        DebugBreak();
    }

    WCHAR clientIp[16] = { 0 };

    if (InetNtop(AF_INET, &clientAddr.sin_addr, clientIp, 16) == nullptr)
    {
        wprintf(L"InetNtop Error \n");
        DebugBreak();
    }

    Session* newSession = sessionFreeList.Alloc();

    newSession->sessionId_ = sessionId++;
    newSession->socket_ = clientSocket;
    newSession->isDelete_ = 0;
    newSession->lastRecvTime_ = timeGetTime();

    sessions.insert(std::unordered_map<unsigned int, Session*>::value_type(newSession->sessionId_, newSession));

    CreateCharacter(newSession);
}

void SendPacketUnicast(SessionId sessionId, CPacket* packet)
{
    Session* target = sessions.at(sessionId);

    int dataSize = packet->GetDataSize();
    int enqueueHeaderReturn = target->sendQueue_.Enqueue(packet->GetBufferPtr() + LibraryHeaderSize, dataSize);

    if (enqueueHeaderReturn != dataSize)
    {
        wprintf(L"EnqueueFail in SendPacketUnicast %d \n ", target->sessionId_);

        Disconnect(target);
    }
}

void SendPacketSectorOne(int sectorX, int sectorY, SessionId except, CPacket* packet)
{
    int dataSize = packet->GetDataSize();
    int enqueueHeaderReturn;

    Character* target;
    std::list<Character*>::iterator iter;

    for (iter = sector[sectorY][sectorX].begin(); iter != sector[sectorY][sectorX].end(); ++iter)
    {
        target = *iter;

        if ((target->sessionId_ == except) || (target->characterSession_->isDelete_ == 1))
        {
            continue;
        }

        enqueueHeaderReturn = target->characterSession_->sendQueue_.Enqueue(packet->GetBufferPtr()+ LibraryHeaderSize, dataSize);

        if (enqueueHeaderReturn != dataSize)
        {
            wprintf(L"EnqueueFail in SendPacketUnicast%d \n ", target->sessionId_);

            Disconnect(target->characterSession_);
        }
    }
}

void SendPacketAroundRemoveSector(SessionId sessionId, CPacket* packet, SectorAround* around)
{
    for (unsigned int index = 0; index < around->count_; ++index)
    {
        SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, NULL, packet);
    }
}

void SendPacketAroundAddSector(SessionId sessionId, CPacket* packet, SectorAround* around)
{
    for (unsigned int index = 0; index < around->count_; ++index)
    {
        SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, NULL, packet);
    }
}

void SendPacketAround(SessionId sessionId, CPacket* packet, bool sendMe)
{
    Character* target = characterMap.at(sessionId);
    SectorAround around;

    GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &around);

    if (sendMe)
    {
        for (unsigned int index = 0; index < around.count_; ++index)
        {
            SendPacketSectorOne(around.around_[index].x_, around.around_[index].y_, NULL, packet);
        }
    }
    else
    {
        for (unsigned int index = 0; index < around.count_; ++index)
        {
            SendPacketSectorOne(around.around_[index].x_, around.around_[index].y_, target->sessionId_, packet);
        }
    }
}

void Receive(Session* target)
{
    int directEnqueueSize = target->receiveQueue_.DirectEnqueueSize();

    int recvError;
    int recvReturn = recv(target->socket_, target->receiveQueue_.GetRearBufferPtr(), directEnqueueSize, 0);

    if (recvReturn == SOCKET_ERROR)
    {
        recvError = WSAGetLastError();

        if (recvError != WSAEWOULDBLOCK)
        {
            Disconnect(target);
            return;
        }
    }

    if (recvReturn == 0)
    {
        Disconnect(target);
        return;
    }

    target->receiveQueue_.MoveRear(recvReturn);

    unsigned int receiveQueueSize;

    if (recvReturn > 0)
    {
        while (true)
        {
            receiveQueueSize = target->receiveQueue_.GetUseSize();

            if (receiveQueueSize == 0)
            {
                break;
            }

            PacketHeader header;

            if (receiveQueueSize < sizeof(PacketHeader))
            {
                break;
            }

            if (target->receiveQueue_.Peek(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header))
            {
                break;
            }

            if (header.byCode_ != PacketCode)
            {
                wprintf(L"Header.byCode_ != PacketCode\n");

                Disconnect(target);
                break;
            }

            if (receiveQueueSize < sizeof(header) + header.bySize_)
            {
                break;
            }

            unsigned int receiveQueueDequeueHeaderSize = target->receiveQueue_.MoveFront(sizeof(header));

            cPacketBuffer->Clear();

            unsigned int receiveQueueDequeuePacketSize =
                target->receiveQueue_.Dequeue(cPacketBuffer->GetBufferPtr()+ LibraryHeaderSize, header.bySize_);

            if (receiveQueueDequeuePacketSize != header.bySize_)
            {
                wprintf(L"## ReceiveQDequeuePacketSize != header.bySize_ : %d \n", receiveQueueDequeuePacketSize);

                Disconnect(target);
                break;
            }

            cPacketBuffer->MoveWritePosition(receiveQueueDequeuePacketSize);

            target->lastRecvTime_ = timeGetTime();

            PacketProc(target->sessionId_, header.byType_, cPacketBuffer);
        }
    }
}

void ServerControl()
{
    static bool controlMode = false;

    if (_kbhit())
    {
        WCHAR controlKey = _getwch();

        if (L'u' == controlKey || L'U' == controlKey)
        {
            controlMode = true;
        }

        if ((controlMode && L'q' == controlKey) || L'q' == controlKey)
        {
        }
    }
}

void DeleteDisconnect()
{
    if (deleteList.size() > 0)
    {
        Session* session;
        Character* deleteTarget;
        unsigned int sessionId;

        std::list<unsigned int>::iterator iter;

        for (iter = deleteList.begin(); iter != deleteList.end(); ++iter)
        {
            sessionId = *iter;
            session = sessions.at(sessionId);
            deleteTarget = characterMap.at(sessionId);

            FreeCharacter(deleteTarget);

            characterMap.erase(sessionId);

            closesocket(session->socket_);
            session->receiveQueue_.ClearBuffer();
            session->sendQueue_.ClearBuffer();

            sessionFreeList.Free(session);
            sessions.erase(sessionId);
        }

        deleteList.clear();
    }
}

void SendAll(Session* target)
{
    int directDequeueSize = target->sendQueue_.DirectDequeueSize();

    int sendReturn = send(target->socket_, target->sendQueue_.GetFrontBufferPtr(), directDequeueSize, 0);

    if (sendReturn == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        if (error != WSAEWOULDBLOCK)
        {
            Disconnect(target);
            return;
        }
    }

    if (sendReturn != directDequeueSize)
    {
        wprintf(L"SendSize : %d , SendReturn : %d \n", directDequeueSize, sendReturn);

        Disconnect(target);
        return;
    }

    int moveFrontSize = target->sendQueue_.MoveFront(directDequeueSize);

    if (moveFrontSize != directDequeueSize)
    {
        wprintf(L"MoveFrontSize : %d , DirectDequeueSize : %d \n", moveFrontSize, directDequeueSize);
        DebugBreak();
    }
}

void Initialize()
{
    WSADATA wsa;

    int startupResult = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (startupResult != 0)
    {
        wprintf(L"WSAStartup %d \n", startupResult);
        DebugBreak();
    }
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET)
    {
        int error = WSAGetLastError();

        wprintf(L"ListenSocket Error %d \n", error);

        DebugBreak();
    }

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(ServerPort);

    int bindReturn = bind(listenSocket, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr));

    if (bindReturn == SOCKET_ERROR)
    {
        bindReturn = WSAGetLastError();

        wprintf(L"bind Error : %d \n", bindReturn);

        DebugBreak();
    }

    LINGER linger;
    linger.l_linger = 0;
    linger.l_onoff = 1;

    int socketOption = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&linger), sizeof(linger));

    if (socketOption == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        printf("setsockopt Error %d ", error);

        DebugBreak();
    }

    DWORD noDelay = 1;

    int noDelayOption = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

    if (noDelayOption == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        printf("setsockopt Error %d ", error);

        DebugBreak();
    }

    int listenReturn = listen(listenSocket, SOMAXCONN_HINT(7000));

    if (listenReturn == SOCKET_ERROR)
    {
        listenReturn = WSAGetLastError();

        wprintf(L"Listen Error : %d \n", listenReturn);

        DebugBreak();
    }

    u_long on = 1;
    int ioctlSocketError = ioctlsocket(listenSocket, FIONBIO, &on);

    if (ioctlSocketError == INVALID_SOCKET)
    {
        ioctlSocketError = WSAGetLastError();

        wprintf(L"IoctlSocketError Error : %d \n", ioctlSocketError);

        DebugBreak();
    }
}