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
	Character* Target = CharacterMap.at(NewSession);

	CharacterMap.erase(NewSession);
	Sector[Target->characterSectorPos_.y_][Target->characterSectorPos_.x_].remove(Target);


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

	Character* Target;
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
	Character* new_player = CharacterFreeList.Alloc();
	new_player->sessionId_ = new_session;
	new_player->SessionIDForContents = (unsigned int)new_session;
	new_player->direction_ = dfPACKET_MOVE_DIR_RR;
	new_player->action_ = dfPACKET_MOVE_DIR_RR;
	new_player->x_ = rand() % 6399;
	new_player->y_ = rand() % 6399;

	//new_player->X = 100;
	//new_player->Y = 100;

	new_player->hp_ = 100;
	new_player->characterSectorPos_.x_ = new_player->x_ / SECTORXSIZE;
	new_player->characterSectorPos_.y_ = new_player->y_ / SECTORYSIZE;
	new_player->oldSectorPos_.x_ = SECTORMAXX;
	new_player->oldSectorPos_.y_ = SECTORMAXY;
	new_player->isMove_ = false;
	new_player->IsDelete = false;


	//필요해보이진 않으나 일단 컨텐츠에서는 sessionID를 DWORD로 사용하기로 되어있으니 이렇게 구분. 

	Sector[new_player->characterSectorPos_.y_][new_player->characterSectorPos_.x_].push_back(new_player);
	CharacterMap.insert(std::unordered_map<__int64, Character*>::value_type(new_player->sessionId_, new_player));


	CPacket* packet_create_my_character = CPacket::Alloc();
	MakePacketCreateMyCharacter(new_player, packet_create_my_character, new_player->SessionIDForContents, new_player->direction_, new_player->x_, new_player->y_, new_player->hp_);
	CPacket::Free(packet_create_my_character);

	CPacket* packet_create_other_character = CPacket::Alloc();
	MakePacketCreateOtherCharacter(new_player, packet_create_other_character, new_player->SessionIDForContents, new_player->direction_, new_player->x_, new_player->y_, new_player->hp_);
	CPacket::Free(packet_create_other_character);


	//나를 남에게.
	SectorAround CreateForMe;
	GetSectorAround(new_player->characterSectorPos_.x_, new_player->characterSectorPos_.y_, &CreateForMe);

	for (unsigned int i = 0; i < CreateForMe.count_; i++)
	{
		std::list<Character*>::iterator Iter;
		for (Iter = Sector[CreateForMe.around_[i].y_][CreateForMe.around_[i].x_].begin(); Iter != Sector[CreateForMe.around_[i].y_][CreateForMe.around_[i].x_].end(); ++Iter)
		{
			Character* Target = *Iter;
			if ((Target->SessionIDForContents == new_player->SessionIDForContents) || (Target->IsDelete == 1))
			{
				continue;
			}

			CPacket* OtherCharacter = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(new_player, OtherCharacter, Target->SessionIDForContents, Target->direction_, Target->x_, Target->y_, Target->hp_);
			CPacket::Free(OtherCharacter);


			if (Target->isMove_ == true)
			{
				CPacket* MoveStartForMePacket = CPacket::Alloc();
				MakePacketMoveStartForMe(new_player, MoveStartForMePacket, Target->SessionIDForContents, Target->action_, Target->x_, Target->y_);
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




void MMOTCPServer_Single::MakePacketMoveStart(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)11 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketMoveStartForMe(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)11 << ID << Direction << X << Y;
	SendPacketUnicast(Target, Packet);
}


void MMOTCPServer_Single::MakePacketMoveStop(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)13 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketCreateMyCharacter(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{

	*Packet << (unsigned char)0 << ID << Direction << X << Y << HP;


	SendPacketUnicast(Target, Packet);
}


void MMOTCPServer_Single::MakePacketCreateOtherCharacterForMe(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{
	*Packet << (unsigned char)1 << ID << Direction << X << Y << HP;
	SendPacketUnicast(Target, Packet);
}


void MMOTCPServer_Single::MakePacketCreateOtherCharacter(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{
	*Packet << (unsigned char)1 << ID << Direction << X << Y << HP;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketDeleteCharacter(Character* Target, CPacket* Packet, unsigned int ID)
{
	*Packet << (unsigned char)2 << ID;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketDamage(Character* Target, CPacket* Packet, unsigned int AttackID, unsigned int DamageID, unsigned char DamageHP)
{
	*Packet << (unsigned char)30 << AttackID << DamageID << DamageHP;
	SendPacketAround(Target, Packet, true);
}


void MMOTCPServer_Single::MakePacketAttack1(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)21 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketAttack2(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)23 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}


void MMOTCPServer_Single::MakePacketAttack3(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)25 << ID << Direction << X << Y;
	SendPacketAround(Target, Packet);
}

void MMOTCPServer_Single::MakePacketEcho(Character* Target, CPacket* Packet, unsigned int Time)
{
	*Packet << (unsigned char)253 << Time;
	SendPacketUnicast(Target, Packet);
}

void MMOTCPServer_Single::MakePacketDeleteCharacterRemoveSector(Character* Target, CPacket* Packet, SectorAround* Around, unsigned int ID)
{
	*Packet << (unsigned char)2 << ID;
	SendPacketAroundRemoveSector(Packet, Around);
}

void MMOTCPServer_Single::MakePacketCreateCharacterAddSector(Character* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP)
{
	*Packet << (unsigned char)1 << ID << Direction << X << Y << HP;
	SendPacketAroundAddSector(Packet, Around);
}

void MMOTCPServer_Single::MakePacketMoveStartAddSector(Character* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)11 << ID << Direction << X << Y;
	SendPacketAroundAddSector(Packet, Around);
}

void MMOTCPServer_Single::MakePacketDeleteCharacterForMe(Character* Target, CPacket* Packet, unsigned int ID)
{
	*Packet << (unsigned char)2 << ID;
	SendPacketUnicast(Target, Packet);

}

void MMOTCPServer_Single::MakePacketSync(Character* Target, CPacket* Packet, unsigned int ID, unsigned short X, unsigned short Y)
{
	*Packet << (unsigned char)251 << ID << X << Y;
	SendPacketAround(Target, Packet, true);
}


void MMOTCPServer_Single::SendPacketUnicast(Character* Target, CPacket* Packet)
{
	SendPacket(Target->sessionId_, Packet);
}

void MMOTCPServer_Single::SendPacketAround(Character* Target, CPacket* Packet, bool SendMe)
{

	SectorAround Around;
	GetSectorAround(Target->characterSectorPos_.x_, Target->characterSectorPos_.y_, &Around);


	if (SendMe)
	{
		for (unsigned int Index = 0; Index < Around.count_; Index++)
		{
			SendPacketSectorOne(Around.around_[Index].x_, Around.around_[Index].y_, NULL, Packet);
		}
	}
	else
	{
		for (unsigned int Index = 0; Index < Around.count_; Index++)
		{
			SendPacketSectorOne(Around.around_[Index].x_, Around.around_[Index].y_, Target->SessionIDForContents, Packet);
		}
	}

}

//TODO : 이 둘의 코드는 같은데 왜 분리를 해놨는가? 
// 
//타겟은 왜 있는거지? 

void MMOTCPServer_Single::SendPacketAroundRemoveSector(CPacket* Packet, SectorAround* Around)
{
	for (unsigned int Index = 0; Index < Around->count_; Index++)
	{
		SendPacketSectorOne(Around->around_[Index].x_, Around->around_[Index].y_, NULL, Packet);
	}
}

void MMOTCPServer_Single::SendPacketAroundAddSector(CPacket* Packet, SectorAround* Around)
{
	for (unsigned int Index = 0; Index < Around->count_; Index++)
	{
		SendPacketSectorOne(Around->around_[Index].x_, Around->around_[Index].y_, NULL, Packet);
	}
}

void MMOTCPServer_Single::SendPacketSectorOne(int SectorX, int SectorY, unsigned int ExceptSessionID, CPacket* Packet)
{
	Character* Target;
	std::list<Character*>::iterator Iter;
	for (Iter = Sector[SectorY][SectorX].begin(); Iter != Sector[SectorY][SectorX].end(); ++Iter)
	{
		Target = *Iter;
		if ((Target->SessionIDForContents == ExceptSessionID) || (Target->IsDelete == 1))
		{
			continue;
		}
		SendPacket(Target->sessionId_, Packet);
	}
}





bool MMOTCPServer_Single::NetPacketProc_MoveStart(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);
		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->x_, Target->y_);
		CPacket::Free(SyncPacket);


		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
	}
	else
	{

		Target->x_ = X;
		Target->y_ = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target);
		}
	}



	Target->isMove_ = true;
	Target->action_ = Direction;

	switch (Direction)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		Target->direction_ = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		Target->direction_ = dfPACKET_MOVE_DIR_LL;
		break;

	}

	CPacket* MoveStartPacket = CPacket::Alloc();
	MakePacketMoveStart(Target, MoveStartPacket, Target->SessionIDForContents, Direction, Target->x_, Target->y_);
	CPacket::Free(MoveStartPacket);


	return true;
}

