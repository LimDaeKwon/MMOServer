#include "SelectServer.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include "GameDefine.h"
#include "CPacket.h"

SelectServer::SelectServer() :
    gameLoopThread_(nullptr),
    listenSocket_(INVALID_SOCKET),
    sessionFreeList_(DefaultMaxSessionCount),
    sessionId_(1),
    frameMs_(0),
    oldTick_(0),
    maxSessionCount_(DefaultMaxSessionCount),
    packetCode_(PacketCode)
{
}

SelectServer::~SelectServer()
{
}

bool SelectServer::Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int maxSessionCount, unsigned char packetCode, unsigned int frameMs)
{
    WSADATA wsa;

    int startupResult = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (startupResult != 0)
    {
        wprintf(L"WSAStartup %d \n", startupResult);
        DebugBreak();
    }
    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket_ == INVALID_SOCKET)
    {
        int error = WSAGetLastError();

        wprintf(L"ListenSocket Error %d \n", error);

        DebugBreak();
    }

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    InetPtonA(AF_INET, serverIp, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(serverPort);

    int bindReturn = bind(listenSocket_, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr));

    if (bindReturn == SOCKET_ERROR)
    {
        bindReturn = WSAGetLastError();

        wprintf(L"bind Error : %d \n", bindReturn);

        DebugBreak();
    }

    LINGER linger;
    linger.l_linger = 0;
    linger.l_onoff = 1;

    int socketOption = setsockopt(listenSocket_, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&linger), sizeof(linger));

    if (socketOption == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        printf("setsockopt Error %d ", error);

        DebugBreak();
    }

    DWORD noDelay = 0;

    if (nagle == 0)
    {
        noDelay = 1;
    }

    int noDelayOption = setsockopt(listenSocket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

    if (noDelayOption == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        printf("setsockopt Error %d ", error);

        DebugBreak();
    }

    int listenReturn = listen(listenSocket_, SOMAXCONN_HINT(7000));

    if (listenReturn == SOCKET_ERROR)
    {
        listenReturn = WSAGetLastError();

        wprintf(L"Listen Error : %d \n", listenReturn);

        DebugBreak();
    }

    u_long on = 1;
    int ioctlSocketError = ioctlsocket(listenSocket_, FIONBIO, &on);

    if (ioctlSocketError == INVALID_SOCKET)
    {
        ioctlSocketError = WSAGetLastError();

        wprintf(L"IoctlSocketError Error : %d \n", ioctlSocketError);

        DebugBreak();
    }

    maxSessionCount_ = maxSessionCount;
    packetCode_ = packetCode;
	frameMs_ = frameMs;
    gameLoopThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, GameLoopThread, this, 0, nullptr));





	return true;
}

void SelectServer::Network()
{
    fd_set readSet;
    fd_set writeSet;

    timeval timeValue;
    timeValue.tv_sec = 0;
    timeValue.tv_usec = 0;

    int count = 0;

    unsigned int startId;

    if (!sessions_.size())
    {
        AcceptClient();
    }

    std::unordered_map<unsigned int, Session*>::iterator iter;
    iter = sessions_.begin();

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_SET(listenSocket_, &readSet);

    Session* target;

    for (; iter != sessions_.end(); ++iter)
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
                if (FD_ISSET(listenSocket_, &readSet))
                {
                    AcceptClient();
                }

                std::unordered_map<unsigned int, Session*>::iterator iterSession = sessions_.find(startId);

                for (; iterSession != sessions_.end(); ++iterSession)
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
            FD_SET(listenSocket_, &readSet);

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
            if (FD_ISSET(listenSocket_, &readSet))
            {
                AcceptClient();
            }

            std::unordered_map<unsigned int, Session*>::iterator iterSession = sessions_.find(startId);

            for (; iterSession != sessions_.end(); ++iterSession)
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

    DeleteDisconnect();


}

void SelectServer::AcceptClient()
{
    int acceptError;
    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);

    SOCKET clientSocket = accept(listenSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

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

    if (sessions_.size() >= maxSessionCount_)
    {
        closesocket(clientSocket);
        return;
    }

    Session* newSession = sessionFreeList_.Alloc();

    newSession->sessionId_ = sessionId_++;
    newSession->socket_ = clientSocket;
    newSession->isDelete_ = 0;
    newSession->lastRecvTime_ = timeGetTime();

    sessions_.insert(std::unordered_map<unsigned int, Session*>::value_type(newSession->sessionId_, newSession));

	OnAccept(newSession->sessionId_);
}

void SelectServer::SendPacket(SessionId sessionId, CPacket* packet)
{
    Session* target = sessions_.at(sessionId);

    if (target->isDelete_)
    {
        return;
    }

    int dataSize = packet->GetDataSize();
    int enqueueHeaderReturn = target->sendQueue_.Enqueue(packet->GetBufferPtr() + LibraryHeaderSize, dataSize);

    if (enqueueHeaderReturn != dataSize)
    {
        wprintf(L"EnqueueFail in SendPacketUnicast %d \n ", target->sessionId_);

        Disconnect(target->sessionId_);
    }
}

