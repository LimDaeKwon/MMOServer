#include "MMOTCPServer_Single.h"
#include "ContentsCPacket.h"
#include <process.h>
#include "PacketDefine.h"
#include <conio.h>


MMOTCPServerSingle::MMOTCPServerSingle() : messageDataFreeList_(1000), characterFreeList_(1000)
{

	wprintf(L"MMOTCPServerSingle\n");

	oldTick_ = timeGetTime();
	oldTickForCheck_ = oldTick_;

	logicThreadHandle_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LogicThread, this, 0, nullptr));

}

MMOTCPServerSingle::~MMOTCPServerSingle()
{

}

bool MMOTCPServerSingle::OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort)
{
	return false;
}

void MMOTCPServerSingle::OnAccept(const wchar_t* serverIp, unsigned short serverPort, __int64 sessionId)
{


	MessageData* messageData = messageDataFreeList_.Alloc();
	messageData->sessionId_ = sessionId;
	messageData->contentsPacket_ = nullptr;
	messageData->packetType_ = 0;
	messageData->type_ = MessageTypeAccept;

	messageQueue_.Enqueue(messageData);


}

void MMOTCPServerSingle::OnRelease(__int64 sessionId)
{

	MessageData* messageData = messageDataFreeList_.Alloc();
	messageData->sessionId_ = sessionId;
	messageData->contentsPacket_ = nullptr;
	messageData->packetType_ = 0;
	messageData->type_ = MessageTypeRelease;

	messageQueue_.Enqueue(messageData);


}

void MMOTCPServerSingle::OnMessage(__int64 sessionId, BYTE packetType, CPacket* contentsSendPacket)
{


	MessageData* messageData = messageDataFreeList_.Alloc();
	messageData->sessionId_ = sessionId;
	contentsSendPacket->IncreaseRefCount();
	messageData->contentsPacket_ = contentsSendPacket;
	messageData->packetType_ = packetType;
	messageData->type_ = MessageTypePacket;

	messageQueue_.Enqueue(messageData);

}

void MMOTCPServerSingle::OnError(int errorCode, const wchar_t* errorLog)
{

}


unsigned int WINAPI MMOTCPServerSingle::LogicThread(LPVOID thisPtr)
{
	MMOTCPServerSingle* server = static_cast<MMOTCPServerSingle*>(thisPtr);

	while (true)
	{

		server->MessageLoop();

		server->Update();

	}

	return 0;
}

void MMOTCPServerSingle::MessageLoop()
{
	while (1)
	{
		MessageData* msg = nullptr;
		if (!messageQueue_.Dequeue(&msg))
		{
			break;
		}

		if (!MessageProc(msg))
		{
			DebugBreak();
		}

		if (msg->contentsPacket_ != nullptr)
		{
			CPacket::Free(msg->contentsPacket_);
		}
		messageDataFreeList_.Free(msg);
	}
}


bool MMOTCPServerSingle::MessageProc(MessageData* msg)
{
	switch (msg->type_)
	{
	case MessageTypeAccept:
	{
		CreateCharacter(msg->sessionId_);
		break;
	}
	case MessageTypePacket:
	{
		PacketProc(msg);
		break;
	}
	case MessageTypeRelease:
	{
		ReleaseCharacter(msg->sessionId_);
		break;

	}

	}
	return true;
}

void MMOTCPServerSingle::ReleaseCharacter(__int64 newSession)
{
	Character* target = characterMap_.at(newSession);

	characterMap_.erase(newSession);
	sector_[target->characterSectorPos_.y_][target->characterSectorPos_.x_].remove(target);


	if (target->sessionIdForContents_ == 0)
	{
		DebugBreak();
	}
	CPacket* deleteCharacterPacket = CPacket::Alloc();
	MakePacketDeleteCharacter(target, deleteCharacterPacket, target->sessionIdForContents_);
	CPacket::Free(deleteCharacterPacket);

	characterFreeList_.Free(target);

	//wprintf(L"## Disconnect id : %d \n", targetSession->SessionID);



}




