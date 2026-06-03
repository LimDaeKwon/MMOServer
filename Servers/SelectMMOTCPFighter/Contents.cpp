#include "Contents.h"
#include "Character.h"
#include "Session.h"
#include "Sector.h"
#include "GameDefine.h"
#include "PacketDefine.h"
#include "Network.h"

#include "NetworkProxy.h"
#include "NetworkStub.h"
#include "CPacket.h"
#include <unordered_map>
#include "RingBuffer.h"
#include <list>
#include "ObjectFreeList.h"
#include "Windows.h"

std::list<Character*> Sector[SectorMaxY][SectorMaxX];
std::list<unsigned int> DeleteList;

std::unordered_map<unsigned int, Character*> CharacterMap;

//td::unordered_map<unsigned int, unsigned int> whydelete;

ObjectFreeList<Character> CharacterFreeList(10000);



CPacket* GlobalCPacket = CPacket::Alloc();

void CreateCharater(Session* NewSession)
{

	Character* NewPlayer = CharacterFreeList.Alloc();
	NewPlayer->characterSession_ = NewSession;
	NewPlayer->sessionId_ = NewSession->sessionId_;

	NewPlayer->direction_ = PacketMoveDirectionRR;
	NewPlayer->action_ = PacketMoveDirectionRR;

	NewPlayer->x_ = rand() % 6399;
	NewPlayer->y_ = rand() % 6399;



	NewPlayer->hp_ = DefaultHp;

	NewPlayer->characterSectorPos_.x_ = NewPlayer->x_ / SectorXSize;
	NewPlayer->characterSectorPos_.y_ = NewPlayer->y_ / SectorYSize;
	NewPlayer->oldSectorPos_.x_ = SectorMaxX;
	NewPlayer->oldSectorPos_.y_ = SectorMaxY;

	NewPlayer->isMove_ = false;

	Sector[NewPlayer->characterSectorPos_.y_][NewPlayer->characterSectorPos_.x_].push_back(NewPlayer);
	CharacterMap.insert(std::unordered_map<unsigned int, Character*>::value_type(NewPlayer->sessionId_, NewPlayer));



	GlobalCPacket->Clear();

	MakePacketCreateMyCharacter(NewPlayer->characterSession_, GlobalCPacket, NewPlayer->sessionId_, NewPlayer->direction_, NewPlayer->x_, NewPlayer->y_, NewPlayer->hp_);


	GlobalCPacket->Clear();
	MakePacketCreateOtherCharacter(NewPlayer->characterSession_, GlobalCPacket, NewPlayer->sessionId_, NewPlayer->direction_, NewPlayer->x_, NewPlayer->y_, NewPlayer->hp_);
	//나를 남에게.

	SectorAround CreateForMe;
	GetSectorAround(NewPlayer->characterSectorPos_.x_, NewPlayer->characterSectorPos_.y_, &CreateForMe);


	for (unsigned int i = 0; i < CreateForMe.count_; i++)
	{
		std::list<Character*>::iterator Iter;
		for (Iter = Sector[CreateForMe.around_[i].y_][CreateForMe.around_[i].x_].begin();
			Iter != Sector[CreateForMe.around_[i].y_][CreateForMe.around_[i].x_].end(); ++Iter)
		{
			Character* Target = *Iter;
			if ((Target->sessionId_ == NewPlayer->sessionId_) || (Target->characterSession_->isDelete_ == 1))
			{
				continue;
			}

			//CPacket OtherCharacter;
			GlobalCPacket->Clear();
			MakePacketCreateOtherCharacterForMe(NewPlayer->characterSession_, GlobalCPacket, Target->sessionId_, Target->direction_, Target->x_, Target->y_, Target->hp_);

			if (Target->isMove_ == true)
			{
				///CPacket Packet_SC_MOVE_START;
				GlobalCPacket->Clear();
				MakePacketMoveStartForMe(NewPlayer->characterSession_, GlobalCPacket, Target->sessionId_, Target->action_, Target->x_, Target->y_);
			}

		}
	}


}

unsigned int OldTick = timeGetTime();
static unsigned int OldTickforCheck = timeGetTime();
int GlobalLoop;

