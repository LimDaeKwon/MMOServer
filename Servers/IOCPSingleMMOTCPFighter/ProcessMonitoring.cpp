#include "ProcessMonitoring.h"
#include <process.h>



ProcessMonitoring::ProcessMonitoring() 
{
	UpdateThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, this, 0, nullptr);



	wchar_t path[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, path, MAX_PATH);

	// 파일명만 뽑기
	wchar_t* exeName = wcsrchr(path, L'\\');
	exeName++;

	wchar_t* movePtr = exeName;
	while (1)
	{
		if (*movePtr == L'.')
		{
			*movePtr = L'\0';
			break;
		}
		movePtr++;
	}
	//exeName
	//"\Process(ChatDummy_20221114)\Private Bytes"
	wsprintf(ProcessUserMemoryQueryStr, L"\\Process(%s)\\Private Bytes", exeName);
	
	PdhOpenQuery(NULL, NULL, &ProcessUserMemoryQuery);
	PdhAddCounter(ProcessUserMemoryQuery, ProcessUserMemoryQueryStr, NULL, &ProcessUserMemoryTotal);
	PdhCollectQueryData(ProcessUserMemoryQuery);
	
	wsprintf(ProcessNPMemoryQueryStr, L"\\Process(%s)\\Pool Nonpaged Bytes", exeName);
	PdhOpenQuery(NULL, NULL, &ProcessNPMemoryQuery);
	PdhAddCounter(ProcessNPMemoryQuery, ProcessNPMemoryQueryStr, NULL, &ProcessNPMemoryTotal);
	PdhCollectQueryData(ProcessNPMemoryQuery);

}

ProcessMonitoring::~ProcessMonitoring()
{
}

double ProcessMonitoring::GetProcessUserMemory()
{
	return ProcessUserMemoryCounterVal.doubleValue;
}

double ProcessMonitoring::GetProcessNPMemory()
{
	return ProcessNPMemoryCounterVal.doubleValue;
}

unsigned int WINAPI ProcessMonitoring::UpdateThread(LPVOID this_ptr)
{
	ProcessMonitoring* Monitor = static_cast<ProcessMonitoring*>(this_ptr);

	while (true)
	{
		Sleep(100);
		Monitor->UpdateCpuTime();
		// 1초마다 갱신
		PdhCollectQueryData(Monitor->ProcessUserMemoryQuery);
		PdhGetFormattedCounterValue(Monitor->ProcessUserMemoryTotal, PDH_FMT_DOUBLE, NULL, &Monitor->ProcessUserMemoryCounterVal);

		PdhCollectQueryData(Monitor->ProcessNPMemoryQuery);
		PdhGetFormattedCounterValue(Monitor->ProcessNPMemoryTotal, PDH_FMT_DOUBLE, NULL, &Monitor->ProcessNPMemoryCounterVal);

	}

	return 0;
}


void ProcessMonitoring::UpdateCpuTime(void)
{
	c.UpdateCpuTime();
}

float ProcessMonitoring::ProcessTotal(void)
{
	return c.ProcessTotal();
}
float ProcessMonitoring::ProcessUser(void)
{
	return c.ProcessUser();
}
float ProcessMonitoring::ProcessKernel(void)
{
	return c.ProcessKernel();
}


