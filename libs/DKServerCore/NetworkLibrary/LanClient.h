#pragma once

#include <WinSock2.h>

#include "CoreDefines.h"
#include "LockFreeObjectFreeList.h"
#include "ContentsCPacket.h"
#include "LockFreeQueueCas2.h"
#include "RingBuffer.h"

class LanClient
{
public:
    LanClient();
    virtual ~LanClient();

    struct PacketHeader
    {
        unsigned short length_;
    };

    struct MyOverlapped
    {
        WSAOVERLAPPED overlapped_;
        int type_;
    };

    struct BufferCount
    {
        CPacket* buffers_[DKServerCore::MaxBatchSize];
        long count_ = 0;
    };

    struct ClientSession
    {
        TLockFreeQueue<CPacket*> sendBuffer_;

        MyOverlapped sendOverlapped_;
        MyOverlapped recvOverlapped_;

        RingBuffer recvBuffer_;

        SOCKET sock_ = INVALID_SOCKET;

        bool closeFlag_ = false;
        bool sendFlag_ = false;
        long ioCount_ = 0;
        long sendCount_ = 0;

        BufferCount bufferCount_;
    };

    bool Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int headerSize);

    bool Stop();
    void Close();

    ClientSession* SessionAlloc(int* emptyIndex, unsigned long long clientSock);

    void SendCompletion(ClientSession* target);
    void RecvCompletion(ClientSession* target, DWORD cbTransferred);

    void SendPacket(ContentsCPacket sendPacket);
    void SendPost(ClientSession* target);

    void ReceiveFirst(ClientSession* newSession);
    void RecvProc(ClientSession* target);
    void Receive(ClientSession* target);
    void AddHeader(CPacket* packetBuffer);
    void Release(ClientSession* target);

    void ClearSendBuffer(ClientSession* target);

    static unsigned int WINAPI ClientWorkerThread(void* thisPointer);

    virtual void OnConnect() = 0;
    virtual void OnRelease() = 0;
    virtual void OnMessage(ContentsCPacket* sendPacket) = 0;
    virtual void OnError(int errorCode, const wchar_t* errorLog) = 0;

    HANDLE handleIocp_ = nullptr;
    HANDLE workerThread_ = nullptr;

    ClientSession* clientSession_ = nullptr;

    unsigned int headerSize_ = 0;

    long sendMessageCount_ = 0;
};