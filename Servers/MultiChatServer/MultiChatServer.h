#pragma once

#include "NetLibrary.h"
#include "TLSObjectFreeList.h"
#include "ChatMonitoringClient.h"
#include "GameDefine.h"
#include "Player.h"
#include "ProcessMonitoring.h"
#include "Sector.h"

#include <cpp_redis/cpp_redis>
#include <list>
#include <unordered_map>

#pragma comment(lib, "cpp_redis.lib")
#pragma comment(lib, "tacopie.lib")
#pragma comment(lib, "ws2_32.lib")

class DKParser;

class MultiChatServer : public NetLibrary
{
public:
    MultiChatServer();
    virtual ~MultiChatServer() override;

    bool Start(const DKServerCore::IocpServerStartConfig& config, DKParser& parser);

    DWORD GetLogicTPS();
    DWORD GetLoginTPS();
    DWORD GetSectorMoveTPS();
    DWORD GetChatTPS();
    DWORD GetHeartBeatTPS();

    DWORD GetDCWrongPacket();
    DWORD GetDCLoginAgain();
    DWORD GetDCAuthFailed();
    DWORD GetDCDuplicateLogin();

    DWORD GetLoginPlayer();
    DWORD GetUnloginPlayer();

    ChatMonitoringClient monitoringClient_;

protected:
    virtual bool OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort) override;
    virtual void OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId) override;
    virtual void OnRelease(__int64 sessionId) override;
    virtual void OnMessage(__int64 sessionId, ContentsCPacket* contentsPacket) override;
    virtual void OnError(int errorCode, const wchar_t* errorLog) override;
    virtual void OnInitializeTPS() override;

private:
    bool StartMonitoringClient(DKParser& parser);
    bool ConnectRedis(DKParser& parser);
    bool AuthToken(__int64 accountNo, char* sessionKey);

    void NetPacketProcLogin(Player* target, __int64 accountNo, wchar_t* id, wchar_t* nickname, char* sessionKey);
    void NetPacketProcSectorMove(Player* target, __int64 accountNo, unsigned short sectorX, unsigned short sectorY);
    void NetPacketProcMessage(Player* target, __int64 accountNo, unsigned short messageLength, wchar_t* message);
    void NetPacketProcHeartbeat(Player* target);

    void GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector);
    void LockSectorMove(unsigned short firstX, unsigned short firstY, unsigned short secondX, unsigned short secondY);
    void UnlockSectorMove(unsigned short firstX, unsigned short firstY, unsigned short secondX, unsigned short secondY);

private:
    TLSObjectFreeList<Player> playerPool_;

    std::unordered_map<__int64, Player*> playerMap_;
    SRWLOCK playerMapLock_;

    std::unordered_map<__int64, Player*> accountMap_;
    SRWLOCK accountMapLock_;

    std::list<Player*> sectorList_[SectorMaxY][SectorMaxX];
    SRWLOCK sectorLock_[SectorMaxY][SectorMaxX];

    cpp_redis::client* connection_{ nullptr };
    SRWLOCK connectionLock_;

    ProcessMonitoring processMonitoring_;

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
    DWORD dcLoginAgain_{ 0 };
    DWORD dcAuthFailed_{ 0 };
    DWORD dcDuplicateLogin_{ 0 };

    DWORD loginPlayer_{ 0 };
    DWORD unloginPlayer_{ 0 };
};