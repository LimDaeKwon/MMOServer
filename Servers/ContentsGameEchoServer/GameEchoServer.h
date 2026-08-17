#pragma once

#include "ContentsNetLibrary.h"
#include "TLSObjectFreeList.h"
#include "GameMonitoringClient.h"
#include "ProcessMonitoring.h"
#include "ServerStartConfig.h"

#include <unordered_map>

class DKParser;

class GameEchoServer : public ContentsNetLibrary
{
public:
    GameEchoServer();
    virtual ~GameEchoServer() override;

    bool Start(const DKServerCore::IocpServerStartConfig& config, DKParser& parser);

    virtual void* GetPlayerPointer(__int64 sessionId) override;
    void FreePlayer(Player* target);

    DWORD GetLogicTPS();
    DWORD GetLoginTPS();
    DWORD GetHeartbeatTPS();

    DWORD GetDCWrongPacket();
    DWORD GetDCAuthFailed();
    DWORD GetDCDuplicateLogin();

    DWORD GetLoginPlayer();
    DWORD GetUnloginPlayer();

protected:
    virtual bool OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort) override;
    virtual void OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId) override;
    virtual void OnRelease(__int64 sessionId) override;
    virtual void OnMessage(__int64 sessionId, ContentsCPacket* contentsPacket) override;
    virtual void OnError(int errorCode, const wchar_t* errorLog) override;
    virtual void OnInitializeTPS() override;

private:
    bool StartMonitoringClient(DKParser& parser);

private:
    TLSObjectFreeList<Player> playerPool_;

    std::unordered_map<__int64, Player*> playerMap_;
    SRWLOCK playerMapLock_;

    GameMonitoringClient monitoringClient_;
    ProcessMonitoring processMonitoring_;

    DWORD logicTps_{ 0 };
    DWORD loginTps_{ 0 };
    DWORD heartbeatTps_{ 0 };

    DWORD logicCount_{ 0 };
    DWORD loginCount_{ 0 } ;
    DWORD heartbeatCount_{ 0 };

    DWORD dcWrongPacket_{ 0 };
    DWORD dcAuthFailed_{ 0 };
    DWORD dcDuplicateLogin_{ 0 };

    DWORD loginPlayer_{ 0 };
    DWORD unloginPlayer_{ 0 };
};