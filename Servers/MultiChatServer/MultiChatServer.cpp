#include "MultiChatServer.h"
#include "ContentsCPacket.h"
#include <process.h>






MultiChatServer::MultiChatServer() : PlayerPool(0)
{
	InitializeSRWLock(&PlayerMapLock);
	InitializeSRWLock(&AccountMapLock);

	for (int i = 0; i < MAXSECTORY; i++)
	{
		for (int j = 0; j < MAXSECTORX; j++)
		{
			InitializeSRWLock(&SectorLock[i][j]);
		}
	}

	monitoringClient_.Start("127.0.0.1", 5670, 1, 2);

	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);
	Connection_ = new cpp_redis::client;

	InitializeSRWLock(&connectionLock_);

	Connection_->connect();

}


MultiChatServer::~MultiChatServer()
{

}

bool MultiChatServer::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void MultiChatServer::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 SessionID)
{

	Player* NewPlayer = PlayerPool.Alloc();
	NewPlayer->SessionID = SessionID;
	NewPlayer->SectorX = INITSECTORX;
	NewPlayer->AuthFlag = false;
	NewPlayer->Duplicate = false;
	InterlockedIncrement(&UnloginPlayer);

	AcquireSRWLockExclusive(&PlayerMapLock);
	PlayerMap.insert(std::unordered_map<__int64, Player*>::value_type(NewPlayer->SessionID, NewPlayer));
	ReleaseSRWLockExclusive(&PlayerMapLock);



}

void MultiChatServer::OnRelease(__int64 SessionID)
{
	Player* TargetPlayer;
	AcquireSRWLockExclusive(&PlayerMapLock);
	std::unordered_map<__int64, Player*>::iterator PI = PlayerMap.find(SessionID);
	if (PI == PlayerMap.end())
	{
		//왜 없니
		//__debugbreak();
	}
	TargetPlayer = PI->second;
	PlayerMap.erase(TargetPlayer->SessionID);
	ReleaseSRWLockExclusive(&PlayerMapLock);


	if (TargetPlayer->SectorX != INITSECTORX)
	{
		AcquireSRWLockExclusive(&SectorLock[TargetPlayer->SectorY][TargetPlayer->SectorX]);

		std::list<Player*>::iterator LI = SectorList[TargetPlayer->SectorY][TargetPlayer->SectorX].begin();

		for (; LI != SectorList[TargetPlayer->SectorY][TargetPlayer->SectorX].end(); ++LI)
		{
			Player* Target = *LI;
			if (Target->SessionID == TargetPlayer->SessionID)
			{
				SectorList[TargetPlayer->SectorY][TargetPlayer->SectorX].erase(LI);
				break;
			}
		}
		ReleaseSRWLockExclusive(&SectorLock[TargetPlayer->SectorY][TargetPlayer->SectorX]);

	}

	if (TargetPlayer->AuthFlag)
	{
		InterlockedDecrement(&LoginPlayer);
	}
	else
	{
		InterlockedDecrement(&UnloginPlayer);
	}

	AcquireSRWLockExclusive(&AccountMapLock);
	std::unordered_map<__int64, Player*>::iterator AI = AccountMap.find(TargetPlayer->AccountNo);
	if (AI != AccountMap.end())
	{
		if (TargetPlayer->SessionID == AI->second->SessionID)
		{
			AccountMap.erase(TargetPlayer->AccountNo);
		}
	}
	ReleaseSRWLockExclusive(&AccountMapLock);
	PlayerPool.Free(TargetPlayer);

}

