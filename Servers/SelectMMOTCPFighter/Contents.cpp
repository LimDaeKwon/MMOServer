#include "Contents.h"
#include "Character.h"
#include "Session.h"


#include "Network.h"
#include "CPacket.h"

#include "GameDefine.h"

#include "NetworkProxy.h"
#include "NetworkStub.h"
#include "ObjectFreeList.h"
#include "PacketDefine.h"
#include "RingBuffer.h"
#include "Sector.h"


#include <Windows.h>

#include <list>
#include <unordered_map>

std::list<Character*> sector[SectorMaxY][SectorMaxX];
std::list<unsigned int> deleteList;

std::unordered_map<unsigned int, Character*> characterMap;

//td::unordered_map<unsigned int, unsigned int> whydelete;

ObjectFreeList<Character> characterFreeList(10000);



CPacket* globalCPacket = CPacket::Alloc();

void CreateCharacter(Session* newSession)
{

	Character* newPlayer = characterFreeList.Alloc();
	newPlayer->characterSession_ = newSession;
	newPlayer->sessionId_ = newSession->sessionId_;

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

	sector[newPlayer->characterSectorPos_.y_][newPlayer->characterSectorPos_.x_].push_back(newPlayer);
	characterMap.insert(std::unordered_map<unsigned int, Character*>::value_type(newPlayer->sessionId_, newPlayer));



	globalCPacket->Clear();

	MakePacketCreateMyCharacter(newPlayer->characterSession_, globalCPacket, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);


	globalCPacket->Clear();
	MakePacketCreateOtherCharacter(newPlayer->characterSession_, globalCPacket, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
	//나를 남에게.

	SectorAround createForMe;
	GetSectorAround(newPlayer->characterSectorPos_.x_, newPlayer->characterSectorPos_.y_, &createForMe);


	for (unsigned int i = 0; i < createForMe.count_; i++)
	{
		std::list<Character*>::iterator iter;
		for (iter = sector[createForMe.around_[i].y_][createForMe.around_[i].x_].begin();
			iter != sector[createForMe.around_[i].y_][createForMe.around_[i].x_].end(); ++iter)
		{
			Character* target = *iter;
			if ((target->sessionId_ == newPlayer->sessionId_) || (target->characterSession_->isDelete_ == 1))
			{
				continue;
			}

			//CPacket OtherCharacter;
			globalCPacket->Clear();
			MakePacketCreateOtherCharacterForMe(newPlayer->characterSession_, globalCPacket, target->sessionId_, target->direction_, target->x_, target->y_, target->hp_);

			if (target->isMove_ == true)
			{
				///CPacket Packet_SC_MOVE_START;
				globalCPacket->Clear();
				MakePacketMoveStartForMe(newPlayer->characterSession_, globalCPacket, target->sessionId_, target->action_, target->x_, target->y_);
			}

		}
	}


}

unsigned int oldTick = timeGetTime();
static unsigned int oldTickForCheck = timeGetTime();
int globalLoop;

void Update()
{

	unsigned int tick = timeGetTime();

	unsigned int frame;

	globalLoop++;
	frame = tick - oldTick;

	static int count = 0;

	//얘가 그냥 프레임. 
	//


	if (frame >= 40)
	{
		int fixUpdate = (frame / 40);
		std::unordered_map<unsigned int, Character*>::iterator iter;
		for (iter = characterMap.begin(); iter != characterMap.end(); ++iter)
		{
			Character* target = iter->second;

			if (tick - target->characterSession_->lastRecvTime_ > NetworkPacketRecvTimeout)
			{
				//printf("타임 웨잇으로 인한 종료당하기. %d %d \n\n", target->CharacterSession->Sessionid , tick - target->CharacterSession->LastRecvtime);
				//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(target->CharacterSession->Sessionid, TIMEOUT));


				Disconnect(target->characterSession_);
				continue;
			}

			if (target->isMove_ && (target->characterSession_->isDelete_ == 0))
			{
				for (int i = 0; i < fixUpdate; ++i)
				{
					GameRun(target);
				}

			}

		}
		count += (frame / 40);
		oldTick += (40 * (frame / 40));

		if (tick - oldTickForCheck >= 1000)
		{
			if (count > 25)
			{
				wprintf(L"FixedUpdate : %d   Loop : %d \n", fixUpdate, globalLoop);

			}
			count = 0;
			oldTickForCheck += 1000;
			globalLoop = 0;
		}
	}

	DeleteDisconnect();
}

void GameRun(Character* target)
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



void HitCheck(Character* attackCharacter, int attackNumber)
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

		//PrintHitCheckSector(&hitCheckSector);

		for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
		{
			std::list<Character*>::iterator iter;
			for (iter = sector[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].begin(); iter != sector[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].end(); ++iter)
			{
				Character* target = *iter;

				if ((attackCharacter == target) || (target->characterSession_->isDelete_ == 1) || (attackCharacter->x_ < target->x_))
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

					globalCPacket->Clear();

					MakePacketDamage(target->characterSession_, globalCPacket, attackCharacter->sessionId_, target->sessionId_, target->hp_);

					if (target->hp_ == 0)
					{
						Disconnect(target->characterSession_);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(target->CharacterSession->Sessionid, HP));

					}

					return;
				}

			}
		}


	}
	else
	{
		GetSectorAroundForHitRight(attackCharacter, boundaryX, boundaryY, &hitCheckSector);
		//PrintHitCheckSector(&hitCheckSector);
		for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
		{
			std::list<Character*>::iterator iter;
			for (iter = sector[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].begin(); iter != sector[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].end(); ++iter)
			{
				Character* target = *iter;

				if ((attackCharacter == target) || (target->characterSession_->isDelete_ == 1) || (attackCharacter->x_ > target->x_))
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

					globalCPacket->Clear();

					//wprintf(L"## Senddamage Packet : Attackid : %d targetid %d  \n", attackCharacter->Sessionid, target->Sessionid);

					MakePacketDamage(target->characterSession_, globalCPacket, attackCharacter->sessionId_, target->sessionId_, target->hp_);
					//SendPacketBroadcast(NULL, &Pakcet_SC_DAMAGE);

					if (target->hp_ == 0)
					{
						//printf(" HP가 0 이라 종료 당하는 녀석 : %d \n\n", target->Sessionid);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(target->CharacterSession->Sessionid, TIMEOUT));

						Disconnect(target->characterSession_);
					}

					return;
				}

			}
		}


	}


	//std::unordered_map<unsigned int, CHARACTER*>::iterator iter;
	//for (iter = characterMap.begin(); iter != characterMap.end(); ++iter)
	//{
	//	CHARACTER* target = iter->second;

	//	if ((attackCharacter == target) || target->CharacterSession->IsDelete == 1)
	//	{
	//		continue;
	//	}
	//	
	//}


}


