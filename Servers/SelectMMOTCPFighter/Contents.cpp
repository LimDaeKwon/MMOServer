#include "Contents.h"
#include "Character.h"
#include "Session.h"
#include "Sector.h"

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

std::list<Character*> Sector[SECTORMAXY][SECTORMAXX];
std::list<unsigned int> DeleteList;

std::unordered_map<unsigned int, Character*> CharacterMap;

//td::unordered_map<unsigned int, unsigned int> whydelete;

ObjectFreeList<Character> CharacterFreeList(10000);



CPacket* GlobalCPacket = CPacket::Alloc();

void CreateCharater(Session* NewSession)
{

	Character* NewPlayer = CharacterFreeList.Alloc();
	NewPlayer->characterSession_ = NewSession;
	NewPlayer->sessionId_ = NewSession->SessionID;

	NewPlayer->direction_ = dfPACKET_MOVE_DIR_RR;
	NewPlayer->action_ = dfPACKET_MOVE_DIR_RR;

	NewPlayer->x_ = rand() % 6399;
	NewPlayer->y_ = rand() % 6399;



	NewPlayer->hp_ = DEFAULTHP;

	NewPlayer->characterSectorPos_.X = NewPlayer->x_ / SECTORXSIZE;
	NewPlayer->characterSectorPos_.Y = NewPlayer->y_ / SECTORYSIZE;
	NewPlayer->oldSectorPos_.X = SECTORMAXX;
	NewPlayer->oldSectorPos_.Y = SECTORMAXY;

	NewPlayer->isMove_ = false;

	Sector[NewPlayer->characterSectorPos_.Y][NewPlayer->characterSectorPos_.X].push_back(NewPlayer);
	CharacterMap.insert(std::unordered_map<unsigned int, Character*>::value_type(NewPlayer->sessionId_, NewPlayer));



	GlobalCPacket->Clear();

	MakePacketCreateMyCharacter(NewPlayer->characterSession_, GlobalCPacket, NewPlayer->sessionId_, NewPlayer->direction_, NewPlayer->x_, NewPlayer->y_, NewPlayer->hp_);


	GlobalCPacket->Clear();
	MakePacketCreateOtherCharacter(NewPlayer->characterSession_, GlobalCPacket, NewPlayer->sessionId_, NewPlayer->direction_, NewPlayer->x_, NewPlayer->y_, NewPlayer->hp_);
	//나를 남에게.

	SectorAround CreateForMe;
	GetSectorAround(NewPlayer->characterSectorPos_.X, NewPlayer->characterSectorPos_.Y, &CreateForMe);


	for (unsigned int i = 0; i < CreateForMe.Count; i++)
	{
		std::list<Character*>::iterator Iter;
		for (Iter = Sector[CreateForMe.Around[i].Y][CreateForMe.Around[i].X].begin();
			Iter != Sector[CreateForMe.Around[i].Y][CreateForMe.Around[i].X].end(); ++Iter)
		{
			Character* Target = *Iter;
			if ((Target->sessionId_ == NewPlayer->sessionId_) || (Target->characterSession_->IsDelete == 1))
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

			if (Tick - Target->characterSession_->LastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT)
			{
				//printf("타임 웨잇으로 인한 종료당하기. %d %d \n\n", Target->CharacterSession->SessionID , Tick - Target->CharacterSession->LastRecvTime);
				//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, TIMEOUT));


				Disconnect(Target->characterSession_);
				continue;
			}
			
			if (Target->isMove_ && (Target->characterSession_->IsDelete == 0))
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

		Target->x_ -= 6 ;
		Target->y_ -= 4 ;

		break;



	case dfPACKET_MOVE_DIR_UU:
		if (Target->y_ - 4 < dfRANGE_MOVE_TOP)
		{
			Target->isMove_ = false;
			return;
		}


		Target->y_ -= 4 ;

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
		Target->x_ += 6 ;

		break;


	case dfPACKET_MOVE_DIR_RD:

		if (Target->y_ + 4 >= dfRANGE_MOVE_BOTTOM || Target->x_ + 6 >= dfRANGE_MOVE_RIGHT)
		{
			Target->isMove_ = false;
			return;
		}


		Target->x_ += 6 ;
		Target->y_ += 4 ;

		break;



	case dfPACKET_MOVE_DIR_DD:
		if (Target->y_ + 4 >= dfRANGE_MOVE_BOTTOM)
		{
			Target->isMove_ = false;
			return;
		}

		Target->y_ += 4 ;

		break;

	case dfPACKET_MOVE_DIR_LD:
		if (Target->y_ + 4 >= dfRANGE_MOVE_BOTTOM || Target->x_ - 6 < dfRANGE_MOVE_LEFT)
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

		for (unsigned int i = 0; i < HitCheckSector.Count; ++i)
		{
			std::list<Character*>::iterator Iter;
			for (Iter = Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].begin(); Iter != Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].end(); ++Iter)
			{
				Character* Target = *Iter;

				if ((AttackCharacter == Target) || (Target->characterSession_->IsDelete == 1) || (AttackCharacter->x_ < Target->x_))
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
		for (unsigned int i = 0; i < HitCheckSector.Count; ++i)
		{
			std::list<Character*>::iterator Iter;
			for (Iter = Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].begin(); Iter != Sector[HitCheckSector.Around[i].Y][HitCheckSector.Around[i].X].end(); ++Iter)
			{
				Character* Target = *Iter;

				if ((AttackCharacter == Target) || (Target->characterSession_->IsDelete == 1) || (AttackCharacter->x_ > Target->x_))
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


bool NetPacketProc_MoveStart(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	ID = Target->sessionId_;

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
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


	GlobalCPacket->Clear();
	MakePacketMoveStart(Target->characterSession_, GlobalCPacket, Target->sessionId_, Direction, Target->x_, Target->y_);

	return true;
}

bool NetPacketProc_MoveStop(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	ID = Target->sessionId_;
	//여기서 처리. 클라의 좌표를 인정해준다.64

	//여기서 한번 돌려줘야함. run을.. 

	if ((abs(Target->x_ - X) > dfERROR_RANGE) || (abs(Target->y_ - Y) > dfERROR_RANGE))
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


	GlobalCPacket->Clear();

	MakePacketMoveStop(Target->characterSession_, GlobalCPacket, Target->sessionId_, Target->direction_, Target->x_, Target->y_);

	return true;
}

bool NetPacketProc_Attack1(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
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

bool NetPacketProc_Attack2(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	Character* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
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

bool NetPacketProc_Attack3(Session* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{

	Character* Target = CharacterMap.at(TargetSession->SessionID);

	if (abs(Target->x_ - X) > dfERROR_RANGE || abs(Target->y_ - Y) > dfERROR_RANGE)
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


bool NetPacketProc_Echo(Session* Target, unsigned int Time)
{

	//CPacket Packet_SC_Echo;
	GlobalCPacket->Clear();

	MakePacketEcho(Target, GlobalCPacket, Time);

	return true;
}

void Disconnect(Session* TargetSession)
{
	if (TargetSession->IsDelete == 1)
	{
		return;
	}

	DeleteList.push_back(TargetSession->SessionID);
	TargetSession->IsDelete = 1;

	Character* Target = CharacterMap.at(TargetSession->SessionID);

	Sector[Target->characterSectorPos_.Y][Target->characterSectorPos_.X].remove(Target);

	GlobalCPacket->Clear();
	MakePacketDeleteCharacter(TargetSession, GlobalCPacket, TargetSession->SessionID);
	//wprintf(L"## Disconnect ID : %d \n", TargetSession->SessionID);

}


void GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector)
{
	//if(SectorX - 1)
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

}

void GetSectorAroundForHitLeft(Character* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->characterSectorPos_.X;
	int SectorY = Target->characterSectorPos_.Y;

	

	AroundSector->Count = 0;

	AroundSector->Around[AroundSector->Count].X = SectorX;
	AroundSector->Around[AroundSector->Count].Y = SectorY;
	AroundSector->Count++;

	int TargetValidPosX = ((Target->x_ - BoundaryX) / SECTORXSIZE);
	int TargetValidPosYAbove = ((Target->y_ - BoundaryY) / SECTORYSIZE);
	int TargetValidPosYBelow = ((Target->y_ + BoundaryY) / SECTORYSIZE);
	

	
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

void GetSectorAroundForHitRight(Character* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
{
	int SectorX = Target->characterSectorPos_.X;
	int SectorY = Target->characterSectorPos_.Y;

	AroundSector->Count = 0;

	AroundSector->Around[AroundSector->Count].X = SectorX;
	AroundSector->Around[AroundSector->Count].Y = SectorY;
	AroundSector->Count++;

	int TargetValidPosX = ((Target->x_ + BoundaryX) / SECTORXSIZE);
	int TargetValidPosYAbove = ((Target->y_ - BoundaryY) / SECTORYSIZE);
	int TargetValidPosYBelow = ((Target->y_ + BoundaryY) / SECTORYSIZE);

	if (SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX )
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

	if (SectorY - 1 >= 0 && SectorX + 1 < SECTORMAXX && TargetValidPosX != SectorX  && TargetValidPosYAbove != SectorY)
	{
		AroundSector->Around[AroundSector->Count].X = SectorX + 1;
		AroundSector->Around[AroundSector->Count].Y = SectorY - 1;
		AroundSector->Count++;
	}

}


void GetUpdateSectorAround(Character* Target, SectorAround* RemoveSector, SectorAround* AddSector)
{
	//나중에 뺄 생각.



	RemoveSector->Count = 0;
	AddSector->Count = 0;

	// ->
	if ( (Target->characterSectorPos_.Y == Target->oldSectorPos_.Y) && (Target->characterSectorPos_.X > Target->oldSectorPos_.X))
	{
		
		if (Target->characterSectorPos_.Y == 0)
		{
			if (Target->oldSectorPos_.X != 0)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
				RemoveSector->Count++;
			}
			

			if (Target->characterSectorPos_.X + 1 != SECTORMAXX)
			{
				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
				AddSector->Count++;
			}

			return;

		}

		if(Target->characterSectorPos_.Y == SECTORMAXY - 1)
		{
			if (Target->oldSectorPos_.X != 0)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
				RemoveSector->Count++;
			}
			

			if (Target->characterSectorPos_.X + 1 != SECTORMAXX)
			{
				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
				AddSector->Count++;

			}
			
			return;

		}

		if (Target->oldSectorPos_.X != 0)
		{
			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
			RemoveSector->Count++;

		}


		if (Target->characterSectorPos_.X + 1 != SECTORMAXX)
		{
			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
			AddSector->Count++;
		}

		return;

	}
	

	

	//<-


	if ((Target->characterSectorPos_.Y == Target->oldSectorPos_.Y) && (Target->characterSectorPos_.X < Target->oldSectorPos_.X))
	{

		if (Target->characterSectorPos_.Y == 0)
		{
			if (Target->characterSectorPos_.X != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
				AddSector->Count++;
			}


			if (Target->oldSectorPos_.X + 1 != SECTORMAXX)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
				RemoveSector->Count++;
			}


			return;

		}

		if (Target->characterSectorPos_.Y == SECTORMAXY - 1)
		{
			if (Target->characterSectorPos_.X != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
				AddSector->Count++;
			}


			if (Target->oldSectorPos_.X + 1 != SECTORMAXX)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
				RemoveSector->Count++;
			}

			return;

		}

		if (Target->characterSectorPos_.X != 0)
		{
			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
			AddSector->Count++;

		}


		if (Target->oldSectorPos_.X + 1 != SECTORMAXX)
		{
			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
			RemoveSector->Count++;
		}

		return;

	}


	//위..
	if ((Target->characterSectorPos_.Y < Target->oldSectorPos_.Y) && (Target->characterSectorPos_.X == Target->oldSectorPos_.X))
	{

		if (Target->characterSectorPos_.X == 0)
		{
			if (Target->characterSectorPos_.Y != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X;
				AddSector->Count++;
			}


			if (Target->oldSectorPos_.Y + 1 != SECTORMAXY)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
				RemoveSector->Count++;
			}


			return;

		}


		if (Target->characterSectorPos_.X == SECTORMAXX - 1)
		{
			if (Target->characterSectorPos_.Y != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X ;
				AddSector->Count++;
			}


			if (Target->oldSectorPos_.Y + 1 != SECTORMAXY)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X ;
				RemoveSector->Count++;
			}

			return;

		}

		if (Target->characterSectorPos_.Y != 0)
		{
			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X ;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
			AddSector->Count++;

		}


		if (Target->oldSectorPos_.Y + 1 != SECTORMAXY)
		{
			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X;
			RemoveSector->Count++;
		}

		return;

	}


	//아래..
	if ((Target->characterSectorPos_.Y > Target->oldSectorPos_.Y) && (Target->characterSectorPos_.X == Target->oldSectorPos_.X))
	{

		if (Target->characterSectorPos_.X == 0)
		{
			if (Target->oldSectorPos_.Y != 0)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
				RemoveSector->Count++;

			}


			if (Target->characterSectorPos_.Y + 1 != SECTORMAXY)
			{
				

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X;
				AddSector->Count++;
			}


			return;

		}


		if (Target->characterSectorPos_.X == SECTORMAXX - 1)
		{
			if (Target->oldSectorPos_.Y != 0)
			{

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X;
				RemoveSector->Count++;

			}


			if (Target->characterSectorPos_.Y + 1 != SECTORMAXY)
			{
				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X;
				AddSector->Count++;
			}

			return;

		}

		if (Target->oldSectorPos_.Y != 0)
		{
			
			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->oldSectorPos_.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->oldSectorPos_.X;
			RemoveSector->Count++;
		}


		if (Target->characterSectorPos_.Y + 1 != SECTORMAXY)
		{
			

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->characterSectorPos_.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->characterSectorPos_.X - 1;
			AddSector->Count++;
		}

		return;

	}

			
	

	SectorAround Old;
	SectorAround Cur;

	GetSectorAround(Target->oldSectorPos_.X, Target->oldSectorPos_.Y, &Old);
	GetSectorAround(Target->characterSectorPos_.X, Target->characterSectorPos_.Y, &Cur);


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

}



bool SectorUpdateCharacter(Character* Target)
{
	int TargetCurPosX = (Target->x_ / SECTORXSIZE);
	int TargetCurPosY = (Target->y_ / SECTORYSIZE);

	if ((Target->characterSectorPos_.X != TargetCurPosX) || (Target->characterSectorPos_.Y != TargetCurPosY))
	{

		Target->oldSectorPos_.X = Target->characterSectorPos_.X;
		Target->oldSectorPos_.Y = Target->characterSectorPos_.Y;
		Target->characterSectorPos_.X = TargetCurPosX;
		Target->characterSectorPos_.Y = TargetCurPosY;

		Sector[Target->oldSectorPos_.Y][Target->oldSectorPos_.X].remove(Target);
		Sector[Target->characterSectorPos_.Y][Target->characterSectorPos_.X].push_back(Target);
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
	for (unsigned int i = 0; i < Remove.Count; i++)
	{
		std::list<Character*>::iterator Iter;
		for (Iter = Sector[Remove.Around[i].Y][Remove.Around[i].X].begin(); Iter != Sector[Remove.Around[i].Y][Remove.Around[i].X].end(); ++Iter)
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



	for (unsigned int i = 0; i < Add.Count; i++)
	{
		std::list<Character*>::iterator IterCreate;
		for (IterCreate = Sector[Add.Around[i].Y][Add.Around[i].X].begin();
			IterCreate != Sector[Add.Around[i].Y][Add.Around[i].X].end(); ++IterCreate)
		{
			Character* CreateCharacter = *IterCreate;
			if ((CreateCharacter->sessionId_ == Target->sessionId_) || (CreateCharacter->characterSession_->IsDelete == 1))
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
	for (unsigned int i = 0; i < AddSector->Count; i++)
	{
		wprintf(L"%d %d    ", AddSector->Around[i].X, AddSector->Around[i].Y);

	}
	wprintf(L"\nRem : "); 

	for (unsigned int i = 0; i < RemoveSector->Count; i++)
	{
		wprintf(L"%d %d    ", RemoveSector->Around[i].X, RemoveSector->Around[i].Y);

	}
	wprintf(L"\n\n");

}
void PrintHitCheckSector(SectorAround* HitCheckSector)
{

	wprintf(L"\nHitCheckSector : ");

	for (unsigned int i = 0; i < HitCheckSector->Count; i++)
	{
		wprintf(L"%d %d    ", HitCheckSector->Around[i].X, HitCheckSector->Around[i].Y);

	}
	wprintf(L"\n\n");


}
//
//void SectorUpdate(CHARACTER* Target)
//{
//
//}