void Update()
{

	unsigned int Tick = timeGetTime();

	unsigned int Frame;
	
	GlobalLoop++;
	Frame = Tick - OldTick;

	static int Count = 0;

	//얘가 그냥 프레임. 
	//
	

	if (Frame >= 40)
	{
		int FixUpdate = (Frame / 40);
		std::unordered_map<unsigned int, Character*>::iterator Iter;
		for (Iter = CharacterMap.begin(); Iter != CharacterMap.end(); ++Iter)
		{
			Character* Target = Iter->second;

			if (Tick - Target->characterSession_->lastRecvTime_ > NetworkPacketRecvTimeout)
			{
				//printf("타임 웨잇으로 인한 종료당하기. %d %d \n\n", Target->CharacterSession->SessionID , Tick - Target->CharacterSession->LastRecvTime);
				//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, TIMEOUT));


				Disconnect(Target->characterSession_);
				continue;
			}
			
			if (Target->isMove_ && (Target->characterSession_->isDelete_ == 0))
			{
				for(int i = 0 ; i < FixUpdate; ++i)
				{
					GameRun(Target);
				}
				
			}

		}
		Count += (Frame / 40);
		OldTick += (40 * (Frame / 40));

		if (Tick - OldTickforCheck >= 1000)
		{
			if (Count > 25)
			{
				wprintf(L"FixedUpdate : %d   Loop : %d \n", FixUpdate, GlobalLoop);
				
			}
			Count = 0;
			OldTickforCheck += 1000;
			GlobalLoop = 0;
		}
	}

	DeleteDisconnect();
}

void GameRun(Character* Target)
{

	switch (Target->action_)
	{
	case PacketMoveDirectionLL:
		if (Target->x_ - 6 < RangeMoveLeft)
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ -= 6;

		break;

	case PacketMoveDirectionLU:
		if (Target->y_ - 4 < RangeMoveTop || Target->x_ - 6 < RangeMoveLeft)
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ -= 6 ;
		Target->y_ -= 4 ;

		break;



	case PacketMoveDirectionUU:
		if (Target->y_ - 4 < RangeMoveTop)
		{
			Target->isMove_ = false;
			return;
		}


		Target->y_ -= 4 ;

		break;


	case PacketMoveDirectionRU:

		if ((Target->y_ - 4 < RangeMoveTop) || (Target->x_ + 6 >= RangeMoveRight))
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ += 6;
		Target->y_ -= 4;

		break;


	case PacketMoveDirectionRR:

		if (Target->x_ + 6 >= RangeMoveRight)
		{
			Target->isMove_ = false;
			return;
		}
		Target->x_ += 6 ;

		break;


	case PacketMoveDirectionRD:

		if (Target->y_ + 4 >= RangeMoveBottom || Target->x_ + 6 >= RangeMoveRight)
		{
			Target->isMove_ = false;
			return;
		}


		Target->x_ += 6 ;
		Target->y_ += 4 ;

		break;



	case PacketMoveDirectionDD:
		if (Target->y_ + 4 >= RangeMoveBottom)
		{
			Target->isMove_ = false;
			return;
		}

		Target->y_ += 4 ;

		break;

	case PacketMoveDirectionLD:
		if (Target->y_ + 4 >= RangeMoveBottom || Target->x_ - 6 < RangeMoveLeft)
		{
			Target->isMove_ = false;
			return;
		}

		Target->x_ -= 6 ;
		Target->y_ += 4 ;

		break;
	}


	if (SectorUpdateCharacter(Target))
	{

		SectorUpdate(Target);

	}

}



