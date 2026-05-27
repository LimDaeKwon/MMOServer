#pragma once

#include "NetLibrary.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "CommonProtocol.h"
#include "LockFreeObjectFreeList.h"
#include <unordered_map>
#include <list>
#include "LoginMonitoringClient.h"
#include "ProcessMonitoring.h"
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")
#include "include/mysql.h"
#include "include/errmsg.h"



class LoginServer : public NetLibrary
{
public:


	LoginServer();
	virtual ~LoginServer();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);
	virtual void OnMessage(__int64 session_ID, ContentsCPacket* send_packet);
	virtual void OnError(int errorcode, const wchar_t* error_log);
	virtual void OnInitializeTPS();

	void ProcLogin(INT64 SessionID, INT64 AccountNo, char* SessionKey);

	BYTE ProcAuth(INT64 AccountNo, char* SessionKey, WCHAR* ID,WCHAR* Nickname);
	DWORD LoginTPS = 0;

	DWORD LogicCount;
	DWORD LoginCount;

	DWORD DCWrongPacket;
	DWORD DCAuthFailed;
	DWORD DCDuplicateLogin;

	DWORD GetLoginTPS();

	DWORD GetDCWrongPacket();
	DWORD GetDCAuthFailed();
	DWORD GetDCDuplicateLogin();

	DWORD TLSDBConnectionIndex;

	struct TLSDBConnection
	{
		MYSQL conn;
		MYSQL* Connection = nullptr;
	};

	cpp_redis::client* Connection_;
	SRWLOCK connectionLock_;


	void StoreSessionKey(INT64 AccountNo, char* SessionKey);


	WCHAR	GameServerIP[16] = L"10.0.1.1";	// 접속대상 게임,채팅 서버 정보
	USHORT	GameServerPort = 21740;

	WCHAR	ChatServerIP[16] = L"10.0.1.1";	// 접속대상 게임,채팅 서버 정보
	USHORT	ChatServerPort = 21720;


	LoginMonitoringClient monitoringClient_;
	ProcessMonitoring processMonitoring_;


};

//차단한다면? onaccept와 onrelease에서도 무언가 할 것이 있지 않을까 



//
////------------------------------------------------------
//// Login Server
////------------------------------------------------------
//en_PACKET_CS_LOGIN_SERVER = 100,
//
////------------------------------------------------------------
//// 로그인 서버로 클라이언트 로그인 요청
////
////	{
////		WORD	Type
////
////		INT64	AccountNo
////		char	SessionKey[64]
////	}
////
////------------------------------------------------------------
//en_PACKET_CS_LOGIN_REQ_LOGIN,
//

//

//enum en_PACKET_CS_LOGIN_RES_LOGIN
//{
//	dfLOGIN_STATUS_NONE = -1,		// 미인증상태
//	dfLOGIN_STATUS_FAIL = 0,		// 세션오류
//	dfLOGIN_STATUS_OK = 1,		// 성공
//	dfLOGIN_STATUS_GAME = 2,		// 게임중
//	dfLOGIN_STATUS_ACCOUNT_MISS = 3,		// account 테이블에 AccountNo 없음
//	dfLOGIN_STATUS_SESSION_MISS = 4,		// Session 테이블에 AccountNo 없음
//	dfLOGIN_STATUS_STATUS_MISS = 5,		// Status 테이블에 AccountNo 없음
//	dfLOGIN_STATUS_NOSERVER = 6,		// 서비스중인 서버가 없음.
//};

