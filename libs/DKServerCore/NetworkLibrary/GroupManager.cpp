
#include "ContentsNetLibrary.h"
#include "GroupManager.h"
GroupManager::GroupManager()
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

	return false;
}

bool GroupManager::RemoveGroup(GroupId groupId)
{
	AcquireSRWLockExclusive(&groupMapLock_);

	std::unordered_map<__int64, ContentGroupBase*>::iterator GI = groupMap_.find(groupId);
	if (GI == groupMap_.end())
	{
		// 특이사항. 
		//DebugBreak();
	}

	groupMap_.erase(groupId);

	ReleaseSRWLockExclusive(&groupMapLock_);


	return false;
}

ContentGroupBase* GroupManager::FindGroup(GroupId groupId)
{
	return nullptr;
}

bool GroupManager::RegisterSession(Session* session)
{
	session->currentGroupId_ = defaultGroupId_;
	EnterGroup(session, defaultGroupId_);

	return false;
}

bool GroupManager::EnterGroup(Session* session, GroupId groupId)
{

	std::unordered_map<__int64, ContentGroupBase*>::iterator GI = groupMap_.find(groupId);
	ContentGroupBase* contentsGroup = GI->second;
	//스레드에 메시지 인큐. 
	contentsGroup->PushEnter(session->session_id);
	session->currentGroupId_ = groupId;


	return false;
}

bool GroupManager::LeaveGroup(Session* session)
{
	std::unordered_map<__int64, ContentGroupBase*>::iterator GI = groupMap_.find(session->currentGroupId_);
	ContentGroupBase* contentsGroup = GI->second;
	contentsGroup->PushLeave(session->session_id);

	session->currentGroupId_ = defaultGroupId_;


	return false;
}

bool GroupManager::MoveGroup(Session* session, GroupId groupId)
{
	std::unordered_map<__int64, ContentGroupBase*>::iterator GI;

	GI = groupMap_.find(session->currentGroupId_);
	ContentGroupBase* contentsGroup = GI->second;
	contentsGroup->OnLeave(session->session_id);


	GI = groupMap_.find(groupId);
	contentsGroup = GI->second;
	session->currentGroupId_ = groupId;
	contentsGroup->PushEnter(session->session_id);


	return false;
}

int GroupManager::GetGroupPlayerSize(GroupId groupId)
{
	AcquireSRWLockShared(&groupMapLock_);

	int playerNum = 0;

	std::unordered_map<__int64, ContentGroupBase*>::iterator GI;
	GI = groupMap_.find(groupId);
	if (GI != groupMap_.end())
	{
		ContentGroupBase* target = GI->second;
		playerNum = target->GetPlayerNum();
	}

	ReleaseSRWLockShared(&groupMapLock_);

	return playerNum;
}

int GroupManager::GetGroupFPS(GroupId groupId)
{
	AcquireSRWLockShared(&groupMapLock_);

	int TPS = 0;

	std::unordered_map<__int64, ContentGroupBase*>::iterator GI;
	GI = groupMap_.find(groupId);
	if (GI != groupMap_.end())
	{
		ContentGroupBase* target = GI->second;
		TPS = target->GetFPS();
	}

	ReleaseSRWLockShared(&groupMapLock_);
	return TPS;
}

bool GroupManager::RouteMessage(Session* session, ContentsCPacket* packet)
{

	std::unordered_map<__int64, ContentGroupBase*>::iterator GI = groupMap_.find(session->currentGroupId_);
	ContentGroupBase* contentsGroup = GI->second;

	contentsGroup->PushMessages(session->session_id, packet);

	return false;
}

void GroupManager::GroupOnInitializeTPS()
{
	AcquireSRWLockShared(&groupMapLock_);

	for (std::unordered_map<GroupId, ContentGroupBase*>::iterator iter = groupMap_.begin(); iter != groupMap_.end(); ++iter)
	{
		ContentGroupBase* target = iter->second;
		target->OnInitializeTPS();
	}
	ReleaseSRWLockShared(&groupMapLock_);
}





unsigned int __stdcall GroupManager::FrameThread(LPVOID this_ptr)
{
	GroupManager* groupManager = static_cast<GroupManager*>(this_ptr);

	DWORD oldTick = timeGetTime();

	while (1)
	{
		DWORD tick = timeGetTime();
		AcquireSRWLockShared(&groupManager->groupMapLock_);

		for (std::unordered_map<GroupId, ContentGroupBase*>::iterator iter = groupManager->groupMap_.begin(); iter != groupManager->groupMap_.end(); ++iter)
		{
			ContentGroupBase* target = iter->second;

			if (tick - oldTick < target->GetFrame())
			{
				target->PushUpdate();
			}
		}
		ReleaseSRWLockShared(&groupManager->groupMapLock_);

		if (tick - oldTick < 20)
		{
			Sleep(tick - oldTick);
		}
		oldTick += 20;

	}

	return 0;
}
