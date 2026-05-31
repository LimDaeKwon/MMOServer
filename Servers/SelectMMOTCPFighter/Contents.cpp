#include "Contents.h"
#include "PacketDefine.h"
#include "Network.h"
#include "Windows.h"
#include "NetworkProxy.h"
#include "NetworkStub.h"
#include "CPacket.h"
#include <unordered_map>
#include "RingBuffer.h"
#include <list>
#include "ObjectFreeList.h"



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

struct SESSION
{
	unsigned int LastRecvTime;
	unsigned int SessionID;

	SOCKET Socket;
	RingBuffer SendQ;
	RingBuffer ReceiveQ;


	unsigned int IsDelete;


};


struct CHARACTER
{
	unsigned int SessionID;
	unsigned int Action;
	SESSION* CharacterSession;
	unsigned char Direction;

	short X;
	short Y;
	unsigned char HP;
	bool IsMove;

	SectorPos OldSectorPos;
	SectorPos CharacterSectorPos;


};

std::list<CHARACTER*> Sector[SECTORMAXY][SECTORMAXX];
std::list<unsigned int> DeleteList;

std::unordered_map<unsigned int, CHARACTER*> CharacterMap;

//td::unordered_map<unsigned int, unsigned int> whydelete;

ObjectFreeList<CHARACTER> CharacterFreeList(10000);



CPacket* GlobalCPacket = CPacket::Alloc();