bool MMOTCPServer_Single::NetPacketProc_MoveStop(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{
	//여기서 처리. 클라의 좌표를 인정해준다.64

	//여기서 한번 돌려줘야함. run을.. 

	if ((abs(Target->x_ - X) > dfERROR_RANGE) || (abs(Target->y_ - Y) > dfERROR_RANGE))
	{
		//Disconnect(Target->CharacterSession);
		//wprintf(L"MoveStop OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->x_, Target->y_);
		CPacket::Free(SyncPacket);


		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);

	}
	else
	{


		Target->x_ = X;
		Target->y_ = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); 
		}


	}

	//wprintf(L"## StopPacket : X  %d   Y  %d  \n", X, Y);

	Target->isMove_ = false;


	Target->action_ = Direction;

	switch (Direction)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		Target->direction_ = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		Target->direction_ = dfPACKET_MOVE_DIR_LL;
		break;

	}


	CPacket* MoveStopPacket = CPacket::Alloc();
	MakePacketMoveStop(Target, MoveStopPacket, Target->SessionIDForContents, Direction, Target->x_, Target->y_);
	CPacket::Free(MoveStopPacket);


	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Attack1(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{
	unsigned int ID;

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->x_, Target->y_);
		CPacket::Free(SyncPacket);
		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);


	}
	else
	{

		Target->x_ = X;
		Target->y_ = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}



	ID = Target->SessionIDForContents;
	Target->direction_ = Direction;

	CPacket* Attack1Packet = CPacket::Alloc();
	MakePacketAttack1(Target, Attack1Packet, ID, Direction, Target->x_, Target->y_);
	CPacket::Free(Attack1Packet);


	HitCheck(Target, 1);

	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Attack2(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{
	unsigned int ID;

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->x_, Target->y_);
		CPacket::Free(SyncPacket);

		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);

	}
	else
	{

		Target->x_ = X;
		Target->y_ = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}



	ID = Target->SessionIDForContents;
	Target->direction_ = Direction;

	CPacket* Attack2Packet = CPacket::Alloc();
	MakePacketAttack2(Target, Attack2Packet, ID, Direction, Target->x_, Target->y_);
	CPacket::Free(Attack2Packet);

	HitCheck(Target, 2);

	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Attack3(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y)
{

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
	{

		CPacket* SyncPacket = CPacket::Alloc();
		MakePacketSync(Target, SyncPacket, Target->SessionIDForContents, Target->x_, Target->y_);
		CPacket::Free(SyncPacket);


		//wprintf(L"MoveStart OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
	}
	else
	{

		Target->x_ = X;
		Target->y_ = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}


	unsigned int ID;
	ID = Target->SessionIDForContents;
	Target->direction_ = Direction;

	CPacket* Attack3Packet = CPacket::Alloc();
	MakePacketAttack3(Target, Attack3Packet, ID, Direction, Target->x_, Target->y_);
	CPacket::Free(Attack3Packet);

	HitCheck(Target, 3);
	return true;
}

