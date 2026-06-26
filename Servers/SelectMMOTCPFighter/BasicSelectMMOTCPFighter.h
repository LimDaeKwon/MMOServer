#pragma once

#include "SelectServerPQ.h"
#include "ObjectFreeList.h"
#include "GameDefine.h"
#include "Character.h"



class BasicSelectMMOTCPFighter : public SelectServerPQ
{
public:
	BasicSelectMMOTCPFighter();
	virtual ~BasicSelectMMOTCPFighter();

public:

	virtual void OnAccept(SessionId sessionId);
	virtual void OnMessage(SessionId sessionId, unsigned char packetType, CPacket* packet);
	virtual void OnRelease(SessionId sessionId);
	virtual void OnUpdate();

	void GameRun(Character* target);

	void HitCheck(Character* attackCharacter, int attackNumber);

	void GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector);

	void GetSectorAroundForHit(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector);

	SectorUpdateAround* GetUpdateSectorAround(Character* target);

	bool SectorUpdateCharacter(Character* target);

	void SectorUpdate(Character* target);

	void SendPacketAround(SessionId except, CPacket* packet, bool sendMe = false);

	void SendPacketSectorOne(int sectorX, int sectorY, SessionId except, CPacket* packet);

	void SendPacketToSectors(CPacket* packet, SectorAround* around, SessionId exceptSessionId = InvalidSessionId);

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

	void MakePacketDeleteCharacterRemoveSector(CPacket* packet, SectorAround* around, SessionId id);

	void MakePacketDeleteCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id);

	void MakePacketCreateCharacterAddSector(CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);

	void MakePacketMoveStartAddSector(CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y);

	void MakePacketSync(SessionId sessionId, CPacket* packet, SessionId id, unsigned short x, unsigned short y);

	bool NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);

	bool NetPacketProcEcho(SessionId sessionId, unsigned int time);


private:


	Character* CreateCharacter(SessionId sessionId);
	void RegisterCharacter(Character* newPlayer);
	void SendNewCharacterCreate(Character* newPlayer);
	void SendExistingCharactersToNewCharacter(Character* newPlayer);

	void SendCharacterDelete(Character* character);
	void UnregisterCharacter(Character* character);
	void ReleaseCharacter(Character* character);

	bool IsClientPositionValid(Character* target, unsigned short x, unsigned short y);
	void SendSync(Character* target);
	void ApplyClientPosition(Character* target, unsigned short x, unsigned short y);
	void SyncOrApplyClientPosition(Character* target, unsigned short x, unsigned short y);
	void UpdateCharacterFacingDirection(Character* target, unsigned char direction);

	bool CanHitTarget(const Character* attackCharacter, const Character* target, int boundaryX, int boundaryY);

	void AddSectorPosition(SectorAround* aroundSector, int sectorX, int sectorY);

	void SendRemoveSectorUpdate(Character* target, SectorAround* removeSector);
	void SendAddSectorUpdate(Character* target, SectorAround* addSector);

	void InitializeSectorUpdateAround();

	void BuildSectorUpdateAround(int oldSectorX, int oldSectorY, int curSectorX, int curSectorY, SectorUpdateAround* sectorUpdateAround);


	std::list<Character*> sectorCharacterList_[SectorMaxY][SectorMaxX];

	std::unordered_map<SessionId, Character*> characterMap_;

	ObjectFreeList<Character> characterFreeList_;

	SectorUpdateAround sectorUpdateAround_[SectorMaxY][SectorMaxX][3][3];



};
