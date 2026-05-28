#include "ChattingServer_Single.h"
#include "ContentsCPacket.h"
#include <process.h>

ChattingServerSingle::ChattingServerSingle() :MessageDataFreeList(0), PlayerPool(0)
{
	LogicThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, LogicThread, this, 0, nullptr);

	MessageEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("WaitEvent"));

	if (MessageEvent == NULL)
	{
		printf("CreateEvent failed (%d)\n", GetLastError());
		DebugBreak();
	}

}

ChattingServerSingle::~ChattingServerSingle()
{

}

bool ChattingServerSingle::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void ChattingServerSingle::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID)
{


	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = session_ID;
	msg_data->contents_packet = nullptr;
	msg_data->type = ACCEPT;
	
	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);


}

void ChattingServerSingle::OnRelease(__int64 session_ID)
{
	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = session_ID;
	msg_data->contents_packet = nullptr;
	msg_data->type = RELEASE;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);
}

void ChattingServerSingle::OnMessage(__int64 session_ID, ContentsCPacket* contents_send_packet)
{


	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = session_ID;
	msg_data->contents_packet = contents_send_packet;
	msg_data->type = MESSAGE;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);

}

void ChattingServerSingle::OnError(int errorcode, const wchar_t* error_log)
{

}

void ChattingServerSingle::OnInitializeTPS()
{

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

void ChattingServerSingle::AcceptProc(MessageData* msg_data)
{
	Player* NewPlayer = PlayerPool.Alloc();
	NewPlayer->SessionID = msg_data->session_ID;
	NewPlayer->SectorX = INITSECTORX;
	NewPlayer->AuthFlag = false;
	NewPlayer->Duplicate = false;
	UnloginPlayer++;
	PlayerMap.insert(std::unordered_map<__int64, Player*>::value_type(NewPlayer->SessionID, NewPlayer));

}

void ChattingServerSingle::MessageProc(MessageData* msg_data)
{
	WORD MessageType;

	ContentsCPacket ContentsPacket = msg_data->contents_packet;

	ContentsPacket >> MessageType;

	Player* TargetPlayer;
	std::unordered_map<__int64, Player*>::iterator PI = PlayerMap.find(msg_data->session_ID);
	if (PI == PlayerMap.end())
	{
		__debugbreak();
		//음.. 문제가 있는 상황이다. 
	}
	TargetPlayer = PI->second;

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
				DCWrongPacket++;
				Disconnect(msg_data->session_ID);
				break;
			}


			ContentsPacket >> AccountNo;
			ContentsPacket.GetData((char*)ID, 40);
			ContentsPacket.GetData((char*)Nickname, 40);
			ContentsPacket.GetData((char*)SessionKey, sizeof(SessionKey));

			NetPacketProc_Login(TargetPlayer, AccountNo, ID, Nickname, SessionKey);
			LoginCount++;

			break;
		}
		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		{

			INT64	AccountNo;
			WORD	SectorX;
			WORD	SectorY;

			if (ContentsPacket.GetDataSize() != 12)
			{
				DCWrongPacket++;
				Disconnect(msg_data->session_ID);
				break;
			}

			ContentsPacket >> AccountNo;
			ContentsPacket >> SectorX;
			ContentsPacket >> SectorY;

			//유효성 검사
			if (SectorX > MAXSECTORX || SectorY > MAXSECTORY)
			{
				Disconnect(msg_data->session_ID);
				DCWrongPacket++;
				break;
			}

			NetPacketProc_SectorMove(TargetPlayer, AccountNo, SectorX, SectorY);
			SectorMoveCount++;

			break;
		}
		case en_PACKET_CS_CHAT_REQ_MESSAGE:
		{

			INT64	AccountNo;
			WORD	MessageLen;
			WCHAR	Message[MAXCHATSIZE]; // null 미포함

			if (ContentsPacket.GetDataSize() > 116)
			{
				DCWrongPacket++;
				Disconnect(msg_data->session_ID);
				break;
			}

			ContentsPacket >> AccountNo;
			ContentsPacket >> MessageLen;

			if (MessageLen > 106)
			{
				DCWrongPacket++;
				Disconnect(msg_data->session_ID);
				break;
			}

			ContentsPacket.GetData((char*)Message, MessageLen);
			Message[MessageLen/2] = L'\0';

			NetPacketProc_Message(TargetPlayer, AccountNo, MessageLen, Message);
			ChatCount++;

			break;
		}
		case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		{
			NetPacketProc_Heartbeat(TargetPlayer);
			HeartBeatCount++;
			break;
		}
		default:
		{
			//잘못된 데이터
			DCWrongPacket++;
			Disconnect(msg_data->session_ID);
			break;
		}

	}
	LogicCount++;

}




