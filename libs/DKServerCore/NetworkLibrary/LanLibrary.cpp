#include "LanLibrary.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <iostream>
#include <process.h>
#include <stdlib.h>
#include <unordered_map>

#include "Profiler.h"
#include "CPacket.h"
#include "CPacketQueue.h"
#include "ContentsCPacket.h"

#pragma comment(lib, "ws2_32.lib")

unsigned int __stdcall LanLibrary::AcceptThread(void* thisPointer)
{
    LanLibrary* thisForAccept = static_cast<LanLibrary*>(thisPointer);

    while (true)
    {
        SOCKET clientSock = accept(thisForAccept->listenSock_, nullptr, nullptr);

        if (clientSock == INVALID_SOCKET)
        {
            int error = WSAGetLastError();

            if (error == 10004)
            {
                break;
            }

            wprintf(L"accept Error %d ", error);
        }

        Session* newSession = thisForAccept->SessionAlloc(thisForAccept->FindEmptySession(), clientSock);

        if (newSession == nullptr)
        {
            thisForAccept->dcSessionFull_++;
            closesocket(clientSock);
            continue;
        }

        InterlockedIncrement(&thisForAccept->acceptCount_);

        CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(clientSock),
            thisForAccept->handleIocp_,
            reinterpret_cast<ULONG_PTR>(newSession),
            0);

        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        getpeername(clientSock, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

        WCHAR addr[INET_ADDRSTRLEN];

        if (InetNtopW(AF_INET, &clientAddr.sin_addr, addr, INET_ADDRSTRLEN) == nullptr)
        {
            wprintf(L"InetNtop Error \n");
            DebugBreak();
        }

        thisForAccept->OnAccept(addr, ntohs(clientAddr.sin_port), newSession->sessionId_);

        thisForAccept->ReceiveFirst(newSession);
    }

    return 0;
}

unsigned int __stdcall LanLibrary::WorkerThread(void* thisPointer)
{
    LanLibrary* thisForWorker = static_cast<LanLibrary*>(thisPointer);

    while (true)
    {
        DWORD cbTransferred = 0;
        MyOverlapped* overlapPointer = nullptr;
        Session* target = nullptr;

        int retval = GetQueuedCompletionStatus(
            thisForWorker->handleIocp_,
            &cbTransferred,
            reinterpret_cast<PULONG_PTR>(&target),
            reinterpret_cast<LPOVERLAPPED*>(&overlapPointer),
            INFINITE);

        if (overlapPointer == nullptr && cbTransferred == 0 && target == nullptr)
        {
            PostQueuedCompletionStatus(thisForWorker->handleIocp_, 0, 0, nullptr);
            break;
        }

        if (cbTransferred == 0)
        {
        }
        else if (retval == 0)
        {
            int error = WSAGetLastError();

            if (!(error == 64 || error == 995 || error == 1236 || error == 121))
            {
            }
        }
        else
        {
            if (overlapPointer == nullptr && cbTransferred == 2 && target != nullptr)
            {
                thisForWorker->SendPost(target);
                continue;
            }
            else if (overlapPointer == nullptr && cbTransferred == 1 && target != nullptr)
            {
                thisForWorker->Release(target);
                continue;
            }
            else if (overlapPointer->type_ == DKServerCore::RecvIoType)
            {
                thisForWorker->RecvCompletion(target, cbTransferred);
            }
            else if (overlapPointer->type_ == DKServerCore::SendIoType)
            {
                thisForWorker->SendCompletion(target);
            }
        }

        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            thisForWorker->Release(target);
        }
    }

    return 0;
}

unsigned int __stdcall LanLibrary::MonitorThread(void* thisPointer)
{
    LanLibrary* thisForMonitor = static_cast<LanLibrary*>(thisPointer);

    unsigned int oldTick = timeGetTime();

    while (true)
    {
        thisForMonitor->acceptTps_ = thisForMonitor->acceptCount_;
        thisForMonitor->recvMessageTps_ = thisForMonitor->recvMessageCount_;
        thisForMonitor->sendMessageTps_ = thisForMonitor->sendMessageCount_;

        InterlockedExchange(&thisForMonitor->acceptCount_, 0);
        InterlockedExchange(&thisForMonitor->recvMessageCount_, 0);
        InterlockedExchange(&thisForMonitor->sendMessageCount_, 0);

        thisForMonitor->OnInitializeTPS();

        unsigned int tick = timeGetTime();
        unsigned int frame = tick - oldTick;
        oldTick += 1000;

        if (frame < 1000)
        {
            Sleep(1000 - frame);
        }
    }

    return 0;
}

