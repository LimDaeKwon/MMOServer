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


//class ContentsCPacket;

//서버들로부터 주기적으로 데이터를 받고
//클라이언트들로 주기적으로 데이터를 보내준다. 


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


	//귀찮은데 그냥 뉴딜리트? 
	//몇 개 없기도 한데 흠..
	//일단 하고 고민

	//모니터링서버의 스레드 설계
	//굳이 멀티일 필요도 없다. 
	//그냥 락 걸까. 



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


	//각각의 변수들 선언

	LoginServerMonitorData loginServerData_;
	GameServerMonitorData gameServerData_;
	ChatServerMonitorData chatServerData_;
	SystemMonitorData systemData_;

	//이렇게 다 가지고 있음. 

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
	HANDLE LogicThreadHandle;
	HANDLE MessageEvent;

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