void SelectServer::Disconnect(SessionId sessionId)
{
    Session* target = sessions_.at(sessionId);
    if (target->isDelete_ == 1)
    {
        return;
    }
    target->isDelete_ = 1;
}

void SelectServer::DeleteDisconnect()
{
    std::unordered_map<unsigned int, Session*>::iterator iter = sessions_.begin();

    while (iter != sessions_.end())
    {
        Session* session = iter->second;

        if (session->isDelete_ == 0)
        {
            ++iter;
            continue;
        }

        SessionId sessionId = session->sessionId_;
        closesocket(session->socket_);
        session->receiveQueue_.ClearBuffer();
        session->sendQueue_.ClearBuffer();
		OnRelease(sessionId);
        sessionFreeList_.Free(session);
        iter = sessions_.erase(iter);
    }
}

void SelectServer::TimeOut()
{
    unsigned int currentTime = timeGetTime();
    std::unordered_map<unsigned int, Session*>::iterator iter;
    for (iter = sessions_.begin(); iter != sessions_.end(); ++iter)
    {
        Session* target = iter->second;
        if (target->isDelete_ == 1)
        {
            continue;
        }
        if (currentTime - target->lastRecvTime_ > NetworkPacketRecvTimeout)
        {
            Disconnect(target->sessionId_);
        }
    }
}

void SelectServer::Receive(Session* target)
{

    int directEnqueueSize = target->receiveQueue_.DirectEnqueueSize();

    int recvError;
    int recvReturn = recv(target->socket_, target->receiveQueue_.GetRearBufferPtr(), directEnqueueSize, 0);

    if (recvReturn == SOCKET_ERROR)
    {
        recvError = WSAGetLastError();

        if (recvError != WSAEWOULDBLOCK)
        {
            Disconnect(target->sessionId_);
            return;
        }
    }

    if (recvReturn == 0)
    {
        Disconnect(target->sessionId_);
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

            if (header.byCode_ != packetCode_)
            {
                wprintf(L"Header.byCode_ != PacketCode\n");

                Disconnect(target->sessionId_);
                break;
            }

            if (receiveQueueSize < sizeof(header) + header.bySize_)
            {
                break;
            }

            target->receiveQueue_.MoveFront(sizeof(header));
            
            CPacket* packetBuffer = CPacket::Alloc();

            unsigned int receiveQueueDequeuePacketSize =
                target->receiveQueue_.Dequeue(packetBuffer->GetBufferPtr() + LibraryHeaderSize, header.bySize_);

            if (receiveQueueDequeuePacketSize != header.bySize_)
            {
                wprintf(L"## ReceiveQDequeuePacketSize != header.bySize_ : %d \n", receiveQueueDequeuePacketSize);

                CPacket::Free(packetBuffer);
                Disconnect(target->sessionId_);
                break;
            }

            packetBuffer->MoveWritePosition(receiveQueueDequeuePacketSize);

            target->lastRecvTime_ = timeGetTime();

			OnMessage(target->sessionId_, header.byType_, packetBuffer);
            CPacket::Free(packetBuffer);
        }
    }


}

void SelectServer::SendAll(Session* target)
{
    int directDequeueSize = target->sendQueue_.DirectDequeueSize();

    int sendReturn = send(target->socket_, target->sendQueue_.GetFrontBufferPtr(), directDequeueSize, 0);

    if (sendReturn == SOCKET_ERROR)
    {
        int error = WSAGetLastError();

        if (error != WSAEWOULDBLOCK)
        {
            Disconnect(target->sessionId_);
            return;
        }
    }

    if (sendReturn != directDequeueSize)
    {
        wprintf(L"SendSize : %d , SendReturn : %d \n", directDequeueSize, sendReturn);

        Disconnect(target->sessionId_);
        return;
    }

    int moveFrontSize = target->sendQueue_.MoveFront(directDequeueSize);

    if (moveFrontSize != directDequeueSize)
    {
        wprintf(L"MoveFrontSize : %d , DirectDequeueSize : %d \n", moveFrontSize, directDequeueSize);
        DebugBreak();
    }

}

bool SelectServer::TryUpdate()
{

    DWORD tick = timeGetTime();

    unsigned int frame = tick - oldTick_;
    if (frame > frameMs_)
    {
		TimeOut();
        unsigned int fixUpdate = (frame / 40);

        for (unsigned int i = 0; i < fixUpdate; ++i)
        {
			OnUpdate();
        }

        oldTick_ += (frameMs_ * (frame / frameMs_));
    }

    return false;
}

void SelectServer::InitOldTick()
{
	oldTick_ = timeGetTime();
}

unsigned int __stdcall SelectServer::GameLoopThread(void* thisPointer)
{
	SelectServer* thisForGameLoop = static_cast<SelectServer*>(thisPointer);
    thisForGameLoop->InitOldTick();

    while (true)
    {
        thisForGameLoop->Network();
        
        thisForGameLoop->TryUpdate();
        //ServerControl();

    }


}

