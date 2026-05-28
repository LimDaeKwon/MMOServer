#include <iostream>
#include "GameEchoServer.h"

#include "conio.h"
#include "Profiler.h"

#include "CrashDump.h"

#include "SystemMonitoring.h"
#include "ProcessMonitoring.h"


CrashDump zz;

#pragma comment(lib, "winmm.lib")

#define FILENAME "GameEchoServer.config"


enum thread_setting_enum // 여기에 SendBuffer 사이즈 설정 옵션도 넣기. 
{
	IP, PORT, THREADS, CONCURRENT, NAGLE, SESSIONS, HEADERSIZE, SYNCASYNC, SENDTHREADS, PACKETCODE
};
unsigned int GlobalChecksum;

#define RFLAG 0x80000000

bool ParseThreadDataFile(const char* file_name);

char ThreadData[10][200];


int main()
{

	timeBeginPeriod(1);
	InitProfile();
	ParseThreadDataFile(FILENAME);


	GameEchoServer* GameEchoInstance = new GameEchoServer;
	GameEchoInstance->Start(ThreadData[IP], atoi(ThreadData[PORT]), atoi(ThreadData[THREADS]), atoi(ThreadData[CONCURRENT]), atoi(ThreadData[NAGLE]), atoi(ThreadData[SESSIONS]), atoi(ThreadData[HEADERSIZE]), atoi(ThreadData[SYNCASYNC]), atoi(ThreadData[SENDTHREADS]), atoi(ThreadData[PACKETCODE])); 



	while (1)
	{
		wprintf(L"\n------------------------------------------------------------ \n");

		wprintf(L"CPakcet \n");
		wprintf(L"UseSize  : %d   Capacity  :  %d\n", CPacket::GetUseSize(), CPacket::GetCapacity());
		wprintf(L"Contents \n");
		wprintf(L"AcceptTotal  :  %lld  \n", GameEchoInstance->GetAcceptTotal());
		wprintf(L"Session  :  %d  \n", GameEchoInstance->GetSessionNum());
		wprintf(L"UnloginPlayer  :  %d  Player  :  %d   \n", GameEchoInstance->GetUnloginPlayer(), GameEchoInstance->GetLoginPlayer());


		wprintf(L"\n------------------------------------------------------------ \n");
		wprintf(L"Disconnect \n");
		wprintf(L"DisconnectTotal  :  %d \n", GameEchoInstance->GetDisconnectCount());
		wprintf(L"DCWrongPacket :  %d	    DCAuthFailed  :  %d \n", GameEchoInstance->GetDCWrongPacket(), GameEchoInstance->GetDCAuthFailed());
		wprintf(L"DCUnloginTimeout  :  %d     DCLoginTimeout  :  %d \n", GameEchoInstance->GetDCUnloginTimeout(), GameEchoInstance->GetDCLoginTimeout());
		wprintf(L"DCSendBufferFull  :  %d     DCDuplicateLogin  :  %d\n", GameEchoInstance->GetDCSendBufferFull(), GameEchoInstance->GetDCDuplicateLogin());
		wprintf(L"DCPacketCodeError  :  %d   DCSessionFull  :  %d \n", GameEchoInstance->GetDCPacketCodeError(), GameEchoInstance->GetDCSessionFull());
		wprintf(L"DCDecodeError  :  %d  DCImpossiblePacketLength  :  %d \n", GameEchoInstance->GetDCDecodeError(), GameEchoInstance->GetDCImpossiblePacketLength());

		wprintf(L"\n------------------------------------------------------------ \n");
		wprintf(L"FPS \n");
		wprintf(L"AccpetFPS  :  %d GameFPS  :  %d \n", GameEchoInstance->groupManager_.GetGroupFPS(0), GameEchoInstance->groupManager_.GetGroupFPS(1));


		wprintf(L"TPS \n");
		wprintf(L"AccpetTPS  :  %d \n", GameEchoInstance->GetAcceptTPS());
		wprintf(L"Game RecvTPS  :  %d     SendTPS  :  %d  \n", GameEchoInstance->GetRecvMessageTPS(), GameEchoInstance->GetSendMessageTPS());


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