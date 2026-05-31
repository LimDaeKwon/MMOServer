#include "ContentsNetLibrary.h"
#include "ContentsCPacket.h"
#include "ContentGroupBase.h"

#include <process.h>

ContentGroupBase::ContentGroupBase()
    : groupId_(0),
    messageQueue_(),
    messageDataFreeList_(0),
    messageEvent_(nullptr),
    logicThreadHandle_(nullptr),
    contentsServer_(nullptr),
    frameMS_(0)
{
    messageEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    if (messageEvent_ == nullptr)
    {
        DebugBreak();
    }
}

void ContentGroupBase::Start()
{
    logicThreadHandle_ = reinterpret_cast<HANDLE>( _beginthreadex(nullptr, 0, GroupThread, this, 0, nullptr));

    if (logicThreadHandle_ == nullptr)
    {
        DebugBreak();
    }
}

DWORD ContentGroupBase::GetFrame() const
{
    return frameMS_;
}

void ContentGroupBase::PushEnter(SessionId sessionId)
{
    MessageData* messageData = messageDataFreeList_.Alloc();

    messageData->sessionId_ = sessionId;
    messageData->contentsPacket_ = nullptr;
    messageData->messageType_ = Enter;

    messageQueue_.Enqueue(messageData);
    SetEvent(messageEvent_);
}

void ContentGroupBase::PushUpdate()
{
    MessageData* messageData = messageDataFreeList_.Alloc();

    messageData->sessionId_ = 0;
    messageData->contentsPacket_ = nullptr;
    messageData->messageType_ = Update;

    messageQueue_.Enqueue(messageData);
    SetEvent(messageEvent_);
}

void ContentGroupBase::PushLeave(SessionId sessionId)
{
    MessageData* messageData = messageDataFreeList_.Alloc();

    messageData->sessionId_ = sessionId;
    messageData->contentsPacket_ = nullptr;
    messageData->messageType_ = Leave;

    messageQueue_.Enqueue(messageData);
    SetEvent(messageEvent_);
}

void ContentGroupBase::PushMessages(SessionId sessionId, ContentsCPacket* packet)
{
    MessageData* messageData = messageDataFreeList_.Alloc();

    messageData->sessionId_ = sessionId;
    messageData->contentsPacket_ = packet;
    messageData->messageType_ = Message;

    messageQueue_.Enqueue(messageData);
    SetEvent(messageEvent_);
}

void ContentGroupBase::AttachServer(ContentsNetLibrary* contentsServer)
{
    contentsServer_ = contentsServer;
}

unsigned int __stdcall ContentGroupBase::GroupThread(void* thisPointer)
{
    ContentGroupBase* contents = static_cast<ContentGroupBase*>(thisPointer);
    DWORD oldTick = timeGetTime();

    while (true)
    {
        WaitForSingleObject(contents->messageEvent_, 1);

        while (true)
        {
            MessageData* messageData = nullptr;

            if (contents->messageQueue_.Dequeue(&messageData) == false)
            {
                break;
            }

            switch (messageData->messageType_)
            {
            case ContentGroupBase::Enter:
            {
                contents->OnEnter(messageData->sessionId_);
                break;
            }

            case ContentGroupBase::Message:
            {
                contents->OnMessage(
                    messageData->sessionId_,
                    messageData->contentsPacket_);

                break;
            }

            case ContentGroupBase::Leave:
            {
                contents->OnLeave(messageData->sessionId_);
                break;
            }

            case ContentGroupBase::Update:
            {
                contents->OnUpdate();
                break;
            }

            default:
            {
                break;
            }
            }

            contents->messageDataFreeList_.Free(messageData);
        }

        DWORD tick = timeGetTime();

        if (contents->frameMS_ > 0 && tick - oldTick > contents->frameMS_)
        {
            contents->OnUpdate();

            oldTick += contents->frameMS_ * ((tick - oldTick) / contents->frameMS_);
        }
    }

    return 0;
}