#include "ContentsNetLibrary.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <iostream>
#include <process.h>
#include <unordered_map>

#include "Profiler.h"
#include "CPacket.h"
#include "ContentsCPacket.h"

#pragma comment(lib, "ws2_32.lib")

unsigned int __stdcall ContentsNetLibrary::AcceptThread(void* thisPointer)
{
    ContentsNetLibrary* thisForAccept = static_cast<ContentsNetLibrary*>(thisPointer);

    while (1)
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

        thisForAccept->acceptTotal_++;

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
        }

        thisForAccept->OnAccept(addr, ntohs(clientAddr.sin_port), newSession->sessionId_);

        thisForAccept->groupManager_.RegisterSession(newSession);

        thisForAccept->ReceiveFirst(newSession);
    }

    return 0;
}

unsigned int __stdcall ContentsNetLibrary::WorkerThread(void* thisPointer)
{
    ContentsNetLibrary* thisForWorker = static_cast<ContentsNetLibrary*>(thisPointer);

    while (1)
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

unsigned int __stdcall ContentsNetLibrary::MonitorThread(void* thisPointer)
{
    ContentsNetLibrary* thisForMonitor = static_cast<ContentsNetLibrary*>(thisPointer);

    unsigned int oldTick = timeGetTime();

    while (1)
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

unsigned int __stdcall ContentsNetLibrary::HeartbeatThread(void* thisPointer)
{
    ContentsNetLibrary* thisForHeartbeat = static_cast<ContentsNetLibrary*>(thisPointer);

    unsigned int localCount = 0;

    while (1)
    {
        localCount++;
        Sleep(1000);
    }

    return 0;
}

unsigned int __stdcall ContentsNetLibrary::SendThread(void* thisPointer)
{
    ContentsNetLibrary* thisForSendThread = static_cast<ContentsNetLibrary*>(thisPointer);

    while (1)
    {
        for (unsigned int i = 0; i < thisForSendThread->maxSession_; ++i)
        {
            if (thisForSendThread->sessionArray_[i].useFlag_)
            {
                thisForSendThread->SendPost(&thisForSendThread->sessionArray_[i]);
            }
        }
    }

    return 0;
}

void ContentsNetLibrary::ThreadSendPost(Session* target)
{
    int localCount = InterlockedIncrement(&target->ioCount_);

    if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            Release(target);
        }

        return;
    }

    SendPost(target);

    if (InterlockedDecrement(&target->ioCount_) == 0)
    {
        Release(target);
    }
}

ContentsNetLibrary::ContentsNetLibrary()
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

ContentsNetLibrary::~ContentsNetLibrary()
{
}

bool ContentsNetLibrary::Start(
    const char* serverIp,
    unsigned int serverPort,
    unsigned int workerNum,
    unsigned int concurrentThreads,
    unsigned int nagle,
    unsigned int sessions,
    unsigned int header,
    unsigned int sync,
    unsigned int sendThreads,
    unsigned char packetCode)
{
    maxSession_ = sessions;
    sessionNum_ = 0;
    sessionArray_ = new Session[maxSession_];
    headerSize_ = header;

    int** temp = new int* [maxSession_];

    timeout_ = 30;
    unloginTimeout_ = 3;
    packetCode_ = packetCode;

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

    if (sync)
    {
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

    sendThreads_ = sendThreads;

    for (unsigned int i = 0; i < sendThreads; ++i)
    {
        reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, SendThread, this, 0, nullptr));
    }

    return true;
}

bool ContentsNetLibrary::Stop()
{
    PostQueuedCompletionStatus(handleIocp_, 0, 0, nullptr);

    WaitForMultipleObjects(threadsNum_, threads_, TRUE, INFINITE);

    closesocket(listenSock_);

    WaitForSingleObject(acceptThread_, INFINITE);

    return true;
}

void ContentsNetLibrary::Disconnect(__int64 sessionId)
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

    InterlockedIncrement(&disconnectCount_);
    InterlockedExchange8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 1);
    CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);

    if (InterlockedDecrement(&target->ioCount_) == 0)
    {
        PostQueuedCompletionStatus(handleIocp_, 1, reinterpret_cast<ULONG_PTR>(target), nullptr);
    }
}

Session* ContentsNetLibrary::SessionAlloc(int* emptyIndex, unsigned long long clientSock)
{
    if (emptyIndex == nullptr)
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
    newSession->loginFlag_ = false;
    newSession->lastRecvTime_ = GetTickCount64();

    InterlockedIncrement(&newSession->ioCount_);
    InterlockedExchange8(reinterpret_cast<volatile char*>(&newSession->useFlag_), 1);
    InterlockedAnd(&newSession->ioCount_, 0x7fffffff);
    InterlockedExchange8(reinterpret_cast<volatile char*>(&newSession->disconnectFlag_), 0);
    newSession->recvOverlapped_.type_ = DKServerCore::RecvIoType;
    newSession->sendOverlapped_.type_ = DKServerCore::SendIoType;

    return newSession;
}