void HitCheck(Character* AttackCharacter, int AttackNumber)
{

	int BoundaryX = 0;
	int BoundaryY = 0;

	int Damage = 0;



	switch (AttackNumber)
	{

	case 1:
		BoundaryX = Attack1RangeX;
		BoundaryY = Attack1RangeY;
		Damage = Attack1Damage;
		break;

	case 2:
		BoundaryX = Attack2RangeX;
		BoundaryY = Attack2RangeY;
		Damage = Attack2Damage;
		break;

	case 3:
		BoundaryX = Attack3RangeX;
		BoundaryY = Attack3RangeY;
		Damage = Attack3Damage;
		break;

	default:
		DebugBreak();
	}

	//섹터기준 처리로 바꾸기. 


	SectorAround HitCheckSector;

	if (AttackCharacter->direction_ == PacketMoveDirectionLL)
	{
		GetSectorAroundForHitLeft(AttackCharacter, BoundaryX, BoundaryY, &HitCheckSector);

		//PrintHitCheckSector(&HitCheckSector);

		for (unsigned int i = 0; i < HitCheckSector.count_; ++i)
		{
			std::list<Character*>::iterator Iter;
			for (Iter = Sector[HitCheckSector.around_[i].y_][HitCheckSector.around_[i].x_].begin(); Iter != Sector[HitCheckSector.around_[i].y_][HitCheckSector.around_[i].x_].end(); ++Iter)
			{
				Character* Target = *Iter;

				if ((AttackCharacter == Target) || (Target->characterSession_->isDelete_ == 1) || (AttackCharacter->x_ < Target->x_))
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

					GlobalCPacket->Clear();

					MakePacketDamage(Target->characterSession_, GlobalCPacket, AttackCharacter->sessionId_, Target->sessionId_, Target->hp_);

					if (Target->hp_ == 0)
					{
						Disconnect(Target->characterSession_);
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

				if ((AttackCharacter == Target) || (Target->characterSession_->isDelete_ == 1) || (AttackCharacter->x_ > Target->x_))
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

					GlobalCPacket->Clear();

					//wprintf(L"## SendDamage Packet : AttackID : %d TargetID %d  \n", AttackCharacter->SessionID, Target->SessionID);

					MakePacketDamage(Target->characterSession_, GlobalCPacket, AttackCharacter->sessionId_, Target->sessionId_, Target->hp_);
					//SendPacketBroadcast(NULL, &Pakcet_SC_DAMAGE);

					if (Target->hp_ == 0)
					{
						//printf(" HP가 0 이라 종료 당하는 녀석 : %d \n\n", Target->SessionID);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, TIMEOUT));

						Disconnect(Target->characterSession_);
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


bool NetPacketProcMoveStart(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->sessionId_);
	unsigned int ID;

	ID = Target->sessionId_;

	if (abs(Target->x_ - X) > ErrorRange || abs(Target->y_ - Y) > ErrorRange)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->characterSession_, GlobalCPacket, Target->sessionId_, Target->x_, Target->y_);


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



	Target->isMove_ = true;
	Target->action_ = Direction;

	switch (Direction)
	{
	case PacketMoveDirectionRR:
	case PacketMoveDirectionRU:
	case PacketMoveDirectionRD:
		Target->direction_ = PacketMoveDirectionRR;
		break;

	case PacketMoveDirectionLU:
	case PacketMoveDirectionLL:
	case PacketMoveDirectionLD:
		Target->direction_ = PacketMoveDirectionLL;
		break;

	}


	GlobalCPacket->Clear();
	MakePacketMoveStart(Target->characterSession_, GlobalCPacket, Target->sessionId_, Direction, Target->x_, Target->y_);

	return true;
}

bool NetPacketProcMoveStop(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->sessionId_);
	unsigned int ID;

	ID = Target->sessionId_;
	//여기서 처리. 클라의 좌표를 인정해준다.64

	//여기서 한번 돌려줘야함. run을.. 

	if ((abs(Target->x_ - X) > ErrorRange) || (abs(Target->y_ - Y) > ErrorRange))
	{
		//Disconnect(Target->CharacterSession);
		//wprintf(L"MoveStop OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
		GlobalCPacket->Clear();
		MakePacketSync(Target->characterSession_, GlobalCPacket, Target->sessionId_, Target->x_, Target->y_);

	}
	else
	{


		Target->x_ = X;
		Target->y_ = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
		}


	}

	//wprintf(L"## StopPacket : X  %d   Y  %d  \n", X, Y);

	Target->isMove_ = false;


	Target->action_ = Direction;

	switch (Direction)
	{
	case PacketMoveDirectionRR:
	case PacketMoveDirectionRU:
	case PacketMoveDirectionRD:
		Target->direction_ = PacketMoveDirectionRR;
		break;

	case PacketMoveDirectionLU:
	case PacketMoveDirectionLL:
	case PacketMoveDirectionLD:
		Target->direction_ = PacketMoveDirectionLL;
		break;

	}


	GlobalCPacket->Clear();

	MakePacketMoveStop(Target->characterSession_, GlobalCPacket, Target->sessionId_, Target->direction_, Target->x_, Target->y_);

	return true;
}

bool NetPacketProcAttack1(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->sessionId_);
	unsigned int ID;

	if (abs(Target->x_ - X) > ErrorRange || abs(Target->y_ - Y) > ErrorRange)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->characterSession_, GlobalCPacket, Target->sessionId_, Target->x_, Target->y_);


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



	ID = Target->sessionId_;
	Target->direction_ = Direction;

	GlobalCPacket->Clear();

	//wprintf(L"## SendAttack1 Packet : ID :  %d , TargetDir  : %d  \n", ID, Direction);
	MakePacketAttack1(Target->characterSession_, GlobalCPacket, ID, Direction, Target->x_, Target->y_);
	HitCheck(Target, 1);

	return true;
}