bool MMOTCPServerSingle::PacketProc(MessageData* msg)
{
	Character* target;
	target = characterMap_.at(msg->sessionId_);


	switch (msg->packetType_)
	{
	case PacketCsMoveStart:
	{
		unsigned char direction;
		unsigned short x;
		unsigned short y;
		*msg->contentsPacket_ >> direction >> x >> y;
		return NetPacketProcMoveStart(target, direction, x, y);
		break;
	}

	case PacketCsMoveStop:
	{
		unsigned char direction;
		unsigned short x;
		unsigned short y;
		*msg->contentsPacket_ >> direction >> x >> y;
		return NetPacketProcMoveStop(target, direction, x, y);
		break;
	}

	case PacketCsAttack1:
	{
		unsigned char direction;
		unsigned short x;
		unsigned short y;
		*msg->contentsPacket_ >> direction >> x >> y;
		return NetPacketProcAttack1(target, direction, x, y);
		break;
	}

	case PacketCsAttack2:
	{
		unsigned char direction;
		unsigned short x;
		unsigned short y;
		*msg->contentsPacket_ >> direction >> x >> y;
		return NetPacketProcAttack2(target, direction, x, y);
		break;
	}

	case PacketCsAttack3:
	{
		unsigned char direction;
		unsigned short x;
		unsigned short y;
		*msg->contentsPacket_ >> direction >> x >> y;
		return NetPacketProcAttack3(target, direction, x, y);
		break;
	}

	case PacketCsEcho:
	{
		unsigned int time;
		*msg->contentsPacket_ >> time;
		return NetPacketProcEcho(target, time);
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


void MMOTCPServerSingle::CreateCharacter(__int64 newSession)
{
	Character* newPlayer = characterFreeList_.Alloc();
	newPlayer->sessionId_ = newSession;
	newPlayer->sessionIdForContents_ = static_cast<unsigned int>(newSession);
	newPlayer->direction_ = PacketMoveDirectionRR;
	newPlayer->action_ = PacketMoveDirectionRR;
	newPlayer->x_ = rand() % 6399;
	newPlayer->y_ = rand() % 6399;

	//newPlayer->x = 100;
	//newPlayer->y = 100;

	newPlayer->hp_ = 100;
	newPlayer->characterSectorPos_.x_ = newPlayer->x_ / SectorXSize;
	newPlayer->characterSectorPos_.y_ = newPlayer->y_ / SectorYSize;
	newPlayer->oldSectorPos_.x_ = SectorMaxX;
	newPlayer->oldSectorPos_.y_ = SectorMaxY;
	newPlayer->isMove_ = false;
	newPlayer->isDelete_ = false;


	sector_[newPlayer->characterSectorPos_.y_][newPlayer->characterSectorPos_.x_].push_back(newPlayer);
	characterMap_.insert(std::unordered_map<__int64, Character*>::value_type(newPlayer->sessionId_, newPlayer));


	CPacket* packetCreateMyCharacter = CPacket::Alloc();
	MakePacketCreateMyCharacter(newPlayer, packetCreateMyCharacter, newPlayer->sessionIdForContents_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
	CPacket::Free(packetCreateMyCharacter);

	CPacket* packetCreateOtherCharacter = CPacket::Alloc();
	MakePacketCreateOtherCharacter(newPlayer, packetCreateOtherCharacter, newPlayer->sessionIdForContents_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
	CPacket::Free(packetCreateOtherCharacter);


	//나를 남에게.
	SectorAround createForMe;
	GetSectorAround(newPlayer->characterSectorPos_.x_, newPlayer->characterSectorPos_.y_, &createForMe);

	for (unsigned int i = 0; i < createForMe.count_; i++)
	{
		std::list<Character*>::iterator iter;
		for (iter = sector_[createForMe.around_[i].y_][createForMe.around_[i].x_].begin(); iter != sector_[createForMe.around_[i].y_][createForMe.around_[i].x_].end(); ++iter)
		{
			Character* target = *iter;
			if ((target->sessionIdForContents_ == newPlayer->sessionIdForContents_) || (target->isDelete_ == 1))
			{
				continue;
			}

			CPacket* otherCharacter = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(newPlayer, otherCharacter, target->sessionIdForContents_, target->direction_, target->x_, target->y_, target->hp_);
			CPacket::Free(otherCharacter);


			if (target->isMove_ == true)
			{
				CPacket* moveStartForMePacket = CPacket::Alloc();
				MakePacketMoveStartForMe(newPlayer, moveStartForMePacket, target->sessionIdForContents_, target->action_, target->x_, target->y_);
				CPacket::Free(moveStartForMePacket);
			}

		}
	}
}



int MMOTCPServerSingle::ServerControl()
{
	static bool controlMode = false;


	if (_kbhit())
	{
		WCHAR controlKey = _getwch();

		if (L'u' == controlKey || L'U' == controlKey)
		{
			controlMode = true;

		}

		if (controlMode && L'q' == controlKey || L'q' == controlKey)
		{
			Stop();
			return -1;

			//원하는 기능 처리.

		}

	}

	return 0;
}




void MMOTCPServerSingle::MakePacketMoveStart(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScMoveStart << id << direction << x << y;
	SendPacketAround(target, packet);
}


void MMOTCPServerSingle::MakePacketMoveStartForMe(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScMoveStart << id << direction << x << y;
	SendPacketUnicast(target, packet);
}


void MMOTCPServerSingle::MakePacketMoveStop(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScMoveStop << id << direction << x << y;
	SendPacketAround(target, packet);
}


void MMOTCPServerSingle::MakePacketCreateMyCharacter(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{

	*packet << PacketScCreateMyCharacter << id << direction << x << y << hp;


	SendPacketUnicast(target, packet);
}


void MMOTCPServerSingle::MakePacketCreateOtherCharacterForMe(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
	*packet << PacketScCreateOtherCharacter << id << direction << x << y << hp;
	SendPacketUnicast(target, packet);
}


void MMOTCPServerSingle::MakePacketCreateOtherCharacter(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
	*packet << PacketScCreateOtherCharacter << id << direction << x << y << hp;
	SendPacketAround(target, packet);
}


void MMOTCPServerSingle::MakePacketDeleteCharacter(Character* target, CPacket* packet, unsigned int id)
{
	*packet << PacketScDeleteCharacter << id;
	SendPacketAround(target, packet);
}


void MMOTCPServerSingle::MakePacketDamage(Character* target, CPacket* packet, unsigned int attackId, unsigned int damageId, unsigned char damageHp)
{
	*packet << PacketScDamage << attackId << damageId << damageHp;
	SendPacketAround(target, packet, true);
}


void MMOTCPServerSingle::MakePacketAttack1(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScAttack1 << id << direction << x << y;
	SendPacketAround(target, packet);
}


void MMOTCPServerSingle::MakePacketAttack2(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScAttack2 << id << direction << x << y;
	SendPacketAround(target, packet);
}


void MMOTCPServerSingle::MakePacketAttack3(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScAttack3 << id << direction << x << y;
	SendPacketAround(target, packet);
}

void MMOTCPServerSingle::MakePacketEcho(Character* target, CPacket* packet, unsigned int time)
{
	*packet << PacketScEcho << time;
	SendPacketUnicast(target, packet);
}

void MMOTCPServerSingle::MakePacketDeleteCharacterRemoveSector(Character* target, CPacket* packet, SectorAround* around, unsigned int id)
{
	*packet << PacketScDeleteCharacter << id;
	SendPacketAroundRemoveSector(packet, around);
}

void MMOTCPServerSingle::MakePacketCreateCharacterAddSector(Character* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
	*packet << PacketScCreateOtherCharacter << id << direction << x << y << hp;
	SendPacketAroundAddSector(packet, around);
}

void MMOTCPServerSingle::MakePacketMoveStartAddSector(Character* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScMoveStart << id << direction << x << y;
	SendPacketAroundAddSector(packet, around);
}

void MMOTCPServerSingle::MakePacketDeleteCharacterForMe(Character* target, CPacket* packet, unsigned int id)
{
	*packet << PacketScDeleteCharacter << id;
	SendPacketUnicast(target, packet);

}

void MMOTCPServerSingle::MakePacketSync(Character* target, CPacket* packet, unsigned int id, unsigned short x, unsigned short y)
{
	*packet << PacketScSync << id << x << y;
	SendPacketAround(target, packet, true);
}


void MMOTCPServerSingle::SendPacketUnicast(Character* target, CPacket* packet)
{
	SendPacket(target->sessionId_, packet);
}

void MMOTCPServerSingle::SendPacketAround(Character* target, CPacket* packet, bool sendMe)
{

	SectorAround around;
	GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &around);


	if (sendMe)
	{
		for (unsigned int index = 0; index < around.count_; index++)
		{
			SendPacketSectorOne(around.around_[index].x_, around.around_[index].y_, 0, packet);
		}
	}
	else
	{
		for (unsigned int index = 0; index < around.count_; index++)
		{
			SendPacketSectorOne(around.around_[index].x_, around.around_[index].y_, target->sessionIdForContents_, packet);
		}
	}

}

//TODO : 이 둘의 코드는 같은데 왜 분리를 해놨는가?
//
//타겟은 왜 있는거지?

void MMOTCPServerSingle::SendPacketAroundRemoveSector(CPacket* packet, SectorAround* around)
{
	for (unsigned int index = 0; index < around->count_; index++)
	{
		SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, 0, packet);
	}
}

void MMOTCPServerSingle::SendPacketAroundAddSector(CPacket* packet, SectorAround* around)
{
	for (unsigned int index = 0; index < around->count_; index++)
	{
		SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, 0, packet);
	}
}

void MMOTCPServerSingle::SendPacketSectorOne(int sectorX, int sectorY, unsigned int exceptSessionId, CPacket* packet)
{
	Character* target;
	std::list<Character*>::iterator iter;
	for (iter = sector_[sectorY][sectorX].begin(); iter != sector_[sectorY][sectorX].end(); ++iter)
	{
		target = *iter;
		if ((target->sessionIdForContents_ == exceptSessionId) || (target->isDelete_ == 1))
		{
			continue;
		}
		SendPacket(target->sessionId_, packet);
	}
}





bool MMOTCPServerSingle::NetPacketProcMoveStart(Character* target, unsigned char direction, unsigned short x, unsigned short y)
{

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{
		//Disconnect(target->CharacterSession);
		CPacket* syncPacket = CPacket::Alloc();
		MakePacketSync(target, syncPacket, target->sessionIdForContents_, target->x_, target->y_);
		CPacket::Free(syncPacket);


		//wprintf(L"MoveStart OutOfRange  Server x y  :  %d   %d  /   Client x y  :   %d   %d  \n", target->x, target->y, x, y);
	}
	else
	{

		target->x_ = x;
		target->y_ = y;

		if (SectorUpdateCharacter(target))
		{
			SectorUpdate(target);
		}
	}



	target->isMove_ = true;
	target->action_ = direction;

	switch (direction)
	{
	case PacketMoveDirectionRR:
	case PacketMoveDirectionRU:
	case PacketMoveDirectionRD:
		target->direction_ = PacketMoveDirectionRR;
		break;

	case PacketMoveDirectionLU:
	case PacketMoveDirectionLL:
	case PacketMoveDirectionLD:
		target->direction_ = PacketMoveDirectionLL;
		break;

	}

	CPacket* moveStartPacket = CPacket::Alloc();
	MakePacketMoveStart(target, moveStartPacket, target->sessionIdForContents_, direction, target->x_, target->y_);
	CPacket::Free(moveStartPacket);


	return true;
}

bool MMOTCPServerSingle::NetPacketProcMoveStop(Character* target, unsigned char direction, unsigned short x, unsigned short y)
{
	//여기서 처리. 클라의 좌표를 인정해준다.64

	//여기서 한번 돌려줘야함. run을..

	if ((abs(target->x_ - x) > ErrorRange) || (abs(target->y_ - y) > ErrorRange))
	{
		//Disconnect(target->CharacterSession);
		//wprintf(L"MoveStop OutOfRange  Server x y  :  %d   %d  /   Client x y  :   %d   %d  \n", target->x, target->y, x, y);
		CPacket* syncPacket = CPacket::Alloc();
		MakePacketSync(target, syncPacket, target->sessionIdForContents_, target->x_, target->y_);
		CPacket::Free(syncPacket);


		//wprintf(L"MoveStart OutOfRange  Server x y  :  %d   %d  /   Client x y  :   %d   %d  \n", target->x, target->y, x, y);

	}
	else
	{


		target->x_ = x;
		target->y_ = y;

		if (SectorUpdateCharacter(target))
		{
			SectorUpdate(target);
		}


	}

	//wprintf(L"## Stoppacket : x  %d   y  %d  \n", x, y);

	target->isMove_ = false;


	target->action_ = direction;

	switch (direction)
	{
	case PacketMoveDirectionRR:
	case PacketMoveDirectionRU:
	case PacketMoveDirectionRD:
		target->direction_ = PacketMoveDirectionRR;
		break;

	case PacketMoveDirectionLU:
	case PacketMoveDirectionLL:
	case PacketMoveDirectionLD:
		target->direction_ = PacketMoveDirectionLL;
		break;

	}


	CPacket* moveStopPacket = CPacket::Alloc();
	MakePacketMoveStop(target, moveStopPacket, target->sessionIdForContents_, direction, target->x_, target->y_);
	CPacket::Free(moveStopPacket);


	return true;
}

bool MMOTCPServerSingle::NetPacketProcAttack1(Character* target, unsigned char direction, unsigned short x, unsigned short y)
{
	unsigned int id;

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{
		//Disconnect(target->CharacterSession);


		CPacket* syncPacket = CPacket::Alloc();
		MakePacketSync(target, syncPacket, target->sessionIdForContents_, target->x_, target->y_);
		CPacket::Free(syncPacket);
		//wprintf(L"MoveStart OutOfRange  Server x y  :  %d   %d  /   Client x y  :   %d   %d  \n", target->x, target->y, x, y);


	}
	else
	{

		target->x_ = x;
		target->y_ = y;

		if (SectorUpdateCharacter(target))
		{
			SectorUpdate(target); // 지연처리 해주자..
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}



	id = target->sessionIdForContents_;
	target->direction_ = direction;

	CPacket* attack1Packet = CPacket::Alloc();
	MakePacketAttack1(target, attack1Packet, id, direction, target->x_, target->y_);
	CPacket::Free(attack1Packet);


	HitCheck(target, 1);

	return true;
}

bool MMOTCPServerSingle::NetPacketProcAttack2(Character* target, unsigned char direction, unsigned short x, unsigned short y)
{
	unsigned int id;

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{
		//Disconnect(target->CharacterSession);


		CPacket* syncPacket = CPacket::Alloc();
		MakePacketSync(target, syncPacket, target->sessionIdForContents_, target->x_, target->y_);
		CPacket::Free(syncPacket);

		//wprintf(L"MoveStart OutOfRange  Server x y  :  %d   %d  /   Client x y  :   %d   %d  \n", target->x, target->y, x, y);

	}
	else
	{

		target->x_ = x;
		target->y_ = y;

		if (SectorUpdateCharacter(target))
		{
			SectorUpdate(target); // 지연처리 해주자..
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}



	id = target->sessionIdForContents_;
	target->direction_ = direction;

	CPacket* attack2Packet = CPacket::Alloc();
	MakePacketAttack2(target, attack2Packet, id, direction, target->x_, target->y_);
	CPacket::Free(attack2Packet);

	HitCheck(target, 2);

	return true;
}

bool MMOTCPServerSingle::NetPacketProcAttack3(Character* target, unsigned char direction, unsigned short x, unsigned short y)
{

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{

		CPacket* syncPacket = CPacket::Alloc();
		MakePacketSync(target, syncPacket, target->sessionIdForContents_, target->x_, target->y_);
		CPacket::Free(syncPacket);


		//wprintf(L"MoveStart OutOfRange  Server x y  :  %d   %d  /   Client x y  :   %d   %d  \n", target->x, target->y, x, y);
	}
	else
	{

		target->x_ = x;
		target->y_ = y;

		if (SectorUpdateCharacter(target))
		{
			SectorUpdate(target); // 지연처리 해주자..
		}

		//섹터 업데이트 지연처리 해줘야함.,.
	}


	unsigned int id;
	id = target->sessionIdForContents_;
	target->direction_ = direction;

	CPacket* attack3Packet = CPacket::Alloc();
	MakePacketAttack3(target, attack3Packet, id, direction, target->x_, target->y_);
	CPacket::Free(attack3Packet);

	HitCheck(target, 3);
	return true;
}

bool MMOTCPServerSingle::NetPacketProcEcho(Character* target, unsigned int time)
{

	CPacket* echoPacket = CPacket::Alloc();
	MakePacketEcho(target, echoPacket, time);
	CPacket::Free(echoPacket);

	return false;


}

bool MMOTCPServerSingle::SectorUpdateCharacter(Character* target)
{
	int targetCurPosX = (target->x_ / SectorXSize);
	int targetCurPosY = (target->y_ / SectorYSize);

	if ((target->characterSectorPos_.x_ != targetCurPosX) || (target->characterSectorPos_.y_ != targetCurPosY))
	{

		target->oldSectorPos_.x_ = target->characterSectorPos_.x_;
		target->oldSectorPos_.y_ = target->characterSectorPos_.y_;
		target->characterSectorPos_.x_ = targetCurPosX;
		target->characterSectorPos_.y_ = targetCurPosY;

		sector_[target->oldSectorPos_.y_][target->oldSectorPos_.x_].remove(target);
		sector_[target->characterSectorPos_.y_][target->characterSectorPos_.x_].push_back(target);
		//printf("섹터 변경 \nOld : %d  %d Cur  : %d  %d \n" , target->OldSectorPos.x, target->OldSectorPos.y,targetCurPosX, targetCurPosY);



		return true;
	}

	return false;

}

void MMOTCPServerSingle::SectorUpdate(Character* target)
{

	SectorAround Remove;
	SectorAround Add;

	GetUpdateSectorAround(target, &Remove, &Add);

	//PrintUpdateSector(&Remove, &Add);

	CPacket* deleteCharacterRemoveSectorPacket = CPacket::Alloc();
	MakePacketDeleteCharacterRemoveSector(target, deleteCharacterRemoveSectorPacket, &Remove, target->sessionIdForContents_); //패킷 만들어서 Remove 시켜줘야하는데..
	CPacket::Free(deleteCharacterRemoveSectorPacket);



	//Remove에 있는 애들의 삭제를 나에게 보냄.
	for (unsigned int i = 0; i < Remove.count_; i++)
	{
		std::list<Character*>::iterator iter;
		for (iter = sector_[Remove.around_[i].y_][Remove.around_[i].x_].begin(); iter != sector_[Remove.around_[i].y_][Remove.around_[i].x_].end(); ++iter)
		{
			CPacket* deleteCharacterForMePacket = CPacket::Alloc();
			MakePacketDeleteCharacterForMe(target, deleteCharacterForMePacket, (*iter)->sessionIdForContents_);
			CPacket::Free(deleteCharacterForMePacket);



			//printf("Remove에 있는 애들의 삭제를 나에게 보냄. 지움 당하는 아이디 %d  : 받는 아이디 %d \n\n", target->CharacterSession->SessionID, (*iter)->SessionID);

		}
	}


	//add에 있는 애들에게 나의 생성을 보냄.
	CPacket* createCharacterAddSectorPacket = CPacket::Alloc();
	MakePacketCreateCharacterAddSector(target, createCharacterAddSectorPacket, &Add, target->sessionIdForContents_, target->direction_, target->x_, target->y_, target->hp_);
	CPacket::Free(createCharacterAddSectorPacket);


	//이동 정보도 보내줘야함.
		//
		//
	CPacket* moveStartAddSectorPacket = CPacket::Alloc();
	MakePacketMoveStartAddSector(target, moveStartAddSectorPacket, &Add, target->sessionIdForContents_, target->action_, target->x_, target->y_);
	CPacket::Free(moveStartAddSectorPacket);



	for (unsigned int i = 0; i < Add.count_; i++)
	{
		std::list<Character*>::iterator iterCreate;
		for (iterCreate = sector_[Add.around_[i].y_][Add.around_[i].x_].begin();
			iterCreate != sector_[Add.around_[i].y_][Add.around_[i].x_].end(); ++iterCreate)
		{
			Character* createCharacter = *iterCreate;
			if ((createCharacter->sessionId_ == target->sessionId_) || (createCharacter->isDelete_ == 1))
			{
				continue;
			}

			CPacket* createOtherCharacterForMePacket = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(target, createOtherCharacterForMePacket, createCharacter->sessionIdForContents_, createCharacter->direction_, createCharacter->x_, createCharacter->y_, createCharacter->hp_);
			CPacket::Free(createOtherCharacterForMePacket);

			if (createCharacter->isMove_ == true)
			{

				CPacket* moveStartForMePacket = CPacket::Alloc();
				MakePacketMoveStartForMe(target, moveStartForMePacket, createCharacter->sessionIdForContents_, createCharacter->action_, createCharacter->x_, createCharacter->y_);
				CPacket::Free(moveStartForMePacket);
			}


		}
	}


}

void MMOTCPServerSingle::HitCheck(Character* attackCharacter, int attackNumber)
{
	int boundaryX = 0;
	int boundaryY = 0;

	int damage = 0;



	switch (attackNumber)
	{

	case 1:
		boundaryX = Attack1RangeX;
		boundaryY = Attack1RangeY;
		damage = Attack1Damage;
		break;

	case 2:
		boundaryX = Attack2RangeX;
		boundaryY = Attack2RangeY;
		damage = Attack2Damage;
		break;

	case 3:
		boundaryX = Attack3RangeX;
		boundaryY = Attack3RangeY;
		damage = Attack3Damage;
		break;

	default:
		DebugBreak();
	}

	//섹터기준 처리로 바꾸기.


	SectorAround hitCheckSector;

	if (attackCharacter->direction_ == PacketMoveDirectionLL)
	{
		GetSectorAroundForHitLeft(attackCharacter, boundaryX, boundaryY, &hitCheckSector);

		//PrinthitCheckSector(&hitCheckSector);

		for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
		{
			std::list<Character*>::iterator iter;
			for (iter = sector_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].begin(); iter != sector_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].end(); ++iter)
			{
				Character* target = *iter;

				if ((attackCharacter == target) || (target->isDelete_ == 1) || (attackCharacter->x_ < target->x_))
				{
					continue;
				}


				if (abs(attackCharacter->y_ - target->y_) <= boundaryY && abs(attackCharacter->x_ - target->x_) <= boundaryX)
				{

					if (damage >= target->hp_)
					{
						target->hp_ = 0;
					}
					else
					{
						target->hp_ -= damage;
					}

					CPacket* damagePacket = CPacket::Alloc();
					MakePacketDamage(target, damagePacket, attackCharacter->sessionIdForContents_, target->sessionIdForContents_, target->hp_);
					CPacket::Free(damagePacket);



					if (target->hp_ == 0)
					{
						Disconnect(target->sessionId_);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(target->CharacterSession->SessionID, hp));

					}

					return;
				}

			}
		}


	}
	else
	{
		GetSectorAroundForHitRight(attackCharacter, boundaryX, boundaryY, &hitCheckSector);
		//PrinthitCheckSector(&hitCheckSector);
		for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
		{
			std::list<Character*>::iterator iter;
			for (iter = sector_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].begin(); iter != sector_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].end(); ++iter)
			{
				Character* target = *iter;

				if ((attackCharacter == target) || (target->isDelete_ == 1) || (attackCharacter->x_ > target->x_))
				{
					continue;
				}


				if (abs(attackCharacter->y_ - target->y_) <= boundaryY && abs(attackCharacter->x_ - target->x_) <= boundaryX)
				{

					if (damage >= target->hp_)
					{
						target->hp_ = 0;
					}
					else
					{
						target->hp_ -= damage;
					}

					CPacket* damagePacket = CPacket::Alloc();
					MakePacketDamage(target, damagePacket, attackCharacter->sessionIdForContents_, target->sessionIdForContents_, target->hp_);
					CPacket::Free(damagePacket);

					if (target->hp_ == 0)
					{
						//printf(" HP가 0 이라 종료 당하는 녀석 : %d \n\n", target->SessionID);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(target->CharacterSession->SessionID, TIMEOUT));

						Disconnect(target->sessionId_);
					}

					return;
				}

			}
		}


	}


	//std::unordered_map<unsigned int, CHARACTER*>::iterator iter;
	//for (iter = characterMap_.begin(); iter != characterMap_.end(); ++iter)
	//{
	//	CHARACTER* target = iter->second;

	//	if ((attackCharacter == target) || target->CharacterSession->isDelete_ == 1)
	//	{
	//		continue;
	//	}
	//
	//}




}


