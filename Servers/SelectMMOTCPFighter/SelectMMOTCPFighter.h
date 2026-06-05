#pragma once
#include "SelectServer.h"
#include "GameDefine.h"
#include "ObjectFreeList.h"


struct Character;
struct SectorAround;

class SelectMMOTCPFighter : public SelectServer
{
public:	
	SelectMMOTCPFighter();
	virtual ~SelectMMOTCPFighter();



public:

	virtual void OnAccept(SessionId sessionId);
	virtual void OnMessage(SessionId sessionId, unsigned char packetType, CPacket* packet);
	virtual void OnRelease(SessionId sessionId);
	virtual void OnUpdate();

	void GameRun(Character* target);

	void CreateCharacter(Session* newSession);

	void HitCheck(Character* attackCharacter, int attackNumber);

	void ReleaseInContents(SessionId sessionId);

	void GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector);

	void GetSectorAroundForHitLeft(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector);

	void GetSectorAroundForHitRight(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector);

	void GetUpdateSectorAround(Character* target, SectorAround* removeSector, SectorAround* addSector);

	bool SectorUpdateCharacter(Character* target);

	void SectorUpdate(Character* target);

	void FreeCharacter(Character* target);

	void PrintUpdateSector(SectorAround* removeSector, SectorAround* addSector);

	void PrintHitCheckSector(SectorAround* hitCheckSector);

	
	
	
	void SendPacketAround(SessionId except, CPacket* packet, bool sendMe = false);

	void SendPacketSectorOne(int sectorX, int sectorY, SessionId except, CPacket* packet);

	void SendPacketAroundRemoveSector(SessionId target, CPacket* packet, SectorAround* around);

	void SendPacketAroundAddSector(SessionId target, CPacket* packet, SectorAround* around);
	

	void MakePacketMoveStart(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketMoveStartForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketMoveStop(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketCreateMyCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

	void MakePacketCreateOtherCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

	void MakePacketCreateOtherCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

	void MakePacketDeleteCharacter(SessionId sessionId, CPacket* packet, SessionId id);

	void MakePacketDamage(SessionId sessionId, CPacket* packet, SessionId attackId, SessionId damageId, unsigned char damageHp);

	void MakePacketAttack1(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketAttack2(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketAttack3(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketEcho(SessionId sessionId, CPacket* packet, unsigned int time);

	void MakePacketDeleteCharacterRemoveSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id);

	void MakePacketDeleteCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id);

	void MakePacketCreateCharacterAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

	void MakePacketMoveStartAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketSync(SessionId sessionId, CPacket* packet, SessionId id, unsigned short x, unsigned short y);


	////
	bool PacketProc(SessionId sessionId, unsigned char packetType, CPacket* packetBuffer);

	bool NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcEcho(SessionId sessionId, unsigned int time);


private:

	std::list<Character*> sector[SectorMaxY][SectorMaxX];

	std::unordered_map<SessionId, Character*> characterMap;

	ObjectFreeList<Character> characterFreeList;

	CPacket* globalCPacket;

};