bool NetPacketProcMoveStart(Session* targetSession, unsigned char direction, unsigned short x, unsigned short y)
{
	Character* target = characterMap.at(targetSession->sessionId_);
	unsigned int id;

	id = target->sessionId_;

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{
		//Disconnect(target->CharacterSession);


		globalCPacket->Clear();

		MakePacketSync(target->characterSession_, globalCPacket, target->sessionId_, target->x_, target->y_);


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


	globalCPacket->Clear();
	MakePacketMoveStart(target->characterSession_, globalCPacket, target->sessionId_, direction, target->x_, target->y_);

	return true;
}

bool NetPacketProcMoveStop(Session* targetSession, unsigned char direction, unsigned short x, unsigned short y)
{
	Character* target = characterMap.at(targetSession->sessionId_);
	unsigned int id;

	id = target->sessionId_;
	//여기서 처리. 클라의 좌표를 인정해준다.64

	//여기서 한번 돌려줘야함. run을.. 

	if ((abs(target->x_ - x) > ErrorRange) || (abs(target->y_ - y) > ErrorRange))
	{
		//Disconnect(target->CharacterSession);
		//wprintf(L"MoveStop OutOfRange  Server x y  :  %d   %d  /   Client x y  :   %d   %d  \n", target->x, target->y, x, y);
		globalCPacket->Clear();
		MakePacketSync(target->characterSession_, globalCPacket, target->sessionId_, target->x_, target->y_);

	}
	else
	{


		target->x_ = x;
		target->y_ = y;

		if (SectorUpdateCharacter(target))
		{
			SectorUpdate(target); // 지연처리 해주자.. 
		}


	}

	//wprintf(L"## StopPacket : x  %d   y  %d  \n", x, y);

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


	globalCPacket->Clear();

	MakePacketMoveStop(target->characterSession_, globalCPacket, target->sessionId_, target->direction_, target->x_, target->y_);

	return true;
}

bool NetPacketProcAttack1(Session* targetSession, unsigned char direction, unsigned short x, unsigned short y)
{
	Character* target = characterMap.at(targetSession->sessionId_);
	unsigned int id;

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{
		//Disconnect(target->CharacterSession);


		globalCPacket->Clear();

		MakePacketSync(target->characterSession_, globalCPacket, target->sessionId_, target->x_, target->y_);


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



	id = target->sessionId_;
	target->direction_ = direction;

	globalCPacket->Clear();

	//wprintf(L"## SendAttack1 Packet : id :  %d , targetDir  : %d  \n", id, direction);
	MakePacketAttack1(target->characterSession_, globalCPacket, id, direction, target->x_, target->y_);
	HitCheck(target, 1);

	return true;
}

bool NetPacketProcAttack2(Session* targetSession, unsigned char direction, unsigned short x, unsigned short y)
{
	Character* target = characterMap.at(targetSession->sessionId_);
	unsigned int id;

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{
		//Disconnect(target->CharacterSession);


		globalCPacket->Clear();

		MakePacketSync(target->characterSession_, globalCPacket, target->sessionId_, target->x_, target->y_);


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


	id = target->sessionId_;
	target->direction_ = direction;

	globalCPacket->Clear();

	//wprintf(L"## SendAttack2 Packet : id :  %d , targetDir  : %d  \n", id, direction);
	MakePacketAttack2(target->characterSession_, globalCPacket, id, direction, target->x_, target->y_);
	HitCheck(target, 2);

	return true;
}

bool NetPacketProcAttack3(Session* targetSession, unsigned char direction, unsigned short x, unsigned short y)
{

	Character* target = characterMap.at(targetSession->sessionId_);

	if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
	{
		//Disconnect(target->CharacterSession);


		globalCPacket->Clear();

		MakePacketSync(target->characterSession_, globalCPacket, target->sessionId_, target->x_, target->y_);


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
	id = target->sessionId_;

	target->direction_ = direction;

	globalCPacket->Clear();

	//wprintf(L"## SendAttack3 Packet : id :  %d , targetDir  : %d  \n", id, direction);
	MakePacketAttack3(target->characterSession_, globalCPacket, id, direction, target->x_, target->y_);
	HitCheck(target, 3);
	return true;
}


bool NetPacketProcEcho(Session* target, unsigned int time)
{

	//CPacket Packet_SC_Echo;
	globalCPacket->Clear();

	MakePacketEcho(target, globalCPacket, time);

	return true;
}

void Disconnect(Session* targetSession)
{
	if (targetSession->isDelete_ == 1)
	{
		return;
	}

	deleteList.push_back(targetSession->sessionId_);
	targetSession->isDelete_ = 1;

	Character* target = characterMap.at(targetSession->sessionId_);

	sector[target->characterSectorPos_.y_][target->characterSectorPos_.x_].remove(target);

	globalCPacket->Clear();
	MakePacketDeleteCharacter(targetSession, globalCPacket, targetSession->sessionId_);
	//wprintf(L"## Disconnect id : %d \n", targetSession->Sessionid);

}


void GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
{
	//if(sectorX - 1)
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

}

void GetSectorAroundForHitLeft(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
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

void GetSectorAroundForHitRight(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
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


void GetUpdateSectorAround(Character* target, SectorAround* removeSector, SectorAround* addSector)
{
	//나중에 뺄 생각.



	removeSector->count_ = 0;
	addSector->count_ = 0;

	// ->
	if ((target->characterSectorPos_.y_ == target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ > target->oldSectorPos_.x_))
	{

		if (target->characterSectorPos_.y_ == 0)
		{
			if (target->oldSectorPos_.x_ != 0)
			{
				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
				removeSector->count_++;
			}


			if (target->characterSectorPos_.x_ + 1 != SectorMaxX)
			{
				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
				addSector->count_++;
			}

			return;

		}

		if (target->characterSectorPos_.y_ == SectorMaxY - 1)
		{
			if (target->oldSectorPos_.x_ != 0)
			{
				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
				removeSector->count_++;
			}


			if (target->characterSectorPos_.x_ + 1 != SectorMaxX)
			{
				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
				addSector->count_++;

			}

			return;

		}

		if (target->oldSectorPos_.x_ != 0)
		{
			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
			removeSector->count_++;

		}


		if (target->characterSectorPos_.x_ + 1 != SectorMaxX)
		{
			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
			addSector->count_++;
		}

		return;

	}




	//<-


	if ((target->characterSectorPos_.y_ == target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ < target->oldSectorPos_.x_))
	{

		if (target->characterSectorPos_.y_ == 0)
		{
			if (target->characterSectorPos_.x_ != 0)
			{
				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
				addSector->count_++;
			}


			if (target->oldSectorPos_.x_ + 1 != SectorMaxX)
			{
				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
				removeSector->count_++;
			}


			return;

		}

		if (target->characterSectorPos_.y_ == SectorMaxY - 1)
		{
			if (target->characterSectorPos_.x_ != 0)
			{
				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
				addSector->count_++;
			}


			if (target->oldSectorPos_.x_ + 1 != SectorMaxX)
			{
				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
				removeSector->count_++;
			}

			return;

		}

		if (target->characterSectorPos_.x_ != 0)
		{
			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
			addSector->count_++;

		}


		if (target->oldSectorPos_.x_ + 1 != SectorMaxX)
		{
			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
			removeSector->count_++;
		}

		return;

	}


	//위..
	if ((target->characterSectorPos_.y_ < target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ == target->oldSectorPos_.x_))
	{

		if (target->characterSectorPos_.x_ == 0)
		{
			if (target->characterSectorPos_.y_ != 0)
			{
				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
				addSector->count_++;
			}


			if (target->oldSectorPos_.y_ + 1 != SectorMaxY)
			{
				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
				removeSector->count_++;
			}


			return;

		}


		if (target->characterSectorPos_.x_ == SectorMaxX - 1)
		{
			if (target->characterSectorPos_.y_ != 0)
			{
				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
				addSector->count_++;
			}


			if (target->oldSectorPos_.y_ + 1 != SectorMaxY)
			{
				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
				removeSector->count_++;
			}

			return;

		}

		if (target->characterSectorPos_.y_ != 0)
		{
			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
			addSector->count_++;

		}


		if (target->oldSectorPos_.y_ + 1 != SectorMaxY)
		{
			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
			removeSector->count_++;
		}

		return;

	}


	//아래..
	if ((target->characterSectorPos_.y_ > target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ == target->oldSectorPos_.x_))
	{

		if (target->characterSectorPos_.x_ == 0)
		{
			if (target->oldSectorPos_.y_ != 0)
			{
				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
				removeSector->count_++;

			}


			if (target->characterSectorPos_.y_ + 1 != SectorMaxY)
			{


				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
				addSector->count_++;
			}


			return;

		}


		if (target->characterSectorPos_.x_ == SectorMaxX - 1)
		{
			if (target->oldSectorPos_.y_ != 0)
			{

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
				removeSector->count_++;

				removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
				removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
				removeSector->count_++;

			}


			if (target->characterSectorPos_.y_ + 1 != SectorMaxY)
			{
				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
				addSector->count_++;

				addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
				addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
				addSector->count_++;
			}

			return;

		}

		if (target->oldSectorPos_.y_ != 0)
		{

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
			removeSector->count_++;

			removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
			removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
			removeSector->count_++;
		}


		if (target->characterSectorPos_.y_ + 1 != SectorMaxY)
		{


			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
			addSector->count_++;

			addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
			addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
			addSector->count_++;
		}

		return;

	}




	SectorAround oldSectorAround;
	SectorAround curSectorAround;

	GetSectorAround(target->oldSectorPos_.x_, target->oldSectorPos_.y_, &oldSectorAround);
	GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &curSectorAround);


	unsigned int removeIndex;
	for (unsigned int i = 0; i < oldSectorAround.count_; i++)
	{
		for (removeIndex = 0; removeIndex < curSectorAround.count_; removeIndex++)
		{
			if (oldSectorAround.around_[i].x_ == curSectorAround.around_[removeIndex].x_ && oldSectorAround.around_[i].y_ == curSectorAround.around_[removeIndex].y_)
			{
				break;
			}
		}
		if (removeIndex == curSectorAround.count_)
		{
			removeSector->around_[removeSector->count_].x_ = oldSectorAround.around_[i].x_;
			removeSector->around_[removeSector->count_].y_ = oldSectorAround.around_[i].y_;
			removeSector->count_++;
		}
	}

	addSector->count_ = 0;
	unsigned int j;
	for (unsigned int i = 0; i < curSectorAround.count_; i++)
	{
		for (j = 0; j < oldSectorAround.count_; j++)
		{
			if (oldSectorAround.around_[j].x_ == curSectorAround.around_[i].x_ && oldSectorAround.around_[j].y_ == curSectorAround.around_[i].y_)
			{
				break;
			}
		}
		if (j == oldSectorAround.count_)
		{
			addSector->around_[addSector->count_].x_ = curSectorAround.around_[i].x_;
			addSector->around_[addSector->count_].y_ = curSectorAround.around_[i].y_;
			addSector->count_++;
		}
	}

}



bool SectorUpdateCharacter(Character* target)
{
	int targetcurSectorAroundPosX = (target->x_ / SectorXSize);
	int targetcurSectorAroundPosY = (target->y_ / SectorYSize);

	if ((target->characterSectorPos_.x_ != targetcurSectorAroundPosX) || (target->characterSectorPos_.y_ != targetcurSectorAroundPosY))
	{

		target->oldSectorPos_.x_ = target->characterSectorPos_.x_;
		target->oldSectorPos_.y_ = target->characterSectorPos_.y_;
		target->characterSectorPos_.x_ = targetcurSectorAroundPosX;
		target->characterSectorPos_.y_ = targetcurSectorAroundPosY;

		sector[target->oldSectorPos_.y_][target->oldSectorPos_.x_].remove(target);
		sector[target->characterSectorPos_.y_][target->characterSectorPos_.x_].push_back(target);
		//printf("섹터 변경 \noldSectorAround : %d  %d curSectorAround  : %d  %d \n" , target->oldSectorAroundSectorPos.x, target->oldSectorAroundSectorPos.y,targetcurSectorAroundPosX, targetcurSectorAroundPosY);



		return true;
	}

	return false;
}
void SectorUpdate(Character* target)
{

	SectorAround removeSector;
	SectorAround addSector;

	GetUpdateSectorAround(target, &removeSector, &addSector);

	//PrintUpdateSector(&removeSector, &addSector);




	globalCPacket->Clear();

	MakePacketDeleteCharacterRemoveSector(target->characterSession_, globalCPacket, &removeSector, target->sessionId_); //패킷 만들어서 removeSector 시켜줘야하는데.. 



	//removeSector에 있는 애들의 삭제를 나에게 보냄. 
	for (unsigned int i = 0; i < removeSector.count_; i++)
	{
		std::list<Character*>::iterator iter;
		for (iter = sector[removeSector.around_[i].y_][removeSector.around_[i].x_].begin(); iter != sector[removeSector.around_[i].y_][removeSector.around_[i].x_].end(); ++iter)
		{
			globalCPacket->Clear();
			MakePacketDeleteCharacterForMe(target->characterSession_, globalCPacket, (*iter)->sessionId_);


			//printf("removeSector에 있는 애들의 삭제를 나에게 보냄. 지움 당하는 아이디 %d  : 받는 아이디 %d \n\n", target->CharacterSession->Sessionid, (*iter)->Sessionid);

		}
	}


	//add에 있는 애들에게 나의 생성을 보냄. 
	globalCPacket->Clear();
	MakePacketCreateCharacterAddSector(target->characterSession_, globalCPacket, &addSector, target->sessionId_, target->direction_, target->x_, target->y_, target->hp_);
	//이동 정보도 보내줘야함. 
	// 
	// 
	globalCPacket->Clear();
	MakePacketMoveStartAddSector(target->characterSession_, globalCPacket, &addSector, target->sessionId_, target->action_, target->x_, target->y_);



	for (unsigned int i = 0; i < addSector.count_; i++)
	{
		std::list<Character*>::iterator iterCreate;
		for (iterCreate = sector[addSector.around_[i].y_][addSector.around_[i].x_].begin();
			iterCreate != sector[addSector.around_[i].y_][addSector.around_[i].x_].end(); ++iterCreate)
		{
			Character* createCharacter = *iterCreate;
			if ((createCharacter->sessionId_ == target->sessionId_) || (createCharacter->characterSession_->isDelete_ == 1))
			{
				continue;
			}

			globalCPacket->Clear();

			MakePacketCreateOtherCharacterForMe(target->characterSession_, globalCPacket, createCharacter->sessionId_, createCharacter->direction_, createCharacter->x_, createCharacter->y_, createCharacter->hp_);

			if (createCharacter->isMove_ == true)
			{

				globalCPacket->Clear();
				MakePacketMoveStartForMe(target->characterSession_, globalCPacket, createCharacter->sessionId_, createCharacter->action_, createCharacter->x_, createCharacter->y_);
			}

		}
	}



}
void FreeCharacter(Character* target)
{
	characterFreeList.Free(target);
}
void PrintUpdateSector(SectorAround* removeSector, SectorAround* addSector)
{
	wprintf(L"addSector : ");
	for (unsigned int i = 0; i < addSector->count_; i++)
	{
		wprintf(L"%d %d    ", addSector->around_[i].x_, addSector->around_[i].y_);

	}
	wprintf(L"\nRem : ");

	for (unsigned int i = 0; i < removeSector->count_; i++)
	{
		wprintf(L"%d %d    ", removeSector->around_[i].x_, removeSector->around_[i].y_);

	}
	wprintf(L"\n\n");

}
void PrintHitCheckSector(SectorAround* hitCheckSector)
{

	wprintf(L"\nhitCheckSector : ");

	for (unsigned int i = 0; i < hitCheckSector->count_; i++)
	{
		wprintf(L"%d %d    ", hitCheckSector->around_[i].x_, hitCheckSector->around_[i].y_);

	}
	wprintf(L"\n\n");


}
//
//void SectorUpdate(CHARACTER* target)
//{
//
//}