void ContentsNetLibrary::SendCompletion(Session* target)
{
    for (int i = 0; i < target->bufferCount_.count_; ++i)
    {
        CPacket::Free(target->bufferCount_.buffers_[i]);
    }

    target->bufferCount_.count_ = 0;
    InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);
    SendPost(target);
}

void ContentsNetLibrary::RecvCompletion(Session* target, DWORD cbTransferred)
{
    target->loginFlag_ = true;
    target->recvBuffer_.MoveRear(cbTransferred);
    target->lastRecvTime_ = GetTickCount64();
    RecvProc(target);
    Receive(target);
}

void ContentsNetLibrary::SendPacket(__int64 sessionId, ContentsCPacket contentsPacket)
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

    if (!sendPacket->encodingFlag_)
    {
        EnterCriticalSection(&sendPacket->encodingLock_);

        if (!sendPacket->encodingFlag_)
        {
            sendPacket->encodingFlag_ = 1;
            NetAddHeader(sendPacket);
        }

        LeaveCriticalSection(&sendPacket->encodingLock_);
    }

    int enqueueReturn = target->sendBuffer_.Enqueue(sendPacket);

    InterlockedIncrement(&target->sendCount_);

    if (enqueueReturn == false)
    {
        CPacket::Free(sendPacket);
        Disconnect(target->sessionId_);
        InterlockedIncrement(&dcSendBufferFull_);

        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            PostQueuedCompletionStatus(handleIocp_, 1, reinterpret_cast<ULONG_PTR>(target), nullptr);
        }

        return;
    }

    if (!sendThreads_)
    {
        PostQueuedCompletionStatus(handleIocp_, 2, reinterpret_cast<ULONG_PTR>(target), nullptr);
    }

    if (InterlockedDecrement(&target->ioCount_) == 0)
    {
        PostQueuedCompletionStatus(handleIocp_, 1, reinterpret_cast<ULONG_PTR>(target), nullptr);
    }
}

void ContentsNetLibrary::SendPost(Session* target)
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

        wsaSendReturn = WSASend(
            target->sock_,
            localWsaBuf,
            bufCount,
            &sendBytes,
            0,
            &target->sendOverlapped_.overlapped_,
            nullptr);

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

