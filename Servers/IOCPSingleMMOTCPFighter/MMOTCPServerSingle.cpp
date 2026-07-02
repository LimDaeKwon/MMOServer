#include "MMOTCPServerSingle.h"
#include "ContentsCPacket.h"
#include <process.h>
#include <conio.h>
#include "Profiler.h"
#include "PacketDefine.h"


MMOTCPServerSingle::MMOTCPServerSingle() : messageDataFreeList_(1000), characterFreeList_(1000)
{

	wprintf(L"MMOTCPServerSingle\n");

	oldTick_ = timeGetTime();
	oldTickForCheck_ = oldTick_;
	frameMs_ = 40;

	InitializeSectorUpdateAround();

	logicThreadHandle_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, LogicThread, this, 0, nullptr));




}

MMOTCPServerSingle::~MMOTCPServerSingle()
{

}

bool MMOTCPServerSingle::OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort)
{
	return false;
}

void MMOTCPServerSingle::OnAccept(const wchar_t* serverIp, unsigned short serverPort, SessionId sessionId)
{
	Profile profile(L"OnAccept");

	MessageData* messageData = messageDataFreeList_.Alloc();
	messageData->sessionId_ = sessionId;
	messageData->contentsPacket_ = nullptr;
	messageData->packetType_ = 0;
	messageData->type_ = MessageTypeAccept;

	messageQueue_.Enqueue(messageData);


}

void MMOTCPServerSingle::OnRelease(SessionId sessionId)
{
	Profile profile(L"OnRelease");

	MessageData* messageData = messageDataFreeList_.Alloc();
	messageData->sessionId_ = sessionId;
	messageData->contentsPacket_ = nullptr;
	messageData->packetType_ = 0;
	messageData->type_ = MessageTypeRelease;

	messageQueue_.Enqueue(messageData);


}

