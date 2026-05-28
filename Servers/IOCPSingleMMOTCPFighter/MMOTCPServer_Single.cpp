#include "MMOTCPServer_Single.h"
#include "ContentsCPacket.h"
#include <process.h>
#include "PacketDefine.h"
#include <conio.h>


MMOTCPServer_Single::MMOTCPServer_Single() :MessageDataFreeList(1000), CharacterFreeList(1000)
{

	wprintf(L"MMOTCPServer_Single\n");

	OldTick = timeGetTime();
	OldTickforCheck = OldTick;

	LogicThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, LogicThread, this, 0, nullptr);

}

MMOTCPServer_Single::~MMOTCPServer_Single()
{

}

bool MMOTCPServer_Single::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void MMOTCPServer_Single::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID)
{


	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = session_ID;
	msg_data->contents_packet = nullptr;
	msg_data->type = ACCEPT;

	MessageQueue.Enqueue(msg_data);


}

void MMOTCPServer_Single::OnRelease(__int64 session_ID)
{

	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = session_ID;
	msg_data->contents_packet = nullptr;
	msg_data->type = RELEASE;

	MessageQueue.Enqueue(msg_data);


}

void MMOTCPServer_Single::OnMessage(__int64 session_ID, CPacket* contents_send_packet)
{


	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = session_ID;
	contents_send_packet->IncreaseRefCount();
	msg_data->contents_packet = contents_send_packet;
	msg_data->type = PACKET;

	MessageQueue.Enqueue(msg_data);

}

void MMOTCPServer_Single::OnError(int errorcode, const wchar_t* error_log)
{

}


unsigned int WINAPI MMOTCPServer_Single::LogicThread(LPVOID this_ptr)
{
	MMOTCPServer_Single* server = static_cast<MMOTCPServer_Single*>(this_ptr);

	while (true)
	{

		server->MessageLoop();

		server->Update();

	}

	return 0;
}

void MMOTCPServer_Single::MessageLoop()
{
	while (1)
	{
		MessageData* msg = nullptr;
		if (!MessageQueue.Dequeue(&msg))
		{
			break;
		}

		if (!MessageProc(msg))
		{
			DebugBreak();
		}

		if (msg->contents_packet != nullptr)
		{
			CPacket::Free(msg->contents_packet);
		}
		MessageDataFreeList.Free(msg);
	}
}


bool MMOTCPServer_Single::MessageProc(MessageData* msg)
{
	switch (msg->type)
	{
	case ACCEPT:
	{
		CreateCharater(msg->session_ID);
		break;
	}
	case PACKET:
	{
		PacketProc(msg);
		break;
	}
	case RELEASE:
	{
		ReleaseCharacter(msg->session_ID);
		break;

	}

	}
	return true;
}

void MMOTCPServer_Single::ReleaseCharacter(__int64 NewSession)
{
	CHARACTER* Target = CharacterMap.at(NewSession);

	CharacterMap.erase(NewSession);
	Sector[Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X].remove(Target);


	if (Target->SessionIDForContents == 0)
	{
		DebugBreak();
	}
	CPacket* DeleteCharacterPacket = CPacket::Alloc();
	MakePacketDeleteCharacter(Target, DeleteCharacterPacket, Target->SessionIDForContents);
	CPacket::Free(DeleteCharacterPacket);

	CharacterFreeList.Free(Target);

	//wprintf(L"## Disconnect ID : %d \n", TargetSession->SessionID);



}




bool MMOTCPServer_Single::PacketProc(MessageData* msg)
{
	BYTE ByType;
	*msg->contents_packet >> ByType;

	CHARACTER* Target;
	Target = CharacterMap.at(msg->session_ID);


	switch (ByType)
	{
	case dfPACKET_CS_MOVE_START:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*msg->contents_packet >> Direction >> X >> Y;
		return NetPacketProc_MoveStart(Target, Direction, X, Y);
		break;
	}

	case dfPACKET_CS_MOVE_STOP:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*msg->contents_packet >> Direction >> X >> Y;
		return NetPacketProc_MoveStop(Target, Direction, X, Y);
		break;
	}

	case dfPACKET_CS_ATTACK1:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*msg->contents_packet >> Direction >> X >> Y;
		return NetPacketProc_Attack1(Target, Direction, X, Y);
		break;
	}

	case dfPACKET_CS_ATTACK2:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*msg->contents_packet >> Direction >> X >> Y;
		return NetPacketProc_Attack2(Target, Direction, X, Y);
		break;
	}

	case dfPACKET_CS_ATTACK3:
	{
		unsigned char Direction;
		unsigned short X;
		unsigned short Y;
		*msg->contents_packet >> Direction >> X >> Y;
		return NetPacketProc_Attack3(Target, Direction, X, Y);
		break;
	}

	case dfPACKET_CS_ECHO:
	{
		unsigned int Time;
		*msg->contents_packet >> Time;
		return NetPacketProc_Echo(Target, Time);
		break;
	}
	default:
	{
		return false;
		break;
	}

	}

	return false;
}


