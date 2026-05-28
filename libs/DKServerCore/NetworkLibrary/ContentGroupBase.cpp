#include "ContentGroupBase.h"
#include <process.h>




ContentGroupBase::ContentGroupBase():messageDataFreeList_(0)
{
	
	messageEvent_ = CreateEvent(NULL, FALSE, FALSE, nullptr);

	if (messageEvent_ == NULL)
	{
		//printf("CreateEvent failed (%d)\n", GetLastError());
		//이제 이런거 다 로그로 대체.
		DebugBreak();
	}

}

void ContentGroupBase::Start()
{
	logicThreadHandle_ = (HANDLE)_beginthreadex(nullptr, 0, GroupThread, this, 0, nullptr);

}

DWORD ContentGroupBase::GetFrame()
{
	return frameMS_;
}



void ContentGroupBase::PushEnter(SessionId sessionId)
{

	MessageData* msg_data = messageDataFreeList_.Alloc();
	msg_data->sessionId = sessionId;
	//msg_data->contentsPackets = nullptr;
	msg_data->messageType = ENTER;

	messageQueue_.Enqueue(msg_data);
	SetEvent(messageEvent_);


}

void ContentGroupBase::PushUpdate()
{
	MessageData* msg_data = messageDataFreeList_.Alloc();
	msg_data->sessionId = NULL;
	//msg_data->contentsPackets = nullptr;
	msg_data->messageType = UPDATE;

	messageQueue_.Enqueue(msg_data);
	SetEvent(messageEvent_);
}

void ContentGroupBase::PushLeave(SessionId sessionId)
{
	MessageData* msg_data = messageDataFreeList_.Alloc();
	msg_data->sessionId = sessionId;
	//msg_data->contentsPacket = nullptr;
	msg_data->messageType = LEAVE;

	messageQueue_.Enqueue(msg_data);
	SetEvent(messageEvent_);

}

void ContentGroupBase::PushMessages(SessionId sessionId, ContentsCPacket* packet)
{
	MessageData* msg_data = messageDataFreeList_.Alloc();
	msg_data->sessionId = sessionId;

	msg_data->contentsPackets = packet;

	msg_data->messageType = MESSAGE;

	messageQueue_.Enqueue(msg_data);
	SetEvent(messageEvent_);

}

void ContentGroupBase::AttachServer(ContentsNetLibrary* contentsServer)
{
	contentsServer_ = contentsServer;
}


unsigned int __stdcall ContentGroupBase::GroupThread(LPVOID this_ptr)
{

	ContentGroupBase* contents = static_cast<ContentGroupBase*>(this_ptr);
	DWORD oldTick = timeGetTime();

	while (true)
	{
		WaitForSingleObject(contents->messageEvent_, 1);

		while (true)
		{
			MessageData* msg = nullptr;
			if (!contents->messageQueue_.Dequeue(&msg))
			{
				break;
			}

			switch (msg->messageType)
			{
			case ContentGroupBase::ENTER:
			{
				contents->OnEnter(msg->sessionId);
				break;
			}
			case ContentGroupBase::MESSAGE:
			{

				contents->OnMessage(msg->sessionId, msg->contentsPackets);

				break;
			}
			case ContentGroupBase::LEAVE:
			{
				contents->OnLeave(msg->sessionId);
				break;
			}
			default:
			{
				break;
			}
			}
			contents->messageDataFreeList_.Free(msg);
		}
		//프레임 돌림
		// UPDATE:

		DWORD tick = timeGetTime();
		if (tick - oldTick > contents->frameMS_)
		{
			contents->OnUpdate();
			oldTick += contents->frameMS_ * ((tick - oldTick) / contents->frameMS_);

		}
		


	}
	return 0;
}