bool NetPacketProcAttack2(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->sessionId_);
	unsigned int ID;

	if (abs(Target->x_ - X) > ErrorRange || abs(Target->y_ - Y) > ErrorRange)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->characterSession_, GlobalCPacket, Target->sessionId_, Target->x_, Target->y_);


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


	ID = Target->sessionId_;
	Target->direction_ = Direction;

	GlobalCPacket->Clear();

	//wprintf(L"## SendAttack2 Packet : ID :  %d , TargetDir  : %d  \n", ID, Direction);
	MakePacketAttack2(Target->characterSession_, GlobalCPacket, ID, Direction, Target->x_, Target->y_);
	HitCheck(Target, 2);

	return true;
}

bool NetPacketProcAttack3(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{

	Character* Target = CharacterMap.at(TargetSession->sessionId_);

	if (abs(Target->x_ - X) > ErrorRange || abs(Target->y_ - Y) > ErrorRange)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->characterSession_, GlobalCPacket, Target->sessionId_, Target->x_, Target->y_);


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
	ID = Target->sessionId_;

	Target->direction_ = Direction;

	GlobalCPacket->Clear();

	//wprintf(L"## SendAttack3 Packet : ID :  %d , TargetDir  : %d  \n", ID, Direction);
	MakePacketAttack3(Target->characterSession_, GlobalCPacket, ID, Direction, Target->x_, Target->y_);
	HitCheck(Target, 3);
	return true;
}


bool NetPacketProcEcho(Session* Target, unsigned int Time)
{

	//CPacket Packet_SC_Echo;
	GlobalCPacket->Clear();

	MakePacketEcho(Target, GlobalCPacket, Time);

	return true;
}

void Disconnect(Session* TargetSession)
{
	if (TargetSession->isDelete_ == 1)
	{
		return;
	}

	DeleteList.push_back(TargetSession->sessionId_);
	TargetSession->isDelete_ = 1;

	Character* Target = CharacterMap.at(TargetSession->sessionId_);

	Sector[Target->characterSectorPos_.y_][Target->characterSectorPos_.x_].remove(Target);

	GlobalCPacket->Clear();
	MakePacketDeleteCharacter(TargetSession, GlobalCPacket, TargetSession->sessionId_);
	//wprintf(L"## Disconnect ID : %d \n", TargetSession->SessionID);

}


void GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector)
{
	//if(SectorX - 1)
	AroundSector->count_ = 0;

	AroundSector->around_[AroundSector->count_].x_ = SectorX;
	AroundSector->around_[AroundSector->count_].y_ = SectorY;
	AroundSector->count_++;



	if (SectorX + 1 < SectorMaxX)
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


	if (SectorY + 1 < SectorMaxY)
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

	if (SectorY + 1 < SectorMaxY && SectorX + 1 < SectorMaxX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < SectorMaxX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SectorMaxY && SectorX - 1 >= 0)
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

}

