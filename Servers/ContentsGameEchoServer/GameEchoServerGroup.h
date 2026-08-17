#pragma once

#include "ContentGroupBase.h"

#include <unordered_map>

struct Player;

class AuthGroup : public ContentGroupBase
{
public:
    AuthGroup(GroupId groupId, DWORD frameMs = INFINITE);
    virtual ~AuthGroup() override;

    virtual void OnEnter(SessionId sessionId) override;
    virtual void OnLeave(SessionId sessionId) override;
    virtual void OnMessage(SessionId sessionId, ContentsCPacket* packet) override;
    virtual void OnRelease(SessionId sessionId) override;
    virtual void OnUpdate() override;
    virtual int GetPlayerNum() override;
    virtual int GetFPS() override;
    virtual void OnInitializeTPS() override;

private:
    std::unordered_map<SessionId, Player*> authPlayerMap_;

    long updateCount_{ 0 };
    long updateTps_{ 0 };
};

class EchoGroup : public ContentGroupBase
{
public:
    EchoGroup(GroupId groupId, DWORD frameMs = INFINITE);
    virtual ~EchoGroup() override;

    virtual void OnEnter(SessionId sessionId) override;
    virtual void OnLeave(SessionId sessionId) override;
    virtual void OnMessage(SessionId sessionId, ContentsCPacket* packet) override;
    virtual void OnUpdate() override;
    virtual void OnRelease(SessionId sessionId) override;
    virtual int GetPlayerNum() override;
    virtual int GetFPS() override;
    virtual void OnInitializeTPS() override;

private:
    std::unordered_map<SessionId, Player*> echoPlayerMap_;

    long updateCount_{ 0 };
    long updateTps_{ 0 };
};