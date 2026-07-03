#pragma once

#include "IOCPServer.h"
#include "TLSObjectFreeList.h"
#include "ObjectFreeList.h"
#include "GameDefine.h"
#include "Character.h"

#include <list>
#include <unordered_map>
#include <vector>

class MMOTCPServerSingle : public IOCPServer
{
public:
	MMOTCPServerSingle();
	virtual ~MMOTCPServerSingle();

	virtual bool OnConnectionRequest(const wchar_t* serverIp, unsigned short serverPort) override;
	virtual void OnAccept(const wchar_t* serverIp, unsigned short serverPort, SessionId sessionId) override;
	virtual void OnRelease(SessionId sessionId) override;
	virtual void OnMessage(SessionId sessionId, BYTE packetType, CPacket* sendPacket) override;
	virtual void OnError(int errorCode, const wchar_t* errorLog) override;


	int GetMessageQueueSize();

private:

	enum MessageType
	{
		MessageTypeAccept,
		MessageTypePacket,
		MessageTypeRelease,
	};

	struct MessageData
	{
		WORD type_;
		SessionId sessionId_;
		BYTE packetType_;
		CPacket* contentsPacket_;
	};


private:
	static unsigned int WINAPI LogicThread(LPVOID thisPtr);

	void MessageLoop();
	bool MessageProc(MessageData* messageData);
	bool PacketProc(MessageData* messageData);

	void AcceptProc(SessionId sessionId);
	void ReleaseProc(SessionId sessionId);

	Character* CreateCharacter(SessionId sessionId);
	void RegisterCharacter(Character* newPlayer);
	void SendNewCharacterCreate(Character* newPlayer);
	void SendExistingCharactersToNewCharacter(Character* newPlayer);

	void SendCharacterDelete(Character* character);
	void UnregisterCharacter(Character* character);
	void ReleaseCharacter(Character* character);

	void Update();
	void GameRun(Character* target);

	bool NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcEcho(SessionId sessionId, unsigned int time);

	bool IsClientPositionValid(Character* target, unsigned short x, unsigned short y);
	void SendSync(Character* target);
	void ApplyClientPosition(Character* target, unsigned short x, unsigned short y);
	void SyncOrApplyClientPosition(Character* target, unsigned short x, unsigned short y);
	void UpdateCharacterFacingDirection(Character* target, unsigned char direction);

	void HitCheck(Character* attackCharacter, int attackNumber);
	bool CanHitTarget(const Character* attackCharacter, const Character* target, int boundaryX, int boundaryY);

	void GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector);
	void GetSectorAroundForHit(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector);
	void AddSectorPosition(SectorAround* aroundSector, int sectorX, int sectorY);

	bool SectorUpdateCharacter(Character* target);
	void SectorUpdate(Character* target);
	SectorUpdateAround* GetUpdateSectorAround(Character* target);
	void InitializeSectorUpdateAround();
	void BuildSectorUpdateAround(int oldSectorX, int oldSectorY, int curSectorX, int curSectorY, SectorUpdateAround* sectorUpdateAround);

	void SendRemoveSectorUpdate(Character* target, SectorAround* removeSector);
	void SendAddSectorUpdate(Character* target, SectorAround* addSector);

	void SendPacketUnicast(Character* target, CPacket* packet);
	void SendPacketAround(Character* target, CPacket* packet, bool sendMe = false);
	void SendPacketToSectors(CPacket* packet, SectorAround* around, SessionId exceptSessionId = InvalidSessionId);
	void SendPacketSectorOne(int sectorX, int sectorY, SessionId exceptSessionId, CPacket* packet);

	void MakePacketMoveStart(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketMoveStartForMe(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketMoveStop(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketCreateMyCharacter(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);
	void MakePacketCreateOtherCharacterForMe(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);
	void MakePacketCreateOtherCharacter(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);
	void MakePacketDeleteCharacter(Character* target, CPacket* packet, unsigned int id);
	void MakePacketDamage(Character* target, CPacket* packet, unsigned int attackId, unsigned int damageId, unsigned char damageHp);
	void MakePacketAttack1(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketAttack2(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketAttack3(Character* target, CPacket* packet, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketEcho(Character* target, CPacket* packet, unsigned int time);
	void MakePacketDeleteCharacterRemoveSector(CPacket* packet, SectorAround* around, unsigned int id);
	void MakePacketDeleteCharacterForMe(Character* target, CPacket* packet, unsigned int id);
	void MakePacketCreateCharacterAddSector(CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);
	void MakePacketMoveStartAddSector(CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketSync(Character* target, CPacket* packet, unsigned int id, unsigned short x, unsigned short y);

	int ServerControl();

	void AddMovingCharacter(Character* target);
	void RemoveMovingCharacter(Character* target);
	
	

private:
	TLockFreeQueue<MessageData*> messageQueue_;
	TLSObjectFreeList<MessageData> messageDataFreeList_;
	HANDLE logicThreadHandle_;

	ObjectFreeList<Character> characterFreeList_;
	std::unordered_map<SessionId, Character*> characterMap_;
	std::list<Character*> sectorCharacterList_[SectorMaxY][SectorMaxX];

	SectorUpdateAround sectorUpdateAround_[SectorMaxY][SectorMaxX][3][3];

	std::vector<Character*> movingCharacters_;
	
	unsigned int oldTick_;
	unsigned int oldTickForCheck_;
	unsigned int frameMs_;

	int globalLoop_ = 0;
	int count_ = 0;

	

};

