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

constexpr const char* ThreadFileName = "MMOTCPFighterThreadSetting.config";
constexpr const char* GameFileName = "GameSetting.config";



enum ThreadSettingType
{
	ThreadSettingIp, ThreadSettingPort, ThreadSettingThreads, ThreadSettingConcurrent, ThreadSettingNagle, ThreadSettingSessions, ThreadSettingHeaderSize
};
unsigned int GlobalChecksum;


bool ParseThreadDataFile(const char* fileName);
bool ParseGameDataFile(const char* fileName);

char threadData[7][200];

char gameData[7][200];

int main()
{
	timeBeginPeriod(1);

	InitProfile();

	ParseThreadDataFile(ThreadFileName);
	ParseGameDataFile(GameFileName);

	//로직이 싱글인지 아닌지.


	MMOTCPServerSingle gameInstance;

	unsigned int port = atoi(threadData[ThreadSettingPort]);

	gameInstance.Start(threadData[ThreadSettingIp], port, atoi(threadData[ThreadSettingThreads]), atoi(threadData[ThreadSettingConcurrent]), atoi(threadData[ThreadSettingNagle]), atoi(threadData[ThreadSettingSessions]), atoi(threadData[ThreadSettingHeaderSize]));


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


bool ParseThreadDataFile(const char* fileName)
{
	FILE* workerInformation;
	int fileSize;
	char* fileBuffer;

	if (fopen_s(&workerInformation, fileName, "rb") == 0)
	{
		if (workerInformation == nullptr)
		{
			return false;
		}


		fseek(workerInformation, 0, SEEK_END);
		fileSize = ftell(workerInformation);
		rewind(workerInformation);


		fileBuffer = static_cast<char*>(malloc(fileSize));
		if (fileBuffer == nullptr)
		{
			printf("Error: Memory allocation failed.\n");
			DebugBreak();
			return false;
		}

		size_t bytesRead = fread_s(fileBuffer, fileSize, 1, fileSize, workerInformation);
		if (bytesRead != fileSize)
		{

			printf("Error: Failed to read entire file. Expected %d bytes, Read: %zu bytes.\n", fileSize, bytesRead);

			return false;
		}

		int startPosition = 0;
		int endPosition = 0;

		//시작위치 + 0d 0a 넘기기.

		for (int i = 0; i < fileSize; ++i)
		{
			if (fileBuffer[i] == '{')
			{
				startPosition = i + 2;
				break;
			}
		}

		for (int i = fileSize - 1; i >= 0; --i)
		{


			if (fileBuffer[i] == '}')
			{
				endPosition = i;
				break;
			}
		}

		int index = 0;

		int size;
		for (int i = startPosition; i < endPosition; ++i)
		{


			if (fileBuffer[i] == ':')
			{
				for (int j = i + 1; ; ++j)
				{
					if (fileBuffer[j] == 0x0d)
					{
						size = j - i - 1;
						memcpy_s(threadData[index], size, fileBuffer + i + 1, size);
						threadData[index][size] = '\0';
						index++;
						break;
					}
				}
				startPosition = i + size;
			}
		}

		free(fileBuffer);

		fclose(workerInformation);
	}


	return true;

}


bool ParseGameDataFile(const char* fileName)
{
	FILE* workerInformation;
	int fileSize;
	char* fileBuffer;

	if (fopen_s(&workerInformation, fileName, "rb") == 0)
	{
		if (workerInformation == nullptr)
		{
			return false;
		}


		fseek(workerInformation, 0, SEEK_END);
		fileSize = ftell(workerInformation);
		rewind(workerInformation);


		fileBuffer = static_cast<char*>(malloc(fileSize));
		if (fileBuffer == nullptr)
		{
			printf("Error: Memory allocation failed.\n");
			DebugBreak();
			return false;
		}

		size_t bytesRead = fread_s(fileBuffer, fileSize, 1, fileSize, workerInformation);
		if (bytesRead != fileSize)
		{

			printf("Error: Failed to read entire file. Expected %d bytes, Read: %zu bytes.\n", fileSize, bytesRead);

			return false;
		}

		int startPosition = 0;
		int endPosition = 0;

		//시작위치 + 0d 0a 넘기기.

		for (int i = 0; i < fileSize; ++i)
		{
			if (fileBuffer[i] == '{')
			{
				startPosition = i + 2;
				break;
			}
		}

		for (int i = fileSize - 1; i >= 0; --i)
		{


			if (fileBuffer[i] == '}')
			{
				endPosition = i;
				break;
			}
		}

		int index = 0;

		int size;
		for (int i = startPosition; i < endPosition; ++i)
		{


			if (fileBuffer[i] == ':')
			{
				for (int j = i + 1; ; ++j)
				{
					if (fileBuffer[j] == 0x0d)
					{
						size = j - i - 1;
						memcpy_s(gameData[index], size, fileBuffer + i + 1, size);
						gameData[index][size] = '\0';
						index++;
						break;
					}
				}
				startPosition = i + size;
			}
		}

		free(fileBuffer);

		fclose(workerInformation);
	}


	return true;

}