void MMOTCPServerSingle::GetSectorAroundForHitLeft(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
{
	int sectorX = target->characterSectorPos_.x_;
	int sectorY = target->characterSectorPos_.y_;



	aroundSector->count_ = 0;

	aroundSector->around_[aroundSector->count_].x_ = sectorX;
	aroundSector->around_[aroundSector->count_].y_ = sectorY;
	aroundSector->count_++;

	int targetValidPosX = ((target->x_ - boundaryX) / SectorXSize);
	int targetValidPosYAbove = ((target->y_ - boundaryY) / SectorYSize);
	int targetValidPosYBelow = ((target->y_ + boundaryY) / SectorYSize);



	if (sectorX - 1 >= 0 && targetValidPosX != sectorX)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY;
		aroundSector->count_++;
	}

	if (sectorY + 1 < SectorMaxY && targetValidPosYBelow != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX;
		aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
		aroundSector->count_++;
	}

	if (sectorY - 1 >= 0 && targetValidPosYAbove != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX;
		aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
		aroundSector->count_++;
	}

	if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
		aroundSector->count_++;
	}
	if (sectorY - 1 >= 0 && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
		aroundSector->count_++;
	}

}

void MMOTCPServerSingle::GetSectorAroundForHitRight(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
{
	int sectorX = target->characterSectorPos_.x_;
	int sectorY = target->characterSectorPos_.y_;

	aroundSector->count_ = 0;

	aroundSector->around_[aroundSector->count_].x_ = sectorX;
	aroundSector->around_[aroundSector->count_].y_ = sectorY;
	aroundSector->count_++;

	int targetValidPosX = ((target->x_ + boundaryX) / SectorXSize);
	int targetValidPosYAbove = ((target->y_ - boundaryY) / SectorYSize);
	int targetValidPosYBelow = ((target->y_ + boundaryY) / SectorYSize);

	if (sectorX + 1 < SectorMaxX && targetValidPosX != sectorX)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY;
		aroundSector->count_++;
	}

	if (sectorY + 1 < SectorMaxY && targetValidPosYBelow != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX;
		aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
		aroundSector->count_++;
	}

	if (sectorY - 1 >= 0 && targetValidPosYAbove != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX;
		aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
		aroundSector->count_++;
	}

	if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
		aroundSector->count_++;
	}

	if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
		aroundSector->count_++;
	}

}

