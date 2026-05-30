#pragma once

#include <Pdh.h>
#include "CPUUsage.h"
#pragma comment(lib,"Pdh.lib")


class ProcessMonitoring
{
public:

	ProcessMonitoring();
	virtual ~ProcessMonitoring();

	//프로세스 유저 메모리
	WCHAR ProcessUserMemoryQueryStr[200];
	PDH_HQUERY ProcessUserMemoryQuery;
	PDH_HCOUNTER ProcessUserMemoryTotal;
	PDH_FMT_COUNTERVALUE ProcessUserMemoryCounterVal;

	double GetProcessUserMemory();
	
	int GetProcessUserMemoryMBytes();
	////* cpu process 사용률 // 그걸로
	////* 프로세스 유저할당 메모리 "\Process(ChatDummy_20221114)\Private Bytes"
	////* 프로세스 논페이지 메모리 \Process(ChatDummy_20221114)\Pool Nonpaged Bytes
	

	//np사용률
	WCHAR ProcessNPMemoryQueryStr[200];
	PDH_HQUERY ProcessNPMemoryQuery;
	PDH_HCOUNTER ProcessNPMemoryTotal;
	PDH_FMT_COUNTERVALUE ProcessNPMemoryCounterVal;
	double GetProcessNPMemory();


	HANDLE UpdateThreadHandle;
	static unsigned int WINAPI UpdateThread(LPVOID this_ptr);

	CpuUsage c;

	void UpdateCpuTime(void);

	float ProcessTotal(void);
	float ProcessUser(void);
	float ProcessKernel(void);

};
