#pragma once

#include <Windows.h>
#include <string>

#include "CoreDefines.h"

#define DK_ENABLE_PROFILE

#ifdef DK_ENABLE_PROFILE
#define PRO_BEGIN(tagName) ProfileBegin(tagName)
#define PRO_END(tagName) ProfileEnd(tagName)
#define INIT_PROFILE_TLS() InitProfile()
#else
#define PRO_BEGIN(tagName)
#define PRO_END(tagName)
#define INIT_PROFILE_TLS()
#endif

class ProfileManagement
{
public:
    int flag_ = 0;
    WCHAR name_[64] = { 0 };
    LARGE_INTEGER startTime_;
    __int64 totalTime_ = 0;
    __int64 min_[2] = { 0 };
    __int64 max_[2] = { 0 };
    __int64 call_ = 0;
    unsigned long long beginFlag_ = 0;
    DWORD threadId_ = 0;
};

void ProfileBegin(const WCHAR* tagName);
void ProfileEnd(const WCHAR* tagName);
void ProfileDataOutText(const WCHAR* fileName);
void ProfileReset();
void InitProfile();

extern bool Enabled;
void SetEnabled(bool enabled);


class Profile
{
public:
    Profile(const WCHAR* newTag)
        : tag_(newTag)
    {
        if (Enabled)
        {
            begin_ = true;
            PRO_BEGIN(newTag);
        }
        
    }

    ~Profile()
    {
        if (begin_)
        {
            PRO_END(tag_);
        }
        
    }

private:
    const WCHAR* tag_;
    bool begin_;
};