void ChattingServerSingle::ReleaseProc(MessageData* msg_data)
{
	Player* TargetPlayer;

	std::unordered_map<__int64, Player*>::iterator PI = PlayerMap.find(msg_data->session_ID);
	if (PI != PlayerMap.end())
	{
		TargetPlayer = PI->second;
		if (TargetPlayer->SectorX != INITSECTORX)
		{
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

		}

		if (TargetPlayer->AuthFlag)
		{
			LoginPlayer--;
		}
		else
		{
			UnloginPlayer--;
		}

		std::unordered_map<__int64, Player*>::iterator AI = AccountMap.find(TargetPlayer->AccountNo);
		if (AI != AccountMap.end())
		{
			if (TargetPlayer->SessionID == AI->second->SessionID)
			{
				AccountMap.erase(TargetPlayer->AccountNo);
			}
		}



		if (PlayerMap.erase(TargetPlayer->SessionID) == 0)
		{
			//문제가 있다. 파악. 
		}
		PlayerPool.Free(TargetPlayer);

	}
	
	

	//섹터에서 플레이어 삭제



}


void ChattingServerSingle::NetPacketProc_Login(Player* TargetPlayer, INT64 AccountNo, WCHAR* ID, WCHAR* NickName, char* SessionKey)
{

	//토근과 AccountNo를 가지고 다시 한번 인증과정을 거침. // 아마 DB에 접근할 것 .
	//DB스레드풀이 나와야 함. 어차피 블락킹일거니 여러 개 만들어 놓자. 
	//하고 다시 인큐. 

	//세팅조차 인증이 끝나야 해주고 싶은데... 어차피 넘기려해도 멤카피가 필요하긴 함. 세팅해주자. 
	TargetPlayer->AccountNo = AccountNo;
	TargetPlayer->LastRecvTime = GetTickCount64();
	wmemcpy_s(TargetPlayer->ID, 20, ID, 20);
	wmemcpy_s(TargetPlayer->NickName, 20, NickName, 20);

	std::unordered_map<__int64, Player*>::iterator AI = AccountMap.find(AccountNo);
	if (AI != AccountMap.end())
	{
		Player* DisconnectPlayer = AI->second;
		//중복 로그인
		Disconnect(DisconnectPlayer->SessionID);
		DisconnectPlayer->Duplicate = true;
		DCDuplicateLogin++;
		if (AccountMap.erase(AccountNo) == 0)
		{
			__debugbreak();
			//문제가 있다. 파악. 
		}
	}

	AccountMap.insert(std::unordered_map<__int64, Player*>::value_type(TargetPlayer->AccountNo, TargetPlayer));

	//	std::unordered_map<__int64, Player*>::iterator PI = PlayerMap.find(msg_data->session_ID);
	
	//TargetPlayer = PI->second;

	TargetPlayer->AuthFlag = TRUE;

	ContentsCPacket login_packet = ContentsCPacket::MakeContentsPacket();
	login_packet << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << BYTE(TRUE) << TargetPlayer->AccountNo;

	SendPacket(TargetPlayer->SessionID, login_packet);
	UnloginPlayer--;
	LoginPlayer++;


}

void ChattingServerSingle::NetPacketProc_SectorMove(Player* TargetPlayer, INT64 AccountNo, WORD SectorX, WORD SectorY)
{
	if (TargetPlayer->SectorX == INITSECTORX) // 최초의 섹터 진입 , 내것만 추가하면 돼
	{
		TargetPlayer->SectorX = SectorX;
		TargetPlayer->SectorY = SectorY;

		SectorList[TargetPlayer->SectorY][TargetPlayer->SectorX].push_back(TargetPlayer);
	}
	else
	{
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

		TargetPlayer->SectorX = SectorX;
		TargetPlayer->SectorY = SectorY;

		SectorList[TargetPlayer->SectorY][TargetPlayer->SectorX].push_back(TargetPlayer);
	}

	ContentsCPacket SectorMovePacket = ContentsCPacket::MakeContentsPacket();

	SectorMovePacket << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << TargetPlayer->AccountNo << TargetPlayer->SectorX << TargetPlayer->SectorY;

	SendPacket(TargetPlayer->SessionID, SectorMovePacket);



}