unsigned int __stdcall LanLibrary::HeartbeatThread(void* thisPointer)
{
    LanLibrary* thisForHeartbeat = static_cast<LanLibrary*>(thisPointer);

    unsigned int localCount = 0;

    while (true)
    {
        localCount++;
        Sleep(1000);
    }

    return 0;
}

LanLibrary::LanLibrary()
    : acceptTps_(0)
    , recvMessageTps_(0)
    , sendMessageTps_(0)
    , acceptCount_(0)
    , recvMessageCount_(0)
    , sendMessageCount_(0)
    , maxSession_(0)
    , sessionNum_(0)
    , threadsNum_(0)
    , uniqueId_(0)
    , indexList_(0, false)
{
}

LanLibrary::~LanLibrary()
{
}

bool LanLibrary::Start(
    const char* serverIp,
    unsigned int serverPort,
    unsigned int workerNum,
    unsigned int concurrentThreads,
    unsigned int nagle,
    unsigned int sessions,
    unsigned int header)
{
    maxSession_ = sessions;
    sessionNum_ = 0;
    sessionArray_ = new Session[maxSession_];
    headerSize_ = header;

    int** temp = new int* [maxSession_];

    timeout_ = 30;
    unloginTimeout_ = 3;

    for (unsigned int i = 0; i < maxSession_; ++i)
    {
        temp[i] = indexList_.Alloc();
        *temp[i] = i;
    }

    for (unsigned int i = 0; i < maxSession_; ++i)
    {
        indexList_.Free(temp[i]);
    }

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        DebugBreak();
    }

    handleIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrentThreads);

    if (handleIocp_ == nullptr)
    {
        DebugBreak();
    }

    listenSock_ = socket(AF_INET, SOCK_STREAM, 0);

    if (listenSock_ == INVALID_SOCKET)
    {
        int error = WSAGetLastError();
        wprintf(L"ListenSocket Error %d \n", error);

        DebugBreak();
    }

    SOCKADDR_IN serverAddress;
    ZeroMemory(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    InetPtonA(AF_INET, serverIp, &serverAddress.sin_addr);
    serverAddress.sin_port = htons(serverPort);

    int bindReturn = bind(
        listenSock_,
        reinterpret_cast<const sockaddr*>(&serverAddress),
        sizeof(serverAddress));

    if (bindReturn == SOCKET_ERROR)
    {
        bindReturn = WSAGetLastError();

        wprintf(L"BindReturn Error : %d \n", bindReturn);

        DebugBreak();
    }

    DWORD optionVal = 0;

    int socketOptionReturn = setsockopt(
        listenSock_,
        SOL_SOCKET,
        SO_SNDBUF,
        reinterpret_cast<const char*>(&optionVal),
        sizeof(optionVal));

    if (socketOptionReturn == SOCKET_ERROR)
    {
        socketOptionReturn = WSAGetLastError();

        wprintf(L"SocketOptionReturn Error : %d \n", socketOptionReturn);
        DebugBreak();
    }

    LINGER linger;
    linger.l_linger = 0;
    linger.l_onoff = 1;

    int socketOption = setsockopt(
        listenSock_,
        SOL_SOCKET,
        SO_LINGER,
        reinterpret_cast<const char*>(&linger),
        sizeof(linger));

    if (socketOption == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        printf("setsockopt Error %d ", error);

        DebugBreak();
    }

    if (nagle)
    {
        DWORD noDelay = 1;

        int noDelayOption = setsockopt(
            listenSock_,
            IPPROTO_TCP,
            TCP_NODELAY,
            reinterpret_cast<const char*>(&noDelay),
            sizeof(noDelay));

        if (noDelayOption == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            printf("setsockopt Error %d ", error);

            DebugBreak();
        }
    }

    int listenReturn = listen(listenSock_, SOMAXCONN_HINT(7000));

    if (listenReturn == SOCKET_ERROR)
    {
        listenReturn = WSAGetLastError();

        wprintf(L"Listen Error : %d \n", listenReturn);
        DebugBreak();
    }

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

    threadsNum_ = workerNum;

    threads_ = new HANDLE[threadsNum_];

    for (unsigned int i = 0; i < threadsNum_; ++i)
    {
        threads_[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr));

        if (threads_[i] == nullptr)
        {
            wprintf(L"_beginthreadex Failed \n");
            DebugBreak();
        }
    }

    acceptThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr));

    if (acceptThread_ == nullptr)
    {
        wprintf(L"_beginthreadex Failed \n");
        DebugBreak();
    }

    monitorThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, MonitorThread, this, 0, nullptr));

    if (monitorThread_ == nullptr)
    {
        wprintf(L"_beginthreadex Failed \n");
        DebugBreak();
    }

    heartbeatThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, HeartbeatThread, this, 0, nullptr));

    if (heartbeatThread_ == nullptr)
    {
        wprintf(L"_beginthreadex Failed \n");
        DebugBreak();
    }

    return true;
}

