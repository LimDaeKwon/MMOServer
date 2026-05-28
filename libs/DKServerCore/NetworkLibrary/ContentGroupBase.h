#pragma once

#include"LockFreeQueueCas2.h"
#include "MessageDataQueue.h"

using SessionId =__int64;
using GroupId = __int64;


class ContentsCPacket;
class ContentsNetLibrary;


struct MessageData
{
    int messageType;
    __int64 sessionId;
    ContentsCPacket* contentsPackets;
};




class ContentGroupBase
{
public:

    enum Type
    {
        ENTER,
        MESSAGE,
        LEAVE,
        UPDATE
    };

    ContentGroupBase();

    virtual ~ContentGroupBase() = default;

    void PushEnter(SessionId sessionId);
    void PushLeave(SessionId sessionId);
    void PushMessages(SessionId sessionId, ContentsCPacket* packet);
    void PushUpdate();
    void AttachServer(ContentsNetLibrary* contentsServer);
    virtual int GetPlayerNum() = 0;
    virtual int GetFPS() = 0;


    void Start();
    DWORD GetFrame();

    virtual void OnEnter(SessionId sessionId) = 0;
    virtual void OnLeave(SessionId sessionId) = 0;
    virtual void OnMessage(SessionId sessionId, ContentsCPacket* packet) = 0;
    virtual void OnUpdate() = 0;
    virtual void OnInitializeTPS() = 0;

    static unsigned int WINAPI GroupThread(LPVOID this_ptr);

protected:

    GroupId groupId_;

    //TLockFreeQueue<MessageData*> messageQueue_;
    MessageDataQueue messageQueue_;

    TLSObjectFreeList<MessageData> messageDataFreeList_;
    HANDLE messageEvent_;
    HANDLE logicThreadHandle_;
    ContentsNetLibrary* contentsServer_;
    DWORD frameMS_ = 0;


};



//어셉트가 왔다. 
//Session의 currentGroupId_를 디폴트그룹으로 세팅해주고 디폴트그룹으로 OnEnter를 넣어준다.
//그리고 Session에 대해 recv를 걸어준다. 

//메시지가 왔다.
//해당 Session의 currentGroupId_를 확인하고 IContentsGroup의 맵을 통해 그룹 인스턴스를 얻고 해당 그룹의 큐로 넣어준다. 


//



//라이브러리 측에서는 IContentsGroup의 OnMessage , OnEnter , OnLeave를 저 그룹의 큐로 넣어준다.
// 
// 
// 
// 
// 
// OnMessage라는 메시지를 넘겨주는 것이고 컨텐츠측에서는 OnMessage를 그때 실행하면 된다. 
// 
// 
// 라이브러리에는 IContentsGroup의 배열 혹은 맵이 있어야한다. 
// 컨텐츠 생성 시점에 RegisterGroup이라는 함수를 호출시켜야 할 듯 . 
// RegisterGroup-> 라이브러리의 함수. 
//이 그룹을 등록하려면? 
// 
// 
// 
//이 그룹의 등록 시점은? 
// 