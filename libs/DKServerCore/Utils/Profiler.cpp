#include "Profiler.h"

#include <climits>
#include <cstdio>
#include <cstring>
#include <vector>

DWORD tlsIndex;
LARGE_INTEGER frequency;
bool Enabled = true;

std::vector<ProfileManagement*> profileList;

void ProfileBegin(const WCHAR* tagName)
{
    ProfileManagement* profileManagement =
        static_cast<ProfileManagement*>(TlsGetValue(tlsIndex));

    if (profileManagement == nullptr)
    {
        profileManagement = new ProfileManagement[DKServerCore::ProfilerMaxIndex];
        memset(profileManagement, 0, sizeof(ProfileManagement) * DKServerCore::ProfilerMaxIndex);

        TlsSetValue(tlsIndex, profileManagement);

        profileManagement->threadId_ = GetCurrentThreadId();
        profileList.push_back(profileManagement);
    }

    int index = 0;

    for (index = 0; index < DKServerCore::ProfilerMaxIndex; ++index)
    {
        if (profileManagement[index].flag_ == 1)
        {
            if (wcscmp(profileManagement[index].name_, tagName) == 0)
            {
                LARGE_INTEGER start;

                QueryPerformanceCounter(&start);

                profileManagement[index].startTime_ = start;

                if (InterlockedExchange(
                    reinterpret_cast<volatile LONG*>(&profileManagement[index].beginFlag_),
                    1) == 1)
                {
                    DebugBreak();
                }

                return;
            }
        }
        else
        {
            break;
        }
    }

    LARGE_INTEGER start;

    profileManagement[index].flag_ = 1;

    wmemcpy(profileManagement[index].name_, tagName, wcslen(tagName));

    QueryPerformanceCounter(&start);

    profileManagement[index].startTime_ = start;
    profileManagement[index].min_[0] = INT_MAX;
    profileManagement[index].max_[0] = -1;
    profileManagement[index].min_[1] = INT_MAX;
    profileManagement[index].max_[1] = -1;

    if (InterlockedExchange(
        reinterpret_cast<volatile LONG*>(&profileManagement[index].beginFlag_),
        1) == 1)
    {
        DebugBreak();
    }
}

void ProfileEnd(const WCHAR* tagName)
{
    ProfileManagement* profileManagement =
        static_cast<ProfileManagement*>(TlsGetValue(tlsIndex));

    for (int index = 0; index < DKServerCore::ProfilerMaxIndex; ++index)
    {
        if (wcscmp(profileManagement[index].name_, tagName) == 0)
        {
            if (InterlockedExchange(
                reinterpret_cast<volatile LONG*>(&profileManagement[index].beginFlag_),
                0) == 0)
            {
                DebugBreak();
            }

            LARGE_INTEGER end;
            LARGE_INTEGER elapsedTime;

            QueryPerformanceCounter(&end);

            elapsedTime.QuadPart =
                end.QuadPart - profileManagement[index].startTime_.QuadPart;

            profileManagement[index].totalTime_ += elapsedTime.QuadPart;

            if (elapsedTime.QuadPart < profileManagement[index].min_[0])
            {
                profileManagement[index].min_[1] = profileManagement[index].min_[0];
                profileManagement[index].min_[0] = elapsedTime.QuadPart;
            }
            else if (elapsedTime.QuadPart < profileManagement[index].min_[1])
            {
                profileManagement[index].min_[1] = elapsedTime.QuadPart;
            }

            if (elapsedTime.QuadPart > profileManagement[index].max_[0])
            {
                profileManagement[index].max_[1] = profileManagement[index].max_[0];
                profileManagement[index].max_[0] = elapsedTime.QuadPart;
            }
            else if (elapsedTime.QuadPart > profileManagement[index].max_[1])
            {
                profileManagement[index].max_[1] = elapsedTime.QuadPart;
            }

            ++profileManagement[index].call_;

            break;
        }
    }
}

void ProfileDataOutText(const WCHAR* fileName)
{
    for (int profileIndex = 0; profileIndex < static_cast<int>(profileList.size()); ++profileIndex)
    {
        ProfileManagement* profileManagement = profileList[profileIndex];

        WCHAR outputFileName[MAX_PATH];

        swprintf_s(outputFileName, MAX_PATH, L"%s%02d.txt", fileName, profileManagement->threadId_);

        FILE* writeFile = nullptr;

        if (_wfopen_s(&writeFile, outputFileName, L"wb") != 0)
        {
            wprintf(L"File Open Failed\n");
            return;
        }

        fwprintf(writeFile, L"---------------------------------------------------------------------------------------------------------------------\n" L"%20s %20s %20s %25s %20s \n" L"---------------------------------------------------------------------------------------------------------------------\n", L"Name", L"Average", L"Min", L"Max", L"Call");

        for (int index = 0; index < DKServerCore::ProfilerMaxIndex; ++index)
        {
            if (profileManagement[index].flag_ == 1)
            {
                fwprintf(
                    writeFile,
                    L"%20s  %20s§Á  %20s§Á  %20s§Á  %10s\n",
                    profileManagement[index].name_,
                    std::to_wstring(static_cast<double>(profileManagement[index].totalTime_ - profileManagement[index].min_[0] - profileManagement[index].max_[0] - profileManagement[index].min_[1] - profileManagement[index].max_[1]) / (profileManagement[index].call_ - 4) / frequency.QuadPart * 1000000).c_str(),
                    std::to_wstring(static_cast<double>((profileManagement[index].min_[0] + profileManagement[index].min_[1]) / static_cast<double>(2)) / frequency.QuadPart * 1000000).c_str(),
                    std::to_wstring(static_cast<double>((profileManagement[index].max_[0] + profileManagement[index].max_[1]) / static_cast<double>(2)) / frequency.QuadPart * 1000000).c_str(),
                    std::to_wstring(profileManagement[index].call_ - 4).c_str());
            }
            else
            {
                break;
            }
        }

        fwprintf(writeFile, L"---------------------------------------------------------------------------------------------------------------------\n");

        wprintf(L"WriteFileComplete %d\n", profileManagement->threadId_);

        fclose(writeFile);
    }
}

void ProfileReset()
{
    for (int profileIndex = 0; profileIndex < static_cast<int>(profileList.size()); ++profileIndex)
    {
        ProfileManagement* profileManagement = profileList[profileIndex];

        for (int index = 0; index < DKServerCore::ProfilerMaxIndex; ++index)
        {
            if (profileManagement[index].flag_ == 1)
            {
                profileManagement[index].totalTime_ = 0;
                profileManagement[index].call_ = 0;
                profileManagement[index].min_[0] = INT_MAX;
                profileManagement[index].max_[0] = -1;
                profileManagement[index].min_[1] = INT_MAX;
                profileManagement[index].max_[1] = -1;
            }
            else
            {
                break;
            }
        }

        wprintf(L"ResetComplete %d\n", profileManagement->threadId_);
    }
}

void InitProfile()
{
    tlsIndex = TlsAlloc();

    QueryPerformanceFrequency(&frequency);
}

void SetEnabled(bool enabled)
{
	Enabled = enabled;
}
