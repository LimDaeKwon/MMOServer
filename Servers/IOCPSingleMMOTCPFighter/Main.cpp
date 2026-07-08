// NetworkLibrary.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "MMOTCPServerSingle.h"
#include "MMOTCPServerSingleRB.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "queue"
#include "ServerStartConfig.h"
#include "DKParser.h"

using namespace std;


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
		while (1)
		{

		}
		return 1;
	}

	DKServerCore::IocpServerStartConfig config;
	if (!SetConfigValue(parser, config))
	{
		while (1)
		{

		}
		return 1;
	}
	unsigned int bufferMode;
	if (!parser.GetUnsignedInt("IOCP", "BUFFERMODE", &bufferMode))
	{
		printf("Missing or invalid config: [IOCP] BUFFERMODE\n");
		while (1)
		{

		}
		return false;
	}
	MMOTCPServerSingleRB* gameInstanceRB;
	MMOTCPServerSingle* gameInstance;

	int localCount = 0;
	if (bufferMode == 0) // 링버퍼 모드
	{
		gameInstanceRB = new MMOTCPServerSingleRB;

		gameInstanceRB->Start(config);
		while (localCount != 600)
		{

			cout << "RB Message Queue Size: " << gameInstanceRB->GetMessageQueueSize() << endl;
			localCount++;
			Sleep(1000);


			//서버 컨트롤
			//그 순간 서버의 덤프를 남긴다 -> 메모리를 자료구조라고 봤을 때
			// 누군가가 쓰고있을 때 읽어도 되는가?>
			//
		}

		wprintf(L"SendPost Avg : %.3f us / Call : %lld\n", gameInstanceRB->GetSendPostAverageMicroSecond(), gameInstanceRB->GetSendPostProfileCall());

	}
	else
	{
		gameInstance = new MMOTCPServerSingle;

		gameInstance->Start(config);
		while (localCount != 600)
		{

			cout << "Message Queue Size: " << gameInstance->GetMessageQueueSize() << endl;

			localCount++;
			Sleep(1000);


			//서버 컨트롤
			//그 순간 서버의 덤프를 남긴다 -> 메모리를 자료구조라고 봤을 때
			// 누군가가 쓰고있을 때 읽어도 되는가?>
			//

		}

		wprintf(L"SendPost Avg : %.3f us / Call : %lld\n", gameInstance->GetSendPostAverageMicroSecond(), gameInstance->GetSendPostProfileCall());
	}

	ProfileDataOutText(L"SelectMMOTCPFighter_Profile.txt");



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
