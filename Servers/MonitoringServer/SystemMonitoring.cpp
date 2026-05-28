#include "SystemMonitoring.h"
#include <process.h>




SystemMonitoring::SystemMonitoring()
{
	UpdateThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, 0, nullptr);

	const wchar_t* InterfaceName[NETWORK_INTERFACE_COUNT] =
	{
		L"Intel[R] Ethernet Controller X540-AT2 _2",
		L"Intel[R] I210 Gigabit Network Connection",
		L"Intel[R] I210 Gigabit Network Connection _2"
	};


	PdhOpenQuery(NULL, NULL, &ServerNPQuery);
	PdhAddCounter(ServerNPQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &ServerNPTotal);
	PdhCollectQueryData(ServerNPQuery);

	PdhOpenQuery(NULL, NULL, &ServerAMQuery);
	PdhAddCounter(ServerAMQuery, L"\\Memory\\Available MBytes", NULL, &ServerAMTotal);
	PdhCollectQueryData(ServerAMQuery);

	wchar_t SendPath[256];
	wchar_t RecvPath[256];

	for (int i = 0; i < NETWORK_INTERFACE_COUNT; i++)
	{
		wsprintf(SendPath, L"\\Network Interface(%s)\\Bytes Sent/sec", InterfaceName[i]);
		wsprintf(RecvPath, L"\\Network Interface(%s)\\Bytes Received/sec", InterfaceName[i]);

		PdhOpenQuery(NULL, NULL, &ServerNetSendQuery[i]);
		PdhAddCounter(ServerNetSendQuery[i], SendPath, NULL, &ServerNetSendTotal[i]);
		PdhCollectQueryData(ServerNetSendQuery[i]);

		PdhOpenQuery(NULL, NULL, &ServerNetRecvQuery[i]);
		PdhAddCounter(ServerNetRecvQuery[i], RecvPath, NULL, &ServerNetRecvTotal[i]);
		PdhCollectQueryData(ServerNetRecvQuery[i]);
	}

}

SystemMonitoring::~SystemMonitoring()
{
}

double SystemMonitoring::GetServerNonPagedBytes()
{
	return ServerNPCounterVal.doubleValue;
}

int SystemMonitoring::GetServerNonPagedMBytes()
{
	LONGLONG nonPagedBytes = static_cast<LONGLONG>(ServerNPCounterVal.doubleValue);
	int nonPagedMBytes = static_cast<int>(nonPagedBytes / (1024LL * 1024LL));

	return nonPagedMBytes;
}

double SystemMonitoring::GetServerAvailableMBytes()
{
	return ServerAMCounterVal.doubleValue;
}

double SystemMonitoring::GetServerNetSendKBytes()
{
	double TotalSend = 0;

	for (int i = 0; i < NETWORK_INTERFACE_COUNT; i++)
	{
		TotalSend += ServerNetSendCounterVal[i].doubleValue;
	}

	return TotalSend / 1024.0;
}

double SystemMonitoring::GetServerNetRecvKBytes()
{
	double TotalRecv = 0;

	for (int i = 0; i < NETWORK_INTERFACE_COUNT; i++)
	{
		TotalRecv += ServerNetRecvCounterVal[i].doubleValue;
	}

	return TotalRecv / 1024.0;
}

unsigned int WINAPI SystemMonitoring::UpdateThread(LPVOID this_ptr)
{
	SystemMonitoring* Monitor = static_cast<SystemMonitoring*>(this_ptr);

	while (true)
	{
		Sleep(1000);
		Monitor->UpdateCpuTime();
		// 1초마다 갱신
		PdhCollectQueryData(Monitor->ServerNPQuery);
		PdhGetFormattedCounterValue(Monitor->ServerNPTotal, PDH_FMT_DOUBLE, NULL, &Monitor->ServerNPCounterVal);
		
		PdhCollectQueryData(Monitor->ServerAMQuery);
		PdhGetFormattedCounterValue(Monitor->ServerAMTotal, PDH_FMT_DOUBLE, NULL, &Monitor->ServerAMCounterVal);

		for (int i = 0; i < NETWORK_INTERFACE_COUNT; i++)
		{
			PdhCollectQueryData(Monitor->ServerNetSendQuery[i]);
			PdhGetFormattedCounterValue(Monitor->ServerNetSendTotal[i], PDH_FMT_DOUBLE, NULL, &Monitor->ServerNetSendCounterVal[i]);

			PdhCollectQueryData(Monitor->ServerNetRecvQuery[i]);
			PdhGetFormattedCounterValue(Monitor->ServerNetRecvTotal[i], PDH_FMT_DOUBLE, NULL, &Monitor->ServerNetRecvCounterVal[i]);
		}

	}

	return 0;
}


void SystemMonitoring::UpdateCpuTime(void)
{
	c.UpdateCpuTime();
}
float SystemMonitoring::ProcessorTotal(void)
{
	return c.ProcessorTotal();
}
float SystemMonitoring::ProcessorUser(void)
{
	return c.ProcessorUser();
}
float SystemMonitoring::ProcessorKernel(void)
{
	return c.ProcessorKernel();
}
float SystemMonitoring::ProcessTotal(void)
{
	return c.ProcessTotal();
}
float SystemMonitoring::ProcessUser(void)
{
	return c.ProcessUser();
}
float SystemMonitoring::ProcessKernel(void)
{
	return c.ProcessKernel();
}