bool MMOTCPServer_Single::NetPacketProc_Echo(Character* Target, unsigned int Time)
{

	CPacket* EchoPacket = CPacket::Alloc();
	MakePacketEcho(Target, EchoPacket, Time);
	CPacket::Free(EchoPacket);

	return false;


}

bool MMOTCPServer_Single::SectorUpdateCharacter(Character* Target)
{
	int TargetCurPosX = (Target->x_ / SECTORXSIZE);
	int TargetCurPosY = (Target->y_ / SECTORYSIZE);

	if ((Target->characterSectorPos_.x_ != TargetCurPosX) || (Target->characterSectorPos_.y_ != TargetCurPosY))
	{

		Target->oldSectorPos_.x_ = Target->characterSectorPos_.x_;
		Target->oldSectorPos_.y_ = Target->characterSectorPos_.y_;
		Target->characterSectorPos_.x_ = TargetCurPosX;
		Target->characterSectorPos_.y_ = TargetCurPosY;

		Sector[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_].remove(Target);
		Sector[Target->characterSectorPos_.y_][Target->characterSectorPos_.x_].push_back(Target);
		//printf("섹터 변경 \nOld : %d  %d Cur  : %d  %d \n" , Target->OldSectorPos.X, Target->OldSectorPos.Y,TargetCurPosX, TargetCurPosY);



		return true;
	}

	return false;

}

