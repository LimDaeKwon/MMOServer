#pragma once

//#include "LanNetworkLibraryServerForEchoTest.h"
#include "NetLibrary.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "CommonProtocol.h"
#include "ObjectFreeList.h"
#include <unordered_map>
#include <list>

//class ContentsCPacket;


class ChattingServerSingle : public NetLibrary
{
public:
	enum Type
	{
		ACCEPT,
		MESSAGE,
		RELEASE,
	};

	struct Player
	{
		INT64 SessionID;
		INT64 LastRecvTime;//하트비트용
		INT64 AccountNo;

		WORD SectorX;
		WORD SectorY;

		WCHAR ID[20];
		WCHAR NickName[20];

		bool AuthFlag; //인증되지 않은 유저로부터 온 메시지는 결함이니까.
		bool Duplicate;
	};

	ObjectFreeList<Player> PlayerPool;
	std::unordered_map<__int64, Player*> PlayerMap;
	std::unordered_map<__int64, Player*> AccountMap;


	struct SectorPos
	{
		unsigned int X;
		unsigned int Y;

	};

	struct SectorAround
	{
		unsigned int Count;
		SectorPos Around[9];
	};

	std::list<Player*> SectorList[MAXSECTORY][MAXSECTORX];

	struct MessageData
	{
		int type;
		__int64 session_ID;
		ContentsCPacket* contents_packet;
	};


	LockFreeQueue<MessageData*> MessageQueue;
	TLSObjectFreeList<MessageData> MessageDataFreeList;
	HANDLE MessageEvent;



	ChattingServerSingle();
	virtual ~ChattingServerSingle();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);
	virtual void OnMessage(__int64 session_ID, ContentsCPacket* send_packet);
	virtual void OnError(int errorcode, const wchar_t* error_log);
	virtual void OnInitializeTPS();

	void AcceptProc(MessageData* msg_data);
	void MessageProc(MessageData* msg_data);
	void ReleaseProc(MessageData* msg_data);


	void NetPacketProc_Login(Player* target, INT64 AccountNo,WCHAR* ID, WCHAR* NickName, char* SessionKey);
	void NetPacketProc_SectorMove(Player* target, INT64 AccountNo, WORD SectorX, WORD SectorY);
	void NetPacketProc_Message(Player* target, INT64 AccountNo,WORD MessageLen, WCHAR* Message);
	void NetPacketProc_Heartbeat(Player* target);

	void GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector);

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
	DWORD GetDCAuthFailed();
	DWORD GetDCDuplicateLogin();

	DWORD GetLoginPlayer();
	DWORD GetUnloginPlayer();

	DWORD GetLogicQueueSize();
	

	HANDLE LogicThreadHandle;
	static unsigned int WINAPI LogicThread(LPVOID this_ptr);





};