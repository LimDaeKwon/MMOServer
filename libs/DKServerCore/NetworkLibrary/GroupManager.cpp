#include "ContentsNetLibrary.h"
#include "GroupManager.h"


GroupManager::GroupManager()
    : defaultGroupId_(0)
{
    InitializeSRWLock(&groupMapLock_);
}

GroupManager::~GroupManager()
{
}

bool GroupManager::AddGroup(ContentGroupBase* group, GroupId groupId)
{
    AcquireSRWLockExclusive(&groupMapLock_);

    groupMap_.insert(std::pair<GroupId, ContentGroupBase*>(groupId, group));

    ReleaseSRWLockExclusive(&groupMapLock_);

    return true;
}

bool GroupManager::RemoveGroup(GroupId groupId)
{
    AcquireSRWLockExclusive(&groupMapLock_);

    std::unordered_map<GroupId, ContentGroupBase*>::iterator groupIterator =
        groupMap_.find(groupId);

    if (groupIterator == groupMap_.end())
    {
    }

    groupMap_.erase(groupId);

    ReleaseSRWLockExclusive(&groupMapLock_);

    return true;
}

ContentGroupBase* GroupManager::FindGroup(GroupId groupId)
{
    AcquireSRWLockShared(&groupMapLock_);

    ContentGroupBase* group = nullptr;

    std::unordered_map<GroupId, ContentGroupBase*>::iterator groupIterator =
        groupMap_.find(groupId);

    if (groupIterator != groupMap_.end())
    {
        group = groupIterator->second;
    }

    ReleaseSRWLockShared(&groupMapLock_);

    return group;
}

bool GroupManager::RegisterSession(Session* session)
{
    session->currentGroupId_ = defaultGroupId_;

    EnterGroup(session, defaultGroupId_);

    return true;
}

bool GroupManager::EnterGroup(Session* session, GroupId groupId)
{
    ContentGroupBase* contentsGroup = FindGroup(groupId);

    if (contentsGroup == nullptr)
    {
        return false;
    }


    contentsGroup->PushEnter(session->sessionId_);
    session->currentGroupId_ = groupId;

    return true;
}

bool GroupManager::LeaveGroup(Session* session)
{
    ContentGroupBase* contentsGroup = FindGroup(session->currentGroupId_);

    if (contentsGroup == nullptr)
    {
        return false;
    }



    contentsGroup->PushLeave(session->sessionId_);

    session->currentGroupId_ = defaultGroupId_;

    return true;
}

bool GroupManager::MoveGroup(Session* session, GroupId groupId)
{
    ContentGroupBase* contentsGroup = FindGroup(session->currentGroupId_);

    if (contentsGroup == nullptr)
    {
        return false;
    }



    contentsGroup->OnLeave(session->sessionId_);

    contentsGroup = FindGroup(groupId);

    session->currentGroupId_ = groupId;
    contentsGroup->PushEnter(session->sessionId_);

    return true;
}

int GroupManager::GetGroupPlayerSize(GroupId groupId)
{
    ContentGroupBase* target = FindGroup(groupId);

    int playerNum = 0;

    if (target != nullptr)
    {
        playerNum = target->GetPlayerNum();
    }

    return playerNum;
}

int GroupManager::GetGroupFPS(GroupId groupId)
{
    ContentGroupBase* target = FindGroup(groupId);

    int fps = 0;

    if (target != nullptr)
    {
        fps = target->GetFPS();
    }

    return fps;
}

bool GroupManager::RouteMessage(Session* session, ContentsCPacket* packet)
{
    ContentGroupBase* contentsGroup = FindGroup(session->currentGroupId_);

    if (contentsGroup == nullptr)
    {
        return false;
    }


    contentsGroup->PushMessages(session->sessionId_, packet);

    return true;
}

void GroupManager::GroupOnInitializeTPS()
{
    AcquireSRWLockShared(&groupMapLock_);

    for (std::unordered_map<GroupId, ContentGroupBase*>::iterator iter = groupMap_.begin();
        iter != groupMap_.end();
        ++iter)
    {
        ContentGroupBase* target = iter->second;

        target->OnInitializeTPS();
    }

    ReleaseSRWLockShared(&groupMapLock_);
}

unsigned int __stdcall GroupManager::FrameThread(void* thisPointer)
{
    GroupManager* groupManager = static_cast<GroupManager*>(thisPointer);

    DWORD oldTick = timeGetTime();

    while (true)
    {
        DWORD tick = timeGetTime();

        AcquireSRWLockShared(&groupManager->groupMapLock_);

        for (std::unordered_map<GroupId, ContentGroupBase*>::iterator iter =
            groupManager->groupMap_.begin();
            iter != groupManager->groupMap_.end();
            ++iter)
        {
            ContentGroupBase* target = iter->second;

            if (tick - oldTick < target->GetFrame())
            {
                target->PushUpdate();
            }
        }

        ReleaseSRWLockShared(&groupManager->groupMapLock_);

        DWORD elapsedTick = tick - oldTick;

        if (elapsedTick < 20)
        {
            Sleep(20 - elapsedTick);
        }
        oldTick += 20;
    }

    return 0;
}