#include "LanClient.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <process.h>

#pragma comment(lib, "Ws2_32.lib")

LanClient::LanClient()
{
}

LanClient::~LanClient()
{

}

bool LanClient::Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int header)
{
    headerSize_ = header;

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        return false;
    }

    handleIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

    if (handleIocp_ == nullptr)
    {
        return false;
    }

    clientSession_ = new ClientSession;

    clientSession_->sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (clientSession_->sock_ == INVALID_SOCKET)
    {
        int error = WSAGetLastError();
        wprintf(L"socket Error %d", error);
        return false;
    }

    DWORD zero = 0;
    setsockopt(clientSession_->sock_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&zero), sizeof(zero));

    LINGER linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;

    setsockopt(clientSession_->sock_, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&linger), sizeof(linger));

    if (nagle)
    {
        DWORD noDelay = 1;

        setsockopt(clientSession_->sock_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
    }

    SOCKADDR_IN serverAddress;
    ZeroMemory(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    InetPtonA(AF_INET, serverIp, &serverAddress.sin_addr);
    serverAddress.sin_port = htons(serverPort);

    int connectReturn = 0;

    for (int i = 0; i < 3; ++i)
    {
        connectReturn = connect(clientSession_->sock_, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));

        if (connectReturn == 0)
        {
            break;
        }

        connectReturn = WSAGetLastError();
    }

    if (connectReturn != 0)
    {
        DebugBreak();
    }

    if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSession_->sock_),handleIocp_,reinterpret_cast<ULONG_PTR>(clientSession_),0) == nullptr)
    {
        int error = GetLastError();
        wprintf(L"CreateIoCompletionPort Error %d", error);
        DebugBreak();
        return false;
    }

    clientSession_->recvOverlapped_.type_ = DKServerCore::RecvIoType;
    clientSession_->sendOverlapped_.type_ = DKServerCore::SendIoType;
    clientSession_->ioCount_ = 1;

    ReceiveFirst(clientSession_);

    workerThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ClientWorkerThread, this, 0, nullptr));

    if (workerThread_ == nullptr)
    {
        wprintf(L"_beginthreadex Failed");
        DebugBreak();
        return false;
    }

    OnConnect();

    return true;
}

bool LanClient::Stop()
{
    if (handleIocp_)
    {
        PostQueuedCompletionStatus(handleIocp_, 0, 0, nullptr);
    }

    if (workerThread_)
    {
        WaitForSingleObject(workerThread_, INFINITE);
        CloseHandle(workerThread_);
        workerThread_ = nullptr;
    }

    if (handleIocp_)
    {
        CloseHandle(handleIocp_);
        handleIocp_ = nullptr;
    }

    if (clientSession_->sock_ != INVALID_SOCKET)
    {
        closesocket(clientSession_->sock_);
    }

    delete clientSession_;
    clientSession_ = nullptr;

    WSACleanup();

    return true;
}

void LanClient::Close()
{
    if (InterlockedExchange8(reinterpret_cast<volatile char*>(&clientSession_->closeFlag_), 1) == 1)
    {
        return;
    }

    CancelIoEx(reinterpret_cast<HANDLE>(clientSession_->sock_), nullptr);
}

