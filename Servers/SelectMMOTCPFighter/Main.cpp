
#include "SelectMMOTCPFighter.h"
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

    SelectMMOTCPFighter* instance = new SelectMMOTCPFighter();
    instance->Start(config);
    Sleep(900000);
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