void MMOTCPServerSingle::OnMessage(SessionId sessionId, BYTE packetType, CPacket* contentsSendPacket)
{

	Profile profile(L"OnMessage");

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

Character* MMOTCPServerSingle::CreateCharacter(SessionId sessionId)
{
	Character* newPlayer = characterFreeList_.Alloc();
	newPlayer->sessionId_ = sessionId;

	newPlayer->direction_ = PacketMoveDirectionRR;
	newPlayer->action_ = PacketMoveDirectionRR;

	newPlayer->x_ = rand() % 6399;
	newPlayer->y_ = rand() % 6399;

	newPlayer->hp_ = DefaultHp;

	newPlayer->characterSectorPos_.x_ = newPlayer->x_ / SectorXSize;
	newPlayer->characterSectorPos_.y_ = newPlayer->y_ / SectorYSize;
	newPlayer->oldSectorPos_.x_ = SectorMaxX;
	newPlayer->oldSectorPos_.y_ = SectorMaxY;

	newPlayer->isMove_ = false;
	newPlayer->movingIndex_ = -1;


	return newPlayer;
}

void MMOTCPServerSingle::RegisterCharacter(Character* newPlayer)
{
	sectorCharacterList_[newPlayer->characterSectorPos_.y_][newPlayer->characterSectorPos_.x_].push_back(newPlayer);
	characterMap_.insert(std::unordered_map<SessionId, Character*>::value_type(newPlayer->sessionId_, newPlayer));
}

void MMOTCPServerSingle::SendNewCharacterCreate(Character* newPlayer)
{
	CPacket* packetCreateMyCharacter = CPacket::Alloc();
	MakePacketCreateMyCharacter(newPlayer, packetCreateMyCharacter, static_cast<unsigned int>(newPlayer->sessionId_), newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
	CPacket::Free(packetCreateMyCharacter);

	CPacket* packetCreateOtherCharacter = CPacket::Alloc();
	MakePacketCreateOtherCharacter(newPlayer, packetCreateOtherCharacter, static_cast<unsigned int>(newPlayer->sessionId_), newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
	CPacket::Free(packetCreateOtherCharacter);
}

void MMOTCPServerSingle::SendExistingCharactersToNewCharacter(Character* newPlayer)
{
	SectorAround around;
	GetSectorAround(newPlayer->characterSectorPos_.x_, newPlayer->characterSectorPos_.y_, &around);

	for (unsigned int i = 0; i < around.count_; ++i)
	{
		std::list<Character*>::iterator iter;
		for (iter = sectorCharacterList_[around.around_[i].y_][around.around_[i].x_].begin();
			iter != sectorCharacterList_[around.around_[i].y_][around.around_[i].x_].end(); ++iter)
		{
			Character* target = *iter;
			if ((target->sessionId_ == newPlayer->sessionId_))
			{
				continue;
			}

			CPacket* packetCreateOtherCharacterForMe = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(newPlayer, packetCreateOtherCharacterForMe, static_cast<unsigned int>(target->sessionId_), target->direction_, target->x_, target->y_, target->hp_);
			CPacket::Free(packetCreateOtherCharacterForMe);

			if (target->isMove_ == true)
			{
				CPacket* packetMoveStartForMe = CPacket::Alloc();
				MakePacketMoveStartForMe(newPlayer, packetMoveStartForMe, static_cast<unsigned int>(target->sessionId_), target->action_, target->x_, target->y_);
				CPacket::Free(packetMoveStartForMe);
			}
		}
	}

}


void MMOTCPServerSingle::SendCharacterDelete(Character* target)
{
	CPacket* packetDeleteCharacter = CPacket::Alloc();

	MakePacketDeleteCharacter(target, packetDeleteCharacter, static_cast<unsigned int>(target->sessionId_));

	CPacket::Free(packetDeleteCharacter);
}

void MMOTCPServerSingle::UnregisterCharacter(Character* target)
{
	sectorCharacterList_[target->characterSectorPos_.y_][target->characterSectorPos_.x_].remove(target);
	characterMap_.erase(target->sessionId_);
}

void MMOTCPServerSingle::ReleaseCharacter(Character* target)
{
	RemoveMovingCharacter(target);
	UnregisterCharacter(target);
	characterFreeList_.Free(target);
}

bool MMOTCPServerSingle::IsClientPositionValid(Character* target, unsigned short x, unsigned short y)
{
	return !((abs(target->x_ - x) > ErrorRange) || (abs(target->y_ - y) > ErrorRange));
}

void MMOTCPServerSingle::SendSync(Character* target)
{
	CPacket* packetSync = CPacket::Alloc();
	MakePacketSync(target, packetSync, static_cast<unsigned int>(target->sessionId_), target->x_, target->y_);
	CPacket::Free(packetSync);
}

void MMOTCPServerSingle::ApplyClientPosition(Character* target, unsigned short x, unsigned short y)
{
	target->x_ = x;
	target->y_ = y;

	if (SectorUpdateCharacter(target))
	{
		SectorUpdate(target);
	}
}

void MMOTCPServerSingle::SyncOrApplyClientPosition(Character* target, unsigned short x, unsigned short y)
{
	if (IsClientPositionValid(target, x, y) == false)
	{
		SendSync(target);
		return;
	}

	ApplyClientPosition(target, x, y);
}

void MMOTCPServerSingle::UpdateCharacterFacingDirection(Character* target, unsigned char direction)
{
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
}

bool MMOTCPServerSingle::MessageProc(MessageData* msg)
{
	switch (msg->type_)
	{
	case MessageTypeAccept:
	{
		AcceptProc(msg->sessionId_);
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

void MMOTCPServerSingle::ReleaseCharacter(SessionId sessionId)
{
	Profile profile(L"OnRelease");

	Character* target = characterMap_.at(sessionId);

	SendCharacterDelete(target);
	ReleaseCharacter(target);

}




bool MMOTCPServerSingle::PacketProc(MessageData* msg)
{
	SessionId target = msg->sessionId_;


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


void MMOTCPServerSingle::AcceptProc(SessionId sessionId)
{
	Character* newPlayer = CreateCharacter(sessionId);

	RegisterCharacter(newPlayer);
	SendNewCharacterCreate(newPlayer);
	SendExistingCharactersToNewCharacter(newPlayer);
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

void MMOTCPServerSingle::AddMovingCharacter(Character* target)
{
	if (target->movingIndex_ != -1)
	{
		target->isMove_ = true;
		return;
	}

	target->movingIndex_ = static_cast<int>(movingCharacters_.size());
	movingCharacters_.push_back(target);
	target->isMove_ = true;
}

void MMOTCPServerSingle::RemoveMovingCharacter(Character* target)
{
	if (target->movingIndex_ == -1)
	{
		target->isMove_ = false;
		return;
	}

	int removeIndex = target->movingIndex_;
	int lastIndex = static_cast<int>(movingCharacters_.size()) - 1;

	if (removeIndex != lastIndex)
	{
		Character* lastCharacter = movingCharacters_[lastIndex];
		movingCharacters_[removeIndex] = lastCharacter;
		lastCharacter->movingIndex_ = removeIndex;
	}

	movingCharacters_.pop_back();

	target->movingIndex_ = -1;
	target->isMove_ = false;
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

void MMOTCPServerSingle::MakePacketDeleteCharacterRemoveSector(CPacket* packet, SectorAround* around, unsigned int id)
{
	*packet << PacketScDeleteCharacter << id;
	SendPacketToSectors(packet, around);
}

void MMOTCPServerSingle::MakePacketCreateCharacterAddSector(CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
	*packet << PacketScCreateOtherCharacter << id << direction << x << y << hp;
	SendPacketToSectors(packet, around);
}

void MMOTCPServerSingle::MakePacketMoveStartAddSector(CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y)
{
	*packet << PacketScMoveStart << id << direction << x << y;
	SendPacketToSectors(packet, around);
}

void MMOTCPServerSingle::MakePacketDeleteCharacterForMe(Character* target, CPacket* packet, unsigned int id)
{
	*packet << PacketScDeleteCharacter << id;
	SendPacketUnicast(target, packet);

}

void MMOTCPServerSingle::MakePacketSync(Character* target, CPacket* packet, unsigned int id, unsigned short x, unsigned short y)
{
	*packet << PacketScSync << id << x << y;
	SendPacketUnicast(target, packet);
}


void MMOTCPServerSingle::SendPacketUnicast(Character* target, CPacket* packet)
{
	SendPacket(target->sessionId_, packet);
}

void MMOTCPServerSingle::SendPacketAround(Character* target, CPacket* packet, bool sendMe)
{

	Profile profile(L"SendPacketAround");

	SectorAround around;

	GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &around);

	SessionId exceptSessionId;

	if (sendMe == true)
	{
		exceptSessionId = InvalidSessionId;
	}
	else
	{
		exceptSessionId = target->sessionId_;
	}

	SendPacketToSectors(packet, &around, exceptSessionId);

}

void MMOTCPServerSingle::SendPacketToSectors(CPacket* packet, SectorAround* around, SessionId exceptSessionId)
{
	Profile profile(L"SendPacketToSectors");
	for (unsigned int index = 0; index < around->count_; ++index)
	{
		SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, exceptSessionId, packet);
	}
}

void MMOTCPServerSingle::GetSectorAroundForHit(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
{
	int sectorX = target->characterSectorPos_.x_;
	int sectorY = target->characterSectorPos_.y_;

	aroundSector->count_ = 0;

	AddSectorPosition(aroundSector, sectorX, sectorY);

	int targetValidPosX;
	int targetValidPosYAbove = (target->y_ - boundaryY) / SectorYSize;
	int targetValidPosYBelow = (target->y_ + boundaryY) / SectorYSize;

	if (target->direction_ == PacketMoveDirectionLL)
	{
		targetValidPosX = (target->x_ - boundaryX) / SectorXSize;

		if (sectorX - 1 >= 0 && targetValidPosX != sectorX)
		{
			AddSectorPosition(aroundSector, sectorX - 1, sectorY);
		}
	}
	else
	{
		targetValidPosX = (target->x_ + boundaryX) / SectorXSize;

		if (sectorX + 1 < SectorMaxX && targetValidPosX != sectorX)
		{
			AddSectorPosition(aroundSector, sectorX + 1, sectorY);
		}
	}

	if (sectorY + 1 < SectorMaxY && targetValidPosYBelow != sectorY)
	{
		AddSectorPosition(aroundSector, sectorX, sectorY + 1);
	}

	if (sectorY - 1 >= 0 && targetValidPosYAbove != sectorY)
	{
		AddSectorPosition(aroundSector, sectorX, sectorY - 1);
	}

	if (target->direction_ == PacketMoveDirectionLL)
	{
		if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
		{
			AddSectorPosition(aroundSector, sectorX - 1, sectorY + 1);
		}

		if (sectorY - 1 >= 0 && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
		{
			AddSectorPosition(aroundSector, sectorX - 1, sectorY - 1);
		}
	}
	else
	{
		if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
		{
			AddSectorPosition(aroundSector, sectorX + 1, sectorY + 1);
		}

		if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
		{
			AddSectorPosition(aroundSector, sectorX + 1, sectorY - 1);
		}
	}
}

void MMOTCPServerSingle::AddSectorPosition(SectorAround* aroundSector, int sectorX, int sectorY)
{
	aroundSector->around_[aroundSector->count_].x_ = sectorX;
	aroundSector->around_[aroundSector->count_].y_ = sectorY;
	aroundSector->count_++;
}

bool MMOTCPServerSingle::CanHitTarget(const Character* attackCharacter, const Character* target, int boundaryX, int boundaryY)
{
	if (attackCharacter == target)
	{
		return false;
	}

	if (attackCharacter->direction_ == PacketMoveDirectionLL)
	{
		if (attackCharacter->x_ < target->x_)
		{
			return false;
		}
	}
	else
	{
		if (attackCharacter->x_ > target->x_)
		{
			return false;
		}
	}

	if (abs(attackCharacter->x_ - target->x_) > boundaryX)
	{
		return false;
	}

	if (abs(attackCharacter->y_ - target->y_) > boundaryY)
	{
		return false;
	}

	return true;
}

void MMOTCPServerSingle::InitializeSectorUpdateAround()
{
	for (int oldSectorY = 0; oldSectorY < SectorMaxY; ++oldSectorY)
	{
		for (int oldSectorX = 0; oldSectorX < SectorMaxX; ++oldSectorX)
		{
			for (int moveSectorY = -1; moveSectorY <= 1; ++moveSectorY)
			{
				for (int moveSectorX = -1; moveSectorX <= 1; ++moveSectorX)
				{
					int moveIndexX = moveSectorX + 1;
					int moveIndexY = moveSectorY + 1;

					SectorUpdateAround* sectorUpdateAround = &sectorUpdateAround_[oldSectorY][oldSectorX][moveIndexY][moveIndexX];

					sectorUpdateAround->removeSector_.count_ = 0;
					sectorUpdateAround->addSector_.count_ = 0;

					if (moveSectorX == 0 && moveSectorY == 0)
					{
						continue;
					}

					int curSectorX = oldSectorX + moveSectorX;
					int curSectorY = oldSectorY + moveSectorY;

					if (curSectorX < 0 || curSectorX >= SectorMaxX)
					{
						continue;
					}

					if (curSectorY < 0 || curSectorY >= SectorMaxY)
					{
						continue;
					}

					BuildSectorUpdateAround(oldSectorX, oldSectorY, curSectorX, curSectorY, sectorUpdateAround);
				}
			}
		}
	}
}

void MMOTCPServerSingle::BuildSectorUpdateAround(int oldSectorX, int oldSectorY, int curSectorX, int curSectorY, SectorUpdateAround* sectorUpdateAround)
{
	SectorAround oldSectorAround;
	SectorAround curSectorAround;

	GetSectorAround(oldSectorX, oldSectorY, &oldSectorAround);
	GetSectorAround(curSectorX, curSectorY, &curSectorAround);

	sectorUpdateAround->removeSector_.count_ = 0;
	sectorUpdateAround->addSector_.count_ = 0;

	unsigned int removeIndex;

	for (unsigned int i = 0; i < oldSectorAround.count_; ++i)
	{
		for (removeIndex = 0; removeIndex < curSectorAround.count_; ++removeIndex)
		{
			if (oldSectorAround.around_[i].x_ == curSectorAround.around_[removeIndex].x_ && oldSectorAround.around_[i].y_ == curSectorAround.around_[removeIndex].y_)
			{
				break;
			}
		}

		if (removeIndex == curSectorAround.count_)
		{
			AddSectorPosition(&sectorUpdateAround->removeSector_, oldSectorAround.around_[i].x_, oldSectorAround.around_[i].y_);
		}
	}

	unsigned int addIndex;

	for (unsigned int i = 0; i < curSectorAround.count_; ++i)
	{
		for (addIndex = 0; addIndex < oldSectorAround.count_; ++addIndex)
		{
			if (curSectorAround.around_[i].x_ == oldSectorAround.around_[addIndex].x_ && curSectorAround.around_[i].y_ == oldSectorAround.around_[addIndex].y_)
			{
				break;
			}
		}

		if (addIndex == oldSectorAround.count_)
		{
			AddSectorPosition(&sectorUpdateAround->addSector_, curSectorAround.around_[i].x_, curSectorAround.around_[i].y_);
		}
	}
}

void MMOTCPServerSingle::SendPacketSectorOne(int sectorX, int sectorY, SessionId exceptSessionId, CPacket* packet)
{
	Character* target;
	std::list<Character*>::iterator iter;

	for (iter = sectorCharacterList_[sectorY][sectorX].begin(); iter != sectorCharacterList_[sectorY][sectorX].end(); ++iter)
	{
		target = *iter;

		if (target->sessionId_ == exceptSessionId)
		{
			continue;
		}
		SendPacket(target->sessionId_, packet);
	}
}




bool MMOTCPServerSingle::NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{

	Profile profile(L"NetPacketProcMoveStart");

	Character* target = characterMap_.at(sessionId);

	SyncOrApplyClientPosition(target, x, y);

	AddMovingCharacter(target);
	target->action_ = direction;

	UpdateCharacterFacingDirection(target, direction);

	CPacket* packetMoveStart = CPacket::Alloc();
	MakePacketMoveStart(target, packetMoveStart, static_cast<unsigned int>(target->sessionId_), direction, target->x_, target->y_);
	CPacket::Free(packetMoveStart);

	return true;
}

bool MMOTCPServerSingle::NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
	Profile profile(L"NetPacketProcMoveStop");
	Character* target = characterMap_.at(sessionId);

	SyncOrApplyClientPosition(target, x, y);

	RemoveMovingCharacter(target);

	target->action_ = direction;

	UpdateCharacterFacingDirection(target, direction);

	CPacket* packetMoveStop = CPacket::Alloc();
	MakePacketMoveStop(target, packetMoveStop, static_cast<unsigned int>(target->sessionId_), target->direction_, target->x_, target->y_);
	CPacket::Free(packetMoveStop);

	return true;
}

bool MMOTCPServerSingle::NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
	Profile profile(L"NetPacketProcAttack");
	Character* target = characterMap_.at(sessionId);

	SyncOrApplyClientPosition(target, x, y);


	target->direction_ = direction;

	CPacket* packetAttack = CPacket::Alloc();
	MakePacketAttack1(target, packetAttack, static_cast<unsigned int>(target->sessionId_), direction, target->x_, target->y_);
	CPacket::Free(packetAttack);
	HitCheck(target, 1);

	return true;
}
bool MMOTCPServerSingle::NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
	Profile profile(L"NetPacketProcAttack");
	Character* target = characterMap_.at(sessionId);

	SyncOrApplyClientPosition(target, x, y);

	target->direction_ = direction;

	CPacket* packetAttack = CPacket::Alloc();
	MakePacketAttack2(target, packetAttack, static_cast<unsigned int>(target->sessionId_), direction, target->x_, target->y_);
	CPacket::Free(packetAttack);
	HitCheck(target, 2);

	return true;
}

bool MMOTCPServerSingle::NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
	Profile profile(L"NetPacketProcAttack");
	Character* target = characterMap_.at(sessionId);

	SyncOrApplyClientPosition(target, x, y);

	target->direction_ = direction;

	CPacket* packetAttack = CPacket::Alloc();
	MakePacketAttack3(target, packetAttack, static_cast<unsigned int>(target->sessionId_), direction, target->x_, target->y_);
	CPacket::Free(packetAttack);
	HitCheck(target, 3);
	return true;
}

bool MMOTCPServerSingle::NetPacketProcEcho(SessionId sessionId, unsigned int time)
{
	Profile profile(L"NetPacketEcho");
	Character* target = characterMap_.at(sessionId);

	CPacket* packetEcho = CPacket::Alloc();
	MakePacketEcho(target, packetEcho, time);
	CPacket::Free(packetEcho);

	return true;
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

		sectorCharacterList_[target->oldSectorPos_.y_][target->oldSectorPos_.x_].remove(target);
		sectorCharacterList_[target->characterSectorPos_.y_][target->characterSectorPos_.x_].push_back(target);
		//printf("섹터 변경 \nOld : %d  %d Cur  : %d  %d \n" , target->OldSectorPos.x, target->OldSectorPos.y,targetCurPosX, targetCurPosY);



		return true;
	}

	return false;

}

void MMOTCPServerSingle::SectorUpdate(Character* target)
{

	Profile profile(L"SectorUpdate");

	SectorUpdateAround* updateAround = GetUpdateSectorAround(target);

	SendRemoveSectorUpdate(target, &updateAround->removeSector_);
	SendAddSectorUpdate(target, &updateAround->addSector_);

}

void MMOTCPServerSingle::SendRemoveSectorUpdate(Character* target, SectorAround* removeSector)
{
	CPacket* packetDeleteCharacterRemoveSector = CPacket::Alloc();
	MakePacketDeleteCharacterRemoveSector(packetDeleteCharacterRemoveSector, removeSector, static_cast<unsigned int>(target->sessionId_));
	CPacket::Free(packetDeleteCharacterRemoveSector);

	for (unsigned int i = 0; i < removeSector->count_; ++i)
	{
		std::list<Character*>::iterator iter;

		for (iter = sectorCharacterList_[removeSector->around_[i].y_][removeSector->around_[i].x_].begin(); iter != sectorCharacterList_[removeSector->around_[i].y_][removeSector->around_[i].x_].end(); ++iter)
		{
			CPacket* packetDeleteCharacterForMe = CPacket::Alloc();
			MakePacketDeleteCharacterForMe(target, packetDeleteCharacterForMe, static_cast<unsigned int>((*iter)->sessionId_));
			CPacket::Free(packetDeleteCharacterForMe);
		}
	}
}

void MMOTCPServerSingle::SendAddSectorUpdate(Character* target, SectorAround* addSector)
{
	CPacket* packetCreateCharacterAddSector = CPacket::Alloc();
	MakePacketCreateCharacterAddSector(packetCreateCharacterAddSector, addSector, static_cast<unsigned int>(target->sessionId_), target->direction_, target->x_, target->y_, target->hp_);
	CPacket::Free(packetCreateCharacterAddSector);

	CPacket* packetMoveStartAddSector = CPacket::Alloc();
	MakePacketMoveStartAddSector(packetMoveStartAddSector, addSector, static_cast<unsigned int>(target->sessionId_), target->action_, target->x_, target->y_);
	CPacket::Free(packetMoveStartAddSector);

	for (unsigned int i = 0; i < addSector->count_; ++i)
	{
		std::list<Character*>::iterator iter;

		for (iter = sectorCharacterList_[addSector->around_[i].y_][addSector->around_[i].x_].begin(); iter != sectorCharacterList_[addSector->around_[i].y_][addSector->around_[i].x_].end(); ++iter)
		{
			Character* createCharacter = *iter;

			if (createCharacter->sessionId_ == target->sessionId_)
			{
				continue;
			}

			CPacket* packetCreateOtherCharacterForMe = CPacket::Alloc();
			MakePacketCreateOtherCharacterForMe(target, packetCreateOtherCharacterForMe, static_cast<unsigned int>(createCharacter->sessionId_), createCharacter->direction_, createCharacter->x_, createCharacter->y_, createCharacter->hp_);
			CPacket::Free(packetCreateOtherCharacterForMe);

			if (createCharacter->isMove_ == true)
			{
				CPacket* packetMoveStartForMe = CPacket::Alloc();
				MakePacketMoveStartForMe(target, packetMoveStartForMe, static_cast<unsigned int>(createCharacter->sessionId_), createCharacter->action_, createCharacter->x_, createCharacter->y_);
				CPacket::Free(packetMoveStartForMe);
			}
		}
	}
}




void MMOTCPServerSingle::HitCheck(Character* attackCharacter, int attackNumber)
{
	Profile profile(L"HitCheck");
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

	SectorAround hitCheckSector;

	GetSectorAroundForHit(attackCharacter, boundaryX, boundaryY, &hitCheckSector);

	for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
	{
		int sectorX = hitCheckSector.around_[i].x_;
		int sectorY = hitCheckSector.around_[i].y_;

		std::list<Character*>::iterator iter;

		for (iter = sectorCharacterList_[sectorY][sectorX].begin(); iter != sectorCharacterList_[sectorY][sectorX].end(); ++iter)
		{
			Character* target = *iter;

			if (CanHitTarget(attackCharacter, target, boundaryX, boundaryY) == false)
			{
				continue;
			}

			if (damage >= target->hp_)
			{
				target->hp_ = 0;
			}
			else
			{
				target->hp_ -= damage;
			}

			CPacket* packetDamage = CPacket::Alloc();
			MakePacketDamage(target, packetDamage, static_cast<unsigned int>(attackCharacter->sessionId_), static_cast<unsigned int>(target->sessionId_), target->hp_);
			CPacket::Free(packetDamage);

			if (target->hp_ == 0)
			{
				Disconnect(target->sessionId_);
			}

			return;
		}
	}


}

void MMOTCPServerSingle::GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
{

	aroundSector->count_ = 0;

	AddSectorPosition(aroundSector, sectorX, sectorY);

	if (sectorX + 1 < SectorMaxX)
	{
		AddSectorPosition(aroundSector, sectorX + 1, sectorY);
	}

	if (sectorX - 1 >= 0)
	{
		AddSectorPosition(aroundSector, sectorX - 1, sectorY);
	}

	if (sectorY + 1 < SectorMaxY)
	{
		AddSectorPosition(aroundSector, sectorX, sectorY + 1);
	}

	if (sectorY - 1 >= 0)
	{
		AddSectorPosition(aroundSector, sectorX, sectorY - 1);
	}

	if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX)
	{
		AddSectorPosition(aroundSector, sectorX + 1, sectorY + 1);
	}

	if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX)
	{
		AddSectorPosition(aroundSector, sectorX + 1, sectorY - 1);
	}

	if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0)
	{
		AddSectorPosition(aroundSector, sectorX - 1, sectorY + 1);
	}

	if (sectorY - 1 >= 0 && sectorX - 1 >= 0)
	{
		AddSectorPosition(aroundSector, sectorX - 1, sectorY - 1);
	}

}


SectorUpdateAround* MMOTCPServerSingle::GetUpdateSectorAround(Character* target)
{
	Profile profile(L"GetUpdateSectorAround");

	int moveIndexX = static_cast<int>(target->characterSectorPos_.x_) - static_cast<int>(target->oldSectorPos_.x_) + 1;
	int moveIndexY = static_cast<int>(target->characterSectorPos_.y_) - static_cast<int>(target->oldSectorPos_.y_) + 1;

	return &sectorUpdateAround_[target->oldSectorPos_.y_][target->oldSectorPos_.x_][moveIndexY][moveIndexX];
}




void MMOTCPServerSingle::Update()
{


	DWORD tick = timeGetTime();

	unsigned int frame = tick - oldTick_;
	if (frame > frameMs_)
	{
		unsigned int fixUpdate = (frame / 40);

		for (unsigned int fixedIndex = 0; fixedIndex < fixUpdate; ++fixedIndex)
		{
			for (unsigned int movingIndex = 0; movingIndex < movingCharacters_.size();)
			{
				Character* target = movingCharacters_[movingIndex];

				GameRun(target);

				if (target->movingIndex_ != -1)
				{
					++movingIndex;
				}
			}
		}

		oldTick_ += (frameMs_ * (frame / frameMs_));
	}

}




void MMOTCPServerSingle::GameRun(Character* target)
{
	switch (target->action_)
	{
	case PacketMoveDirectionLL:
		if (target->x_ - 6 < RangeMoveLeft)
		{
			RemoveMovingCharacter(target);
			return;
		}

		target->x_ -= 6;

		break;

	case PacketMoveDirectionLU:
		if (target->y_ - 4 < RangeMoveTop || target->x_ - 6 < RangeMoveLeft)
		{
			RemoveMovingCharacter(target);
			return;
		}

		target->x_ -= 6;
		target->y_ -= 4;

		break;



	case PacketMoveDirectionUU:
		if (target->y_ - 4 < RangeMoveTop)
		{
			RemoveMovingCharacter(target);
			return;
		}


		target->y_ -= 4;

		break;


	case PacketMoveDirectionRU:

		if ((target->y_ - 4 < RangeMoveTop) || (target->x_ + 6 >= RangeMoveRight))
		{
			RemoveMovingCharacter(target);
			return;
		}

		target->x_ += 6;
		target->y_ -= 4;

		break;


	case PacketMoveDirectionRR:

		if (target->x_ + 6 >= RangeMoveRight)
		{
			RemoveMovingCharacter(target);
			return;
		}
		target->x_ += 6;

		break;


	case PacketMoveDirectionRD:

		if (target->y_ + 4 >= RangeMoveBottom || target->x_ + 6 >= RangeMoveRight)
		{
			RemoveMovingCharacter(target);
			return;
		}


		target->x_ += 6;
		target->y_ += 4;

		break;



	case PacketMoveDirectionDD:
		if (target->y_ + 4 >= RangeMoveBottom)
		{
			RemoveMovingCharacter(target);
			return;
		}

		target->y_ += 4;

		break;

	case PacketMoveDirectionLD:
		if (target->y_ + 4 >= RangeMoveBottom || target->x_ - 6 < RangeMoveLeft)
		{
			RemoveMovingCharacter(target);
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
