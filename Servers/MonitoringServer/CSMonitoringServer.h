#pragma once

#include "NetLibrary.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "CommonProtocol.h"
#include "LockFreeObjectFreeList.h"
#include <unordered_map>
#include "SystemMonitoring.h"
#include"MonitoringData.h"



class CSMonitoringServer : public NetLibrary
{
public:

	struct User
	{
		INT64 sessionID_;
		int loginFlag_ = 0;
		ULONGLONG timeOut_;
	};

	enum Type
	{
		ACCEPT,			
		PACKET,
		RELEASE,
		TPS
	};

	struct MessageData
	{
		WORD type;
		__int64 session_ID;
		ContentsCPacket* contents_packet;
	};


	
	const char* LoginSessionKey_ = "ajfw@!cv980dSZ[fje#@fdj123948djf";
	

	TLSObjectFreeList<User> userPool_;
	std::unordered_map<__int64, User*> userMap_;


	CSMonitoringServer(LoginServerMonitorData& loginServerData, GameServerMonitorData& gameServerData, ChatServerMonitorData& chatServerData, SystemMonitorData& systemData);
	virtual ~CSMonitoringServer();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);
	virtual void OnMessage(__int64 session_ID, ContentsCPacket* send_packet);
	virtual void OnError(int errorcode, const wchar_t* error_log);
	virtual void OnInitializeTPS();


	void NetPacketProc_ClientLogin(User* target, char* loginSessionKey);


	DWORD LogicTPS;
	DWORD LoginTPS;
	DWORD LoginCount;

	DWORD DCWrongPacket;
	DWORD DCAuthFailed;
	DWORD DCDuplicateLogin;

	DWORD LoginPlayer;
	DWORD UnloginPlayer;

	DWORD GetDCWrongPacket();
	DWORD GetDCAuthFailed();
	DWORD GetDCDuplicateLogin();

	DWORD GetLoginPlayer();
	DWORD GetUnloginPlayer();


	LoginServerMonitorData& loginServerData_;
	GameServerMonitorData& gameServerData_;
	ChatServerMonitorData& chatServerData_;
	SystemMonitorData& systemData_;

	void UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp);

	bool UpdateLoginServerMonitorData(BYTE dataType, int dataValue, int timeStamp);
	bool UpdateGameServerMonitorData(BYTE dataType, int dataValue, int timeStamp);
	bool UpdateChatServerMonitorData(BYTE dataType, int dataValue, int timeStamp);
	bool UpdateSystemMonitorData(BYTE dataType, int dataValue, int timeStamp);


	void SendMonitorData(User* target, BYTE serverNo, BYTE dataType, const MonitorValue& monitorValue);


	void SendLoginServerMonitorData(User* target);
	void SendGameServerMonitorData(User* target);
	void SendChatServerMonitorData(User* target);
	void SendSystemMonitorData(User* target);


	static unsigned int WINAPI LogicThread(LPVOID this_ptr);

	TLockFreeQueue<MessageData*> MessageQueue;
	TLSObjectFreeList<MessageData> MessageDataFreeList;
	HANDLE LogicThreadHandle = nullptr;
	HANDLE MessageEvent = nullptr;

	void AcceptProc(MessageData* msg_data);
	void MessageProc(MessageData* msg_data);
	void ReleaseProc(MessageData* msg_data);
	void TPSProc(MessageData* msg_data);

};


