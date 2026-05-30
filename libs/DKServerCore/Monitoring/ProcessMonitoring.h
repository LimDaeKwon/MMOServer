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



//        //모든 풀은 관리 대상이다..
//        // 캐릭터 풀
//        // 현재 접속중인 캐릭터 , 세션
//        // 언로그인 로그인 타임아웃 초 정보
//        // 채팅서버 로그인 , 언 로그인
//        // 
//        // 로직 스레드 메시지 처리 횟수
//        // 메시지 타입에 따른 초당 처리 횟수
//        // 
//        //cpacket
//        // 채팅서버 ChatServer 실행 여부 ON / OFF
//        // 채팅서버 ChatServer CPU 사용률
//        // 채팅서버 ChatServer 메모리 사용 MByte
//        // 
//        // 서버컴퓨터 CPU 전체 사용률
//        // 
//        // 서버컴퓨터 논페이지 메모리 MByte   L"\\Memory\\Pool Nonpaged Bytes"
//        // 서버컴퓨터 네트워크 수신량 KByte
//        // 서버컴퓨터 네트워크 송신량 KByte
//        // 서버컴퓨터 사용가능 메모리   "\Memory\Available MBytes
//        //어셉트