void MMOTCPServer_Single::CreateCharater(__int64 new_session)
{
	CHARACTER* new_player = CharacterFreeList.Alloc();
	new_player->SessionID = new_session;
	new_player->SessionIDForContents = (unsigned int)new_session;
	new_player->Direction = dfPACKET_MOVE_DIR_RR;
	new_player->Action = dfPACKET_MOVE_DIR_RR;
	new_player->X = rand() % 6399;
	new_player->Y = rand() % 6399;

	//new_player->X = 100;
	//new_player->Y = 100;

	new_player->HP = 100;
	new_player->CharacterSectorPos.X = new_player->X / SECTORXSIZE;
	new_player->CharacterSectorPos.Y = new_player->Y / SECTORYSIZE;
	new_player->OldSectorPos.X = SECTORMAXX;
	new_player->OldSectorPos.Y = SECTORMAXY;
	new_player->IsMove = false;
	new_player->IsDelete = false;


	//필요해보이진 않으나 일단 컨텐츠에서는 sessionID를 DWORD로 사용하기로 되어있으니 이렇게 구분. 

	Sector[new_player->CharacterSectorPos.Y][new_player->CharacterSectorPos.X].push_back(new_player);
	CharacterMap.insert(std::unordered_map<__int64, CHARACTER*>::value_type(new_player->SessionID, new_player));


	CPacket* packet_create_my_character = CPacket::Alloc();
	MakePacketCreateMyCharacter(new_player, packet_create_my_character, new_player->SessionIDForContents, new_player->Direction, new_player->X, new_player->Y, new_player->HP);
	CPacket::Free(packet_create_my_character);

	CPacket* packet_create_other_character = CPacket::Alloc();
	MakePacketCreateOtherCharacter(new_player, packet_create_other_character, new_player->SessionIDForContents, new_player->Direction, new_player->X, new_player->Y, new_player->HP);
	CPacket::Free(packet_create_other_character);


	//나를 남에게.
	SectorAround CreateForMe;
	GetSectorAround(new_player->CharacterSectorPos.X, new_player->CharacterSectorPos.Y, &CreateForMe);

	for (unsigned int i = 0; i < CreateForMe.Count; i++)
	{
		std::list<CHARACTER*>::iterator Iter;
		for (Iter = Sector[CreateForMe.Around[i].Y][CreateForMe.Around[i].X].begin(); Iter != Sector[CreateForMe.Around[i].Y][CreateForMe.Around[i].X].end(); ++Iter)
		{
			CHARACTER* Target = *Iter;
			if ((Target->SessionIDForContents == new_player->SessionIDForContents) || (Target->IsDelete == 1))
			{
				continue;
			}

			CPacket* OtherCharacter = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(new_player, OtherCharacter, Target->SessionIDForContents, Target->Direction, Target->X, Target->Y, Target->HP);
			CPacket::Free(OtherCharacter);


			if (Target->IsMove == true)
			{
				CPacket* MoveStartForMePacket = CPacket::Alloc();
				MakePacketMoveStartForMe(new_player, MoveStartForMePacket, Target->SessionIDForContents, Target->Action, Target->X, Target->Y);
				CPacket::Free(MoveStartForMePacket);
			}

		}
	}
}



int MMOTCPServer_Single::ServerControl()
{
	static bool ControlMode = false;


	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			ControlMode = true;

		}

		if (ControlMode && L'q' == ControlKey || L'q' == ControlKey)
		{
			Stop();
			return -1;

			//원하는 기능 처리.

		}

	}

	return 0;
}




void MMOTCPServer_Single::MakePacketMoveStart(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)11 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketMoveStartForMe(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)11 << ID << Direction << X << Y;
	SendPacketUnicast(Target, Packet);
}


