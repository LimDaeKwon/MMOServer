#include "LoginServer.h"
#include <process.h>





LoginServer::LoginServer()
{
	TLSDBConnectionIndex = TlsAlloc();
	if (TLSDBConnectionIndex == TLS_OUT_OF_INDEXES)
	{
		__debugbreak();
	}



	monitoringClient_.Start("127.0.0.1", 5670, 1, 2);

	InitializeSRWLock(&connectionLock_);

	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);
	Connection_ = new cpp_redis::client;

	Connection_->connect();

}

LoginServer::~LoginServer()
{

}

bool LoginServer::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void LoginServer::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 SessionID)
{

}

void LoginServer::OnRelease(__int64 SessionID)
{

}

void LoginServer::OnMessage(__int64 SessionID, ContentsCPacket* contents_send_packet)
{

	WORD MessageType;

	ContentsCPacket ContentsPacket = contents_send_packet;

	ContentsPacket >> MessageType;


	//로그인 서버 로그인 요청
	//2  위에서 타입을 뺐다. 
	// 
	// 8 64
	//-> 72

	if (MessageType != en_PACKET_CS_LOGIN_REQ_LOGIN && ContentsPacket.GetDataSize() != 72)
	{
		InterlockedIncrement(&DCWrongPacket);
		Disconnect(SessionID);
	}

	INT64	AccountNo;
	char	SessionKey[64];		// 인증토큰
	ContentsPacket >> AccountNo;
	ContentsPacket.GetData((char*)SessionKey, sizeof(SessionKey));

	//로그인 로직
	ProcLogin(SessionID, AccountNo, SessionKey);

}

void LoginServer::ProcLogin(INT64 SessionID, INT64 AccountNo, char* SessionKey)
{


	WCHAR	ID[20];
	WCHAR	Nickname[20];

	//중복 로그인에 대해서는 어떻게 할 것인가...는 고민을 해보자 
	BYTE AuthResult = ProcAuth(AccountNo, SessionKey, ID, Nickname);

	
	
	if (!AuthResult)
	{
		//로그인 실패 없는 계정에 대한 요청이 온 것 . 끊어내자. 
		Disconnect(SessionID);
		InterlockedIncrement(&DCAuthFailed);
		return;
	}
	InterlockedIncrement(&LoginCount);
	//레디스에 세션키 저장
	StoreSessionKey(AccountNo, SessionKey);

	ContentsCPacket login_packet = ContentsCPacket::MakeContentsPacket();
	login_packet << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << (INT64)AccountNo << AuthResult;
	login_packet.PutData((char*)ID, 40);
	login_packet.PutData((char*)Nickname, 40);
	login_packet.PutData((char*)GameServerIP, 32);
	login_packet << (USHORT)GameServerPort;
	login_packet.PutData((char*)ChatServerIP, 32);
	login_packet << (USHORT)ChatServerPort;

	SendPacket(SessionID, login_packet);

}

BYTE LoginServer::ProcAuth(INT64 AccountNo, char* SessionKey, WCHAR* ID, WCHAR* Nickname)
{

	TLSDBConnection* DBConnection = (TLSDBConnection*)TlsGetValue(TLSDBConnectionIndex);
	if (DBConnection == nullptr)
	{
		DBConnection = new TLSDBConnection;
		mysql_init(&DBConnection->conn);

		DBConnection->Connection = mysql_real_connect(&DBConnection->conn, "127.0.0.1", "root", "dlaeornjs", "accountdb", 3306, NULL, 0);

		TlsSetValue(TLSDBConnectionIndex, DBConnection);

		if (DBConnection->Connection == NULL)
		{
			//printf("DBWriterThread connection failed: %s\n", mysql_error(&DBConnection->conn));
			DebugBreak();
		}
		mysql_set_server_option(DBConnection->Connection, MYSQL_OPTION_MULTI_STATEMENTS_ON);
	}

	//DB로 AccountNo를 포함한 모든 정보 조회 
	char Query[256];
	sprintf_s(Query, "SELECT userid, usernick ""FROM accountdb.account ""WHERE accountno = %lld", AccountNo);

	if (mysql_query(DBConnection->Connection, Query) != 0)
	{
		//DebugBreak();
	}

	MYSQL_RES* Result = mysql_store_result(DBConnection->Connection);
	if (Result == nullptr)
	{
		//DebugBreak();
	}

	MYSQL_ROW row;

	if ((row = mysql_fetch_row(Result)) != nullptr)
	{
		MultiByteToWideChar(CP_UTF8, 0, row[0], -1, ID, 20);

		MultiByteToWideChar(CP_UTF8, 0, row[1], -1, Nickname, 20);
	}
	else
	{
		//실패
		return 0;
	}
	mysql_free_result(Result);

	return 1;
}


void LoginServer::OnError(int errorcode, const wchar_t* error_log)
{

}

void LoginServer::OnInitializeTPS()
{

	int timeStamp = static_cast<int>(time(nullptr));

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, 1, timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, static_cast<int>(processMonitoring_.ProcessTotal()), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, static_cast<int>(processMonitoring_.GetProcessUserMemoryMBytes()), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_LOGIN_SESSION, GetSessionNum(), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, LoginTPS, timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, CPacket::GetCapacity(), timeStamp);

	monitoringClient_.SendMonitorData();

	LoginTPS = LoginCount;
	InterlockedExchange(&LoginCount,0);

}


DWORD LoginServer::GetLoginTPS()
{
	return LoginTPS;
}


DWORD LoginServer::GetDCWrongPacket()
{
	return DCWrongPacket;
}

DWORD LoginServer::GetDCAuthFailed()
{
	return DCAuthFailed;
}

DWORD LoginServer::GetDCDuplicateLogin()
{
	return DCDuplicateLogin;
}

void LoginServer::StoreSessionKey(INT64 AccountNo, char* SessionKey)
{
	AcquireSRWLockExclusive(&connectionLock_);
	std::string Value = SessionKey;

	//RedisConnection->Connection.set(RedisConnection->Key, RedisConnection->Value);
	Connection_->set(std::to_string(AccountNo), Value);
	Connection_->sync_commit();

	ReleaseSRWLockExclusive(&connectionLock_);

	//RedisConnection->Connection.send({ "SET",RedisConnection->Key,RedisConnection->Value,"EX","10" });


}
