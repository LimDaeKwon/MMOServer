#pragma once

#include "ContentsNetLibrary.h"

#include "TLSObjectFreeList.h"
#include "CommonProtocol.h"

#include <unordered_map>
#include <list>


#include "LockFreeQueueCas2.h"
#include "ObjectFreeList.h"

#include "GameMonitoringClient.h"
#include "ProcessMonitoring.h"


class GameEchoServer : public ContentsNetLibrary
{
public:
	enum Type
	{
		ACCEPT,
		MESSAGE,
		RELEASE,
	};


	virtual void* GetPlayerPointer(__int64 sessionId)  override;


	GameEchoServer();
	virtual ~GameEchoServer();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);
	virtual void OnMessage(__int64 session_ID, ContentsCPacket* send_packet);
	virtual void OnError(int errorcode, const wchar_t* error_log);
	virtual void OnInitializeTPS();

	DWORD LogicTPS;
	DWORD LoginTPS;
	DWORD HeartBeatTPS;

	DWORD LogicCount;
	DWORD LoginCount;
	DWORD HeartBeatCount;

	DWORD DCWrongPacket;
	DWORD DCAuthFailed;
	DWORD DCDuplicateLogin;

	DWORD LoginPlayer;
	DWORD UnloginPlayer;


	
	DWORD GetLogicTPS();
	DWORD GetLoginTPS();
	DWORD GetHeartBeatTPS();

	DWORD GetDCWrongPacket();
	DWORD GetDCAuthFailed();
	DWORD GetDCDuplicateLogin();

	DWORD GetLoginPlayer();
	DWORD GetUnloginPlayer();

	TLSObjectFreeList<Player> playerPool_;
	std::unordered_map<__int64, Player*> playerMap_;
	SRWLOCK playerMapLock_;


	GameMonitoringClient monitoringClient_;
	ProcessMonitoring processMonitoring_;


};