void MMOTCPServer_Single::MakePacketMoveStop(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)13 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketCreateMyCharacter(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{

	*Packet << (unsigned char)0 << ID << Direction << X << Y << HP;


	SendPacketUnicast(Target, Packet);
}


void MMOTCPServer_Single::MakePacketCreateOtherCharacterForMe(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{
	*Packet << (unsigned char)1 << ID << Direction << X << Y << HP;
	SendPacketUnicast(Target, Packet);
}


void MMOTCPServer_Single::MakePacketCreateOtherCharacter(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{
	*Packet << (unsigned char)1 << ID << Direction << X << Y << HP;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketDeleteCharacter(CHARACTER* Target, CPacket* Packet, unsigned int ID)
{
	*Packet << (unsigned char)2 << ID;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketDamage(CHARACTER* Target, CPacket* Packet, unsigned int AttackID, unsigned int DamageID, unsigned char DamageHP)
{
	*Packet << (unsigned char)30 << AttackID << DamageID << DamageHP;
	SendPacketAround(Target, Packet, true);
}


void MMOTCPServer_Single::MakePacketAttack1(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)21 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketAttack2(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)23 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketAttack3(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)25 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}

void MMOTCPServer_Single::MakePacketEcho(CHARACTER* Target, CPacket* Packet, unsigned int Time)
{
	*Packet << (unsigned char)253 << Time;
	SendPacketUnicast(Target, Packet);
}

void MMOTCPServer_Single::MakePacketDeleteCharacterRemoveSector(CHARACTER* Target, CPacket* Packet, SectorAround* Around, unsigned int ID)
{
	*Packet << (unsigned char)2 << ID;
	SendPacketAroundRemoveSector(Packet, Around);
}

void MMOTCPServer_Single::MakePacketCreateCharacterAddSector(CHARACTER* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{
	*Packet << (unsigned char)1 << ID << Direction << X << Y << HP;
	SendPacketAroundAddSector(Packet, Around);
}

void MMOTCPServer_Single::MakePacketMoveStartAddSector(CHARACTER* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)11 << ID << Direction << X << Y;
	SendPacketAroundAddSector(Packet, Around);
}

void MMOTCPServer_Single::MakePacketDeleteCharacterForMe(CHARACTER* Target, CPacket* Packet, unsigned int ID)
{
	*Packet << (unsigned char)2 << ID;
	SendPacketUnicast(Target, Packet);

}

void MMOTCPServer_Single::MakePacketSync(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)251 << ID << X << Y;
	SendPacketAround(Target, Packet, true);
}


void MMOTCPServer_Single::SendPacketUnicast(CHARACTER* Target, CPacket* Packet)
{
	SendPacket(Target->SessionID, Packet);
}

void MMOTCPServer_Single::SendPacketAround(CHARACTER* Target, CPacket* Packet, bool SendMe)
{

	SectorAround Around;
	GetSectorAround(Target->CharacterSectorPos.X, Target->CharacterSectorPos.Y, &Around);


	if (SendMe)
	{
		for (unsigned int Index = 0; Index < Around.Count; Index++)
		{
			SendPacketSectorOne(Around.Around[Index].X, Around.Around[Index].Y, NULL, Packet);
		}
	}
	else
	{
		for (unsigned int Index = 0; Index < Around.Count; Index++)
		{
			SendPacketSectorOne(Around.Around[Index].X, Around.Around[Index].Y, Target->SessionIDForContents, Packet);
		}
	}

}

//TODO : 이 둘의 코드는 같은데 왜 분리를 해놨는가? 
// 
//타겟은 왜 있는거지? 

void MMOTCPServer_Single::SendPacketAroundRemoveSector(CPacket* Packet, SectorAround* Around)
{
	for (unsigned int Index = 0; Index < Around->Count; Index++)
	{
		SendPacketSectorOne(Around->Around[Index].X, Around->Around[Index].Y, NULL, Packet);
	}
}

void MMOTCPServer_Single::SendPacketAroundAddSector(CPacket* Packet, SectorAround* Around)
{
	for (unsigned int Index = 0; Index < Around->Count; Index++)
	{
		SendPacketSectorOne(Around->Around[Index].X, Around->Around[Index].Y, NULL, Packet);
	}
}

void MMOTCPServer_Single::SendPacketSectorOne(int SectorX, int SectorY, unsigned int ExceptSessionID, CPacket* Packet)
{
	CHARACTER* Target;
	std::list<CHARACTER*>::iterator Iter;
	for (Iter = Sector[SectorY][SectorX].begin(); Iter != Sector[SectorY][SectorX].end(); ++Iter)
	{
		Target = *Iter;
		if ((Target->SessionIDForContents == ExceptSessionID) || (Target->IsDelete == 1))
		{
			continue;
		}
		SendPacket(Target->SessionID, Packet);
	}
}





bool MMOTCPServer_Single::NetPacketProc_MoveStart(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);
		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->X, Target->Y);
		CPacket::Free(SyncPacket);


		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
	}
	else
	{

		Target->X = X;
		Target->Y = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target);
		}
	}



	Target->IsMove = true;
	Target->Action = Direction;

	switch (Direction)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		Target->Direction = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		Target->Direction = dfPACKET_MOVE_DIR_LL;
		break;

	}

	CPacket* MoveStartPacket = CPacket::Alloc();
	MakePacketMoveStart(Target, MoveStartPacket, Target->SessionIDForContents, Direction, Target->X, Target->Y);
	CPacket::Free(MoveStartPacket);


	return true;
}