void MMOTCPServer_Single::SectorUpdate(Character* Target)
{

	SectorAround Remove;
	SectorAround Add;

	GetUpdateSectorAround(Target, &Remove, &Add);

	//PrintUpdateSector(&Remove, &Add);

	CPacket* DeleteCharacterRemoveSectorPacket = CPacket::Alloc();
	MakePacketDeleteCharacterRemoveSector(Target, DeleteCharacterRemoveSectorPacket, &Remove, Target->SessionIDForContents); //패킷 만들어서 Remove 시켜줘야하는데.. 
	CPacket::Free(DeleteCharacterRemoveSectorPacket);



	//Remove에 있는 애들의 삭제를 나에게 보냄. 
	for (unsigned int i = 0; i < Remove.count_; i++)
	{
		std::list<Character*>::iterator Iter;
		for (Iter = Sector[Remove.around_[i].y_][Remove.around_[i].x_].begin(); Iter != Sector[Remove.around_[i].y_][Remove.around_[i].x_].end(); ++Iter)
		{
			CPacket* DeleteCharacterForMePacket = CPacket::Alloc();
			MakePacketDeleteCharacterForMe(Target, DeleteCharacterForMePacket, (*Iter)->SessionIDForContents);
			CPacket::Free(DeleteCharacterForMePacket);



			//printf("Remove에 있는 애들의 삭제를 나에게 보냄. 지움 당하는 아이디 %d  : 받는 아이디 %d \n\n", Target->CharacterSession->SessionID, (*Iter)->SessionID);

		}
	}


	//add에 있는 애들에게 나의 생성을 보냄. 
	CPacket* CreateCharacterAddSectorPacket = CPacket::Alloc();
	MakePacketCreateCharacterAddSector(Target, CreateCharacterAddSectorPacket, &Add, Target->SessionIDForContents, Target->direction_, Target->x_, Target->y_, Target->hp_);
	CPacket::Free(CreateCharacterAddSectorPacket);


	//이동 정보도 보내줘야함. 
		// 
		// 
	CPacket* MoveStartAddSectoroPacket = CPacket::Alloc();
	MakePacketMoveStartAddSector(Target, MoveStartAddSectoroPacket, &Add, Target->SessionIDForContents, Target->action_, Target->x_, Target->y_);
	CPacket::Free(MoveStartAddSectoroPacket);



	for (unsigned int i = 0; i < Add.count_; i++)
	{
		std::list<Character*>::iterator IterCreate;
		for (IterCreate = Sector[Add.around_[i].y_][Add.around_[i].x_].begin();
			IterCreate != Sector[Add.around_[i].y_][Add.around_[i].x_].end(); ++IterCreate)
		{
			Character* CreateCharacter = *IterCreate;
			if ((CreateCharacter->sessionId_ == Target->sessionId_) || (CreateCharacter->IsDelete == 1))
			{
				continue;
			}

			CPacket* CreateOtherCharacterForMePacket = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(Target, CreateOtherCharacterForMePacket, CreateCharacter->SessionIDForContents, CreateCharacter->direction_, CreateCharacter->x_, CreateCharacter->y_, CreateCharacter->hp_);
			CPacket::Free(CreateOtherCharacterForMePacket);

			if (CreateCharacter->isMove_ == true)
			{

				CPacket* MoveStartForMePacket = CPacket::Alloc();
				MakePacketMoveStartForMe(Target, MoveStartForMePacket, CreateCharacter->SessionIDForContents, CreateCharacter->action_, CreateCharacter->x_, CreateCharacter->y_);
				CPacket::Free(MoveStartForMePacket);
			}


		}
	}


}

