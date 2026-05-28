#pragma once

#include "stdio.h"
#include "Windows.h"
#include <string>
#include "conio.h"

//#define PROFILE
#define MAXINDEX  20

#ifdef PROFILE
#define PRO_BEGIN(TagName) ProfileBegin(TagName)
#define PRO_END(TagName) ProfileEnd(TagName)
#define INIT_PROFILE_TLS() InitProfile()

#else
#define PRO_BEGIN(TagName) 
#define PRO_END(TagName) 
#define INIT_PROFILE_TLS() 

#endif



class ProfileManagement
{
public:
	int Flag = 0;
	WCHAR  szName[64] = {0};
	LARGE_INTEGER StartTime;
	__int64 TotalTime= 0;
	__int64 Min[2] = { 0 };
	__int64 Max[2] = { 0 };
	__int64 Call = 0;
	unsigned long long begin_flag = 0;
	DWORD ThreadID = 0;
};


void ProfileBegin(const WCHAR* szName);
void ProfileEnd(const WCHAR* szName);
void ProfileDataOutText(const WCHAR* szFileName);
void ProfileReset();
void InitProfile();

class Profile
{
public:
	Profile(const WCHAR* NewTag)
	{
		PRO_BEGIN(NewTag);
		Tag = NewTag;
	}
	~Profile()
	{
		PRO_END(Tag);
	}

	const WCHAR* Tag;
};