bool MMOTCPServer_Single::NetPacketProc_MoveStop(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{
	//여기서 처리. 클라의 좌표를 인정해준다.64

	//여기서 한번 돌려줘야함. run을.. 

	if ((abs(Target->X - X) > dfERROR_RANGE) || (abs(Target->Y - Y) > dfERROR_RANGE))
	{
		//Disconnect(Target->CharacterSession);
		//wprintf(L"MoveStop OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->X, Target->Y);
		CPacket::Free(SyncPacket);


		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);

	}
	else
	{


		Target->X = X;
		Target->Y = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); 
		}


	}

	//wprintf(L"## StopPacket : X  %d   Y  %d  \n", X, Y);

	Target->IsMove = false;


	Target->Action = Direction;

	switch (Direction)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		Target->Direction = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		Target->Direction = dfPACKET_MOVE_DIR_LL;
		break;

	}


	CPacket* MoveStopPacket = CPacket::Alloc();
	MakePacketMoveStop(Target, MoveStopPacket, Target->SessionIDForContents, Direction, Target->X, Target->Y);
	CPacket::Free(MoveStopPacket);


	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Attack1(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{
	unsigned int ID;

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->X, Target->Y);
		CPacket::Free(SyncPacket);
		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);


	}
	else
	{

		Target->X = X;
		Target->Y = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}



	ID = Target->SessionIDForContents;
	Target->Direction = Direction;

	CPacket* Attack1Packet = CPacket::Alloc();
	MakePacketAttack1(Target, Attack1Packet, ID, Direction, Target->X, Target->Y);
	CPacket::Free(Attack1Packet);


	HitCheck(Target, 1);

	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Attack2(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{
	unsigned int ID;

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->X, Target->Y);
		CPacket::Free(SyncPacket);

		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);

	}
	else
	{

		Target->X = X;
		Target->Y = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}



	ID = Target->SessionIDForContents;
	Target->Direction = Direction;

	CPacket* Attack2Packet = CPacket::Alloc();
	MakePacketAttack2(Target, Attack2Packet, ID, Direction, Target->X, Target->Y);
	CPacket::Free(Attack2Packet);

	HitCheck(Target, 2);

	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Attack3(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{

		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->X, Target->Y);
		CPacket::Free(SyncPacket);


		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
	}
	else
	{

		Target->X = X;
		Target->Y = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}


	unsigned int ID;
	ID = Target->SessionIDForContents;
	Target->Direction = Direction;

	CPacket* Attack3Packet = CPacket::Alloc();
	MakePacketAttack3(Target, Attack3Packet, ID, Direction, Target->X, Target->Y);
	CPacket::Free(Attack3Packet);

	HitCheck(Target, 3);
	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Echo(CHARACTER* Target, unsigned int Time)
{

	CPacket* EchoPacket = CPacket::Alloc();
	MakePacketEcho(Target, EchoPacket, Time);
	CPacket::Free(EchoPacket);

	return false;


}

bool MMOTCPServer_Single::SectorUpdateCharacter(CHARACTER* Target)
{
	int TargetCurPosX = (Target->X / SECTORXSIZE);
	int TargetCurPosY = (Target->Y / SECTORYSIZE);

	if ((Target->CharacterSectorPos.X != TargetCurPosX) || (Target->CharacterSectorPos.Y != TargetCurPosY))
	{

		Target->OldSectorPos.X = Target->CharacterSectorPos.X;
		Target->OldSectorPos.Y = Target->CharacterSectorPos.Y;
		Target->CharacterSectorPos.X = TargetCurPosX;
		Target->CharacterSectorPos.Y = TargetCurPosY;

		Sector[Target->OldSectorPos.Y][Target->OldSectorPos.X].remove(Target);
		Sector[Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X].push_back(Target);
		//printf("섹터 변경 \nOld : %d  %d Cur  : %d  %d \n" , Target->OldSectorPos.X, Target->OldSectorPos.Y,TargetCurPosX, TargetCurPosY);



		return true;
	}

	return false;

}

