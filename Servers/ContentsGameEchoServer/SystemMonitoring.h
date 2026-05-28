#pragma once

#include <Pdh.h>
#include "CPUUsage.h"


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

	////네트워크 송신
	//PDH_HQUERY ServerNetSendQuery;
	//PDH_HCOUNTER ServerNetSendTotal;
	//PDH_FMT_COUNTERVALUE ServerNetSendCounterVal;
	//double GetServerNetSend();

	////네트워크 수신
	//PDH_HQUERY ServerNetRecvQuery;
	//PDH_HCOUNTER ServerNetRecvTotal;
	//PDH_FMT_COUNTERVALUE ServerNetRecvCounterVal;
	//double GetServerNetRecv();


	HANDLE UpdateThreadHandle;
	static unsigned int WINAPI UpdateThread(LPVOID this_ptr);

	CCpuUsage c;

	void UpdateCpuTime(void);
	float ProcessorTotal(void);
	float ProcessorUser(void);
	float ProcessorKernel(void);
	float ProcessTotal(void);
	float ProcessUser(void);
	float ProcessKernel(void);

};
