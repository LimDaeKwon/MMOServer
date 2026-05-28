#include <iostream>
#include "CSMonitoringServer.h"
#include "SSMonitoringServer.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "ProcessMonitoring.h"


CrashDump zz;

#pragma comment(lib, "winmm.lib")

#define FILENAME "MonitoringServer.config"


enum thread_setting_enum // 여기에 SendBuffer 사이즈 설정 옵션도 넣기. 
{
	IP, PORT, THREADS, CONCURRENT, NAGLE, SESSIONS, HEADERSIZE, PACKETCODE
};
unsigned int GlobalChecksum;

#define RFLAG 0x80000000

bool ParseThreadDataFile(const char* file_name);

char ThreadData[8][200];


int main()
{

	ProcessMonitoring b;

	timeBeginPeriod(1);
	InitProfile();
	ParseThreadDataFile(FILENAME);

	SSMonitoringServer* SSMoniterInstance = new SSMonitoringServer;
	SSMoniterInstance->Start(ThreadData[IP], 5670, 2, 2, atoi(ThreadData[NAGLE]), 10, 2);



	CSMonitoringServer* CSMonitoring = new CSMonitoringServer(SSMoniterInstance->loginServerData_, SSMoniterInstance->gameServerData_, SSMoniterInstance->chatServerData_, SSMoniterInstance->systemData_);
	CSMonitoring->Start(ThreadData[IP], atoi(ThreadData[PORT]), atoi(ThreadData[THREADS]), atoi(ThreadData[CONCURRENT]), atoi(ThreadData[NAGLE]), atoi(ThreadData[SESSIONS]), atoi(ThreadData[HEADERSIZE]), atoi(ThreadData[PACKETCODE]));



	while (1)
	{
		wprintf(L"\n------------------------------------------------------------ \n");

		wprintf(L"CPakcet \n");
		wprintf(L"UseSize  : %d   Capacity  :  %d\n", CPacket::GetUseSize(), CPacket::GetCapacity());
		wprintf(L"Contents \n");
		wprintf(L"CSSession  :  %d  SSSession  :  %d  \n", CSMonitoring->GetSessionNum(), SSMoniterInstance->GetSessionNum());
		wprintf(L"UnloginPlayer  :  %d  Player  :  %d   \n", CSMonitoring->GetUnloginPlayer(), CSMonitoring->GetLoginPlayer());


		wprintf(L"\n------------------------------------------------------------ \n");
		wprintf(L"Disconnect \n");
		wprintf(L"DisconnectTotal  :  %d \n", CSMonitoring->GetDisconnectCount());
		wprintf(L"DCWrongPacket :  %d	    DCAuthFailed  :  %d \n", CSMonitoring->GetDCWrongPacket(), CSMonitoring->GetDCAuthFailed());
		wprintf(L"DCUnloginTimeout  :  %d     DCLoginTimeout  :  %d \n", CSMonitoring->GetDCUnloginTimeout(), CSMonitoring->GetDCLoginTimeout());
		wprintf(L"DCSendBufferFull  :  %d     DCDuplicateLogin  :  %d\n", CSMonitoring->GetDCSendBufferFull(), CSMonitoring->GetDCDuplicateLogin());
		wprintf(L"DCPacketCodeError  :  %d   DCSessionFull  :  %d \n", CSMonitoring->GetDCPacketCodeError(), CSMonitoring->GetDCSessionFull());
		wprintf(L"DCDecodeError  :  %d  DCImpossiblePacketLength  :  %d \n", CSMonitoring->GetDCDecodeError(), CSMonitoring->GetDCImpossiblePacketLength());

		wprintf(L"\n------------------------------------------------------------ \n");
		wprintf(L"TPS \n");
		wprintf(L"AccpetTPS  :  %d \n", CSMonitoring->GetAcceptTPS());
		wprintf(L"RecvTPS  :  %d     SendTPS  :  %d \n", CSMonitoring->GetRecvMessageTPS(), CSMonitoring->GetSendMessageTPS());
		wprintf(L"\n------------------------------------------------------------ \n");

		//wprintf(L"Monitoring \n\n");
		//wprintf(L"ProcessNonPagedPool  : %f%%   ServerNonPagedPool  :  %f%% \n", b.GetProcessNPMemory(), a.GetServerNonPagedBytes());
		//wprintf(L"ProcessUseMemory: %f%%  ServerAvailableMemory: %f%%  \n", b.GetProcessUserMemory(), a.GetServerAvailableMBytes());
		//wprintf(L"ProcessCPUUsage: %f%%   ServerCPUUsage: %f%%\n", b.ProcessTotal(), a.ProcessorTotal());





		Sleep(1000);


	}

	Sleep(INFINITE);


	timeEndPeriod(1);

}


bool ParseThreadDataFile(const char* file_name)
{
	FILE* worker_information;
	int file_size;
	char* file_buffer;

	if (fopen_s(&worker_information, file_name, "rb") == 0)
	{
		if (worker_information == NULL)
		{
			return false;
		}


		fseek(worker_information, 0, SEEK_END);
		file_size = ftell(worker_information);
		rewind(worker_information);


		file_buffer = (char*)malloc(file_size);
		if (file_buffer == NULL) {
			printf("Error: Memory allocation failed.\n");
			DebugBreak();
			return false;
		}

		size_t bytesRead = fread_s(file_buffer, file_size, 1, file_size, worker_information);
		if (bytesRead != file_size)
		{

			printf("Error: Failed to read entire file. Expected %d bytes, Read: %zu bytes.\n", file_size, bytesRead);

			return false;
		}

		int start_position = 0;
		int end_postiion = 0;

		//시작위치 + 0d 0a 넘기기. 

		for (int i = 0; i < file_size; ++i)
		{
			if (file_buffer[i] == '{')
			{
				start_position = i + 2;
				break;
			}
		}

		for (int i = file_size - 1; i >= 0; --i)
		{


			if (file_buffer[i] == '}')
			{
				end_postiion = i;
				break;
			}
		}

		int Index = 0;

		int size;
		for (int i = start_position; i < end_postiion; ++i)
		{


			if (file_buffer[i] == ':')
			{
				for (int j = i + 1; ; ++j)
				{
					if (file_buffer[j] == 0x0d)
					{
						size = j - i - 1;
						memcpy_s(ThreadData[Index], size, file_buffer + i + 1, size);
						ThreadData[Index][size] = '\0';
						Index++;
						break;
					}
				}
				start_position = i + size;
			}
		}

		free(file_buffer);

		fclose(worker_information);
	}


	return true;

}