unsigned int __stdcall LanClient::ClientWorkerThread(void* thisPointer)
{
    LanClient* thisForWorker = static_cast<LanClient*>(thisPointer);

    while (true)
    {
        DWORD cbTransferred = 0;
        MyOverlapped* overlapPointer = nullptr;
        ClientSession* target = nullptr;

        int retval = GetQueuedCompletionStatus(thisForWorker->handleIocp_, &cbTransferred, reinterpret_cast<PULONG_PTR>(&target), reinterpret_cast<LPOVERLAPPED*>(&overlapPointer), INFINITE);

        if (overlapPointer == nullptr && cbTransferred == 0 && target == nullptr)
        {
            break;
        }

        if (retval == 0)
        {
            int error = WSAGetLastError();

            if (!(error == 64 || error == 995))
            {
            }
        }
        else
        {
            if (overlapPointer->type_ == DKServerCore::RecvIoType)
            {
                thisForWorker->RecvCompletion(target, cbTransferred);
            }

            if (overlapPointer->type_ == DKServerCore::SendIoType)
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

void LanClient::SendPacket(ContentsCPacket contentsPacket)
{
    ClientSession* target = clientSession_;

    int localCount = InterlockedIncrement(&target->ioCount_);

    if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
    {
        if (InterlockedDecrement(&target->ioCount_) == 0)
        {
            Release(target);
        }

        return;
    }

    CPacket* sendPacket = contentsPacket.packetBuffer_;

    AddHeader(sendPacket);
    sendPacket->IncreaseRefCount();

    int enqueueReturn = target->sendBuffer_.Enqueue(sendPacket);

    InterlockedIncrement(&target->sendCount_);

    if (enqueueReturn == false)
    {

        CPacket::Free(sendPacket);

        wprintf(L"EnqueueFail in SendPacketUnicast Client \n");

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

void LanClient::ReceiveFirst(ClientSession* newSession)
{
    WSABUF wsaBuf;
    wsaBuf.buf = newSession->recvBuffer_.GetRearBufferPtr();
    wsaBuf.len = newSession->recvBuffer_.GetFreeSize();

    DWORD recvBytes;
    DWORD flags = 0;

    ZeroMemory(&newSession->recvOverlapped_.overlapped_, sizeof(newSession->recvOverlapped_.overlapped_));

    int retval = WSARecv(newSession->sock_, &wsaBuf, 1, &recvBytes, &flags, &newSession->recvOverlapped_.overlapped_, 0);

    if (retval == SOCKET_ERROR)
    {
        int wsaRecvError = WSAGetLastError();

        if (wsaRecvError != WSA_IO_PENDING)
        {
            wprintf(L"In First WSARecvError : %d  Client \n", wsaRecvError);

            if (InterlockedDecrement(&newSession->ioCount_) == 0)
            {
                Release(newSession);
            }
        }
    }
}

void LanClient::Release(ClientSession* target)
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

    closesocket(target->sock_);
    target->sock_ = INVALID_SOCKET;

    return;
}

void LanClient::SendPost(ClientSession* target)
{
    if (target->closeFlag_ == 1)
    {
        return;
    }

    if (InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 1) != 0)
    {
        return;
    }

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
        localWsaBuf[bufCount].buf = temp->GetBufferPtr();
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

        return;
    }

    InterlockedAdd(&sendMessageCount_, target->sendCount_);

    InterlockedExchange(&target->sendCount_, 0);

    DWORD sendBytes = 0;

    InterlockedIncrement(&target->ioCount_);

    ZeroMemory(&target->sendOverlapped_.overlapped_, sizeof(target->sendOverlapped_.overlapped_));

    int wsaSendReturn = WSASend(target->sock_, localWsaBuf, bufCount, &sendBytes, 0, &target->sendOverlapped_.overlapped_, nullptr);

    if (wsaSendReturn == SOCKET_ERROR)
    {
        int wsaSendError = WSAGetLastError();

        if (wsaSendError == WSA_IO_PENDING)
        {
            if (target->closeFlag_ == 1)
            {
                CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);
            }
        }
        else
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

void LanClient::SendCompletion(ClientSession* target)
{
    for (int i = 0; i < target->bufferCount_.count_; ++i)
    {
        CPacket::Free(target->bufferCount_.buffers_[i]);
    }

    target->bufferCount_.count_ = 0;

    InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);

    SendPost(target);
}

void LanClient::RecvCompletion(ClientSession* target, DWORD cbTransferred)
{
    target->recvBuffer_.MoveRear(cbTransferred);
    RecvProc(target);
    Receive(target);
}

void LanClient::RecvProc(ClientSession* target)
{

    while (true)
    {
        int targetRecvBufferSize = target->recvBuffer_.GetUseSize();
        PacketHeader header;

        if (targetRecvBufferSize < sizeof(header))
        {
            break;
        }

        if (target->recvBuffer_.Peek(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header))
        {
            break;
        }

        if (targetRecvBufferSize < sizeof(header) + header.length_)
        {
            break;
        }

        target->recvBuffer_.MoveFront(sizeof(header));

        CPacket* packetBuffer = CPacket::Alloc();

        unsigned int receiveDequeuePacketSize = target->recvBuffer_.Dequeue(packetBuffer->GetBufferPtr() + DKServerCore::PacketLibHeaderSize, header.length_);

        if (receiveDequeuePacketSize != header.length_)
        {
            OnError(0, L"## ReceiveQDequeuePacketSize != Header.BySize (Client)");
            CPacket::Free(packetBuffer);
            break;
        }

        packetBuffer->IncreaseRefCount();
        packetBuffer->MoveWritePosition(receiveDequeuePacketSize);

        OnMessage(reinterpret_cast<ContentsCPacket*>(packetBuffer));

        CPacket::Free(packetBuffer);
    }
}

void LanClient::Receive(ClientSession* target)
{
    if (target->closeFlag_)
    {
        return;
    }

    WSABUF recvWsaBuf[2];

    recvWsaBuf[0].buf = target->recvBuffer_.GetRearBufferPtr();
    recvWsaBuf[0].len = target->recvBuffer_.DirectEnqueueSize();
    recvWsaBuf[1].buf = target->recvBuffer_.GetStartBufferPtr();
    recvWsaBuf[1].len = target->recvBuffer_.GetFreeSize() - target->recvBuffer_.DirectEnqueueSize();

    DWORD recvBytes = 0;
    DWORD flags = 0;

    ZeroMemory(&target->recvOverlapped_.overlapped_, sizeof(target->recvOverlapped_.overlapped_));

    InterlockedIncrement(&target->ioCount_);

    int retval = WSARecv(target->sock_, recvWsaBuf, 2, &recvBytes, &flags, &target->recvOverlapped_.overlapped_, 0);

    if (retval == SOCKET_ERROR)
    {
        int wsaRecvError = WSAGetLastError();

        if (wsaRecvError == WSA_IO_PENDING)
        {
            if (target->closeFlag_ == 1)
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

void LanClient::AddHeader(CPacket* packetBuffer)
{
    char* temp = packetBuffer->GetBufferPtr();

    unsigned short len = static_cast<unsigned short>(packetBuffer->GetDataSize());
    *reinterpret_cast<unsigned short*>(temp) = len;
}

void LanClient::ClearSendBuffer(ClientSession* target)
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