#pragma once

#include <Windows.h>

#include <unordered_map>

#include "ContentGroupBase.h"

struct Session;
class ContentsCPacket;

class GroupManager
{
public:
    GroupManager();
    virtual ~GroupManager();

    bool AddGroup(ContentGroupBase* group, GroupId groupId);
    bool RemoveGroup(GroupId groupId);

    ContentGroupBase* FindGroup(GroupId groupId);

    bool RegisterSession(Session* session);
    bool EnterGroup(Session* session, GroupId groupId);
    bool LeaveGroup(Session* session);
    bool MoveGroup(Session* session, GroupId groupId);
    bool ReleaseGroup(Session* session);

    int GetGroupPlayerSize(GroupId groupId);
    int GetGroupFPS(GroupId groupId);

    bool RouteMessage(Session* session, ContentsCPacket* packet);

    void GroupOnInitializeTPS();

    static unsigned int __stdcall FrameThread(void* thisPointer);

private:
    std::unordered_map<GroupId, ContentGroupBase*> groupMap_;

    SRWLOCK groupMapLock_;
    GroupId defaultGroupId_;
};