void MMOTCPServer_Single::SectorUpdate(CHARACTER* Target)
{

	SectorAround Remove;
	SectorAround Add;

	GetUpdateSectorAround(Target, &Remove, &Add);

	//PrintUpdateSector(&Remove, &Add);

	CPacket* DeleteCharacterRemoveSectorPacket = CPacket::Alloc();
	MakePacketDeleteCharacterRemoveSector(Target, DeleteCharacterRemoveSectorPacket, &Remove, Target->SessionIDForContents); //패킷 만들어서 Remove 시켜줘야하는데.. 
	CPacket::Free(DeleteCharacterRemoveSectorPacket);



	//Remove에 있는 애들의 삭제를 나에게 보냄. 
	for (unsigned int i = 0; i < Remove.Count; i++)
	{
		std::list<CHARACTER*>::iterator Iter;
		for (Iter = Sector[Remove.Around[i].Y][Remove.Around[i].X].begin(); Iter != Sector[Remove.Around[i].Y][Remove.Around[i].X].end(); ++Iter)
		{
			CPacket* DeleteCharacterForMePacket = CPacket::Alloc();
			MakePacketDeleteCharacterForMe(Target, DeleteCharacterForMePacket, (*Iter)->SessionIDForContents);
			CPacket::Free(DeleteCharacterForMePacket);



			//printf("Remove에 있는 애들의 삭제를 나에게 보냄. 지움 당하는 아이디 %d  : 받는 아이디 %d \n\n", Target->CharacterSession->SessionID, (*Iter)->SessionID);

		}
	}


	//add에 있는 애들에게 나의 생성을 보냄. 
	CPacket* CreateCharacterAddSectorPacket = CPacket::Alloc();
	MakePacketCreateCharacterAddSector(Target, CreateCharacterAddSectorPacket, &Add, Target->SessionIDForContents, Target->Direction, Target->X, Target->Y, Target->HP);
	CPacket::Free(CreateCharacterAddSectorPacket);


	//이동 정보도 보내줘야함. 
		// 
		// 
	CPacket* MoveStartAddSectoroPacket = CPacket::Alloc();
	MakePacketMoveStartAddSector(Target, MoveStartAddSectoroPacket, &Add, Target->SessionIDForContents, Target->Action, Target->X, Target->Y);
	CPacket::Free(MoveStartAddSectoroPacket);



	for (unsigned int i = 0; i < Add.Count; i++)
	{
		std::list<CHARACTER*>::iterator IterCreate;
		for (IterCreate = Sector[Add.Around[i].Y][Add.Around[i].X].begin();
			IterCreate != Sector[Add.Around[i].Y][Add.Around[i].X].end(); ++IterCreate)
		{
			CHARACTER* CreateCharacter = *IterCreate;
			if ((CreateCharacter->SessionID == Target->SessionID) || (CreateCharacter->IsDelete == 1))
			{
				continue;
			}

			CPacket* CreateOtherCharacterForMePacket = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(Target, CreateOtherCharacterForMePacket, CreateCharacter->SessionIDForContents, CreateCharacter->Direction, CreateCharacter->X, CreateCharacter->Y, CreateCharacter->HP);
			CPacket::Free(CreateOtherCharacterForMePacket);

			if (CreateCharacter->IsMove == true)
			{

				CPacket* MoveStartForMePacket = CPacket::Alloc();
				MakePacketMoveStartForMe(Target, MoveStartForMePacket, CreateCharacter->SessionIDForContents, CreateCharacter->Action, CreateCharacter->X, CreateCharacter->Y);
				CPacket::Free(MoveStartForMePacket);
			}


		}
	}


}