SectorAround cachearound[SectorMaxY][SectorMaxX];


void MMOTCPServerSingle::GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
{

	if (cachearound[sectorY][sectorX].flag_)
	{
		*aroundSector = cachearound[sectorY][sectorX];
		return;
	}

	aroundSector->count_ = 0;
	aroundSector->around_[aroundSector->count_].x_ = sectorX;
	aroundSector->around_[aroundSector->count_].y_ = sectorY;
	aroundSector->count_++;



	if (sectorX + 1 < SectorMaxX)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY;
		aroundSector->count_++;

	}
	if (sectorX - 1 >= 0)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY;
		aroundSector->count_++;
	}


	if (sectorY + 1 < SectorMaxY)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX;
		aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
		aroundSector->count_++;
	}

	if (sectorY - 1 >= 0)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX;
		aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
		aroundSector->count_++;
	}

	if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
		aroundSector->count_++;
	}

	if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
		aroundSector->count_++;
	}

	if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
		aroundSector->count_++;
	}
	if (sectorY - 1 >= 0 && sectorX - 1 >= 0)
	{
		aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
		aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
		aroundSector->count_++;
	}

	cachearound[sectorY][sectorX] = *aroundSector;
	cachearound[sectorY][sectorX].flag_ = 1;

}


////메모리를  너무 많이 사용하는 것 같은데 이걸 최적화라고 할 수 있는가?

