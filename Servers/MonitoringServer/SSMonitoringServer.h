#pragma once

#include "LanLibrary.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "CommonProtocol.h"
#include "LockFreeObjectFreeList.h"
#include <unordered_map>
#include "SystemMonitoring.h"
#include"MonitoringData.h"

#pragma comment(lib, "libmysql.lib")
#include "include/mysql.h"
#include "include/errmsg.h"
#pragma comment(lib, "winmm.lib")



class SSMonitoringServer : public LanLibrary
{
public:

	struct User
	{
		int	serverNo_;
		INT64 sessionID_;
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


	SSMonitoringServer();
	virtual ~SSMonitoringServer();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);
	virtual void OnMessage(__int64 session_ID, ContentsCPacket* send_packet);
	virtual void OnError(int errorcode, const wchar_t* error_log);
	virtual void OnInitializeTPS();


	void NetPacketProc_ServerLogin(User* target, int serverNo);
	void NetPacketProc_DataUpdate(User* target, BYTE dataType, int dataValue, int timeStamp);


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



	LoginServerMonitorData loginServerData_;
	GameServerMonitorData gameServerData_;
	ChatServerMonitorData chatServerData_;
	SystemMonitorData systemData_;


	void UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp);

	bool UpdateLoginServerMonitorData(BYTE dataType, int dataValue, int timeStamp);
	bool UpdateGameServerMonitorData(BYTE dataType, int dataValue, int timeStamp);
	bool UpdateChatServerMonitorData(BYTE dataType, int dataValue, int timeStamp);
	bool UpdateSystemMonitorData(BYTE dataType, int dataValue, int timeStamp);

	void InitLoginServerMonitorData();
	void InitGameServerMonitorData();
	void InitChatServerMonitorData();
	void InitSystemMonitorData();



	void SendMonitorData(User* target, BYTE serverNo, BYTE dataType, const MonitorValue& monitorValue);


	static unsigned int WINAPI LogicThread(LPVOID this_ptr);

	TLockFreeQueue<MessageData*> MessageQueue;
	TLSObjectFreeList<MessageData> MessageDataFreeList;
	HANDLE LogicThreadHandle = nullptr;
	HANDLE MessageEvent = nullptr;

	void AcceptProc(MessageData* msg_data);
	void MessageProc(MessageData* msg_data);
	void ReleaseProc(MessageData* msg_data);
	void TPSProc(MessageData* msg_data);



	SystemMonitoring systemMonitoring_;

	MYSQL conn;
	MYSQL* connection = nullptr;
	int storeCount_ = 0;

	bool InsertMonitorValueLog(int logTime, int serverNo, int type, const MonitorValue& monitorValue);
	void ResetMonitorValueLog(MonitorValue& monitorValue);

	void FlushLoginServerMonitorLog(int logTime);
	void FlushGameServerMonitorLog(int logTime);
	void FlushChatServerMonitorLog(int logTime);
	void FlushSystemMonitorLog(int logTime);
	void FlushAllMonitorLogData();

};


