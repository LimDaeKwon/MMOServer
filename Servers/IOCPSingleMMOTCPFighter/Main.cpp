// NetworkLibrary.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "MMOTCPServerSingle.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "queue"
#include "ServerStartConfig.h"
#include "DKParser.h"



CrashDump zz;


#pragma comment(lib, "winmm.lib")

constexpr const char* ThreadFileName = "MMOTCPFighterThreadSetting.config";
constexpr const char* GameFileName = "GameSetting.config";



enum ThreadSettingType
{
	ThreadSettingIp, ThreadSettingPort, ThreadSettingThreads, ThreadSettingConcurrent, ThreadSettingNagle, ThreadSettingSessions, ThreadSettingHeaderSize
};
unsigned int GlobalChecksum;


bool ParseThreadDataFile(const char* fileName);

bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config);


int main()
{
	timeBeginPeriod(1);

	InitProfile();

	DKParser parser;
	if (!parser.Load(ThreadFileName))
	{
		printf("Config load failed: %s\n", parser.GetLastError().c_str());
		return 1;
	}

	DKServerCore::IocpServerStartConfig config;
	if (!SetConfigValue(parser, config))
	{
		return 1;
	}


	MMOTCPServerSingle* gameInstance = new MMOTCPServerSingle;
	
	gameInstance->Start(config);

	while (1)
	{

		if (_kbhit())
		{
			char c = _getch();
			if (c == 's' || c == 'C')
			{
				ProfileDataOutText(L"ProfileData");
			}

			if (c == 'r' || c == 'R')
			{
				ProfileReset();
			}
		}
		//서버 컨트롤
		//그 순간 서버의 덤프를 남긴다 -> 메모리를 자료구조라고 봤을 때
		// 누군가가 쓰고있을 때 읽어도 되는가?>
		//

	}



	timeEndPeriod(1);

}


bool SetConfigValue(DKParser& parser, DKServerCore::IocpServerStartConfig& config)
{

	if (!parser.GetString("IOCP", "IP", &config.ip))
	{
		printf("Missing config: [IOCP] Ip\n");
		return false;
	}

	if (!parser.GetUnsignedInt("IOCP", "PORT", &config.port))
	{
		printf("Missing or invalid config: [IOCP] PORT\n");
		return false;
	}

	if (!parser.GetUnsignedInt("IOCP", "WORKERTHREADS", &config.workerThreadCount))
	{
		printf("Missing or invalid config: [IOCP] WORKERTHREADS\n");
		return false;
	}

	if (!parser.GetUnsignedInt("IOCP", "CONCURRENTTHREADS", &config.concurrentThreadCount))
	{
		printf("Missing or invalid config: [IOCP] CONCURRENTTHREADS\n");
		return false;
	}

	if (!parser.GetUnsignedInt("IOCP", "NAGLE", &config.nagle))
	{
		printf("Missing or invalid config: [IOCP] NAGLE\n");
		return false;
	}

	if (!parser.GetUnsignedInt("IOCP", "SESSIONS", &config.maxSessionCount))
	{
		printf("Missing or invalid config: [IOCP] SESSIONS\n");
		return false;
	}

	if (!parser.GetUnsignedInt("IOCP", "HEADERSIZE", &config.headerSize))
	{
		printf("Missing or invalid config: [IOCP] HEADERSIZE\n");
		return false;
	}


	if (!parser.GetUnsignedChar("IOCP", "PACKETCODE", &config.packetCode))
	{
		printf("Missing or invalid config: [IOCP] PACKETCODE\n");
		return false;
	}

	return true;

}
