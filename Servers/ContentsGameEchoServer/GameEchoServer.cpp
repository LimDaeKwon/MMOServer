#include "GameEchoServer.h"
#include "ContentsCPacket.h"
#include "GameEchoServerGroup.h"
#include <process.h>



GameEchoServer::GameEchoServer() :playerPool_(0)
{
	InitializeSRWLock(&playerMapLock_);


	AuthGroup* authGroup = new AuthGroup(0,20);
	EchoGroup* echoGroup = new EchoGroup(1,20);

	authGroup->AttachServer(this);
	echoGroup->AttachServer(this);

	authGroup->Start();
	echoGroup->Start();

	groupManager_.AddGroup(authGroup, 0);
	groupManager_.AddGroup(echoGroup, 1);


	monitoringClient_.Start("127.0.0.1", 5670, 1, 2);

}

GameEchoServer::~GameEchoServer()
{

}

bool GameEchoServer::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void GameEchoServer::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 sessionId)
{
	//전역적인 플레이어 관리

	AcquireSRWLockExclusive(&playerMapLock_);
	Player* newPlayer = playerPool_.Alloc();
	newPlayer->sessionId_ = sessionId;
	playerMap_.insert(std::unordered_map<__int64, Player*>::value_type(newPlayer->sessionId_, newPlayer));
	ReleaseSRWLockExclusive(&playerMapLock_);

}

void GameEchoServer::OnRelease(__int64 sessionId)
{
	//전역적인 플레이어 해제

	AcquireSRWLockExclusive(&playerMapLock_);
	std::unordered_map<__int64, Player*>::iterator playerIter = playerMap_.find(sessionId);
	if (playerIter == playerMap_.end())
	{
		//DebugBreak();
	}

	Player* target = playerIter->second;
	playerMap_.erase(sessionId);
	ReleaseSRWLockExclusive(&playerMapLock_);
	playerPool_.Free(target);

}



void* GameEchoServer::GetPlayerPointer(__int64 sessionId)
{
	AcquireSRWLockShared(&playerMapLock_);
	std::unordered_map<__int64, Player*>::iterator playerIter = playerMap_.find(sessionId);
	if (playerIter == playerMap_.end())
	{
		//없음. 
		ReleaseSRWLockShared(&playerMapLock_);
		return nullptr;


	}
	Player* target = playerIter->second;

	ReleaseSRWLockShared(&playerMapLock_);
	return target;
}


void GameEchoServer::OnMessage(__int64 session_ID, ContentsCPacket* contents_send_packet)
{


	//바로 처리할 수 있게 일단 냄겨는 둘까? 

}

void GameEchoServer::OnError(int errorcode, const wchar_t* error_log)
{

}

void GameEchoServer::OnInitializeTPS()
{
	//여기서 프레임을 보내줘버릴까? 


	//dfMONITOR_DATA_TYPE_GAME_SERVER_RUN = 10,		// GameServer 실행 여부 ON / OFF
	//	dfMONITOR_DATA_TYPE_GAME_SERVER_CPU = 11,		// GameServer CPU 사용률
	//	dfMONITOR_DATA_TYPE_GAME_SERVER_MEM = 12,		// GameServer 메모리 사용 MByte
	// 
	//	dfMONITOR_DATA_TYPE_GAME_SESSION = 13,		// 게임서버 세션 수 (컨넥션 수)
	//	dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER = 14,		// 게임서버 AUTH MODE 플레이어 수
	//	dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER = 15,		// 게임서버 GAME MODE 플레이어 수
	// 
	//	dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS = 16,		// 게임서버 Accept 처리 초당 횟수
	//	dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS = 17,		// 게임서버 패킷처리 초당 횟수
	//	dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS = 18,		// 게임서버 패킷 보내기 초당 완료 횟수
	// 
	//	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS = 19,		// 게임서버 DB 저장 메시지 초당 처리 횟수
	//	dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG = 20,		// 게임서버 DB 저장 메시지 큐 개수 (남은 수)
	// 
	//	dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS = 21,		// 게임서버 AUTH 스레드 초당 프레임 수 (루프 수)
	//	dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS = 22,		// 게임서버 GAME 스레드 초당 프레임 수 (루프 수)
	//	dfMONITOR_DATA_TYPE_GAME_PACKET_POOL = 23,		// 게임서버 패킷풀 사용량



	groupManager_.GroupOnInitializeTPS();

	int timeStamp = static_cast<int>(time(nullptr));

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, 1, timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, static_cast<int>(processMonitoring_.ProcessTotal()), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, static_cast<int>(processMonitoring_.GetProcessUserMemoryMBytes()), timeStamp);

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_SESSION, GetSessionNum(), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, groupManager_.GetGroupPlayerSize(0), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, groupManager_.GetGroupPlayerSize(1), timeStamp);

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, GetAcceptTPS(), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, GetRecvMessageTPS(), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, GetSendMessageTPS(), timeStamp);

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, 0, timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, 0, timeStamp);

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, groupManager_.GetGroupFPS(0), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, groupManager_.GetGroupFPS(1), timeStamp);

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, CPacket::GetCapacity(), timeStamp);

	monitoringClient_.SendMonitorData();




	LogicTPS = LogicCount;
	LoginTPS = LoginCount;
	HeartBeatTPS = HeartBeatCount;

	LogicCount = 0;
	LoginCount = 0;
	HeartBeatCount = 0;

}





DWORD GameEchoServer::GetLogicTPS()
{
	return LogicTPS;
}

DWORD GameEchoServer::GetLoginTPS()
{
	return LoginTPS;
}

DWORD GameEchoServer::GetHeartBeatTPS()
{
	return HeartBeatTPS;
}

DWORD GameEchoServer::GetDCWrongPacket()
{
	return DCWrongPacket;
}

DWORD GameEchoServer::GetDCAuthFailed()
{
	return DCAuthFailed;
}

DWORD GameEchoServer::GetDCDuplicateLogin()
{
	return DCDuplicateLogin;
}

DWORD GameEchoServer::GetLoginPlayer()
{
	return LoginPlayer;
}

DWORD GameEchoServer::GetUnloginPlayer()
{
	return UnloginPlayer;
}

