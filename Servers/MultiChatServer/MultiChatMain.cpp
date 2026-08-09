#include <iostream>
#include <memory>

#include "MultiChatServer.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "DKParser.h"
#include "ServerStartConfig.h"

#pragma comment(lib, "Winmm.lib")

CrashDump crashDump;

unsigned int GlobalChecksum;

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config);
void PrintMonitoring(MultiChatServer* chatServer);
void ProcessConsoleInput();

int main()
{
    timeBeginPeriod(1);
    InitProfile();

    DKParser parser;

    if (!parser.Load("MultiChatServer.config"))
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

    std::unique_ptr<MultiChatServer> chatServer = std::make_unique<MultiChatServer>();

    if (!chatServer->Start(config, parser))
    {
        printf("MultiChatServer start failed.\n");
        timeEndPeriod(1);

        return 1;
    }

    while (true)
    {
        PrintMonitoring(chatServer.get());
        Sleep(1000);
        ProcessConsoleInput();
    }
}

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config)
{
    if (!parser.GetString("MULTICHAT", "IP", &config.ip))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MULTICHAT", "PORT", &config.port))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MULTICHAT", "WORKERTHREADS", &config.workerThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MULTICHAT", "CONCURRENTTHREADS", &config.concurrentThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MULTICHAT", "NAGLE", &config.nagle))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MULTICHAT", "SESSIONS", &config.maxSessionCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("MULTICHAT", "HEADERSIZE", &config.headerSize))
    {
        return false;
    }

    if (!parser.GetUnsignedChar("MULTICHAT", "PACKETCODE", &config.packetCode))
    {
        return false;
    }

    return true;
}

void PrintMonitoring(MultiChatServer* chatServer)
{
    if (chatServer == nullptr)
    {
        return;
    }

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"CPacket\n");
    wprintf(L"UseSize : %d    Capacity : %d\n", CPacket::GetUseSize(), CPacket::GetCapacity());

    wprintf(L"\nContents\n");
    wprintf(L"AcceptTotal : %lld\n", chatServer->GetAcceptTotal());
    wprintf(L"Session : %u\n", chatServer->GetSessionNum());
    wprintf(L"UnloginPlayer : %u    Player : %u\n", chatServer->GetUnloginPlayer(), chatServer->GetLoginPlayer());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"Disconnect\n");
    wprintf(L"DisconnectTotal : %u\n", chatServer->GetDisconnectCount());
    wprintf(L"DCWrongPacket : %u    DCAuthFailed : %u\n", chatServer->GetDCWrongPacket(), chatServer->GetDCAuthFailed());
    wprintf(L"DCUnloginTimeout : %u    DCLoginAgain : %u\n", chatServer->GetDCUnloginTimeout(), chatServer->GetDCLoginAgain());
    wprintf(L"DCSendBufferFull : %u    DCDuplicateLogin : %u\n", chatServer->GetDCSendBufferFull(), chatServer->GetDCDuplicateLogin());
    wprintf(L"DCPacketCodeError : %u    DCSessionFull : %u\n", chatServer->GetDCPacketCodeError(), chatServer->GetDCSessionFull());
    wprintf(L"DCDecodeError : %u    DCImpossiblePacketLength : %u\n", chatServer->GetDCDecodeError(), chatServer->GetDCImpossiblePacketLength());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"TPS\n");
    wprintf(L"AcceptTPS : %d\n", chatServer->GetAcceptTPS());
    wprintf(L"RecvTPS : %d    SendTPS : %d    MonitorSendTPS : %d\n", chatServer->GetRecvMessageTPS(), chatServer->GetSendMessageTPS(), chatServer->monitoringClient_.sendMessageCount_);
    wprintf(L"LogicTPS : %u    LoginTPS : %u\n", chatServer->GetLogicTPS(), chatServer->GetLoginTPS());
    wprintf(L"SectorMoveTPS : %u    ChatTPS : %u\n", chatServer->GetSectorMoveTPS(), chatServer->GetChatTPS());

    InterlockedExchange(&chatServer->monitoringClient_.sendMessageCount_, 0);
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
        ProfileDataOutText(L"MultiChatProfileData.txt");
        return;
    }

    if (input == 'r' || input == 'R')
    {
        ProfileReset();
    }
}