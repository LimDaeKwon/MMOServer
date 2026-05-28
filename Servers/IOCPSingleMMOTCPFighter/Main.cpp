// NetworkLibrary.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "MMOTCPServer_Single.h"
#include "windows.h"
#include "conio.h"
#include "Profiler.h"
#include "CrashDump.h"
#include "queue"

CrashDump zz;


#pragma comment(lib, "winmm.lib")

#define THREADFILENAME "MMOTCPFighterThreadSetting.config"
#define GAMEFILENAME "GameSetting.config"



enum thread_setting_enum
{
	IP, PORT, THREADS, CONCURRENT, NAGLE, SESSIONS, HEADERSIZE
};
unsigned int GlobalChecksum;


bool ParseThreadDataFile(const char* file_name);
bool ParseGameDataFile(const char* file_name);

char ThreadData[7][200];

char GameData[7][200];

int main()
{
	timeBeginPeriod(1);

	InitProfile();

	ParseThreadDataFile(THREADFILENAME);
	ParseGameDataFile(GAMEFILENAME);

	//로직이 싱글인지 아닌지. 


	MMOTCPServer_Single game_instance;

	unsigned int Port1 = atoi(ThreadData[PORT]);

	game_instance.Start(ThreadData[IP], Port1, atoi(ThreadData[THREADS]), atoi(ThreadData[CONCURRENT]), atoi(ThreadData[NAGLE]), atoi(ThreadData[SESSIONS]), atoi(ThreadData[HEADERSIZE]));


	while (1)
	{

		if (_kbhit())
		{
			char c = _getch();
			if (c == 's' || c == 'C')
			{
				ProfileDataOutText(L"ProfileData");
			}

			if (c == 'r' || c == 'R')
			{
				ProfileReset();
			}
		}
		//서버 컨트롤
		//그 순간 서버의 덤프를 남긴다 -> 메모리르 자료구조라고 봤을 때 
		// 누군가가 쓰고있을 때 읽어도 되는가?>
		// 

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


bool ParseGameDataFile(const char* file_name)
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
						memcpy_s(GameData[Index], size, file_buffer + i + 1, size);
						GameData[Index][size] = '\0';
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
