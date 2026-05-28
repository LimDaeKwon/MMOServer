#pragma once

#include "ContentGroupBase.h"

#include "ObjectFreeList.h"
#include <unordered_map>






class AuthGroup : public ContentGroupBase
{
public:


    AuthGroup(GroupId groupId, DWORD FrameMS = INFINITE);
    virtual ~AuthGroup() override;

    virtual void OnEnter(SessionId sessionId) override;
    virtual void OnLeave(SessionId sessionId) override;
    virtual void OnMessage(SessionId sessionId, ContentsCPacket* packet) override;
    virtual void OnUpdate() override;
    virtual int GetPlayerNum() override;
    virtual int GetFPS() override;
    virtual void OnInitializeTPS() override;

private:
    std::unordered_map<__int64, Player*> authPlayerMap_;
    long UpdateCount=0;
    long UpdateTPS = 0;

};



class EchoGroup : public ContentGroupBase
{
public:


    EchoGroup(GroupId groupId, DWORD FrameMS = INFINITE);
    virtual ~EchoGroup() override;

    virtual void OnEnter(SessionId sessionId) override;
    virtual void OnLeave(SessionId sessionId) override;
    virtual void OnMessage(SessionId sessionId, ContentsCPacket* packet) override;
    virtual void OnUpdate() override;
    virtual int GetPlayerNum() override;
    virtual int GetFPS() override;
    virtual void OnInitializeTPS() override;
private:
    long UpdateCount=0;
    long UpdateTPS = 0;
    std::unordered_map<__int64, Player*> echoPlayerMap_;

};