void GetSectorAroundForHitLeft(Character* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->characterSectorPos_.x_;
	int SectorY = Target->characterSectorPos_.y_;

	

	AroundSector->count_ = 0;

	AroundSector->around_[AroundSector->count_].x_ = SectorX;
	AroundSector->around_[AroundSector->count_].y_ = SectorY;
	AroundSector->count_++;

	int TargetValidPosX = ((Target->x_ - BoundaryX) / SectorXSize);
	int TargetValidPosYAbove = ((Target->y_ - BoundaryY) / SectorYSize);
	int TargetValidPosYBelow = ((Target->y_ + BoundaryY) / SectorYSize);
	

	
	if (SectorX - 1 >= 0 && TargetValidPosX != SectorX)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX - 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY;
		AroundSector->count_++;
	}
	
	if (SectorY + 1 < SectorMaxY && TargetValidPosYBelow != SectorY)
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

	if (SectorY + 1 < SectorMaxY && SectorX - 1 >= 0 && TargetValidPosX != SectorX && TargetValidPosYBelow != SectorY)
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

void GetSectorAroundForHitRight(Character* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->characterSectorPos_.x_;
	int SectorY = Target->characterSectorPos_.y_;

	AroundSector->count_ = 0;

	AroundSector->around_[AroundSector->count_].x_ = SectorX;
	AroundSector->around_[AroundSector->count_].y_ = SectorY;
	AroundSector->count_++;

	int TargetValidPosX = ((Target->x_ + BoundaryX) / SectorXSize);
	int TargetValidPosYAbove = ((Target->y_ - BoundaryY) / SectorYSize);
	int TargetValidPosYBelow = ((Target->y_ + BoundaryY) / SectorYSize);

	if (SectorX + 1 < SectorMaxX && TargetValidPosX != SectorX )
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY;
		AroundSector->count_++;
	}

	if (SectorY + 1 < SectorMaxY && TargetValidPosYBelow != SectorY)
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

	if (SectorY + 1 < SectorMaxY && SectorX + 1 < SectorMaxX && TargetValidPosX != SectorX && TargetValidPosYBelow != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY + 1;
		AroundSector->count_++;
	}

	if (SectorY - 1 >= 0 && SectorX + 1 < SectorMaxX && TargetValidPosX != SectorX  && TargetValidPosYAbove != SectorY)
	{
		AroundSector->around_[AroundSector->count_].x_ = SectorX + 1;
		AroundSector->around_[AroundSector->count_].y_ = SectorY - 1;
		AroundSector->count_++;
	}

}


