
#include "SelectMMOTCPFighter.h"
#include "CrashDump.h"




#pragma comment(lib, "Winmm.lib")

CrashDump crashDump;

unsigned int GlobalChecksum;

int main()
{
    timeBeginPeriod(1);

	SelectMMOTCPFighter* instance = new SelectMMOTCPFighter();

    //Initialize();

    //while (true)
    //{
    //    Network();

    //    Update();

    //    // ServerControl();
    //}

    //WSACleanup();

	instance->Start("0.0.0.0",25000, 0, 10000, PacketCode, 40);

    Sleep(INFINITE);


    timeEndPeriod(1);

    return 0;
}