void CreateCharater(SESSION* NewSession)
{

	CHARACTER* NewPlayer = CharacterFreeList.Alloc();
	NewPlayer->CharacterSession = NewSession;
	NewPlayer->SessionID = NewSession->SessionID;

	NewPlayer->Direction = dfPACKET_MOVE_DIR_RR;
	NewPlayer->Action = dfPACKET_MOVE_DIR_RR;
	/*NewPlayer->X = rand() % dfRANGE_MOVE_RIGHT;
	NewPlayer->Y = rand() % dfRANGE_MOVE_BOTTOM;*/

	NewPlayer->X = rand() % 6399;
	NewPlayer->Y = rand() % 6399;



	NewPlayer->HP = DEFAULTHP;

	NewPlayer->CharacterSectorPos.X = NewPlayer->X / SECTORXSIZE;
	NewPlayer->CharacterSectorPos.Y = NewPlayer->Y / SECTORYSIZE;
	NewPlayer->OldSectorPos.X = SECTORMAXX;
	NewPlayer->OldSectorPos.Y = SECTORMAXY;

	NewPlayer->IsMove = false;

	Sector[NewPlayer->CharacterSectorPos.Y][NewPlayer->CharacterSectorPos.X].push_back(NewPlayer);
	CharacterMap.insert(std::unordered_map<unsigned int, CHARACTER*>::value_type(NewPlayer->SessionID, NewPlayer));



	GlobalCPacket->Clear();

	MakePacketCreateMyCharacter(NewPlayer->CharacterSession, GlobalCPacket, NewPlayer->SessionID, NewPlayer->Direction, NewPlayer->X, NewPlayer->Y, NewPlayer->HP);


	GlobalCPacket->Clear();
	MakePacketCreateOtherCharacter(NewPlayer->CharacterSession, GlobalCPacket, NewPlayer->SessionID, NewPlayer->Direction, NewPlayer->X, NewPlayer->Y, NewPlayer->HP);
	//나를 남에게.

	SectorAround CreateForMe;
	GetSectorAround(NewPlayer->CharacterSectorPos.X, NewPlayer->CharacterSectorPos.Y, &CreateForMe);


	for (unsigned int i = 0; i < CreateForMe.Count; i++)
	{
		std::list<CHARACTER*>::iterator Iter;
		for (Iter = Sector[CreateForMe.Around[i].Y][CreateForMe.Around[i].X].begin();
			Iter != Sector[CreateForMe.Around[i].Y][CreateForMe.Around[i].X].end(); ++Iter)
		{
			CHARACTER* Target = *Iter;
			if ((Target->SessionID == NewPlayer->SessionID) || (Target->CharacterSession->IsDelete == 1))
			{
				continue;
			}

			//CPacket OtherCharacter;
			GlobalCPacket->Clear();
			MakePacketCreateOtherCharacterForMe(NewPlayer->CharacterSession, GlobalCPacket, Target->SessionID, Target->Direction, Target->X, Target->Y, Target->HP);

			if (Target->IsMove == true)
			{
				///CPacket Packet_SC_MOVE_START;
				GlobalCPacket->Clear();
				MakePacketMoveStartForMe(NewPlayer->CharacterSession, GlobalCPacket, Target->SessionID, Target->Action, Target->X, Target->Y);
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
		std::unordered_map<unsigned int, CHARACTER*>::iterator Iter;
		for (Iter = CharacterMap.begin(); Iter != CharacterMap.end(); ++Iter)
		{
			CHARACTER* Target = Iter->second;

			if (Tick - Target->CharacterSession->LastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT)
			{
				//printf("타임 웨잇으로 인한 종료당하기. %d %d \n\n", Target->CharacterSession->SessionID , Tick - Target->CharacterSession->LastRecvTime);
				//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, TIMEOUT));


				Disconnect(Target->CharacterSession);
				continue;
			}
			
			if (Target->IsMove && (Target->CharacterSession->IsDelete == 0))
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

void GameRun(CHARACTER* Target)
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

		Target->X -= 6 ;
		Target->Y -= 4 ;

		break;



	case dfPACKET_MOVE_DIR_UU:
		if (Target->Y - 4 < dfRANGE_MOVE_TOP)
		{
			Target->IsMove = false;
			return;
		}


		Target->Y -= 4 ;

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
		Target->X += 6 ;

		break;


	case dfPACKET_MOVE_DIR_RD:

		if (Target->Y + 4 >= dfRANGE_MOVE_BOTTOM || Target->X + 6 >= dfRANGE_MOVE_RIGHT)
		{
			Target->IsMove = false;
			return;
		}


		Target->X += 6 ;
		Target->Y += 4 ;

		break;



	case dfPACKET_MOVE_DIR_DD:
		if (Target->Y + 4 >= dfRANGE_MOVE_BOTTOM)
		{
			Target->IsMove = false;
			return;
		}

		Target->Y += 4 ;

		break;

	case dfPACKET_MOVE_DIR_LD:
		if (Target->Y + 4 >= dfRANGE_MOVE_BOTTOM || Target->X - 6 < dfRANGE_MOVE_LEFT)
		{
			Target->IsMove = false;
			return;
		}

		Target->X -= 6 ;
		Target->Y += 4 ;

		break;
	}


	if (SectorUpdateCharacter(Target))
	{

		SectorUpdate(Target);

	}

}



void HitCheck(CHARACTER* AttackCharacter, int AttackNumber)
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

				if ((AttackCharacter == Target) || (Target->CharacterSession->IsDelete == 1) || (AttackCharacter->X < Target->X))
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

					GlobalCPacket->Clear();

					MakePacketDamage(Target->CharacterSession, GlobalCPacket, AttackCharacter->SessionID, Target->SessionID, Target->HP);

					if (Target->HP == 0)
					{
						Disconnect(Target->CharacterSession);
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

				if ((AttackCharacter == Target) || (Target->CharacterSession->IsDelete == 1) || (AttackCharacter->X > Target->X))
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

					GlobalCPacket->Clear();

					//wprintf(L"## SendDamage Packet : AttackID : %d TargetID %d  \n", AttackCharacter->SessionID, Target->SessionID);

					MakePacketDamage(Target->CharacterSession, GlobalCPacket, AttackCharacter->SessionID, Target->SessionID, Target->HP);
					//SendPacketBroadcast(NULL, &Pakcet_SC_DAMAGE);

					if (Target->HP == 0)
					{
						//printf(" HP가 0 이라 종료 당하는 녀석 : %d \n\n", Target->SessionID);
						//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->CharacterSession->SessionID, TIMEOUT));

						Disconnect(Target->CharacterSession);
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


bool NetPacketProc_MoveStart(SESSION* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	CHARACTER* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	ID = Target->SessionID;

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->CharacterSession, GlobalCPacket, Target->SessionID, Target->X, Target->Y);


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


	GlobalCPacket->Clear();
	MakePacketMoveStart(Target->CharacterSession, GlobalCPacket, Target->SessionID, Direction, Target->X, Target->Y);

	return true;
}

bool NetPacketProc_MoveStop(SESSION* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	CHARACTER* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	ID = Target->SessionID;
	//여기서 처리. 클라의 좌표를 인정해준다.64

	//여기서 한번 돌려줘야함. run을.. 

	if ((abs(Target->X - X) > dfERROR_RANGE) || (abs(Target->Y - Y) > dfERROR_RANGE))
	{
		//Disconnect(Target->CharacterSession);
		//wprintf(L"MoveStop OutOfRange  Server X Y  :  %d   %d  /   Client X Y  :   %d   %d  \n", Target->X, Target->Y, X, Y);
		GlobalCPacket->Clear();
		MakePacketSync(Target->CharacterSession, GlobalCPacket, Target->SessionID, Target->X, Target->Y);

	}
	else
	{


		Target->X = X;
		Target->Y = Y;

		if (SectorUpdateCharacter(Target))
		{
			SectorUpdate(Target); // 지연처리 해주자.. 
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


	GlobalCPacket->Clear();

	MakePacketMoveStop(Target->CharacterSession, GlobalCPacket, Target->SessionID, Target->Direction, Target->X, Target->Y);

	return true;
}

bool NetPacketProc_Attack1(SESSION* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	CHARACTER* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->CharacterSession, GlobalCPacket, Target->SessionID, Target->X, Target->Y);


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



	ID = Target->SessionID;
	Target->Direction = Direction;

	GlobalCPacket->Clear();

	//wprintf(L"## SendAttack1 Packet : ID :  %d , TargetDir  : %d  \n", ID, Direction);
	MakePacketAttack1(Target->CharacterSession, GlobalCPacket, ID, Direction, Target->X, Target->Y);
	HitCheck(Target, 1);

	return true;
}

bool NetPacketProc_Attack2(SESSION* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{
	CHARACTER* Target = CharacterMap.at(TargetSession->SessionID);
	unsigned int ID;

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->CharacterSession, GlobalCPacket, Target->SessionID, Target->X, Target->Y);


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


	ID = Target->SessionID;
	Target->Direction = Direction;

	GlobalCPacket->Clear();

	//wprintf(L"## SendAttack2 Packet : ID :  %d , TargetDir  : %d  \n", ID, Direction);
	MakePacketAttack2(Target->CharacterSession, GlobalCPacket, ID, Direction, Target->X, Target->Y);
	HitCheck(Target, 2);

	return true;
}

bool NetPacketProc_Attack3(SESSION* TargetSession, unsigned char Direction, unsigned short X, unsigned short Y)
{

	CHARACTER* Target = CharacterMap.at(TargetSession->SessionID);

	if (abs(Target->X - X) > dfERROR_RANGE || abs(Target->Y - Y) > dfERROR_RANGE)
	{
		//Disconnect(Target->CharacterSession);


		GlobalCPacket->Clear();

		MakePacketSync(Target->CharacterSession, GlobalCPacket, Target->SessionID, Target->X, Target->Y);


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
	ID = Target->SessionID;

	Target->Direction = Direction;

	GlobalCPacket->Clear();

	//wprintf(L"## SendAttack3 Packet : ID :  %d , TargetDir  : %d  \n", ID, Direction);
	MakePacketAttack3(Target->CharacterSession, GlobalCPacket, ID, Direction, Target->X, Target->Y);
	HitCheck(Target, 3);
	return true;
}


bool NetPacketProc_Echo(SESSION* Target, unsigned int Time)
{

	//CPacket Packet_SC_Echo;
	GlobalCPacket->Clear();

	MakePacketEcho(Target, GlobalCPacket, Time);

	return true;
}

void Disconnect(SESSION* TargetSession)
{
	if (TargetSession->IsDelete == 1)
	{
		return;
	}

	DeleteList.push_back(TargetSession->SessionID);
	TargetSession->IsDelete = 1;

	CHARACTER* Target = CharacterMap.at(TargetSession->SessionID);

	Sector[Target->CharacterSectorPos.Y][Target->CharacterSectorPos.X].remove(Target);

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

void GetSectorAroundForHitLeft(CHARACTER* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
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

void GetSectorAroundForHitRight(CHARACTER* Target, int BoundaryX, int BoundaryY, SectorAround* AroundSector)
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


void GetUpdateSectorAround(CHARACTER* Target, SectorAround* RemoveSector, SectorAround* AddSector)
{
	//나중에 뺄 생각.



	RemoveSector->Count = 0;
	AddSector->Count = 0;

	// ->
	if ( (Target->CharacterSectorPos.Y == Target->OldSectorPos.Y) && (Target->CharacterSectorPos.X > Target->OldSectorPos.X))
	{
		
		if (Target->CharacterSectorPos.Y == 0)
		{
			if (Target->OldSectorPos.X != 0)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
				RemoveSector->Count++;
			}
			

			if (Target->CharacterSectorPos.X + 1 != SECTORMAXX)
			{
				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
				AddSector->Count++;
			}

			return;

		}

		if(Target->CharacterSectorPos.Y == SECTORMAXY - 1)
		{
			if (Target->OldSectorPos.X != 0)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
				RemoveSector->Count++;
			}
			

			if (Target->CharacterSectorPos.X + 1 != SECTORMAXX)
			{
				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
				AddSector->Count++;

			}
			
			return;

		}

		if (Target->OldSectorPos.X != 0)
		{
			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
			RemoveSector->Count++;

		}


		if (Target->CharacterSectorPos.X + 1 != SECTORMAXX)
		{
			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
			AddSector->Count++;
		}

		return;

	}
	

	

	//<-


	if ((Target->CharacterSectorPos.Y == Target->OldSectorPos.Y) && (Target->CharacterSectorPos.X < Target->OldSectorPos.X))
	{

		if (Target->CharacterSectorPos.Y == 0)
		{
			if (Target->CharacterSectorPos.X != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
				AddSector->Count++;
			}


			if (Target->OldSectorPos.X + 1 != SECTORMAXX)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
				RemoveSector->Count++;
			}


			return;

		}

		if (Target->CharacterSectorPos.Y == SECTORMAXY - 1)
		{
			if (Target->CharacterSectorPos.X != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
				AddSector->Count++;
			}


			if (Target->OldSectorPos.X + 1 != SECTORMAXX)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
				RemoveSector->Count++;
			}

			return;

		}

		if (Target->CharacterSectorPos.X != 0)
		{
			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
			AddSector->Count++;

		}


		if (Target->OldSectorPos.X + 1 != SECTORMAXX)
		{
			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
			RemoveSector->Count++;
		}

		return;

	}


	//위..
	if ((Target->CharacterSectorPos.Y < Target->OldSectorPos.Y) && (Target->CharacterSectorPos.X == Target->OldSectorPos.X))
	{

		if (Target->CharacterSectorPos.X == 0)
		{
			if (Target->CharacterSectorPos.Y != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X;
				AddSector->Count++;
			}


			if (Target->OldSectorPos.Y + 1 != SECTORMAXY)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
				RemoveSector->Count++;
			}


			return;

		}


		if (Target->CharacterSectorPos.X == SECTORMAXX - 1)
		{
			if (Target->CharacterSectorPos.Y != 0)
			{
				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X ;
				AddSector->Count++;
			}


			if (Target->OldSectorPos.Y + 1 != SECTORMAXY)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X ;
				RemoveSector->Count++;
			}

			return;

		}

		if (Target->CharacterSectorPos.Y != 0)
		{
			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X ;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y - 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
			AddSector->Count++;

		}


		if (Target->OldSectorPos.Y + 1 != SECTORMAXY)
		{
			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y + 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X;
			RemoveSector->Count++;
		}

		return;

	}


	//아래..
	if ((Target->CharacterSectorPos.Y > Target->OldSectorPos.Y) && (Target->CharacterSectorPos.X == Target->OldSectorPos.X))
	{

		if (Target->CharacterSectorPos.X == 0)
		{
			if (Target->OldSectorPos.Y != 0)
			{
				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
				RemoveSector->Count++;

			}


			if (Target->CharacterSectorPos.Y + 1 != SECTORMAXY)
			{
				

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X;
				AddSector->Count++;
			}


			return;

		}


		if (Target->CharacterSectorPos.X == SECTORMAXX - 1)
		{
			if (Target->OldSectorPos.Y != 0)
			{

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
				RemoveSector->Count++;

				RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
				RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X;
				RemoveSector->Count++;

			}


			if (Target->CharacterSectorPos.Y + 1 != SECTORMAXY)
			{
				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
				AddSector->Count++;

				AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
				AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X;
				AddSector->Count++;
			}

			return;

		}

		if (Target->OldSectorPos.Y != 0)
		{
			
			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X - 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X + 1;
			RemoveSector->Count++;

			RemoveSector->Around[RemoveSector->Count].Y = Target->OldSectorPos.Y - 1;
			RemoveSector->Around[RemoveSector->Count].X = Target->OldSectorPos.X;
			RemoveSector->Count++;
		}


		if (Target->CharacterSectorPos.Y + 1 != SECTORMAXY)
		{
			

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X + 1;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X;
			AddSector->Count++;

			AddSector->Around[AddSector->Count].Y = Target->CharacterSectorPos.Y + 1;
			AddSector->Around[AddSector->Count].X = Target->CharacterSectorPos.X - 1;
			AddSector->Count++;
		}

		return;

	}

			
	

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

}



bool SectorUpdateCharacter(CHARACTER* Target)
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
void SectorUpdate(CHARACTER* Target)
{

	SectorAround Remove;
	SectorAround Add;

	GetUpdateSectorAround(Target, &Remove, &Add);

	//PrintUpdateSector(&Remove, &Add);




	GlobalCPacket->Clear();

	MakePacketDeleteCharacterRemoveSector(Target->CharacterSession, GlobalCPacket, &Remove, Target->SessionID); //패킷 만들어서 Remove 시켜줘야하는데.. 



	//Remove에 있는 애들의 삭제를 나에게 보냄. 
	for (unsigned int i = 0; i < Remove.Count; i++)
	{
		std::list<CHARACTER*>::iterator Iter;
		for (Iter = Sector[Remove.Around[i].Y][Remove.Around[i].X].begin(); Iter != Sector[Remove.Around[i].Y][Remove.Around[i].X].end(); ++Iter)
		{
			GlobalCPacket->Clear();
			MakePacketDeleteCharacterForMe(Target->CharacterSession, GlobalCPacket, (*Iter)->SessionID);


			//printf("Remove에 있는 애들의 삭제를 나에게 보냄. 지움 당하는 아이디 %d  : 받는 아이디 %d \n\n", Target->CharacterSession->SessionID, (*Iter)->SessionID);

		}
	}


	//add에 있는 애들에게 나의 생성을 보냄. 
	GlobalCPacket->Clear();
	MakePacketCreateCharacterAddSector(Target->CharacterSession, GlobalCPacket, &Add, Target->SessionID, Target->Direction, Target->X, Target->Y, Target->HP);
	//이동 정보도 보내줘야함. 
	// 
	// 
	GlobalCPacket->Clear();
	MakePacketMoveStartAddSector(Target->CharacterSession, GlobalCPacket, &Add, Target->SessionID, Target->Action, Target->X, Target->Y);



	for (unsigned int i = 0; i < Add.Count; i++)
	{
		std::list<CHARACTER*>::iterator IterCreate;
		for (IterCreate = Sector[Add.Around[i].Y][Add.Around[i].X].begin();
			IterCreate != Sector[Add.Around[i].Y][Add.Around[i].X].end(); ++IterCreate)
		{
			CHARACTER* CreateCharacter = *IterCreate;
			if ((CreateCharacter->SessionID == Target->SessionID) || (CreateCharacter->CharacterSession->IsDelete == 1))
			{
				continue;
			}

			GlobalCPacket->Clear();

			MakePacketCreateOtherCharacterForMe(Target->CharacterSession, GlobalCPacket, CreateCharacter->SessionID, CreateCharacter->Direction, CreateCharacter->X, CreateCharacter->Y, CreateCharacter->HP);

			if (CreateCharacter->IsMove == true)
			{

				GlobalCPacket->Clear();
				MakePacketMoveStartForMe(Target->CharacterSession, GlobalCPacket, CreateCharacter->SessionID, CreateCharacter->Action, CreateCharacter->X, CreateCharacter->Y);
			}

		}
	}



}
void FreeCharacter(CHARACTER* Target)
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
