#include "ChattingServerSingle.h"
#include "CrashDump.h"
#include "DKParser.h"
#include "ProcessMonitoring.h"
#include "Profiler.h"
#include "ServerStartConfig.h"
#include "SystemMonitoring.h"

#include <conio.h>
#include <cstdio>
#include <memory>
#include <Windows.h>

#pragma comment(lib, "Winmm.lib")

CrashDump crashDump;

unsigned int GlobalChecksum;

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config);
void PrintMonitoring(ChattingServerSingle* chatServer, SystemMonitoring& systemMonitoring, ProcessMonitoring& processMonitoring);
void ProcessConsoleInput();

int main()
{
    SystemMonitoring systemMonitoring;
    ProcessMonitoring processMonitoring;

    timeBeginPeriod(1);
    InitProfile();

    DKParser parser;

    if (!parser.Load("SingleChatServer.config"))
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

    std::unique_ptr<ChattingServerSingle> chatServer = std::make_unique<ChattingServerSingle>();

    if (!chatServer->Start(config))
    {
        printf("SingleChatServer start failed.\n");
        timeEndPeriod(1);

        return 1;
    }

    while (true)
    {
        PrintMonitoring(chatServer.get(), systemMonitoring, processMonitoring);
        Sleep(1000);
        ProcessConsoleInput();
    }
}

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config)
{
    if (!parser.GetString("SINGLECHAT", "IP", &config.ip))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SINGLECHAT", "PORT", &config.port))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SINGLECHAT", "WORKERTHREADS", &config.workerThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SINGLECHAT", "CONCURRENTTHREADS", &config.concurrentThreadCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SINGLECHAT", "NAGLE", &config.nagle))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SINGLECHAT", "SESSIONS", &config.maxSessionCount))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SINGLECHAT", "HEADERSIZE", &config.headerSize))
    {
        return false;
    }

    if (!parser.GetUnsignedChar("SINGLECHAT", "PACKETCODE", &config.packetCode))
    {
        return false;
    }

    return true;
}

void PrintMonitoring(ChattingServerSingle* chatServer, SystemMonitoring& systemMonitoring, ProcessMonitoring& processMonitoring)
{
    if (chatServer == nullptr)
    {
        return;
    }

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"CPacket\n");
    wprintf(L"UseSize : %d    Capacity : %d\n", CPacket::GetUseSize(), CPacket::GetCapacity());
    wprintf(L"LogicQueue : %u\n", chatServer->GetLogicQueueSize());

    wprintf(L"\nContents\n");
    wprintf(L"Session : %u\n", chatServer->GetSessionNum());
    wprintf(L"UnloginPlayer : %u    LoginPlayer : %u\n", chatServer->GetUnloginPlayer(), chatServer->GetLoginPlayer());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"Disconnect\n");
    wprintf(L"DisconnectTotal : %u\n", chatServer->GetDisconnectCount());
    wprintf(L"DCWrongPacket : %u    DCAuthFailed : %u\n", chatServer->GetDCWrongPacket(), chatServer->GetDCAuthFailed());
    wprintf(L"DCUnloginTimeout : %u    DCLoginTimeout : %u\n", chatServer->GetDCUnloginTimeout(), chatServer->GetDCLoginTimeout());
    wprintf(L"DCSendBufferFull : %u    DCDuplicateLogin : %u\n", chatServer->GetDCSendBufferFull(), chatServer->GetDCDuplicateLogin());
    wprintf(L"DCPacketCodeError : %u    DCSessionFull : %u\n", chatServer->GetDCPacketCodeError(), chatServer->GetDCSessionFull());
    wprintf(L"DCDecodeError : %u    DCImpossiblePacketLength : %u\n", chatServer->GetDCDecodeError(), chatServer->GetDCImpossiblePacketLength());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"TPS\n");
    wprintf(L"AcceptTPS : %d\n", chatServer->GetAcceptTPS());
    wprintf(L"RecvTPS : %d    SendTPS : %d\n", chatServer->GetRecvMessageTPS(), chatServer->GetSendMessageTPS());
    wprintf(L"LogicTPS : %u    LoginTPS : %u\n", chatServer->GetLogicTPS(), chatServer->GetLoginTPS());
    wprintf(L"SectorMoveTPS : %u    ChatTPS : %u\n", chatServer->GetSectorMoveTPS(), chatServer->GetChatTPS());

    wprintf(L"\n------------------------------------------------------------\n");

    wprintf(L"Monitoring\n");
    wprintf(L"ProcessNonPagedPool : %f%%    ServerNonPagedPool : %f%%\n", processMonitoring.GetProcessNonPagedMemory(), systemMonitoring.GetServerNonPagedBytes());
    wprintf(L"ProcessUseMemory : %f%%    ServerAvailableMemory : %f%%\n", processMonitoring.GetProcessUserMemory(), systemMonitoring.GetServerAvailableMBytes());
    wprintf(L"ProcessCPUUsage : %f%%    ServerCPUUsage : %f%%\n", processMonitoring.ProcessTotal(), systemMonitoring.ProcessorTotal());
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
        ProfileDataOutText(L"SingleChatProfileData.txt");
        return;
    }

    if (input == 'r' || input == 'R')
    {
        ProfileReset();
    }
}