SectorAround cacheUpdatearound[SectorMaxY][SectorMaxX][SectorMaxY][SectorMaxX][2];

void MMOTCPServerSingle::GetUpdateSectorAround(Character* target, SectorAround* removeSector, SectorAround* addSector)
{
	//로직은 좀 더 생각해봐야 함

	if (cacheUpdatearound[target->oldSectorPos_.y_][target->oldSectorPos_.x_][target->characterSectorPos_.y_][target->characterSectorPos_.x_][0].flag_)
	{
		*removeSector = cacheUpdatearound[target->oldSectorPos_.y_][target->oldSectorPos_.x_][target->characterSectorPos_.y_][target->characterSectorPos_.x_][0];
		*addSector = cacheUpdatearound[target->oldSectorPos_.y_][target->oldSectorPos_.x_][target->characterSectorPos_.y_][target->characterSectorPos_.x_][1];
		return;

	}


	removeSector->count_ = 0;
	addSector->count_ = 0;


	SectorAround Old;
	SectorAround Cur;

	GetSectorAround(target->oldSectorPos_.x_, target->oldSectorPos_.y_, &Old);
	GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &Cur);


	unsigned int removeIndex;
	for (unsigned int i = 0; i < Old.count_; i++)
	{
		for (removeIndex = 0; removeIndex < Cur.count_; removeIndex++)
		{
			if (Old.around_[i].x_ == Cur.around_[removeIndex].x_ && Old.around_[i].y_ == Cur.around_[removeIndex].y_)
			{
				break;
			}
		}
		if (removeIndex == Cur.count_)
		{
			removeSector->around_[removeSector->count_].x_ = Old.around_[i].x_;
			removeSector->around_[removeSector->count_].y_ = Old.around_[i].y_;
			removeSector->count_++;
		}
	}

	addSector->count_ = 0;
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
			addSector->around_[addSector->count_].x_ = Cur.around_[i].x_;
			addSector->around_[addSector->count_].y_ = Cur.around_[i].y_;
			addSector->count_++;
		}
	}

	cacheUpdatearound[target->oldSectorPos_.y_][target->oldSectorPos_.x_][target->characterSectorPos_.y_][target->characterSectorPos_.x_][0] = *removeSector;
	cacheUpdatearound[target->oldSectorPos_.y_][target->oldSectorPos_.x_][target->characterSectorPos_.y_][target->characterSectorPos_.x_][1] = *addSector;
	cacheUpdatearound[target->oldSectorPos_.y_][target->oldSectorPos_.x_][target->characterSectorPos_.y_][target->characterSectorPos_.x_][0].flag_ = 1;
	cacheUpdatearound[target->oldSectorPos_.y_][target->oldSectorPos_.x_][target->characterSectorPos_.y_][target->characterSectorPos_.x_][1].flag_ = 1;

}




