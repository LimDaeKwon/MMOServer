#pragma once

#include <WinSock2.h>

#include "CoreDefines.h"
#include "LockFreeObjectFreeList.h"
#include "ContentsCPacket.h"
#include "LockFreeQueueCas2.h"
#include "RingBuffer.h"

class LanNetworkLibraryServer
{
public:
    LanNetworkLibraryServer();
    virtual ~LanNetworkLibraryServer();

    struct LanPacketHeader
    {
        unsigned short length_;
    };

#pragma pack(push, 1)
    struct NetPacketHeader
    {
        BYTE code_;
        BYTE size_;
    };
#pragma pack(pop)

    struct MyOverlapped
    {
        WSAOVERLAPPED overlapped_;
        int type_;
    };

    struct BufferCount
    {
        CPacket* buffers_[DKServerCore::LanNetworkMaxBatchSize];
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

    bool Start(const char* serverIp, unsigned int serverPort, unsigned int threadsCount, unsigned int concurrentThreads, unsigned int nagle, unsigned int sessions, unsigned int headerSize);
    bool Stop();
    int GetSessionCount();
    void Disconnect(__int64 sessionId);

    Session* SessionAlloc(int* emptyIndex, unsigned long long clientSock);

    void SendCompletion(Session* target);
    void RecvCompletion(Session* target, DWORD cbTransferred);

    void SendPacket(__int64 sessionId, CPacket* sendPacket);
    void SendPost(Session* target);

    void ReceiveFirst(Session* newSession);
    void RecvProc(Session* target);
    void Receive(Session* target);
    void LanAddHeader(CPacket* packetBuffer);
    void NetAddHeader(CPacket* packetBuffer);

    void Release(Session* target);

    void RegisterIOCP(HANDLE newSocket, ULONG_PTR key);
    HANDLE CreateIOCP(DWORD concurrent);

    void ReturnReference(Session* target);
    int SetWSABUF(Session* target, WSABUF* wsaBuf);
    void GetClientAddress(SOCKET clientSocket, sockaddr_in& clientAddr, WCHAR* addr);
    void RecursiveCheck(Session* target);
    void CheckSendReturn(Session* target, int sendReturn);
    void CheckRecvReturn(Session* target, int recvReturn);
    bool CheckLibraryPacketCode(BYTE code);

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
    virtual void OnMessage(__int64 sessionId, CPacket* sendPacket) = 0;
    virtual void OnError(int errorCode, const wchar_t* errorLog) = 0;

    int GetAcceptTPS();
    int GetRecvMessageTPS();
    int GetSendMessageTPS();

    unsigned int acceptTps_;
    unsigned int recvMessageTps_;
    unsigned int sendMessageTps_;

    unsigned int acceptCount_;
    unsigned int recvMessageCount_;
    long sendMessageCount_;

    unsigned int maxSession_;
    unsigned int sessionNum_;
    unsigned int threadsNum_;
    __int64 uniqueId_ = 1;

    unsigned int headerSize_;
    unsigned int packetType_ = 1;

    unsigned long long timeout_;
    unsigned long long unloginTimeout_;

    HANDLE handleIocp_;
    SOCKET listenSock_;
    HANDLE* threads_;
    HANDLE acceptThread_;
    HANDLE monitorThread_;
    HANDLE heartbeatThread_;
    Session* sessionArray_;

    LFObjectFreeList<int> indexList_;
};