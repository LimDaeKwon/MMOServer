#include "SystemMonitoring.h"
#include <process.h>




SystemMonitoring::SystemMonitoring()
{
	UpdateThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, 0, nullptr);


	PdhOpenQuery(NULL, NULL, &ServerNPQuery);
	PdhAddCounter(ServerNPQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &ServerNPTotal);
	PdhCollectQueryData(ServerNPQuery);

	PdhOpenQuery(NULL, NULL, &ServerAMQuery);
	PdhAddCounter(ServerAMQuery, L"\\Memory\\Available MBytes", NULL, &ServerAMTotal);
	PdhCollectQueryData(ServerAMQuery);

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