void MMOTCPServer_Single::HitCheck(Character* AttackCharacter, int AttackNumber)
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

	if (AttackCharacter->direction_ == dfPACKET_MOVE_DIR_LL)
	{
		GetSectorAroundForHitLeft(AttackCharacter, BoundaryX, BoundaryY, &HitCheckSector);

		//PrintHitCheckSector(&HitCheckSector);

		for (unsigned int i = 0; i < HitCheckSector.count_; ++i)
		{
			std::list<Character*>::iterator Iter;
			for (Iter = Sector[HitCheckSector.around_[i].y_][HitCheckSector.around_[i].x_].begin(); Iter != Sector[HitCheckSector.around_[i].y_][HitCheckSector.around_[i].x_].end(); ++Iter)
			{
				Character* Target = *Iter;

				if ((AttackCharacter == Target) || (Target->IsDelete == 1) || (AttackCharacter->x_ < Target->x_))
				{
					continue;
				}


				if (abs(AttackCharacter->y_ - Target->y_) <= BoundaryY && abs(AttackCharacter->x_ - Target->x_) <= BoundaryX)
				{

					if (Damage >= Target->hp_)
					{
						Target->hp_ = 0;
					}
					else
					{
						Target->hp_ -= Damage;
					}

					CPacket* DamagePacket = CPacket::Alloc();
					MakePacketDamage(Target, DamagePacket, AttackCharacter->SessionIDForContents, Target->SessionIDForContents, Target->hp_);
					CPacket::Free(DamagePacket);



					if (Target->hp_ == 0)
					{
						Disconnect(Target->sessionId_);
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
		for (unsigned int i = 0; i < HitCheckSector.count_; ++i)
		{
			std::list<Character*>::iterator Iter;
			for (Iter = Sector[HitCheckSector.around_[i].y_][HitCheckSector.around_[i].x_].begin(); Iter != Sector[HitCheckSector.around_[i].y_][HitCheckSector.around_[i].x_].end(); ++Iter)
			{
				Character* Target = *Iter;

				if ((AttackCharacter == Target) || (Target->IsDelete == 1) || (AttackCharacter->x_ > Target->x_))
				{
					continue;
				}


				if (abs(AttackCharacter->y_ - Target->y_) <= BoundaryY && abs(AttackCharacter->x_ - Target->x_) <= BoundaryX)
				{

					if (Damage >= Target->hp_)
					{
						Target->hp_ = 0;
					}
					else
					{
						Target->hp_ -= Damage;
					}

					CPacket* DamagePacket = CPacket::Alloc();
					MakePacketDamage(Target, DamagePacket, AttackCharacter->SessionIDForContents, Target->SessionIDForContents, Target->hp_);
					CPacket::Free(DamagePacket);

					if (Target->hp_ == 0)
					{
						//printf(" HP가 0 이라 종료 당하는 녀석 : %d \n\n", Target->SessionID);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, TIMEOUT));

						Disconnect(Target->sessionId_);
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


void MMOTCPServer_Single::GetSectorAroundForHitLeft(Character* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->characterSectorPos_.x_;
	int SectorY = Target->characterSectorPos_.y_;



	AroundSector->count_ = 0;

	AroundSector->around_[AroundSector->count_].x_ = SectorX;
	AroundSector->around_[AroundSector->count_].y_ = SectorY;
	AroundSector->count_++;

	int TargetValidPosX = ((Target->x_ - BoundaryX) / SECTORXSIZE);
	int TargetValidPosYAbove = ((Target->y_ - BoundaryY) / SECTORYSIZE);
	int TargetValidPosYBelow = ((Target->y_ + BoundaryY) / SECTORYSIZE);



	if (SectorX - 1 >= 0 && TargetValidPosX != SectorX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX - 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SECTORMAXY && TargetValidPosYBelow != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}

	if (SectorY - 1 >= 0 && TargetValidPosYAbove != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SECTORMAXY && SectorX - 1 >= 0 && TargetValidPosX != SectorX && TargetValidPosYBelow != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX - 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}
	if (SectorY - 1 >= 0 && SectorX - 1 >= 0 && TargetValidPosX != SectorX && TargetValidPosYAbove != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX - 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

}

void MMOTCPServer_Single::GetSectorAroundForHitRight(Character* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->characterSectorPos_.x_;
	int SectorY = Target->characterSectorPos_.y_;

	AroundSector->count_ = 0;

	AroundSector->around_[AroundSector->count_].x_ = SectorX;
	AroundSector->around_[AroundSector->count_].y_ = SectorY;
	AroundSector->count_++;

	int TargetValidPosX = ((Target->x_ + BoundaryX) / SECTORXSIZE);
	int TargetValidPosYAbove = ((Target->y_ - BoundaryY) / SECTORYSIZE);
	int TargetValidPosYBelow = ((Target->y_ + BoundaryY) / SECTORYSIZE);

	if (SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SECTORMAXY && TargetValidPosYBelow != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}

	if (SectorY - 1 >= 0 && TargetValidPosYAbove != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SECTORMAXY && SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX && TargetValidPosYBelow != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX && TargetValidPosYAbove != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
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

	AroundSector->count_ = 0;
	AroundSector->around_[AroundSector->count_].x_ = SectorX;
	AroundSector->around_[AroundSector->count_].y_ = SectorY;
	AroundSector->count_++;



	if (SectorX + 1 < SECTORMAXX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY;
		AroundSector->count_++;

	}
	if (SectorX - 1 >= 0)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX - 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY;
		AroundSector->count_++;
	}


	if (SectorY + 1 < SECTORMAXY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}

	if (SectorY - 1 >= 0)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SECTORMAXY && SectorX + 1 < SECTORMAXX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < SECTORMAXX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SECTORMAXY && SectorX - 1 >= 0)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX - 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}
	if (SectorY - 1 >= 0 && SectorX - 1 >= 0)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX - 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

	CacheAround[SectorY][SectorX] = *AroundSector;
	CacheAround[SectorY][SectorX].Flag = 1;

}


////메모리를  너무 많이 사용하는 것 같은데 이걸 최적화라고 할 수 있는가? 

SectorAround CacheUpdateAround[SECTORMAXY][SECTORMAXX][SECTORMAXY][SECTORMAXX][2];

void MMOTCPServer_Single::GetUpdateSectorAround(Character* Target, SectorAround* RemoveSector, SectorAround* AddSector)
{
	//로직은 좀 더 생각해봐야 함

	if (CacheUpdateAround[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_][Target->characterSectorPos_.y_][Target->characterSectorPos_.x_][0].Flag)
	{
		*RemoveSector = CacheUpdateAround[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_][Target->characterSectorPos_.y_][Target->characterSectorPos_.x_][0];
		*AddSector = CacheUpdateAround[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_][Target->characterSectorPos_.y_][Target->characterSectorPos_.x_][1];
		return;

	}


	RemoveSector->count_ = 0;
	AddSector->count_ = 0;


	SectorAround Old;
	SectorAround Cur;

	GetSectorAround(Target->oldSectorPos_.x_, Target->oldSectorPos_.y_, &Old);
	GetSectorAround(Target->characterSectorPos_.x_, Target->characterSectorPos_.y_, &Cur);


	unsigned int RemoveIndex;
	for (unsigned int i = 0; i < Old.count_; i++)
	{
		for (RemoveIndex = 0; RemoveIndex < Cur.count_; RemoveIndex++)
		{
			if (Old.around_[i].x_ == Cur.around_[RemoveIndex].x_ && Old.around_[i].y_ == Cur.around_[RemoveIndex].y_)
			{
				break;
			}
		}
		if (RemoveIndex == Cur.count_)
		{
			RemoveSector->around_[RemoveSector->count_].x_ = Old.around_[i].x_;
			RemoveSector->around_[RemoveSector->count_].y_ = Old.around_[i].y_;
			RemoveSector->count_++;
		}
	}

	AddSector->count_ = 0;
	unsigned int j;
	for (unsigned int i = 0; i < Cur.count_; i++)
	{
		for (j = 0; j < Old.count_; j++)
		{
			if (Old.around_[j].x_ == Cur.around_[i].x_ && Old.around_[j].y_ == Cur.around_[i].y_)
			{
				break;
			}
		}
		if (j == Old.count_)
		{
			AddSector->around_[AddSector->count_].x_ = Cur.around_[i].x_;
			AddSector->around_[AddSector->count_].y_ = Cur.around_[i].y_;
			AddSector->count_++;
		}
	}

	CacheUpdateAround[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_][Target->characterSectorPos_.y_][Target->characterSectorPos_.x_][0] = *RemoveSector;
	CacheUpdateAround[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_][Target->characterSectorPos_.y_][Target->characterSectorPos_.x_][1] = *AddSector;
	CacheUpdateAround[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_][Target->characterSectorPos_.y_][Target->characterSectorPos_.x_][0].Flag = 1;
	CacheUpdateAround[Target->oldSectorPos_.y_][Target->oldSectorPos_.x_][Target->characterSectorPos_.y_][Target->characterSectorPos_.x_][1].Flag = 1;

}


#define FPS 40

void MMOTCPServer_Single::Update()
{

	unsigned int Tick = timeGetTime();
	unsigned int Frame = Tick - OldTick;

	GlobalLoop++;

	//얘가 그냥 프레임. 

	int FixUpdate = (Frame / FPS);
	std::unordered_map<__int64, Character*>::iterator Iter;
	for (Iter = CharacterMap.begin(); Iter != CharacterMap.end(); ++Iter)
	{
		Character* Target = Iter->second;

		if (Target->isMove_ && (Target->IsDelete == 0))
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




void MMOTCPServer_Single::GameRun(Character* Target)
{
	switch (Target->action_)
	{
	case dfPACKET_MOVE_DIR_LL:
		if (Target->x_ - 6 < dfRANGE_MOVE_LEFT)
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ -= 6;

		break;

	case dfPACKET_MOVE_DIR_LU:
		if (Target->y_ - 4 < dfRANGE_MOVE_TOP || Target->x_ - 6 < dfRANGE_MOVE_LEFT)
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ -= 6;
		Target->y_ -= 4;

		break;



	case dfPACKET_MOVE_DIR_UU:
		if (Target->y_ - 4 < dfRANGE_MOVE_TOP)
		{
			Target->isMove_ = false;
			return;
		}


		Target->y_ -= 4;

		break;


	case dfPACKET_MOVE_DIR_RU:

		if ((Target->y_ - 4 < dfRANGE_MOVE_TOP) || (Target->x_ + 6 >= dfRANGE_MOVE_RIGHT))
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ += 6;
		Target->y_ -= 4;

		break;


	case dfPACKET_MOVE_DIR_RR:

		if (Target->x_ + 6 >= dfRANGE_MOVE_RIGHT)
		{
			Target->isMove_ = false;
			return;
		}
		Target->x_ += 6;

		break;


	case dfPACKET_MOVE_DIR_RD:

		if (Target->y_ + 4 >= dfRANGE_MOVE_BOTTOM || Target->x_ + 6 >= dfRANGE_MOVE_RIGHT)
		{
			Target->isMove_ = false;
			return;
		}


		Target->x_ += 6;
		Target->y_ += 4;

		break;



	case dfPACKET_MOVE_DIR_DD:
		if (Target->y_ + 4 >= dfRANGE_MOVE_BOTTOM)
		{
			Target->isMove_ = false;
			return;
		}

		Target->y_ += 4;

		break;

	case dfPACKET_MOVE_DIR_LD:
		if (Target->y_ + 4 >= dfRANGE_MOVE_BOTTOM || Target->x_ - 6 < dfRANGE_MOVE_LEFT)
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ -= 6;
		Target->y_ += 4;

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

void MMOTCPServer_Single::DisconnectContents(Character* Target)
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

