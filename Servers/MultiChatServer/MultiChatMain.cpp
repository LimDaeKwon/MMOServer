#include <iostream>
#include "MultiChatServer.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "SystemMonitoring.h"
#include "ProcessMonitoring.h"


CrashDump zz;

#pragma comment(lib, "winmm.lib")

#define FILENAME "MultiChatThreadSetting.config"


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

	timeBeginPeriod(1);
	InitProfile();
	ParseThreadDataFile(FILENAME);


	MultiChatServer* ChatInstance = new MultiChatServer;
	ChatInstance->Start(ThreadData[IP], atoi(ThreadData[PORT]), atoi(ThreadData[THREADS]), atoi(ThreadData[CONCURRENT]), atoi(ThreadData[NAGLE]), atoi(ThreadData[SESSIONS]), atoi(ThreadData[HEADERSIZE]), atoi(ThreadData[PACKETCODE]));



	while (1)
	{
		wprintf(L"\n------------------------------------------------------------ \n");

		wprintf(L"CPakcet \n");
		wprintf(L"UseSize  : %d   Capacity  :  %d\n", CPacket::GetUseSize(), CPacket::GetCapacity());
		wprintf(L"Contents \n");
		wprintf(L"AcceptTotal  :  %lld  \n", ChatInstance->GetAcceptTotal());
		wprintf(L"Session  :  %d  \n", ChatInstance->GetSessionNum());
		wprintf(L"UnloginPlayer  :  %d  Player  :  %d   \n", ChatInstance->GetUnloginPlayer(), ChatInstance->GetLoginPlayer());


		wprintf(L"\n------------------------------------------------------------ \n");
		wprintf(L"Disconnect \n");
		wprintf(L"DisconnectTotal  :  %d \n", ChatInstance->GetDisconnectCount());
		wprintf(L"DCWrongPacket :  %d	    DCAuthFailed  :  %d \n", ChatInstance->GetDCWrongPacket(), ChatInstance->GetDCAuthFailed());
		wprintf(L"DCUnloginTimeout  :  %d     GetDCLoginAgain  :  %d \n", ChatInstance->GetDCUnloginTimeout(), ChatInstance->GetDCLoginAgain());
		wprintf(L"DCSendBufferFull  :  %d     DCDuplicateLogin  :  %d\n", ChatInstance->GetDCSendBufferFull(), ChatInstance->GetDCDuplicateLogin());
		wprintf(L"DCPacketCodeError  :  %d   DCSessionFull  :  %d \n", ChatInstance->GetDCPacketCodeError(), ChatInstance->GetDCSessionFull());
		wprintf(L"DCDecodeError  :  %d  DCImpossiblePacketLength  :  %d \n", ChatInstance->GetDCDecodeError(), ChatInstance->GetDCImpossiblePacketLength());

		wprintf(L"\n------------------------------------------------------------ \n");
		wprintf(L"TPS \n");
		wprintf(L"AccpetTPS  :  %d \n", ChatInstance->GetAcceptTPS());
		wprintf(L"RecvTPS  :  %d     SendTPS  :  %d  MonitorSendTPS  :  %d\n", ChatInstance->GetRecvMessageTPS(), ChatInstance->GetSendMessageTPS(), ChatInstance->monitoringClient_.send_message_count);
		wprintf(L"LogicTPS  :  %d      LoginTPS  :  %d\n", ChatInstance->GetLogicTPS(), ChatInstance->GetLoginTPS());
		wprintf(L"SectorMoveTPS  :  %d     ChatTPS  :  %d \n", ChatInstance->GetSectorMoveTPS(), ChatInstance->GetChatTPS());

		InterlockedExchange(&ChatInstance->monitoringClient_.send_message_count, 0);




		Sleep(1000);


		if (_kbhit())
		{
			char c = _getch();
			if (c == 's' || c == 'C')
			{
				ProfileDataOutText(L"EchoProfileData");
			}

			if (c == 'r' || c == 'R')
			{
				ProfileReset();
			}
		}
	}




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