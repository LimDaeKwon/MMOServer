#pragma once

#include "ContentGroupBase.h"
#include <unordered_map>

struct Session;

class GroupManager
{
public:


    GroupManager();
    virtual ~GroupManager();

    bool AddGroup(ContentGroupBase* group, GroupId groupId);
    bool RemoveGroup(GroupId groupId);

    ContentGroupBase* FindGroup(GroupId groupId);

    bool RegisterSession(Session* session); // 디폴트 그룹으로 넣어줌. 
    bool EnterGroup(Session* session, GroupId groupId);
    bool LeaveGroup(Session* session);
    bool MoveGroup(Session* session, GroupId groupId);
    int GetGroupPlayerSize(GroupId groupId);
    int GetGroupFPS(GroupId groupId);

    bool RouteMessage(Session* session, ContentsCPacket* packet);

    void GroupOnInitializeTPS();

    static unsigned int WINAPI FrameThread(LPVOID this_ptr);


private:
   

    std::unordered_map<GroupId, ContentGroupBase*> groupMap_;

    SRWLOCK groupMapLock_;
    GroupId defaultGroupId_{ 0 };

};