bool LanLibrary::Stop()
{
    PostQueuedCompletionStatus(handleIocp_, 0, 0, nullptr);

    WaitForMultipleObjects(threadsNum_, threads_, TRUE, INFINITE);

    closesocket(listenSock_);

    WaitForSingleObject(acceptThread_, INFINITE);

    return true;
}

void LanLibrary::Disconnect(__int64 sessionId)
{
    Session* target;
    unsigned int i = FindSession(sessionId);
    target = &sessionArray_[i];

    int localCount = InterlockedIncrement(&target->ioCount_);

    if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            Release(target);
        }

        return;
    }

    if (target->sessionId_ != sessionId)
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            Release(target);
        }

        return;
    }

    InterlockedIncrement(&disconnectCount_);
    InterlockedExchange8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 1);
    CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);

    if (InterlockedDecrement(&target->ioCount_) == 0)
    {
        Release(target);
    }
}

LanLibrary::Session* LanLibrary::SessionAlloc(int* emptyIndex, unsigned long long clientSock)
{

    if (InterlockedIncrement(&sessionNum_) > maxSession_)
    {
		InterlockedDecrement(&sessionNum_);
        return nullptr;
    }


    Session* newSession = &sessionArray_[*emptyIndex];

    newSession->index_ = emptyIndex;


    __int64 i = *newSession->index_;

    newSession->sessionId_ = ++uniqueId_;
    newSession->sessionId_ |= (i << 48);
    newSession->bufferCount_.count_ = 0;
    newSession->sock_ = static_cast<SOCKET>(clientSock);
    newSession->sendFlag_ = false;
    newSession->useFlag_ = true;
    newSession->loginFlag_ = false;
    newSession->lastRecvTime_ = GetTickCount64();

    InterlockedIncrement(&newSession->ioCount_);
    InterlockedAnd(&newSession->ioCount_, 0x7fffffff);
    InterlockedExchange8(reinterpret_cast<volatile char*>(&newSession->disconnectFlag_), 0);
    newSession->recvOverlapped_.type_ = DKServerCore::RecvIoType;
    newSession->sendOverlapped_.type_ = DKServerCore::SendIoType;

    return newSession;
}

void LanLibrary::SendCompletion(Session* target)
{
    for (int i = 0; i < target->bufferCount_.count_; ++i)
    {
        CPacket::Free(target->bufferCount_.buffers_[i]);
    }

    target->bufferCount_.count_ = 0;
    InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);
    SendPost(target);
}

void LanLibrary::RecvCompletion(Session* target, DWORD cbTransferred)
{
    target->loginFlag_ = true;
    target->recvBuffer_.MoveRear(cbTransferred);
    target->lastRecvTime_ = GetTickCount64();
    RecvProc(target);
    Receive(target);
}

void LanLibrary::SendPacket(__int64 sessionId, ContentsCPacket contentsPacket)
{
    Session* target;
    unsigned int i = FindSession(sessionId);

    target = &sessionArray_[i];

    int localCount = InterlockedIncrement(&target->ioCount_);

    if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            PostQueuedCompletionStatus(handleIocp_, 1, reinterpret_cast<ULONG_PTR>(target), nullptr);
        }

        return;
    }

    if (target->sessionId_ != sessionId)
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            PostQueuedCompletionStatus(handleIocp_, 1, reinterpret_cast<ULONG_PTR>(target), nullptr);
        }

        return;
    }

    CPacket* sendPacket = contentsPacket.packetBuffer_;

    sendPacket->IncreaseRefCount();

    int enqueueReturn = target->sendBuffer_.Enqueue(sendPacket);

    InterlockedIncrement(&target->sendCount_);

    if (enqueueReturn == false)
    {

        CPacket::Free(sendPacket);

        wprintf(L"EnqueueFail in SendPacketUnicast Session Id : %lld\n", target->sessionId_);
        Disconnect(target->sessionId_);
        InterlockedIncrement(&dcSendBufferFull_);

        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            PostQueuedCompletionStatus(handleIocp_, 1, reinterpret_cast<ULONG_PTR>(target), nullptr);
        }

        return;
    }

    PostQueuedCompletionStatus(handleIocp_, 2, reinterpret_cast<ULONG_PTR>(target), nullptr);

    if (InterlockedDecrement(&target->ioCount_) == 0)
    {
        PostQueuedCompletionStatus(handleIocp_, 1, reinterpret_cast<ULONG_PTR>(target), nullptr);
    }
}

