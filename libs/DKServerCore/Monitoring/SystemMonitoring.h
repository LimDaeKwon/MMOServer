#pragma once

#include <Pdh.h>
#include "CPUUsage.h"

#define NETWORK_INTERFACE_COUNT 3
class SystemMonitoring
{
public:

	SystemMonitoring();
	virtual ~SystemMonitoring();

	//서버 NPPool
	PDH_HQUERY ServerNPQuery;
	PDH_HCOUNTER ServerNPTotal;
	PDH_FMT_COUNTERVALUE ServerNPCounterVal;

	// 사용가능 메모리 L"\\Memory\\Available MBytes"
	double GetServerNonPagedBytes();

	int GetServerNonPagedMBytes();
	
	PDH_HQUERY ServerAMQuery;
	PDH_HCOUNTER ServerAMTotal;
	PDH_FMT_COUNTERVALUE ServerAMCounterVal;
	double GetServerAvailableMBytes();

	//네트워크 송신
	PDH_HQUERY ServerNetSendQuery[NETWORK_INTERFACE_COUNT];
	PDH_HCOUNTER ServerNetSendTotal[NETWORK_INTERFACE_COUNT];
	PDH_FMT_COUNTERVALUE ServerNetSendCounterVal[NETWORK_INTERFACE_COUNT];

	//네트워크 수신
	PDH_HQUERY ServerNetRecvQuery[NETWORK_INTERFACE_COUNT];
	PDH_HCOUNTER ServerNetRecvTotal[NETWORK_INTERFACE_COUNT];
	PDH_FMT_COUNTERVALUE ServerNetRecvCounterVal[NETWORK_INTERFACE_COUNT];

	double GetServerNetSendKBytes();
	double GetServerNetRecvKBytes();

	HANDLE UpdateThreadHandle;
	static unsigned int WINAPI UpdateThread(LPVOID this_ptr);

	CpuUsage c;

	void UpdateCpuTime(void);
	float ProcessorTotal(void);
	float ProcessorUser(void);
	float ProcessorKernel(void);
	float ProcessTotal(void);
	float ProcessUser(void);
	float ProcessKernel(void);

};