void ChattingServerSingle::NetPacketProc_Message(Player* TargetPlayer, INT64 AccountNo, WORD MessageLen, WCHAR* Message)
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

	//주변 영향권 섹터를 구함 4~9개
	SectorAround AroundSector;
	GetSectorAround(TargetPlayer->SectorX, TargetPlayer->SectorY, &AroundSector);

	for (unsigned int i = 0; i < AroundSector.Count; ++i)
	{
		DWORD AroundX = AroundSector.Around[i].X;
		DWORD AroundY = AroundSector.Around[i].Y;
		std::list<Player*>::iterator LI;

		for (LI = SectorList[AroundY][AroundX].begin(); LI != SectorList[AroundY][AroundX].end(); ++LI)
		{
			Player* Target = *LI;
			SendPacket(Target->SessionID, ChatPacket);
		}
	}
}


void ChattingServerSingle::NetPacketProc_Heartbeat(Player* TargetPlayer)
{

	TargetPlayer->LastRecvTime = GetTickCount64();

}

void ChattingServerSingle::GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector)
{


	AroundSector->Count = 0;
	AroundSector->Around[AroundSector->Count].X = SectorX;
	AroundSector->Around[AroundSector->Count].Y = SectorY;
	AroundSector->Count++;



	if (SectorX + 1 < MAXSECTORX)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY;
		AroundSector->Count++;

	}
	if (SectorX - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY;
		AroundSector->Count++;
	}


	if (SectorY + 1 < MAXSECTORY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorY + 1 < MAXSECTORY && SectorX + 1 < MAXSECTORX)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < MAXSECTORX)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorY + 1 < MAXSECTORY && SectorX - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}
	if (SectorY - 1 >= 0 && SectorX - 1 >= 0)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

	//VisitedAround[SectorY][SectorX] = *AroundSector;
	//VisitedAround[SectorY][SectorX].Flag = 1;


}

unsigned int WINAPI ChattingServerSingle::LogicThread(LPVOID this_ptr)
{
	ChattingServerSingle* server = static_cast<ChattingServerSingle*>(this_ptr);

	while (true)
	{
		WaitForSingleObject(server->MessageEvent, INFINITE);

		while (true)
		{
			MessageData* msg = nullptr;
			if (!server->MessageQueue.Dequeue(&msg))
			{
				continue;
			}

			switch (msg->type)
			{
				case ChattingServerSingle::ACCEPT:
				{
					server->AcceptProc(msg);
					break;
				}
				case ChattingServerSingle::MESSAGE:
				{
					server->MessageProc(msg);
					break;
				}
				case ChattingServerSingle::RELEASE:
				{
					server->ReleaseProc(msg);
					break;
				}
				default:
				{
					break;
				}
			}

			server->MessageDataFreeList.Free(msg);
		}
	}
	return 0;
}


DWORD ChattingServerSingle::GetLogicTPS()
{
	return LogicTPS;
}

DWORD ChattingServerSingle::GetLoginTPS()
{
	return LoginTPS;
}

DWORD ChattingServerSingle::GetSectorMoveTPS()
{
	return SectorMoveTPS;
}

DWORD ChattingServerSingle::GetChatTPS()
{
	return ChatTPS;
}

DWORD ChattingServerSingle::GetHeartBeatTPS()
{
	return HeartBeatTPS;
}

DWORD ChattingServerSingle::GetDCWrongPacket()
{
	return DCWrongPacket;
}

DWORD ChattingServerSingle::GetDCAuthFailed()
{
	return DCAuthFailed;
}

DWORD ChattingServerSingle::GetDCDuplicateLogin()
{
	return DCDuplicateLogin;
}

DWORD ChattingServerSingle::GetLoginPlayer()
{
	return LoginPlayer;
}

DWORD ChattingServerSingle::GetUnloginPlayer()
{
	return UnloginPlayer;
}

DWORD ChattingServerSingle::GetLogicQueueSize()
{
	return MessageQueue.GetSize();
}

