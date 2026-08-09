#pragma once

#include "NetLibrary.h"
#include "GameDefine.h"
#include "LoginMonitoringClient.h"
#include "ProcessMonitoring.h"

#include <cpp_redis/cpp_redis>
#include <string>

#pragma comment(lib, "cpp_redis.lib")
#pragma comment(lib, "tacopie.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")

#include "include/mysql.h"
#include "include/errmsg.h"

class DKParser;

class LoginServer : public NetLibrary
{
public:
    LoginServer();
    virtual ~LoginServer() override;

    bool Start(const DKServerCore::IocpServerStartConfig& config, DKParser& parser);

    DWORD GetLoginTPS();

    DWORD GetDCWrongPacket();
    DWORD GetDCAuthFailed();
    DWORD GetDCDuplicateLogin();

protected:
    virtual bool OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort) override;
    virtual void OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId) override;
    virtual void OnRelease(__int64 sessionId) override;
    virtual void OnMessage(__int64 sessionId, ContentsCPacket* contentsPacket) override;
    virtual void OnError(int errorCode, const wchar_t* errorLog) override;
    virtual void OnInitializeTPS() override;

private:
    struct TLSDBConnection
    {
        MYSQL connectionInfo_;
        MYSQL* connection_{ nullptr };
    };

private:
    bool StartMonitoringClient(DKParser& parser);
    bool ConnectRedis(DKParser& parser);
    bool SetMySqlConfig(DKParser& parser);
    bool SetTargetServerConfig(DKParser& parser);

    void ProcessLogin(__int64 sessionId, __int64 accountNo, char* sessionKey);
    unsigned char ProcessAuth(__int64 accountNo, char* sessionKey, wchar_t* id, wchar_t* nickname);
    void StoreSessionKey(__int64 accountNo, char* sessionKey);

private:
    DWORD tlsDbConnectionIndex_{ TLS_OUT_OF_INDEXES };

    cpp_redis::client* connection_{ nullptr };
    SRWLOCK connectionLock_;

    std::string mysqlIp_;
    unsigned int mysqlPort_{ 0 };
    std::string mysqlUser_;
    std::string mysqlPassword_;
    std::string mysqlDatabase_;

    wchar_t gameServerIp_[ServerIpLength]{};
    unsigned short gameServerPort_{ 0 };

    wchar_t chatServerIp_[ServerIpLength]{};
    unsigned short chatServerPort_{ 0 };

    LoginMonitoringClient monitoringClient_;
    ProcessMonitoring processMonitoring_;

    DWORD loginTps_{ 0 };
    DWORD loginCount_{ 0 };

    DWORD dcWrongPacket_{ 0 };
    DWORD dcAuthFailed_{ 0 };
    DWORD dcDuplicateLogin_{ 0 };
};