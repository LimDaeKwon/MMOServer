#pragma once

#include "LanNetworkLibraryServer.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "ObjectFreeList.h"
#include <unordered_map>
#include <list>
#include "PacketDefine.h"



enum Type
{
	ACCEPT,			// 货 立加
	PACKET,
	RELEASE,				// 立加 秦力
};

struct MessageData
{
	WORD type;
	__int64 session_ID;
	CPacket* contents_packet;
};

struct SectorPos
{
	unsigned int X;
	unsigned int Y;

};

struct SectorAround
{
	unsigned int Count;
	SectorPos Around[9];
	BYTE Flag = 0;
};

struct CHARACTER
{
	__int64 SessionID;
	unsigned int SessionIDForContents;
	unsigned int Action;
	unsigned char Direction;

	short X;
	short Y;
	unsigned char HP;
	bool IsMove;
	bool IsDelete;

	SectorPos OldSectorPos;
	SectorPos CharacterSectorPos;


};





class MMOTCPServer_Single : public LanNetworkLibraryServer
{
public:

	MMOTCPServer_Single();
	virtual ~MMOTCPServer_Single();

	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port);
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID);
	virtual void OnRelease(__int64 session_ID);

	virtual void OnMessage(__int64 session_ID, CPacket* send_packet);

	virtual void OnError(int errorcode, const wchar_t* error_log);


	TLockFreeQueue<MessageData*> MessageQueue;
	TLSObjectFreeList<MessageData> MessageDataFreeList;
	HANDLE LogicThreadHandle;
	static unsigned int WINAPI LogicThread(LPVOID this_ptr);



	void LoginProcess(MessageData* msg_data);
	void messageProcess(MessageData* msg_data);


	void CreateCharater(__int64 NewSession);
	void ReleaseCharacter(__int64 NewSession);


	void GetSectorAround(int SectorX, int SectorY, SectorAround* AroundSector);

	int ServerControl();



	ObjectFreeList<CHARACTER> CharacterFreeList;

	std::list<CHARACTER*> Sector[SECTORMAXY][SECTORMAXX];
	std::list<unsigned int> DeleteList;
	std::unordered_map<__int64, CHARACTER*> CharacterMap;

	unsigned int OldTick;
	unsigned int OldTickforCheck;


	int GlobalLoop = 0;

	int Count = 0;





	bool MessageProc(MessageData* msg);



	// MAKEPACKET
	void MakePacketMoveStart(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketMoveStartForMe(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketMoveStop(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketCreateMyCharacter(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketCreateOtherCharacterForMe(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketCreateOtherCharacter(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketDeleteCharacter(CHARACTER* Target, CPacket* Packet, unsigned int ID);

	void MakePacketDamage(CHARACTER* Target, CPacket* Packet, unsigned int AttackID, unsigned int DamageID, unsigned char DamageHP);

	void MakePacketAttack1(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketAttack2(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketAttack3(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketEcho(CHARACTER* Target, CPacket* Packet, unsigned int Time);

	void MakePacketDeleteCharacterRemoveSector(CHARACTER* Target, CPacket* Packet, SectorAround* Around, unsigned int ID);

	void MakePacketDeleteCharacterForMe(CHARACTER* Target, CPacket* Packet, unsigned int ID);

	void MakePacketCreateCharacterAddSector(CHARACTER* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketMoveStartAddSector(CHARACTER* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketSync(CHARACTER* Target, CPacket* Packet, unsigned int ID, unsigned short X, unsigned short Y);

	void SendPacketUnicast(CHARACTER* Target, CPacket* Packet);

	void SendPacketAround(CHARACTER* Target, CPacket* Packet, bool SendMe = false);

	void SendPacketAroundRemoveSector(CPacket* Packet, SectorAround* Around);

	void SendPacketAroundAddSector(CPacket* Packet, SectorAround* Around);

	void SendPacketSectorOne(int SectorX, int SectorY, unsigned int ExceptSessionID, CPacket* Packet);



	//Stub

	bool PacketProc(MessageData* msg);

	bool NetPacketProc_MoveStart(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_MoveStop(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Attack1(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Attack2(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Attack3(CHARACTER* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Echo(CHARACTER* Target, unsigned int Time);



	bool SectorUpdateCharacter(CHARACTER* Target);

	void SectorUpdate(CHARACTER* Target);

	void HitCheck(CHARACTER* TargetSession, int AttackNumber);

	void GetSectorAroundForHitLeft(CHARACTER* Target, int BoundaryX, int BoundaryY, SectorAround* Around);

	void GetSectorAroundForHitRight(CHARACTER* Target, int BoundaryX, int BoundaryY, SectorAround* Around);

	void GetUpdateSectorAround(CHARACTER* Target, SectorAround* RemoveSector, SectorAround* AddSector);

	void Update();

	void GameRun(CHARACTER* Target);

	void DeleteDisconnect();

	void DisconnectContents(CHARACTER* Target);

	void MessageLoop();


};