void MMOTCPServer_Single::HitCheck(CHARACTER* AttackCharacter, int AttackNumber)
{
	int BoundaryX = 0;
	int BoundaryY = 0;

	int Damage = 0;



	switch (AttackNumber)
	{

	case 1:
		BoundaryX = dfATTACK1_RANGE_X;
		BoundaryY = dfATTACK1_RANGE_Y;
		Damage = dfATTACK1_DAMAGE;
		break;

	case 2:
		BoundaryX = dfATTACK2_RANGE_X;
		BoundaryY = dfATTACK2_RANGE_Y;
		Damage = dfATTACK2_DAMAGE;
		break;

	case 3:
		BoundaryX = dfATTACK3_RANGE_X;
		BoundaryY = dfATTACK3_RANGE_Y;
		Damage = dfATTACK3_DAMAGE;
		break;

	default:
		DebugBreak();
	}

	//섹터기준 처리로 바꾸기. 


	SectorAround HitCheckSector;

	if (AttackCharacter->Direction == dfPACKET_MOVE_DIR_LL)
	{
		GetSectorAroundForHitLeft(AttackCharacter, BoundaryX, BoundaryY, &HitCheckSector);

		//PrintHitCheckSector(&HitCheckSector);

		for (unsigned int i = 0; i < HitCheckSector.Count; ++i)
		{
			std::list<CHARACTER*>::iterator Iter;
			for (Iter = Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].begin(); Iter != Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].end(); ++Iter)
			{
				CHARACTER* Target = *Iter;

				if ((AttackCharacter == Target) || (Target->IsDelete == 1) || (AttackCharacter->X < Target->X))
				{
					continue;
				}


				if (abs(AttackCharacter->Y - Target->Y) <= BoundaryY && abs(AttackCharacter->X - Target->X) <= BoundaryX)
				{

					if (Damage >= Target->HP)
					{
						Target->HP = 0;
					}
					else
					{
						Target->HP -= Damage;
					}

					CPacket* DamagePacket = CPacket::Alloc();
					MakePacketDamage(Target, DamagePacket, AttackCharacter->SessionIDForContents, Target->SessionIDForContents, Target->HP);
					CPacket::Free(DamagePacket);



					if (Target->HP == 0)
					{
						Disconnect(Target->SessionID);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, HP));

					}

					return;
				}

			}
		}


	}
	else
	{
		GetSectorAroundForHitRight(AttackCharacter, BoundaryX, BoundaryY, &HitCheckSector);
		//PrintHitCheckSector(&HitCheckSector);
		for (unsigned int i = 0; i < HitCheckSector.Count; ++i)
		{
			std::list<CHARACTER*>::iterator Iter;
			for (Iter = Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].begin(); Iter != Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].end(); ++Iter)
			{
				CHARACTER* Target = *Iter;

				if ((AttackCharacter == Target) || (Target->IsDelete == 1) || (AttackCharacter->X > Target->X))
				{
					continue;
				}


				if (abs(AttackCharacter->Y - Target->Y) <= BoundaryY && abs(AttackCharacter->X - Target->X) <= BoundaryX)
				{

					if (Damage >= Target->HP)
					{
						Target->HP = 0;
					}
					else
					{
						Target->HP -= Damage;
					}

					CPacket* DamagePacket = CPacket::Alloc();
					MakePacketDamage(Target, DamagePacket, AttackCharacter->SessionIDForContents, Target->SessionIDForContents, Target->HP);
					CPacket::Free(DamagePacket);

					if (Target->HP == 0)
					{
						//printf(" HP가 0 이라 종료 당하는 녀석 : %d \n\n", Target->SessionID);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, TIMEOUT));

						Disconnect(Target->SessionID);
					}

					return;
				}

			}
		}


	}


	//std::unordered_map<unsigned int, CHARACTER*>::iterator Iter;
	//for (Iter = CharacterMap.begin(); Iter != CharacterMap.end(); ++Iter)
	//{
	//	CHARACTER* Target = Iter->second;

	//	if ((AttackCharacter == Target) || Target->CharacterSession->IsDelete == 1)
	//	{
	//		continue;
	//	}
	//	
	//}




}


void MMOTCPServer_Single::GetSectorAroundForHitLeft(CHARACTER* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->CharacterSectorPos.X;
	int SectorY = Target->CharacterSectorPos.Y;



	AroundSector->Count = 0;

	AroundSector->Around[AroundSector->Count].X = SectorX;
	AroundSector->Around[AroundSector->Count].Y = SectorY;
	AroundSector->Count++;

	int TargetValidPosX = ((Target->X - BoundaryX) / SECTORXSIZE);
	int TargetValidPosYAbove = ((Target->Y - BoundaryY) / SECTORYSIZE);
	int TargetValidPosYBelow = ((Target->Y + BoundaryY) / SECTORYSIZE);



	if (SectorX - 1 >= 0 && TargetValidPosX != SectorX)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY;
		AroundSector->Count++;
	}

	if (SectorY + 1 < SECTORMAXY && TargetValidPosYBelow != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0 && TargetValidPosYAbove != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorY + 1 < SECTORMAXY && SectorX - 1 >= 0 && TargetValidPosX != SectorX && TargetValidPosYBelow != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}
	if (SectorY - 1 >= 0 && SectorX - 1 >= 0 && TargetValidPosX != SectorX && TargetValidPosYAbove != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX - 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

}