void GetUpdateSectorAround(Character* Target, SectorAround* RemoveSector, SectorAround* AddSector)
{
	//나중에 뺄 생각.



	RemoveSector->count_ = 0;
	AddSector->count_ = 0;

	// ->
	if ( (Target->characterSectorPos_.y_ == Target->oldSectorPos_.y_) && (Target->characterSectorPos_.x_ > Target->oldSectorPos_.x_))
	{
		
		if (Target->characterSectorPos_.y_ == 0)
		{
			if (Target->oldSectorPos_.x_ != 0)
			{
				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
				RemoveSector->count_++;
			}
			

			if (Target->characterSectorPos_.x_ + 1 != SectorMaxX)
			{
				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
				AddSector->count_++;
			}

			return;

		}

		if(Target->characterSectorPos_.y_ == SectorMaxY - 1)
		{
			if (Target->oldSectorPos_.x_ != 0)
			{
				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
				RemoveSector->count_++;
			}
			

			if (Target->characterSectorPos_.x_ + 1 != SectorMaxX)
			{
				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
				AddSector->count_++;

			}
			
			return;

		}

		if (Target->oldSectorPos_.x_ != 0)
		{
			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
			RemoveSector->count_++;

		}


		if (Target->characterSectorPos_.x_ + 1 != SectorMaxX)
		{
			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
			AddSector->count_++;
		}

		return;

	}
	

	

	//<-


	if ((Target->characterSectorPos_.y_ == Target->oldSectorPos_.y_) && (Target->characterSectorPos_.x_ < Target->oldSectorPos_.x_))
	{

		if (Target->characterSectorPos_.y_ == 0)
		{
			if (Target->characterSectorPos_.x_ != 0)
			{
				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
				AddSector->count_++;
			}


			if (Target->oldSectorPos_.x_ + 1 != SectorMaxX)
			{
				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
				RemoveSector->count_++;
			}


			return;

		}

		if (Target->characterSectorPos_.y_ == SectorMaxY - 1)
		{
			if (Target->characterSectorPos_.x_ != 0)
			{
				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
				AddSector->count_++;
			}


			if (Target->oldSectorPos_.x_ + 1 != SectorMaxX)
			{
				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
				RemoveSector->count_++;
			}

			return;

		}

		if (Target->characterSectorPos_.x_ != 0)
		{
			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
			AddSector->count_++;

		}


		if (Target->oldSectorPos_.x_ + 1 != SectorMaxX)
		{
			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
			RemoveSector->count_++;
		}

		return;

	}


	//위..
	if ((Target->characterSectorPos_.y_ < Target->oldSectorPos_.y_) && (Target->characterSectorPos_.x_ == Target->oldSectorPos_.x_))
	{

		if (Target->characterSectorPos_.x_ == 0)
		{
			if (Target->characterSectorPos_.y_ != 0)
			{
				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_;
				AddSector->count_++;
			}


			if (Target->oldSectorPos_.y_ + 1 != SectorMaxY)
			{
				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
				RemoveSector->count_++;
			}


			return;

		}


		if (Target->characterSectorPos_.x_ == SectorMaxX - 1)
		{
			if (Target->characterSectorPos_.y_ != 0)
			{
				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ ;
				AddSector->count_++;
			}


			if (Target->oldSectorPos_.y_ + 1 != SectorMaxY)
			{
				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ ;
				RemoveSector->count_++;
			}

			return;

		}

		if (Target->characterSectorPos_.y_ != 0)
		{
			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ ;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ - 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
			AddSector->count_++;

		}


		if (Target->oldSectorPos_.y_ + 1 != SectorMaxY)
		{
			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ + 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_;
			RemoveSector->count_++;
		}

		return;

	}


	//아래..
	if ((Target->characterSectorPos_.y_ > Target->oldSectorPos_.y_) && (Target->characterSectorPos_.x_ == Target->oldSectorPos_.x_))
	{

		if (Target->characterSectorPos_.x_ == 0)
		{
			if (Target->oldSectorPos_.y_ != 0)
			{
				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
				RemoveSector->count_++;

			}


			if (Target->characterSectorPos_.y_ + 1 != SectorMaxY)
			{
				

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_;
				AddSector->count_++;
			}


			return;

		}


		if (Target->characterSectorPos_.x_ == SectorMaxX - 1)
		{
			if (Target->oldSectorPos_.y_ != 0)
			{

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
				RemoveSector->count_++;

				RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
				RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_;
				RemoveSector->count_++;

			}


			if (Target->characterSectorPos_.y_ + 1 != SectorMaxY)
			{
				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
				AddSector->count_++;

				AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
				AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_;
				AddSector->count_++;
			}

			return;

		}

		if (Target->oldSectorPos_.y_ != 0)
		{
			
			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ - 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_ + 1;
			RemoveSector->count_++;

			RemoveSector->around_[RemoveSector->count_].y_ = Target->oldSectorPos_.y_ - 1;
			RemoveSector->around_[RemoveSector->count_].x_ = Target->oldSectorPos_.x_;
			RemoveSector->count_++;
		}


		if (Target->characterSectorPos_.y_ + 1 != SectorMaxY)
		{
			

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ + 1;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_;
			AddSector->count_++;

			AddSector->around_[AddSector->count_].y_ = Target->characterSectorPos_.y_ + 1;
			AddSector->around_[AddSector->count_].x_ = Target->characterSectorPos_.x_ - 1;
			AddSector->count_++;
		}

		return;

	}

			
	

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

}



