#pragma once

//#include <Windows.h>

#include "MessageDataQueue.h"
#include "TLSObjectFreeList.h"

using SessionId = __int64;
using GroupId = __int64;

class ContentsCPacket;
class ContentsNetLibrary;

struct MessageData
{
    int messageType_;
    SessionId sessionId_;
    ContentsCPacket* contentsPacket_;
};

class ContentGroupBase
{
public:
    enum MessageType
    {
        Enter,
        Message,
        Leave,
        Update,
        Release
    };

public:
    ContentGroupBase();
    virtual ~ContentGroupBase() = default;

    void Start();

    void PushEnter(SessionId sessionId);
    void PushLeave(SessionId sessionId);
    void PushMessages(SessionId sessionId, ContentsCPacket* packet);
    void PushUpdate();
    void PushRelease(SessionId sessionId);

    void AttachServer(ContentsNetLibrary* contentsServer);

    DWORD GetFrame() const;

    virtual int GetPlayerNum() = 0;
    virtual int GetFPS() = 0;

    virtual void OnEnter(SessionId sessionId) = 0;
    virtual void OnLeave(SessionId sessionId) = 0;
    virtual void OnMessage(SessionId sessionId, ContentsCPacket* packet) = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRelease(SessionId sessionId) = 0;
    virtual void OnInitializeTPS() = 0;

    static unsigned int __stdcall GroupThread(void* thisPointer);

protected:
    GroupId groupId_;

    MessageDataQueue messageQueue_;
    TLSObjectFreeList<MessageData> messageDataFreeList_;

    HANDLE messageEvent_;
    HANDLE logicThreadHandle_;

    ContentsNetLibrary* contentsServer_;

    DWORD frameMS_;
};