void ContentsNetLibrary::ReceiveFirst(Session* newSession)
{
    WSABUF wsaBuf;
    wsaBuf.buf = newSession->recvBuffer_.GetRearBufferPtr();
    wsaBuf.len = newSession->recvBuffer_.GetFreeSize();

    DWORD recvBytes;
    DWORD flags = 0;

    ZeroMemory(&newSession->recvOverlapped_.overlapped_, sizeof(newSession->recvOverlapped_.overlapped_));

    int retval = WSARecv(
        newSession->sock_,
        &wsaBuf,
        1,
        &recvBytes,
        &flags,
        &newSession->recvOverlapped_.overlapped_,
        0);

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

void ContentsNetLibrary::RecvProc(Session* target)
{
    int packetCount = 0;
    CPacket* packetBuffer = nullptr;
    CPacket* decodeBuffer = CPacket::Alloc();

    while (1)
    {
        int targetRecvBufferSize = target->recvBuffer_.GetUseSize();
        NetPacketHeader header;

        if (targetRecvBufferSize < sizeof(NetPacketHeader))
        {
            break;
        }

        if (target->recvBuffer_.Peek(reinterpret_cast<char*>(&header), sizeof(NetPacketHeader)) != sizeof(NetPacketHeader))
        {
            break;
        }

        if (header.code_ != packetCode_)
        {
            Disconnect(target->sessionId_);
            InterlockedIncrement(&dcPacketCodeError_);
            break;
        }

        if (header.len_ > 154)
        {
            Disconnect(target->sessionId_);
            InterlockedIncrement(&dcImpossiblePacketLength_);
            break;
        }

        if (targetRecvBufferSize < sizeof(header) + header.len_)
        {
            break;
        }

        if (packetBuffer == nullptr)
        {
            packetBuffer = CPacket::Alloc();
            packetBuffer->IncreaseRefCount();
        }

        unsigned int receiveDequeuePacketSize = target->recvBuffer_.Dequeue(
            decodeBuffer->GetBufferPtr(),
            DKServerCore::PacketLibHeaderSize + header.len_);

        if (receiveDequeuePacketSize != header.len_ + DKServerCore::PacketLibHeaderSize)
        {
            wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", receiveDequeuePacketSize);
            break;
        }

        if (decodeBuffer->Decode(decodeBuffer->GetReadPosition() - 1, header.len_ + 1, header.randKey_) == false)
        {
            Disconnect(target->sessionId_);
            InterlockedIncrement(&dcDecodeError_);
            CPacket::Free(decodeBuffer);
            break;
        }

        decodeBuffer->MoveWritePosition(receiveDequeuePacketSize - DKServerCore::PacketLibHeaderSize);

        *packetBuffer << decodeBuffer->GetDataSize();
        packetBuffer->PutData(decodeBuffer->GetReadPosition(), decodeBuffer->GetDataSize());

        InterlockedIncrement(&recvMessageCount_);

        decodeBuffer->Clear();

        if (++packetCount == DKServerCore::MaxPacketPack)
        {
            groupManager_.RouteMessage(target, reinterpret_cast<ContentsCPacket*>(packetBuffer));

            CPacket::Free(packetBuffer);
            packetBuffer = nullptr;
            packetCount = 0;
        }
    }

    if (packetCount)
    {
        groupManager_.RouteMessage(target, reinterpret_cast<ContentsCPacket*>(packetBuffer));

        CPacket::Free(packetBuffer);
    }

    CPacket::Free(decodeBuffer);
}

void ContentsNetLibrary::Receive(Session* target)
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

    int retval = WSARecv(
        target->sock_,
        recvWsaBuf,
        2,
        &recvBytes,
        &flags,
        &target->recvOverlapped_.overlapped_,
        0);

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

void ContentsNetLibrary::NetAddHeader(CPacket* packetBuffer)
{
    char* temp = packetBuffer->GetBufferPtr();

    NetPacketHeader netHeader;
    netHeader.code_ = packetCode_;
    netHeader.len_ = packetBuffer->GetDataSize();
    netHeader.randKey_ = rand() % 256;

    netHeader.checkSum_ = packetBuffer->Encode(
        packetBuffer->GetReadPosition(),
        netHeader.len_,
        netHeader.randKey_);

    memcpy_s(temp, DKServerCore::PacketLibHeaderSize, &netHeader, DKServerCore::PacketLibHeaderSize);
}

void ContentsNetLibrary::Release(Session* target)
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
    groupManager_.LeaveGroup(target);

    InterlockedDecrement(&sessionNum_);
    indexList_.Free(target->index_);

    return;
}

int* ContentsNetLibrary::FindEmptySession()
{
    if (InterlockedIncrement(&sessionNum_) > maxSession_)
    {
        return nullptr;
    }

    return indexList_.Alloc();
}

void ContentsNetLibrary::ClearSendBuffer(Session* target)
{
    while (1)
    {
        CPacket* temp;

        if (target->sendBuffer_.Dequeue(&temp) == false)
        {
            break;
        }

        CPacket::Free(temp);
    }
}

void ContentsNetLibrary::MoveGroup(__int64 sessionId, GroupId moveGroupId)
{
    unsigned int i = FindSession(sessionId);

    Session* target = &sessionArray_[i];

    groupManager_.MoveGroup(target, moveGroupId);
}

int ContentsNetLibrary::FindSession(__int64 sessionId)
{
    return static_cast<int>(sessionId >> 48);
}

int ContentsNetLibrary::GetAcceptTPS()
{
    return acceptTps_;
}

int ContentsNetLibrary::GetRecvMessageTPS()
{
    return recvMessageTps_;
}

int ContentsNetLibrary::GetSendMessageTPS()
{
    return sendMessageTps_;
}

DWORD ContentsNetLibrary::GetDisconnectCount()
{
    return disconnectCount_;
}

DWORD ContentsNetLibrary::GetDCUnloginTimeout()
{
    return dcUnloginTimeout_;
}

DWORD ContentsNetLibrary::GetDCLoginTimeout()
{
    return dcLoginTimeout_;
}

DWORD ContentsNetLibrary::GetDCSendBufferFull()
{
    return dcSendBufferFull_;
}

DWORD ContentsNetLibrary::GetDCPacketCodeError()
{
    return dcPacketCodeError_;
}

DWORD ContentsNetLibrary::GetDCDecodeError()
{
    return dcDecodeError_;
}

DWORD ContentsNetLibrary::GetDCSessionFull()
{
    return dcSessionFull_;
}

DWORD ContentsNetLibrary::GetDCImpossiblePacketLength()
{
    return dcImpossiblePacketLength_;
}

DWORD ContentsNetLibrary::GetSessionNum()
{
    return sessionNum_;
}

unsigned long long ContentsNetLibrary::GetAcceptTotal()
{
    return acceptTotal_;
}