void MMOTCPServer_Single::GetSectorAroundForHitRight(CHARACTER* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->CharacterSectorPos.X;
	int SectorY = Target->CharacterSectorPos.Y;

	AroundSector->Count = 0;

	AroundSector->Around[AroundSector->Count].X = SectorX;
	AroundSector->Around[AroundSector->Count].Y = SectorY;
	AroundSector->Count++;

	int TargetValidPosX = ((Target->X + BoundaryX) / SECTORXSIZE);
	int TargetValidPosYAbove = ((Target->Y - BoundaryY) / SECTORYSIZE);
	int TargetValidPosYBelow = ((Target->Y + BoundaryY) / SECTORYSIZE);

	if (SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY;
		AroundSector->Count++;
	}

	if (SectorY + 1 < SECTORMAXY && TargetValidPosYBelow != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0 && TargetValidPosYAbove != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorY + 1 < SECTORMAXY && SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX && TargetValidPosYBelow != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX && TargetValidPosYAbove != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

}

SectorAround CacheAround[SECTORMAXY][SECTORMAXX];


void MMOTCPServer_Single::GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector)
{

	if (CacheAround[SectorY][SectorX].Flag)
	{
		*AroundSector = CacheAround[SectorY][SectorX];
		return;
	}

	AroundSector->Count = 0;
	AroundSector->Around[AroundSector->Count].X = SectorX;
	AroundSector->Around[AroundSector->Count].Y = SectorY;
	AroundSector->Count++;



	if (SectorX + 1 < SECTORMAXX)
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


	if (SectorY + 1 < SECTORMAXY)
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

	if (SectorY + 1 < SECTORMAXY && SectorX + 1 < SECTORMAXX)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY + 1;
		AroundSector->Count++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < SECTORMAXX)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

	if (SectorY + 1 < SECTORMAXY && SectorX - 1 >= 0)
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

	CacheAround[SectorY][SectorX] = *AroundSector;
	CacheAround[SectorY][SectorX].Flag = 1;

}


////메모리를  너무 많이 사용하는 것 같은데 이걸 최적화라고 할 수 있는가? 

SectorAround CacheUpdateAround[SECTORMAXY][SECTORMAXX][SECTORMAXY][SECTORMAXX][2];

void MMOTCPServer_Single::GetUpdateSectorAround(CHARACTER* Target, SectorAround* RemoveSector, SectorAround* AddSector)
{
	//로직은 좀 더 생각해봐야 함

	if (CacheUpdateAround[Target->OldSectorPos.Y][Target->OldSectorPos.X][Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X][0].Flag)
	{
		*RemoveSector = CacheUpdateAround[Target->OldSectorPos.Y][Target->OldSectorPos.X][Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X][0];
		*AddSector = CacheUpdateAround[Target->OldSectorPos.Y][Target->OldSectorPos.X][Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X][1];
		return;

	}


	RemoveSector->Count = 0;
	AddSector->Count = 0;


	SectorAround Old;
	SectorAround Cur;

	GetSectorAround(Target->OldSectorPos.X, Target->OldSectorPos.Y, &Old);
	GetSectorAround(Target->CharacterSectorPos.X, Target->CharacterSectorPos.Y, &Cur);


	unsigned int RemoveIndex;
	for (unsigned int i = 0; i < Old.Count; i++)
	{
		for (RemoveIndex = 0; RemoveIndex < Cur.Count; RemoveIndex++)
		{
			if (Old.Around[i].X == Cur.Around[RemoveIndex].X && Old.Around[i].Y == Cur.Around[RemoveIndex].Y)
			{
				break;
			}
		}
		if (RemoveIndex == Cur.Count)
		{
			RemoveSector->Around[RemoveSector->Count].X = Old.Around[i].X;
			RemoveSector->Around[RemoveSector->Count].Y = Old.Around[i].Y;
			RemoveSector->Count++;
		}
	}

	AddSector->Count = 0;
	unsigned int j;
	for (unsigned int i = 0; i < Cur.Count; i++)
	{
		for (j = 0; j < Old.Count; j++)
		{
			if (Old.Around[j].X == Cur.Around[i].X && Old.Around[j].Y == Cur.Around[i].Y)
			{
				break;
			}
		}
		if (j == Old.Count)
		{
			AddSector->Around[AddSector->Count].X = Cur.Around[i].X;
			AddSector->Around[AddSector->Count].Y = Cur.Around[i].Y;
			AddSector->Count++;
		}
	}

	CacheUpdateAround[Target->OldSectorPos.Y][Target->OldSectorPos.X][Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X][0] = *RemoveSector;
	CacheUpdateAround[Target->OldSectorPos.Y][Target->OldSectorPos.X][Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X][1] = *AddSector;
	CacheUpdateAround[Target->OldSectorPos.Y][Target->OldSectorPos.X][Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X][0].Flag = 1;
	CacheUpdateAround[Target->OldSectorPos.Y][Target->OldSectorPos.X][Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X][1].Flag = 1;

}


#define FPS 40