void MultiChatServer::OnMessage(__int64 SessionID, ContentsCPacket* contents_send_packet)
{

	WORD MessageType;

	ContentsCPacket ContentsPacket = contents_send_packet;

	ContentsPacket >> MessageType;

	Player* TargetPlayer;

	AcquireSRWLockShared(&PlayerMapLock);
	std::unordered_map<__int64, Player*>::iterator PI = PlayerMap.find(SessionID);
	if (PI == PlayerMap.end())
	{
		//__debugbreak();
		//음.. 문제가 있는 상황이다. 
	}
	TargetPlayer = PI->second;
	ReleaseSRWLockShared(&PlayerMapLock);

	//채팅서버 로그인 요청 : 152
	//채팅서버 섹터 이동 요청 12 / 정상적인 섹터의 범위가 온 것인지.
	//채팅서버 메시지 요청 8 + 2 + 106  116

	switch (MessageType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
	{

		INT64	AccountNo;
		WCHAR	ID[20];				// null 포함
		WCHAR	Nickname[20];		// null 포함
		char	SessionKey[64];		// 인증토큰

		if (ContentsPacket.GetDataSize() != 152)
		{
			InterlockedIncrement(&DCWrongPacket);
			Disconnect(SessionID);
			break;
		}

		ContentsPacket >> AccountNo;
		ContentsPacket.GetData((char*)ID, 40);
		ContentsPacket.GetData((char*)Nickname, 40);
		ContentsPacket.GetData((char*)SessionKey, sizeof(SessionKey));

		NetPacketProc_Login(TargetPlayer, AccountNo, ID, Nickname, SessionKey);
		InterlockedIncrement(&LoginCount);

		break;
	}
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
	{

		INT64	AccountNo;
		WORD	SectorX;
		WORD	SectorY;

		if (ContentsPacket.GetDataSize() != 12)
		{
			InterlockedIncrement(&DCWrongPacket);
			Disconnect(SessionID);
			break;
		}

		ContentsPacket >> AccountNo;
		ContentsPacket >> SectorX;
		ContentsPacket >> SectorY;

		//유효성 검사
		if (SectorX >= MAXSECTORX || SectorY >= MAXSECTORY)
		{
			Disconnect(SessionID);
			InterlockedIncrement(&DCWrongPacket);
			break;
		}

		NetPacketProc_SectorMove(TargetPlayer, AccountNo, SectorX, SectorY);
		InterlockedIncrement(&SectorMoveCount);

		break;
	}
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
	{

		INT64	AccountNo;
		WORD	MessageLen;
		WCHAR	Message[MAXCHATSIZE]; // null 미포함

		if (ContentsPacket.GetDataSize() > 116)
		{
			InterlockedIncrement(&DCWrongPacket);
			Disconnect(SessionID);
			break;
		}


		ContentsPacket >> AccountNo;
		ContentsPacket >> MessageLen;

		if (MessageLen > 106)
		{
			InterlockedIncrement(&DCWrongPacket);
			Disconnect(SessionID);
			break;
		}

		ContentsPacket.GetData((char*)Message, MessageLen);
		Message[MessageLen / 2] = L'\0';

		NetPacketProc_Message(TargetPlayer, AccountNo, MessageLen, Message);
		InterlockedIncrement(&ChatCount);

		break;
	}
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
	{
		if (ContentsPacket.GetDataSize() != 0)
		{
			InterlockedIncrement(&DCWrongPacket);
			Disconnect(SessionID);
			break;
		}
		NetPacketProc_Heartbeat(TargetPlayer);
		InterlockedIncrement(&HeartBeatCount);
		break;
	}
	default:
	{
		//잘못된 데이터
		InterlockedIncrement(&DCWrongPacket);
		Disconnect(SessionID);
		break;
	}

	}
	InterlockedIncrement(&LogicCount);



}

void MultiChatServer::OnError(int errorcode, const wchar_t* error_log)
{

}

void MultiChatServer::OnInitializeTPS()
{
	//여기서 하면 돼
	int timeStamp = static_cast<int>(time(nullptr));

	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1, timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, static_cast<int>(processMonitoring_.ProcessTotal()), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, static_cast<int>(processMonitoring_.GetProcessUserMemoryMBytes()), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_SESSION, GetSessionNum(), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, GetLoginPlayer(), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, LogicTPS, timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, CPacket::GetCapacity(), timeStamp);
	monitoringClient_.UpdateMonitorData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, 0, timeStamp);

	monitoringClient_.SendMonitorData();


	LogicTPS = LogicCount;
	LoginTPS = LoginCount;
	SectorMoveTPS = SectorMoveCount;
	ChatTPS = ChatCount;
	HeartBeatTPS = HeartBeatCount;

	LogicCount = 0;
	LoginCount = 0;
	SectorMoveCount = 0;
	ChatCount = 0;
	HeartBeatCount = 0;

}


bool MultiChatServer::AuthToken(INT64 AccountNo, char* SessionKey)
{
	//세션키를 레디스에 조회
	AcquireSRWLockExclusive(&connectionLock_);

	std::string Key = std::to_string(AccountNo);

	std::string Value;
	Connection_->get(Key, [&Value](cpp_redis::reply& reply) { if (reply.is_string()) { Value = reply.as_string(); } });

	Connection_->sync_commit();

	if (memcmp(Value.c_str(), SessionKey, 64) != 0)
	{
		//세션키 오류
		//디버그 브레이크
		//상황이 나오긴 하는지 체크 
		ReleaseSRWLockExclusive(&connectionLock_);
		return false;
	}
	Connection_->del({ Key });
	Connection_->sync_commit();
	ReleaseSRWLockExclusive(&connectionLock_);

	return true;
}