void LanLibrary::SendPost(Session* target)
{
    if (InterlockedOr8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 0) == 1)
    {
        return;
    }

    long localCount = InterlockedIncrement(&target->ioCount_);

    if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            Release(target);
        }

        return;
    }

    int wsaSendReturn;

    if (InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 1) == 0)
    {
        WSABUF localWsaBuf[DKServerCore::MaxBatchSize];

        int bufCount = 0;

        while (bufCount < DKServerCore::MaxBatchSize)
        {
            CPacket* temp = nullptr;

            if (target->sendBuffer_.Dequeue(&temp) == false)
            {
                break;
            }

            target->bufferCount_.buffers_[bufCount] = temp;
            localWsaBuf[bufCount].buf = temp->GetBufferPtr() + DKServerCore::PacketLibHeaderSize - headerSize_;
            localWsaBuf[bufCount].len = temp->GetDataSize() + headerSize_;
            bufCount++;
        }

        target->bufferCount_.count_ = bufCount;

        if (bufCount == 0)
        {
            InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);

            if (target->sendBuffer_.GetSize() != 0)
            {
                SendPost(target);
            }

            if (InterlockedDecrement(&target->ioCount_) == 0)
            {
                Release(target);
            }

            return;
        }

        InterlockedAdd(&sendMessageCount_, target->sendCount_);
        InterlockedExchange(&target->sendCount_, 0);

        DWORD sendBytes = 0;
        ZeroMemory(&target->sendOverlapped_.overlapped_, sizeof(target->sendOverlapped_.overlapped_));

        wsaSendReturn = WSASend(target->sock_,localWsaBuf,bufCount,&sendBytes,0,&target->sendOverlapped_.overlapped_,nullptr);

        if (wsaSendReturn == SOCKET_ERROR)
        {
            int wsaSendError = WSAGetLastError();

            if (wsaSendError == WSA_IO_PENDING)
            {
                if (InterlockedOr8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 0) == 1)
                {
                    CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);
                }
            }

            if (wsaSendError != WSA_IO_PENDING)
            {
                if (wsaSendError == 10038)
                {
                }

                if (InterlockedDecrement(&target->ioCount_) == 0)
                {
                    Release(target);
                    return;
                }
            }
        }
    }
    else
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            Release(target);
        }
    }
}

void LanLibrary::ReceiveFirst(Session* newSession)
{
    WSABUF wsaBuf;
    wsaBuf.buf = newSession->recvBuffer_.GetRearBufferPtr();
    wsaBuf.len = newSession->recvBuffer_.GetFreeSize();

    DWORD recvBytes;
    DWORD flags = 0;

    ZeroMemory(&newSession->recvOverlapped_.overlapped_, sizeof(newSession->recvOverlapped_.overlapped_));

    int retval = WSARecv(newSession->sock_,&wsaBuf,1,&recvBytes,&flags,&newSession->recvOverlapped_.overlapped_,0);

    if (retval == SOCKET_ERROR)
    {
        int wsaRecvError = WSAGetLastError();

        if (wsaRecvError != WSA_IO_PENDING)
        {
            wprintf(L"In First WSARecvError : %d  , Session ID : %lld\n", wsaRecvError, newSession->sessionId_);

            if (InterlockedDecrement(&newSession->ioCount_) == 0)
            {
                Release(newSession);
            }
        }
    }
}

void LanLibrary::RecvProc(Session* target)
{
    while (true)
    {
        int targetRecvBufferSize = target->recvBuffer_.GetUseSize();
        PacketHeader header;

        if (targetRecvBufferSize < sizeof(PacketHeader))
        {
            break;
        }

        if (target->recvBuffer_.Peek(reinterpret_cast<char*>(&header), sizeof(PacketHeader)) != sizeof(PacketHeader))
        {
            break;
        }

        if (targetRecvBufferSize < sizeof(header) + header.length_)
        {
            break;
        }

        CPacket* packetBuffer = CPacket::Alloc();
        packetBuffer->InitLan();

        unsigned int receiveDequeuePacketSize = target->recvBuffer_.Dequeue(packetBuffer->GetBufferPtr(),header.length_ + 2);

        packetBuffer->IncreaseRefCount();
        packetBuffer->MoveWritePosition(receiveDequeuePacketSize - 2);

        InterlockedIncrement(&recvMessageCount_);

        OnMessage(target->sessionId_, reinterpret_cast<ContentsCPacket*>(packetBuffer));

        CPacket::Free(packetBuffer);
    }
}