void MMOTCPServer_Single::Update()
{

	unsigned int Tick = timeGetTime();
	unsigned int Frame = Tick - OldTick;

	GlobalLoop++;

	//얘가 그냥 프레임. 

	int FixUpdate = (Frame / FPS);
	std::unordered_map<__int64, CHARACTER*>::iterator Iter;
	for (Iter = CharacterMap.begin(); Iter != CharacterMap.end(); ++Iter)
	{
		CHARACTER* Target = Iter->second;

		if (Target->IsMove && (Target->IsDelete == 0))
		{
			for (int i = 0; i < FixUpdate; ++i)
			{
				GameRun(Target);
			}

		}

	}
	OldTick += (FPS * (Frame / FPS));

	if (Tick - OldTickforCheck >= 1000)
	{
		//wprintf(L"Count : %d   Loop : %d \n", Count, GlobalLoop);
		if (FixUpdate > 1)
		{

			wprintf(L"FixedUpdate : %d   Loop : %d \n", FixUpdate, GlobalLoop);
		}
		OldTickforCheck += 1000;
		GlobalLoop = 0;
	}

	Frame = timeGetTime() - OldTick;
	if (Frame < FPS)
	{
		Sleep(FPS - Frame);
	}


}




void MMOTCPServer_Single::GameRun(CHARACTER* Target)
{
	switch (Target->Action)
	{
	case dfPACKET_MOVE_DIR_LL:
		if (Target->X - 6 < dfRANGE_MOVE_LEFT)
		{
			Target->IsMove = false;
			return;
		}

		Target->X -= 6;

		break;

	case dfPACKET_MOVE_DIR_LU:
		if (Target->Y - 4 < dfRANGE_MOVE_TOP || Target->X - 6 < dfRANGE_MOVE_LEFT)
		{
			Target->IsMove = false;
			return;
		}

		Target->X -= 6;
		Target->Y -= 4;

		break;



	case dfPACKET_MOVE_DIR_UU:
		if (Target->Y - 4 < dfRANGE_MOVE_TOP)
		{
			Target->IsMove = false;
			return;
		}


		Target->Y -= 4;

		break;


	case dfPACKET_MOVE_DIR_RU:

		if ((Target->Y - 4 < dfRANGE_MOVE_TOP) || (Target->X + 6 >= dfRANGE_MOVE_RIGHT))
		{
			Target->IsMove = false;
			return;
		}

		Target->X += 6;
		Target->Y -= 4;

		break;


	case dfPACKET_MOVE_DIR_RR:

		if (Target->X + 6 >= dfRANGE_MOVE_RIGHT)
		{
			Target->IsMove = false;
			return;
		}
		Target->X += 6;

		break;


	case dfPACKET_MOVE_DIR_RD:

		if (Target->Y + 4 >= dfRANGE_MOVE_BOTTOM || Target->X + 6 >= dfRANGE_MOVE_RIGHT)
		{
			Target->IsMove = false;
			return;
		}


		Target->X += 6;
		Target->Y += 4;

		break;



	case dfPACKET_MOVE_DIR_DD:
		if (Target->Y + 4 >= dfRANGE_MOVE_BOTTOM)
		{
			Target->IsMove = false;
			return;
		}

		Target->Y += 4;

		break;

	case dfPACKET_MOVE_DIR_LD:
		if (Target->Y + 4 >= dfRANGE_MOVE_BOTTOM || Target->X - 6 < dfRANGE_MOVE_LEFT)
		{
			Target->IsMove = false;
			return;
		}

		Target->X -= 6;
		Target->Y += 4;

		break;
	}


	if (SectorUpdateCharacter(Target))
	{

		SectorUpdate(Target);

	}
}

//지연삭제코드. 여기서 삭제하면 된다. 전체를 순회하며 하거나 DeleteList를 만들어도 된다. 

void MMOTCPServer_Single::DeleteDisconnect()
{
	/*if (DeleteList.size() > 0)
	{

		SESSION* Session;
		CHARACTER* DeleteTarget;
		unsigned int Session_ID;
		std::list<unsigned int>::iterator Iter;
		for (Iter = DeleteList.begin(); Iter != DeleteList.end(); ++Iter)
		{
			Session_ID = *Iter;
			Session = Sessions.at(Session_ID);
			DeleteTarget = CharacterMap.at(Session_ID);

			FreeCharacter(DeleteTarget);

			CharacterMap.erase(Session_ID);

			closesocket(Session->Socket);
			Session->ReceiveQ.ClearBuffer();
			Session->SendQ.ClearBuffer();

			SessionFreeList.Free(Session);
			Sessions.erase(Session_ID);

		}

		DeleteList.clear();
	}*/

}

void MMOTCPServer_Single::DisconnectContents(CHARACTER* Target)
{
	//if (TargetSession->IsDelete == 1)
	//{
	//	return;
	//}

	//DeleteList.push_back(TargetSession->SessionID);
	//TargetSession->IsDelete = 1;

	//CHARACTER* Target = CharacterMap.at(TargetSession->SessionID);

	//Sector[Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X].remove(Target);

	//GlobalCPacket.Clear();
	//MakePacketDeleteCharacter(TargetSession, &GlobalCPacket, TargetSession->SessionID);
	////wprintf(L"## Disconnect ID : %d \n", TargetSession->SessionID);

}

