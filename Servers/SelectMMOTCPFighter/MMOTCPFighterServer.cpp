// TCPFighterServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "Network.h"
#include "Contents.h"
#include "ws2tcpip.h"
#include "CrashDump.h"
#pragma comment(lib,"Winmm.lib")

CrashDump a;

unsigned int GlobalChecksum;

//bool Shutdown = false;



int main()
{
	timeBeginPeriod(1);
	Initialize();
	while (1)
	{
		Network();

		Update();

		//ServerControl();

	}

	WSACleanup();
	timeEndPeriod(1);
	return 0;


}