void LanLibrary::Receive(Session* target)
{
    if (InterlockedOr8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 0) == 1)
    {
        return;
    }

    WSABUF recvWsaBuf[2];

    recvWsaBuf[0].buf = target->recvBuffer_.GetRearBufferPtr();
    recvWsaBuf[0].len = target->recvBuffer_.DirectEnqueueSize();
    recvWsaBuf[1].buf = target->recvBuffer_.GetStartBufferPtr();
    recvWsaBuf[1].len = target->recvBuffer_.GetFreeSize() - target->recvBuffer_.DirectEnqueueSize();

    DWORD recvBytes;
    DWORD flags = 0;

    ZeroMemory(&target->recvOverlapped_.overlapped_, sizeof(target->recvOverlapped_.overlapped_));

    InterlockedIncrement(&target->ioCount_);

    int retval = WSARecv(target->sock_,recvWsaBuf,2,&recvBytes,&flags,&target->recvOverlapped_.overlapped_,0);

    if (retval == SOCKET_ERROR)
    {
        int wsaRecvError = WSAGetLastError();

        if (wsaRecvError == WSA_IO_PENDING)
        {
            if (InterlockedOr8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 0) == 1)
            {
                CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);
            }
        }
        else
        {
            if (InterlockedDecrement(&target->ioCount_) == 0)
            {
                Release(target);
            }
        }
    }
}

void LanLibrary::AddHeader(CPacket* packetBuffer)
{
    char* temp = packetBuffer->GetBufferPtr();
    temp += DKServerCore::PacketLibHeaderSize - headerSize_;

    PacketHeader libHeader;
    libHeader.length_ = static_cast<unsigned short>(packetBuffer->GetDataSize());

    *reinterpret_cast<unsigned short*>(temp) = libHeader.length_;
}

void LanLibrary::Release(Session* target)
{
    if (InterlockedCompareExchange(&target->ioCount_, DKServerCore::ReleaseFlag, 0) != 0)
    {
        return;
    }

    for (int i = 0; i < target->bufferCount_.count_; ++i)
    {
        CPacket::Free(target->bufferCount_.buffers_[i]);
    }

    target->bufferCount_.count_ = 0;

    target->recvBuffer_.ClearBuffer();
    ClearSendBuffer(target);

    InterlockedExchange8(reinterpret_cast<volatile char*>(&target->useFlag_), 0);
    InterlockedExchange8(reinterpret_cast<volatile char*>(&target->loginFlag_), 0);

    closesocket(target->sock_);
    target->sock_ = INVALID_SOCKET;

    OnRelease(target->sessionId_);

    indexList_.Free(target->index_);
    InterlockedDecrement(&sessionNum_);

    return;
}

int* LanLibrary::FindEmptySession()
{
    int* tempIndex = indexList_.Alloc();
    return tempIndex;
}

void LanLibrary::ClearSendBuffer(Session* target)
{
    while (true)
    {
        CPacket* temp;

        if (target->sendBuffer_.Dequeue(&temp) == false)
        {
            break;
        }

        CPacket::Free(temp);
    }
}

int LanLibrary::FindSession(__int64 sessionId)
{
    return static_cast<int>(sessionId >> 48);
}

int LanLibrary::GetAcceptTPS()
{
    return acceptTps_;
}

int LanLibrary::GetRecvMessageTPS()
{
    return recvMessageTps_;
}

int LanLibrary::GetSendMessageTPS()
{
    return sendMessageTps_;
}

DWORD LanLibrary::GetDisconnectCount()
{
    return disconnectCount_;
}

DWORD LanLibrary::GetDCUnloginTimeout()
{
    return dcUnloginTimeout_;
}

DWORD LanLibrary::GetDCLoginTimeout()
{
    return dcLoginTimeout_;
}

DWORD LanLibrary::GetDCSendBufferFull()
{
    return dcSendBufferFull_;
}

DWORD LanLibrary::GetDCPacketCodeError()
{
    return dcPacketCodeError_;
}

DWORD LanLibrary::GetDCDecodeError()
{
    return dcDecodeError_;
}

DWORD LanLibrary::GetDCSessionFull()
{
    return dcSessionFull_;
}

DWORD LanLibrary::GetDCImpossiblePacketLength()
{
    return dcImpossiblePacketLength_;
}

DWORD LanLibrary::GetSessionNum()
{
    return sessionNum_;
}