void MMOTCPServerSingle::Update()
{

	unsigned int Tick = timeGetTime();
	unsigned int Frame = Tick - oldTick_;

	globalLoop_++;

	//얘가 그냥 프레임.

	int FixUpdate = (Frame / FixedUpdateFrameMs);
	std::unordered_map<__int64, Character*>::iterator iter;
	for (iter = characterMap_.begin(); iter != characterMap_.end(); ++iter)
	{
		Character* target = iter->second;

		if (target->isMove_ && (target->isDelete_ == 0))
		{
			for (int i = 0; i < FixUpdate; ++i)
			{
				GameRun(target);
			}

		}

	}
	oldTick_ += (FixedUpdateFrameMs * (Frame / FixedUpdateFrameMs));

	if (Tick - oldTickForCheck_ >= 1000)
	{
		//wprintf(L"count_ : %d   Loop : %d \n", count_, globalLoop_);
		if (FixUpdate > 1)
		{

			wprintf(L"FixedUpdate : %d   Loop : %d \n", FixUpdate, globalLoop_);
		}
		oldTickForCheck_ += 1000;
		globalLoop_ = 0;
	}

	Frame = timeGetTime() - oldTick_;
	if (Frame < FixedUpdateFrameMs)
	{
		Sleep(FixedUpdateFrameMs - Frame);
	}


}




