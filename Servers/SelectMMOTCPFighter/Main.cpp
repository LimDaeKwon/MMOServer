
#include "SelectMMOTCPFighter.h"
#include "BasicSelectMMOTCPFighter.h"
#include "CrashDump.h"
#include "Profiler.h"
#include "DKParser.h"
#include "ServerStartConfig.h"

#pragma comment(lib, "Winmm.lib")

CrashDump crashDump;

unsigned int GlobalChecksum;

bool SetConfigValue(DKParser& parser, DKServerCore::SelectServerStartConfig& config);


int main()
{
    timeBeginPeriod(1);
    InitProfile();

    DKParser parser;

    if (!parser.Load("SelectMMOTCPFighter.config"))
    {
        printf("Config load failed: %s\n", parser.GetLastError().c_str());
        return 1;
    }

    DKServerCore::SelectServerStartConfig config;

    if (!SetConfigValue(parser, config))
    {
        return 1;
    }

	unsigned int sendQueueTypeValue;

    if (!parser.GetUnsignedInt("SELECT", "SENDQUEUE", &sendQueueTypeValue))
    {
        return false;
    }

    SelectMMOTCPFighter* selectInstance = nullptr;


    if (sendQueueTypeValue)
    {
		printf("Using SelectMMOTCPFighter\n");
        selectInstance = new SelectMMOTCPFighter();
        selectInstance->Start(config);
    }
    else
    {
        printf("Using BasicSelectMMOTCPFighter\n");
        BasicSelectMMOTCPFighter* instance = new BasicSelectMMOTCPFighter();
        instance->Start(config);
    }

	int loopCount = 0;
    while (selectInstance != nullptr)
    {
        wprintf(L"\n------------------------------------------------------------\n");
        wprintf(L"SelectMMOTCPFighter Monitoring\n");
        wprintf(L"Session  : %u    Character : %u    Moving : %u\n",
            selectInstance->GetSessionCount(),
            selectInstance->GetCharacterCount(),
            selectInstance->GetMovingCharacterCount());

        wprintf(L"\nTPS\n");
        wprintf(L"FrameTPS : %u    UpdateTPS : %u\n",
            selectInstance->GetFrameTPS(),
            selectInstance->GetUpdateTPS());

        wprintf(L"AcceptTPS : %u    RecvPacketTPS : %u    SendPacketTPS : %u   \n",
            selectInstance->GetAcceptTPS(),
            selectInstance->GetRecvPacketTPS(),
            selectInstance->GetSendPacketTPS());

        wprintf(L"MoveStartTPS : %u    MoveStopTPS : %u    AttackTPS : %u    EchoTPS : %u\n",
            selectInstance->GetMoveStartTPS(),
            selectInstance->GetMoveStopTPS(),
            selectInstance->GetAttackTPS(),
            selectInstance->GetEchoTPS());

        wprintf(L"DisconnectTPS : %u    ReleaseTPS : %u\n",
            selectInstance->GetDisconnectTPS(),
            selectInstance->GetReleaseTPS());

        wprintf(L"SyncTPS : %u    SyncTotal : %u\n",
            selectInstance->GetSyncTPS(),
            selectInstance->GetSyncCount());
        if (++loopCount == 9000)
        {
            break;
        }
        Sleep(1000);
    }

	ProfileDataOutText(L"SelectMMOTCPFighter_Profile.txt");


    timeEndPeriod(1);

    return 0;
}


bool SetConfigValue(DKParser& parser, DKServerCore::SelectServerStartConfig& config)
{
    if (!parser.GetString("SELECT", "IP", &config.ip))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SELECT", "PORT", &config.port))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SELECT", "NAGLE", &config.nagle))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SELECT", "SESSIONS", &config.maxSessionCount))
    {
        return false;
    }

    if (!parser.GetUnsignedChar("SELECT", "PACKETCODE", &config.packetCode))
    {
        return false;
    }

    if (!parser.GetUnsignedInt("SELECT", "FRAMEMS", &config.frameMs))
    {
        return false;
    }

    return true;
}
