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
	unsigned int x_;
	unsigned int y_;

};

struct SectorAround
{
	unsigned int count_;
	SectorPos around_[9];
	BYTE Flag = 0;
};

struct Character
{
	__int64 sessionId_;
	unsigned int SessionIDForContents;
	unsigned int action_;
	unsigned char direction_;

	short x_;
	short y_;
	unsigned char hp_;
	bool isMove_;
	bool IsDelete;

	SectorPos oldSectorPos_;
	SectorPos characterSectorPos_;


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



	ObjectFreeList<Character> CharacterFreeList;

	std::list<Character*> Sector[SECTORMAXY][SECTORMAXX];
	std::list<unsigned int> DeleteList;
	std::unordered_map<__int64, Character*> CharacterMap;

	unsigned int OldTick;
	unsigned int OldTickforCheck;


	int GlobalLoop = 0;

	int Count = 0;





	bool MessageProc(MessageData* msg);



	// MAKEPACKET
	void MakePacketMoveStart(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketMoveStartForMe(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketMoveStop(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketCreateMyCharacter(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketCreateOtherCharacterForMe(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketCreateOtherCharacter(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketDeleteCharacter(Character* Target, CPacket* Packet, unsigned int ID);

	void MakePacketDamage(Character* Target, CPacket* Packet, unsigned int AttackID, unsigned int DamageID, unsigned char DamageHP);

	void MakePacketAttack1(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketAttack2(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketAttack3(Character* Target, CPacket* Packet, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketEcho(Character* Target, CPacket* Packet, unsigned int Time);

	void MakePacketDeleteCharacterRemoveSector(Character* Target, CPacket* Packet, SectorAround* Around, unsigned int ID);

	void MakePacketDeleteCharacterForMe(Character* Target, CPacket* Packet, unsigned int ID);

	void MakePacketCreateCharacterAddSector(Character* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y, unsigned char HP);

	void MakePacketMoveStartAddSector(Character* Target, CPacket* Packet, SectorAround* Around, unsigned int ID, unsigned char Direction, unsigned short X, unsigned short Y);

	void MakePacketSync(Character* Target, CPacket* Packet, unsigned int ID, unsigned short X, unsigned short Y);

	void SendPacketUnicast(Character* Target, CPacket* Packet);

	void SendPacketAround(Character* Target, CPacket* Packet, bool SendMe = false);

	void SendPacketAroundRemoveSector(CPacket* Packet, SectorAround* Around);

	void SendPacketAroundAddSector(CPacket* Packet, SectorAround* Around);

	void SendPacketSectorOne(int SectorX, int SectorY, unsigned int ExceptSessionID, CPacket* Packet);



	//Stub

	bool PacketProc(MessageData* msg);

	bool NetPacketProc_MoveStart(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_MoveStop(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Attack1(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Attack2(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Attack3(Character* Target, unsigned char Direction, unsigned short X, unsigned short Y);
	bool NetPacketProc_Echo(Character* Target, unsigned int Time);



	bool SectorUpdateCharacter(Character* Target);

	void SectorUpdate(Character* Target);

	void HitCheck(Character* TargetSession, int AttackNumber);

	void GetSectorAroundForHitLeft(Character* Target, int BoundaryX, int BoundaryY, SectorAround* Around);

	void GetSectorAroundForHitRight(Character* Target, int BoundaryX, int BoundaryY, SectorAround* Around);

	void GetUpdateSectorAround(Character* Target, SectorAround* RemoveSector, SectorAround* AddSector);

	void Update();

	void GameRun(Character* Target);

	void DeleteDisconnect();

	void DisconnectContents(Character* Target);

	void MessageLoop();


};