void MMOTCPServerSingle::GameRun(Character* target)
{
	switch (target->action_)
	{
	case PacketMoveDirectionLL:
		if (target->x_ - 6 < RangeMoveLeft)
		{
			target->isMove_ = false;
			return;
		}

		target->x_ -= 6;

		break;

	case PacketMoveDirectionLU:
		if (target->y_ - 4 < RangeMoveTop || target->x_ - 6 < RangeMoveLeft)
		{
			target->isMove_ = false;
			return;
		}

		target->x_ -= 6;
		target->y_ -= 4;

		break;



	case PacketMoveDirectionUU:
		if (target->y_ - 4 < RangeMoveTop)
		{
			target->isMove_ = false;
			return;
		}


		target->y_ -= 4;

		break;


	case PacketMoveDirectionRU:

		if ((target->y_ - 4 < RangeMoveTop) || (target->x_ + 6 >= RangeMoveRight))
		{
			target->isMove_ = false;
			return;
		}

		target->x_ += 6;
		target->y_ -= 4;

		break;


	case PacketMoveDirectionRR:

		if (target->x_ + 6 >= RangeMoveRight)
		{
			target->isMove_ = false;
			return;
		}
		target->x_ += 6;

		break;


	case PacketMoveDirectionRD:

		if (target->y_ + 4 >= RangeMoveBottom || target->x_ + 6 >= RangeMoveRight)
		{
			target->isMove_ = false;
			return;
		}


		target->x_ += 6;
		target->y_ += 4;

		break;



	case PacketMoveDirectionDD:
		if (target->y_ + 4 >= RangeMoveBottom)
		{
			target->isMove_ = false;
			return;
		}

		target->y_ += 4;

		break;

	case PacketMoveDirectionLD:
		if (target->y_ + 4 >= RangeMoveBottom || target->x_ - 6 < RangeMoveLeft)
		{
			target->isMove_ = false;
			return;
		}

		target->x_ -= 6;
		target->y_ += 4;

		break;
	}


	if (SectorUpdateCharacter(target))
	{

		SectorUpdate(target);

	}
}

