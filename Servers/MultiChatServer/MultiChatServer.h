#pragma once

#include "NetLibrary.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "CommonProtocol.h"
#include "LockFreeObjectFreeList.h"
#include <unordered_map>
#include <list>
#include "ChatMonitoringClient.h"
#include "ProcessMonitoring.h"
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")
//class ContentsCPacket;


class MultiChatServer : public NetLibrary
{
public:

	struct Player
	{
		INT64 SessionID = 0;
		INT64 AccountNo = 0;

		WORD SectorX = 0;
		WORD SectorY = 0;

		WCHAR ID[20];
		WCHAR NickName[20];

		bool AuthFlag = 0; //인증되지 않은 유저로부터 온 메시지는 결함이니까.
		bool Duplicate = 0;

	};

	
	TLSObjectFreeList<Player> PlayerPool;
	std::unordered_map<__int64, Player*> PlayerMap;
	SRWLOCK PlayerMapLock;


	std::unordered_map<__int64, Player*> AccountMap;
	SRWLOCK AccountMapLock;

	struct SectorPos
	{
		WORD X;
		WORD Y;

	};

	struct SectorAround
	{
		unsigned int Count;
		SectorPos Around[9];
	};

	std::list<Player*> SectorList[MAXSECTORY][MAXSECTORX];
	SRWLOCK SectorLock[MAXSECTORY][MAXSECTORX];

	//DWORD TLSRedisConnectionIndex;

	//struct TLSRedisConnection
	//{
	//	cpp_redis::client Connection;
	//	std::string Key;
	//	std::string Value;
	//};

	cpp_redis::client* Connection_;
	SRWLOCK connectionLock_;



	bool AuthToken(INT64 AccountNo, char* SessionKey);

	MultiChatServer();
	virtual ~MultiChatServer();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);
	virtual void OnMessage(__int64 session_ID, ContentsCPacket* send_packet);
	virtual void OnError(int errorcode, const wchar_t* error_log);
	virtual void OnInitializeTPS();


	void NetPacketProc_Login(Player* target, INT64 AccountNo, WCHAR* ID, WCHAR* NickName, char* SessionKey);
	void NetPacketProc_SectorMove(Player* target, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void NetPacketProc_Message(Player* target, INT64 AccountNo, WORD MessageLen, WCHAR* Message);
	void NetPacketProc_Heartbeat(Player* target);

	void GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector);

	void LockSectorMove(WORD FirstX, WORD FirstY, WORD SecondX, WORD SecondY);
	void UnlockSectorMove(WORD FirstX, WORD FirstY, WORD SecondX, WORD SecondY);

	DWORD LogicTPS;
	DWORD LoginTPS;
	DWORD SectorMoveTPS;
	DWORD ChatTPS;
	DWORD HeartBeatTPS;

	DWORD LogicCount;
	DWORD LoginCount;
	DWORD SectorMoveCount;
	DWORD ChatCount;
	DWORD HeartBeatCount;

	DWORD DCWrongPacket;
	DWORD DCLoginAgain;
	DWORD DCAuthFailed;
	DWORD DCDuplicateLogin;


	DWORD LoginPlayer;
	DWORD UnloginPlayer;


	DWORD GetLogicTPS();
	DWORD GetLoginTPS();
	DWORD GetSectorMoveTPS();
	DWORD GetChatTPS();
	DWORD GetHeartBeatTPS();

	DWORD GetDCWrongPacket();
	DWORD GetDCLoginAgain();
	DWORD GetDCAuthFailed();
	DWORD GetDCDuplicateLogin();

	DWORD GetLoginPlayer();
	DWORD GetUnloginPlayer();

	ChatMonitoringClient monitoringClient_;

	ProcessMonitoring processMonitoring_;
};