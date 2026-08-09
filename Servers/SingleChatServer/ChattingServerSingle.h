#pragma once

#include "NetLibrary.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "GameDefine.h"
#include "ObjectFreeList.h"
#include "Player.h"
#include "Sector.h"

#include <unordered_map>
#include <list>

class ChattingServerSingle : public NetLibrary
{
public:
    ChattingServerSingle();
    virtual ~ChattingServerSingle() override;

    DWORD GetLogicTPS();
    DWORD GetLoginTPS();
    DWORD GetSectorMoveTPS();
    DWORD GetChatTPS();
    DWORD GetHeartBeatTPS();

    DWORD GetDCWrongPacket();
    DWORD GetDCAuthFailed();
    DWORD GetDCDuplicateLogin();

    DWORD GetLoginPlayer();
    DWORD GetUnloginPlayer();
    DWORD GetLogicQueueSize();

protected:
    virtual bool OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort) override;
    virtual void OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId) override;
    virtual void OnRelease(__int64 sessionId) override;
    virtual void OnMessage(__int64 sessionId, ContentsCPacket* contentsPacket) override;
    virtual void OnError(int errorCode, const wchar_t* errorLog) override;
    virtual void OnInitializeTPS() override;

private:
    enum MessageType
    {
        MessageTypeAccept,
        MessageTypePacket,
        MessageTypeRelease,
    };

    struct MessageData
    {
        MessageType type_;
        __int64 sessionId_;
        ContentsCPacket* contentsPacket_;
    };

private:
    static unsigned int WINAPI LogicThread(LPVOID thisPtr);

    void AcceptProc(MessageData* messageData);
    void MessageProc(MessageData* messageData);
    void ReleaseProc(MessageData* messageData);

    void NetPacketProcLogin(Player* target, __int64 accountNo, wchar_t* id, wchar_t* nickname, char* sessionKey);
    void NetPacketProcSectorMove(Player* target, __int64 accountNo, unsigned short sectorX, unsigned short sectorY);
    void NetPacketProcMessage(Player* target, __int64 accountNo, unsigned short messageLength, wchar_t* message);
    void NetPacketProcHeartbeat(Player* target);

    void GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector);

private:
    ObjectFreeList<Player> playerPool_;
    std::unordered_map<__int64, Player*> playerMap_;
    std::unordered_map<__int64, Player*> accountMap_;

    std::list<Player*> sectorList_[SectorMaxY][SectorMaxX];

    LockFreeQueue<MessageData*> messageQueue_;
    TLSObjectFreeList<MessageData> messageDataFreeList_;
    HANDLE messageEvent_{ nullptr };

    DWORD logicTps_{ 0 };
    DWORD loginTps_{ 0 };
    DWORD sectorMoveTps_{ 0 };
    DWORD chatTps_{ 0 };
    DWORD heartbeatTps_{ 0 };

    DWORD logicCount_{ 0 };
    DWORD loginCount_{ 0 };
    DWORD sectorMoveCount_{ 0 };
    DWORD chatCount_{ 0 };
    DWORD heartbeatCount_{ 0 };

    DWORD dcWrongPacket_{ 0 };
    DWORD dcAuthFailed_{ 0 };
    DWORD dcDuplicateLogin_{ 0 };

    DWORD loginPlayer_{ 0 };
    DWORD unloginPlayer_{ 0 };

    HANDLE logicThreadHandle_{ nullptr };
};