bool SectorUpdateCharacter(Character* Target)
{
	int TargetCurPosX = (Target->x_ / SectorXSize);
	int TargetCurPosY = (Target->y_ / SectorYSize);

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
void SectorUpdate(Character* Target)
{

	SectorAround Remove;
	SectorAround Add;

	GetUpdateSectorAround(Target, &Remove, &Add);

	//PrintUpdateSector(&Remove, &Add);




	GlobalCPacket->Clear();

	MakePacketDeleteCharacterRemoveSector(Target->characterSession_, GlobalCPacket, &Remove, Target->sessionId_); //패킷 만들어서 Remove 시켜줘야하는데.. 



	//Remove에 있는 애들의 삭제를 나에게 보냄. 
	for (unsigned int i = 0; i < Remove.count_; i++)
	{
		std::list<Character*>::iterator Iter;
		for (Iter = Sector[Remove.around_[i].y_][Remove.around_[i].x_].begin(); Iter != Sector[Remove.around_[i].y_][Remove.around_[i].x_].end(); ++Iter)
		{
			GlobalCPacket->Clear();
			MakePacketDeleteCharacterForMe(Target->characterSession_, GlobalCPacket, (*Iter)->sessionId_);


			//printf("Remove에 있는 애들의 삭제를 나에게 보냄. 지움 당하는 아이디 %d  : 받는 아이디 %d \n\n", Target->CharacterSession->SessionID, (*Iter)->SessionID);

		}
	}


	//add에 있는 애들에게 나의 생성을 보냄. 
	GlobalCPacket->Clear();
	MakePacketCreateCharacterAddSector(Target->characterSession_, GlobalCPacket, &Add, Target->sessionId_, Target->direction_, Target->x_, Target->y_, Target->hp_);
	//이동 정보도 보내줘야함. 
	// 
	// 
	GlobalCPacket->Clear();
	MakePacketMoveStartAddSector(Target->characterSession_, GlobalCPacket, &Add, Target->sessionId_, Target->action_, Target->x_, Target->y_);



	for (unsigned int i = 0; i < Add.count_; i++)
	{
		std::list<Character*>::iterator IterCreate;
		for (IterCreate = Sector[Add.around_[i].y_][Add.around_[i].x_].begin();
			IterCreate != Sector[Add.around_[i].y_][Add.around_[i].x_].end(); ++IterCreate)
		{
			Character* CreateCharacter = *IterCreate;
			if ((CreateCharacter->sessionId_ == Target->sessionId_) || (CreateCharacter->characterSession_->isDelete_ == 1))
			{
				continue;
			}

			GlobalCPacket->Clear();

			MakePacketCreateOtherCharacterForMe(Target->characterSession_, GlobalCPacket, CreateCharacter->sessionId_, CreateCharacter->direction_, CreateCharacter->x_, CreateCharacter->y_, CreateCharacter->hp_);

			if (CreateCharacter->isMove_ == true)
			{

				GlobalCPacket->Clear();
				MakePacketMoveStartForMe(Target->characterSession_, GlobalCPacket, CreateCharacter->sessionId_, CreateCharacter->action_, CreateCharacter->x_, CreateCharacter->y_);
			}

		}
	}



}
void FreeCharacter(Character* Target)
{
	CharacterFreeList.Free(Target);
}
void PrintUpdateSector(SectorAround* RemoveSector, SectorAround* AddSector)
{
	wprintf(L"Add : ");
	for (unsigned int i = 0; i < AddSector->count_; i++)
	{
		wprintf(L"%d %d    ", AddSector->around_[i].x_, AddSector->around_[i].y_);

	}
	wprintf(L"\nRem : "); 

	for (unsigned int i = 0; i < RemoveSector->count_; i++)
	{
		wprintf(L"%d %d    ", RemoveSector->around_[i].x_, RemoveSector->around_[i].y_);

	}
	wprintf(L"\n\n");

}
void PrintHitCheckSector(SectorAround* HitCheckSector)
{

	wprintf(L"\nHitCheckSector : ");

	for (unsigned int i = 0; i < HitCheckSector->count_; i++)
	{
		wprintf(L"%d %d    ", HitCheckSector->around_[i].x_, HitCheckSector->around_[i].y_);

	}
	wprintf(L"\n\n");


}
//
//void SectorUpdate(CHARACTER* Target)
//{
//
//}