//지연삭제코드. 여기서 삭제하면 된다. 전체를 순회하며 하거나 deleteList_를 만들어도 된다.

void MMOTCPServerSingle::DeleteDisconnect()
{
	/*if (deleteList_.size() > 0)
	{

		SESSION* Session;
		CHARACTER* Deletetarget;
		unsigned int Session_ID;
		std::list<unsigned int>::iterator iter;
		for (iter = deleteList_.begin(); iter != deleteList_.end(); ++iter)
		{
			Session_ID = *iter;
			Session = Sessions.at(Session_ID);
			Deletetarget = characterMap_.at(Session_ID);

			FreeCharacter(Deletetarget);

			characterMap_.erase(Session_ID);

			closesocket(Session->Socket);
			Session->ReceiveQ.ClearBuffer();
			Session->SendQ.ClearBuffer();

			SessionFreeList.Free(Session);
			Sessions.erase(Session_ID);

		}

		deleteList_.clear();
	}*/

}

void MMOTCPServerSingle::DisconnectContents(Character* target)
{
	//if (targetSession->isDelete_ == 1)
	//{
	//	return;
	//}

	//deleteList_.push_back(targetSession->SessionID);
	//targetSession->isDelete_ = 1;

	//CHARACTER* target = characterMap_.at(targetSession->SessionID);

	//sector_[target->CharacterSectorPos.y][target->CharacterSectorPos.x].remove(target);

	//GlobalCPacket.Clear();
	//MakePacketDeleteCharacter(targetSession, &GlobalCPacket, targetSession->SessionID);
	////wprintf(L"## Disconnect id : %d \n", targetSession->SessionID);

}
