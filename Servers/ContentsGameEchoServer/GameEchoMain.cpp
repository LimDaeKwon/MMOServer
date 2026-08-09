#include <iostream>
#include <memory>

#include "GameEchoServer.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "DKParser.h"
#include "ServerStartConfig.h"
#include "GameDefine.h"

#pragma comment(lib, "Winmm.lib")

CrashDump crashDump;

unsigned int GlobalChecksum;

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config);
void PrintMonitoring(GameEchoServer* gameEchoServer);
void ProcessConsoleInput();

int main()
{
    timeBeginPeriod(1);
    InitProfile();

    DKParser parser;

    if (!parser.Load("GameEchoServer.config"))
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

    std::unique_ptr<GameEchoServer> gameEchoServer = std::make_unique<GameEchoServer>();

    if (!gameEchoServer->Start(config, parser))
    {
        printf("GameEchoServer start failed.\n");
        timeEndPeriod(1);

        return 1;
    }

    while (true)
    {
        PrintMonitoring(gameEchoServer.get());
        Sleep(1000);
        ProcessConsoleInput();
    }
}

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config)
{
    if (!parser.GetString("GAMEECHO", "IP", &config.ip))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMEECHO", "PORT", &config.port))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMEECHO", "WORKERTHREADS", &config.workerThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMEECHO", "CONCURRENTTHREADS", &config.concurrentThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMEECHO", "NAGLE", &config.nagle))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMEECHO", "SESSIONS", &config.maxSessionCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("GAMEECHO", "HEADERSIZE", &config.headerSize))
    {
        return false;
    }

    if (!parser.GetUnsignedChar("GAMEECHO", "PACKETCODE", &config.packetCode))
    {
        return false;
    }

    return true;
}

void PrintMonitoring(GameEchoServer* gameEchoServer)
{
    if (gameEchoServer == nullptr)
    {
        return;
    }

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"CPacket\n");
    wprintf(L"UseSize : %d    Capacity : %d\n", CPacket::GetUseSize(), CPacket::GetCapacity());

    wprintf(L"\nContents\n");
    wprintf(L"AcceptTotal : %llu\n", gameEchoServer->GetAcceptTotal());
    wprintf(L"Session : %u\n", gameEchoServer->GetSessionNum());
    wprintf(L"UnloginPlayer : %u    Player : %u\n", gameEchoServer->GetUnloginPlayer(), gameEchoServer->GetLoginPlayer());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"Disconnect\n");
    wprintf(L"DisconnectTotal : %u\n", gameEchoServer->GetDisconnectCount());
    wprintf(L"DCWrongPacket : %u    DCAuthFailed : %u\n", gameEchoServer->GetDCWrongPacket(), gameEchoServer->GetDCAuthFailed());
    wprintf(L"DCUnloginTimeout : %u    DCLoginTimeout : %u\n", gameEchoServer->GetDCUnloginTimeout(), gameEchoServer->GetDCLoginTimeout());
    wprintf(L"DCSendBufferFull : %u    DCDuplicateLogin : %u\n", gameEchoServer->GetDCSendBufferFull(), gameEchoServer->GetDCDuplicateLogin());
    wprintf(L"DCPacketCodeError : %u    DCSessionFull : %u\n", gameEchoServer->GetDCPacketCodeError(), gameEchoServer->GetDCSessionFull());
    wprintf(L"DCDecodeError : %u    DCImpossiblePacketLength : %u\n", gameEchoServer->GetDCDecodeError(), gameEchoServer->GetDCImpossiblePacketLength());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"FPS\n");
    wprintf(L"AuthFPS : %d    EchoFPS : %d\n", gameEchoServer->groupManager_.GetGroupFPS(AuthGroupId), gameEchoServer->groupManager_.GetGroupFPS(EchoGroupId));

    wprintf(L"\nTPS\n");
    wprintf(L"AcceptTPS : %d\n", gameEchoServer->GetAcceptTPS());
    wprintf(L"RecvTPS : %d    SendTPS : %d\n", gameEchoServer->GetRecvMessageTPS(), gameEchoServer->GetSendMessageTPS());
}

void ProcessConsoleInput()
{
    if (!_kbhit())
    {
        return;
    }

    char input = _getch();

    if (input == 's' || input == 'S')
    {
        ProfileDataOutText(L"EchoProfileData");
        return;
    }

    if (input == 'r' || input == 'R')
    {
        ProfileReset();
    }
}