void MultiChatServer::NetPacketProc_Login(Player* TargetPlayer, INT64 AccountNo, WCHAR* ID, WCHAR* NickName, char* SessionKey)
{
	if (TargetPlayer->AuthFlag == TRUE)
	{
		//공격 로그인 메시지가 중복으로 옴.
		InterlockedIncrement(&DCLoginAgain);
		Disconnect(TargetPlayer->SessionID);
		return;

	}


	TargetPlayer->AccountNo = AccountNo;
	wmemcpy_s(TargetPlayer->ID, 20, ID, 20);
	wmemcpy_s(TargetPlayer->NickName, 20, NickName, 20);

	if (!AuthToken(AccountNo, SessionKey))
	{
		Disconnect(TargetPlayer->SessionID);
		InterlockedIncrement(&DCAuthFailed);
		return;
	}

	AcquireSRWLockExclusive(&AccountMapLock);
	std::unordered_map<__int64, Player*>::iterator AI = AccountMap.find(AccountNo);
	if (AI != AccountMap.end())
	{
		Player* DisconnectPlayer = AI->second;
		//중복 로그인
		Disconnect(DisconnectPlayer->SessionID);
		DisconnectPlayer->Duplicate = true;
		InterlockedIncrement(&DCDuplicateLogin);
		AccountMap.erase(AccountNo);

	}

	AccountMap.insert(std::unordered_map<__int64, Player*>::value_type(TargetPlayer->AccountNo, TargetPlayer));

	ReleaseSRWLockExclusive(&AccountMapLock);


	TargetPlayer->AuthFlag = TRUE;

	ContentsCPacket login_packet = ContentsCPacket::MakeContentsPacket();
	login_packet << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << BYTE(TRUE) << TargetPlayer->AccountNo;

	SendPacket(TargetPlayer->SessionID, login_packet);
	InterlockedDecrement(&UnloginPlayer);
	InterlockedIncrement(&LoginPlayer);


}

void MultiChatServer::NetPacketProc_SectorMove(Player* TargetPlayer, INT64 AccountNo, WORD SectorX, WORD SectorY)
{


	if (TargetPlayer->SectorX == INITSECTORX) // 최초의 섹터 진입 , 내것만 추가하면 돼
	{
		TargetPlayer->SectorX = SectorX;
		TargetPlayer->SectorY = SectorY;

		AcquireSRWLockExclusive(&SectorLock[TargetPlayer->SectorY][TargetPlayer->SectorX]);
		SectorList[TargetPlayer->SectorY][TargetPlayer->SectorX].push_back(TargetPlayer);
		ReleaseSRWLockExclusive(&SectorLock[TargetPlayer->SectorY][TargetPlayer->SectorX]);


	}
	else
	{
		WORD PrevX = TargetPlayer->SectorX;
		WORD PrevY = TargetPlayer->SectorY;

		TargetPlayer->SectorX = SectorX;
		TargetPlayer->SectorY = SectorY;
		if (!(TargetPlayer->SectorX == PrevX && TargetPlayer->SectorY == PrevY))
		{
			LockSectorMove(PrevX, PrevY, TargetPlayer->SectorX, TargetPlayer->SectorY);
			std::list<Player*>::iterator LI = SectorList[PrevY][PrevX].begin();
			for (; LI != SectorList[PrevY][PrevX].end(); ++LI)
			{
				Player* Target = *LI;

				if (Target->SessionID == TargetPlayer->SessionID)
				{
					SectorList[PrevY][PrevX].erase(LI);
					break;
				}
			}
			SectorList[TargetPlayer->SectorY][TargetPlayer->SectorX].push_back(TargetPlayer);

			UnlockSectorMove(PrevX, PrevY, TargetPlayer->SectorX, TargetPlayer->SectorY);
		}

	}

	ContentsCPacket SectorMovePacket = ContentsCPacket::MakeContentsPacket();

	SectorMovePacket << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << TargetPlayer->AccountNo << TargetPlayer->SectorX << TargetPlayer->SectorY;

	SendPacket(TargetPlayer->SessionID, SectorMovePacket);


}

