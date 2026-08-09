#pragma once

#include <WinSock2.h>
#include "ServerStartConfig.h"
#include "CoreDefines.h"
#include "LockFreeObjectFreeList.h"
#include "ContentsCPacket.h"
#include "LockFreeQueue.h"
#include "LockFreeQueueCas2.h"
#include "RingBuffer.h"

class NetLibrary
{
public:
    NetLibrary();
    virtual ~NetLibrary();

    struct PacketHeader
    {
        unsigned short length_;
    };

#pragma pack(push, 1)
    struct NetPacketHeader
    {
        BYTE code_;
        WORD len_;
        BYTE randKey_;
        BYTE checkSum_;
    };
#pragma pack(pop)

    struct MyOverlapped
    {
        WSAOVERLAPPED overlapped_;
        int type_;
    };

    struct BufferCount
    {
        CPacket* buffers_[DKServerCore::MaxBatchSize];
        long count_;
    };

    struct Session
    {
        TLockFreeQueue<CPacket*> sendBuffer_;

        MyOverlapped sendOverlapped_;
        MyOverlapped recvOverlapped_;

        RingBuffer recvBuffer_;

        SOCKET sock_;
        __int64 sessionId_ = 0;
        bool disconnectFlag_ = false;
        bool sendFlag_ = false;
        long ioCount_ = 0;
        long sendCount_ = 0;

        BufferCount bufferCount_;

        unsigned long long lastRecvTime_ = 0;
        bool useFlag_ = false;
        bool loginFlag_ = false;
        int* index_;
    };

    bool Start(const char* serverIp, unsigned int serverPort, unsigned int threadsCount, unsigned int concurrentThreads, unsigned int nagle, unsigned int sessions, unsigned int headerSize, unsigned char packetCode);
    bool Start(const DKServerCore::IocpServerStartConfig& config);
    bool Stop();
    void Disconnect(__int64 sessionId);

    Session* SessionAlloc(int* emptyIndex, unsigned long long clientSock);

    void SendCompletion(Session* target);
    void RecvCompletion(Session* target, DWORD cbTransferred);

    void SendPacket(__int64 sessionId, ContentsCPacket sendPacket);
    void SendPost(Session* target);

    void ReceiveFirst(Session* newSession);
    void RecvProc(Session* target);
    void Receive(Session* target);
    void AddHeader(CPacket* packetBuffer);
    void NetAddHeader(CPacket* packetBuffer);

    void Release(Session* target);

    int FindSession(__int64 sessionId);
    int* FindEmptySession();

    void ClearSendBuffer(Session* target);

    static unsigned int WINAPI AcceptThread(void* thisPointer);
    static unsigned int WINAPI WorkerThread(void* thisPointer);
    static unsigned int WINAPI MonitorThread(void* thisPointer);
    static unsigned int WINAPI HeartbeatThread(void* thisPointer);

    virtual bool OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort) = 0;
    virtual void OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId) = 0;
    virtual void OnRelease(__int64 sessionId) = 0;
    virtual void OnMessage(__int64 sessionId, ContentsCPacket* sendPacket) = 0;
    virtual void OnError(int errorCode, const wchar_t* errorLog) = 0;
    virtual void OnInitializeTPS() = 0;

    int GetAcceptTPS();
    int GetRecvMessageTPS();
    int GetSendMessageTPS();

    DWORD acceptTps_;
    DWORD recvMessageTps_;
    DWORD sendMessageTps_;

    DWORD acceptCount_;
    DWORD recvMessageCount_;
    long sendMessageCount_;

    DWORD maxSession_;
    DWORD sessionNum_;
    DWORD threadsNum_;
    __int64 uniqueId_ = 1;

    DWORD headerSize_;
    DWORD packetType_ = 1;

    unsigned long long timeout_;
    unsigned long long unloginTimeout_;

    DWORD disconnectCount_;
    DWORD dcUnloginTimeout_;
    DWORD dcLoginTimeout_;
    DWORD dcSendBufferFull_;
    DWORD dcPacketCodeError_;
    DWORD dcDecodeError_;
    DWORD dcSessionFull_;
    DWORD dcImpossiblePacketLength_;

    DWORD GetDisconnectCount();
    DWORD GetDCUnloginTimeout();
    DWORD GetDCLoginTimeout();
    DWORD GetDCSendBufferFull();
    DWORD GetDCPacketCodeError();
    DWORD GetDCDecodeError();
    DWORD GetDCSessionFull();
    DWORD GetDCImpossiblePacketLength();

    DWORD GetSessionNum();
    unsigned long long GetAcceptTotal();

    HANDLE handleIocp_;
    SOCKET listenSock_;
    HANDLE* threads_;
    HANDLE acceptThread_;
    HANDLE monitorThread_;
    HANDLE heartbeatThread_;
    Session* sessionArray_;

    LFObjectFreeList<int> indexList_;
    unsigned char packetCode_ = 0;
    unsigned long long acceptTotal_ = 0;
};