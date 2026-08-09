#include <iostream>
#include <memory>

#include "LoginServer.h"
#include "windows.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "DKParser.h"
#include "ServerStartConfig.h"

#pragma comment(lib, "Winmm.lib")

CrashDump crashDump;

unsigned int GlobalChecksum;

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config);
void PrintMonitoring(LoginServer* loginServer);

int main()
{
    timeBeginPeriod(1);
    InitProfile();

    DKParser parser;

    if (!parser.Load("LoginServer.config"))
    {
        printf("Config load failed: %s\n", parser.GetLastError().c_str());
        timeEndPeriod(1);

        return 1;
    }

    DKServerCore::IocpServerStartConfig config;

    if (!SetConfigValue(parser, config))
    {
        printf("Config value load failed.\n");
        timeEndPeriod(1);

        return 1;
    }

    std::unique_ptr<LoginServer> loginServer = std::make_unique<LoginServer>();

    if (!loginServer->Start(config, parser))
    {
        printf("LoginServer start failed.\n");
        timeEndPeriod(1);

        return 1;
    }

    while (true)
    {
        PrintMonitoring(loginServer.get());
        Sleep(1000);
    }
}

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config)
{
    if (!parser.GetString("LOGIN", "IP", &config.ip))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("LOGIN", "PORT", &config.port))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("LOGIN", "WORKERTHREADS", &config.workerThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("LOGIN", "CONCURRENTTHREADS", &config.concurrentThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("LOGIN", "NAGLE", &config.nagle))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("LOGIN", "SESSIONS", &config.maxSessionCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("LOGIN", "HEADERSIZE", &config.headerSize))
    {
        return false;
    }

    if (!parser.GetUnsignedChar("LOGIN", "PACKETCODE", &config.packetCode))
    {
        return false;
    }

    return true;
}

void PrintMonitoring(LoginServer* loginServer)
{
    if (loginServer == nullptr)
    {
        return;
    }

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"CPacket\n");
    wprintf(L"UseSize : %d    Capacity : %d\n", CPacket::GetUseSize(), CPacket::GetCapacity());

    wprintf(L"\nContents\n");
    wprintf(L"AcceptTotal : %lld\n", loginServer->GetAcceptTotal());
    wprintf(L"Session : %u\n", loginServer->GetSessionNum());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"Disconnect\n");
    wprintf(L"DisconnectTotal : %u\n", loginServer->GetDisconnectCount());
    wprintf(L"DCWrongPacket : %u    DCAuthFailed : %u\n", loginServer->GetDCWrongPacket(), loginServer->GetDCAuthFailed());
    wprintf(L"DCUnloginTimeout : %u    DCLoginTimeout : %u\n", loginServer->GetDCUnloginTimeout(), loginServer->GetDCLoginTimeout());
    wprintf(L"DCSendBufferFull : %u    DCDuplicateLogin : %u\n", loginServer->GetDCSendBufferFull(), loginServer->GetDCDuplicateLogin());
    wprintf(L"DCPacketCodeError : %u    DCSessionFull : %u\n", loginServer->GetDCPacketCodeError(), loginServer->GetDCSessionFull());
    wprintf(L"DCDecodeError : %u    DCImpossiblePacketLength : %u\n", loginServer->GetDCDecodeError(), loginServer->GetDCImpossiblePacketLength());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"TPS\n");
    wprintf(L"AcceptTPS : %d\n", loginServer->GetAcceptTPS());
    wprintf(L"RecvTPS : %d    SendTPS : %d\n", loginServer->GetRecvMessageTPS(), loginServer->GetSendMessageTPS());
    wprintf(L"LoginTPS : %u\n", loginServer->GetLoginTPS());
}