void MultiChatServer::NetPacketProc_Message(Player* TargetPlayer, INT64 AccountNo, WORD MessageLen, WCHAR* Message)
{

	ContentsCPacket ChatPacket = ContentsCPacket::MakeContentsPacket();

	if (TargetPlayer->Duplicate == true)
	{
		return;
	}


	ChatPacket << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE << TargetPlayer->AccountNo;
	ChatPacket.PutData((char*)TargetPlayer->ID, 40);
	ChatPacket.PutData((char*)TargetPlayer->NickName, 40);
	ChatPacket << MessageLen;
	ChatPacket.PutData((char*)Message, MessageLen);

	//주변 영향권 섹터를 순서에 맞게 구함 4~9개
	SectorAround AroundSector;
	GetSectorAround(TargetPlayer->SectorX, TargetPlayer->SectorY, &AroundSector);

	for (unsigned int i = 0; i < AroundSector.Count; ++i)
	{
		DWORD AroundX = AroundSector.Around[i].X;
		DWORD AroundY = AroundSector.Around[i].Y;
		std::list<Player*>::iterator LI;

		AcquireSRWLockShared(&SectorLock[AroundY][AroundX]);

		for (LI = SectorList[AroundY][AroundX].begin(); LI != SectorList[AroundY][AroundX].end(); ++LI)
		{
			Player* Target = *LI;
			SendPacket(Target->SessionID, ChatPacket);
		}
	}

	for (int i = AroundSector.Count - 1; i >= 0; --i)
	{

		DWORD AroundX = AroundSector.Around[i].X;
		DWORD AroundY = AroundSector.Around[i].Y;
		ReleaseSRWLockShared(&SectorLock[AroundY][AroundX]);
	}

}


void MultiChatServer::NetPacketProc_Heartbeat(Player* TargetPlayer)
{



}

void MultiChatServer::GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector)
{
	//1. X가 더 작고 Y가 더 작을수록.
	//2. 

	AroundSector->Count = 0;

	if (SectorY - 1 >= 0 && SectorX - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < MAXSECTORX)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorX - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY;
		AroundSector->Count++;
	}

	AroundSector->Around[AroundSector->Count].X = (WORD)SectorX;
	AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY;
	AroundSector->Count++;


	if (SectorX + 1 < MAXSECTORX)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY;
		AroundSector->Count++;
	}

	if (SectorY + 1 < MAXSECTORY && SectorX - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY + 1;
		AroundSector->Count++;
	}


	if (SectorY + 1 < MAXSECTORY)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY + 1;
		AroundSector->Count++;
	}

	if (SectorY + 1 < MAXSECTORY && SectorX + 1 < MAXSECTORX)
	{
		AroundSector->Around[AroundSector->Count].X = (WORD)SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = (WORD)SectorY + 1;
		AroundSector->Count++;
	}

	//VisitedAround[SectorY][SectorX] = *AroundSector;
	//VisitedAround[SectorY][SectorX].Flag = 1;


}

void MultiChatServer::LockSectorMove(WORD FirstX, WORD FirstY, WORD SecondX, WORD SecondY)
{

	if ((FirstY > SecondY) || (FirstY == SecondY && FirstX > SecondX))
	{
		WORD TempX = FirstX;
		WORD TempY = FirstY;
		FirstX = SecondX;
		FirstY = SecondY;
		SecondX = TempX;
		SecondY = TempY;
	}

	AcquireSRWLockExclusive(&SectorLock[FirstY][FirstX]);
	AcquireSRWLockExclusive(&SectorLock[SecondY][SecondX]);

}


void MultiChatServer::UnlockSectorMove(WORD FirstX, WORD FirstY, WORD SecondX, WORD SecondY)
{

	if ((FirstY > SecondY) || (FirstY == SecondY && FirstX > SecondX))
	{
		WORD TempX = FirstX;
		WORD TempY = FirstY;
		FirstX = SecondX;
		FirstY = SecondY;
		SecondX = TempX;
		SecondY = TempY;
	}

	ReleaseSRWLockExclusive(&SectorLock[SecondY][SecondX]);
	ReleaseSRWLockExclusive(&SectorLock[FirstY][FirstX]);

}


DWORD MultiChatServer::GetLogicTPS()
{
	return LogicTPS;
}

DWORD MultiChatServer::GetLoginTPS()
{
	return LoginTPS;
}

DWORD MultiChatServer::GetSectorMoveTPS()
{
	return SectorMoveTPS;
}

DWORD MultiChatServer::GetChatTPS()
{
	return ChatTPS;
}

DWORD MultiChatServer::GetHeartBeatTPS()
{
	return HeartBeatTPS;
}

DWORD MultiChatServer::GetDCWrongPacket()
{
	return DCWrongPacket;
}

DWORD MultiChatServer::GetDCLoginAgain()
{
	return DCLoginAgain;
}

DWORD MultiChatServer::GetDCAuthFailed()
{
	return DCAuthFailed;
}

DWORD MultiChatServer::GetDCDuplicateLogin()
{
	return DCDuplicateLogin;
}

DWORD MultiChatServer::GetLoginPlayer()
{
	return LoginPlayer;
}

DWORD MultiChatServer::GetUnloginPlayer()
{
	return UnloginPlayer;
}

