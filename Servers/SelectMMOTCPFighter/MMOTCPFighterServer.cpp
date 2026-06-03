#include "Contents.h"
#include "CrashDump.h"
#include "Network.h"

#pragma comment(lib, "Winmm.lib")

CrashDump crashDump;

unsigned int GlobalChecksum;

int main()
{
    timeBeginPeriod(1);

    Initialize();

    while (true)
    {
        Network();

        Update();

        // ServerControl();
    }

    WSACleanup();
    timeEndPeriod(1);

    return 0;
}