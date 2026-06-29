#pragma once

#include "IOCPServer.h"
#include "LockFreeQueue.h"
#include "TLSObjectFreeList.h"
#include "ObjectFreeList.h"
#include "PacketDefine.h"
#include <list>
#include <unordered_map>

using SessionId = __int64;

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

struct SectorPos
{
	unsigned int x_;
	unsigned int y_;
};

struct SectorAround
{
	unsigned int count_;
	SectorPos around_[9];
	BYTE flag_ = 0;
};

struct Character
{
	SessionId sessionId_;
	unsigned int sessionIdForContents_;
	unsigned int action_;
	unsigned char direction_;
	short x_;
	short y_;
	unsigned char hp_;
	bool isMove_;
	bool isDelete_;
	SectorPos oldSectorPos_;
	SectorPos characterSectorPos_;
};

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

	static unsigned int WINAPI LogicThread(LPVOID thisPtr);

	void LoginProcess(MessageData* messageData);
	void MessageProcess(MessageData* messageData);
	void CreateCharacter(SessionId newSession);
	void ReleaseCharacter(SessionId newSession);
	void GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector);
	int ServerControl();
	bool MessageProc(MessageData* messageData);
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
	void MakePacketDeleteCharacterRemoveSector(Character* target, CPacket* packet, SectorAround* around, unsigned int id);
	void MakePacketDeleteCharacterForMe(Character* target, CPacket* packet, unsigned int id);
	void MakePacketCreateCharacterAddSector(Character* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp);
	void MakePacketMoveStartAddSector(Character* target, CPacket* packet, SectorAround* around, unsigned int id, unsigned char direction, unsigned short x, unsigned short y);
	void MakePacketSync(Character* target, CPacket* packet, unsigned int id, unsigned short x, unsigned short y);
	void SendPacketUnicast(Character* target, CPacket* packet);
	void SendPacketAround(Character* target, CPacket* packet, bool sendMe = false);
	void SendPacketAroundRemoveSector(CPacket* packet, SectorAround* around);
	void SendPacketAroundAddSector(CPacket* packet, SectorAround* around);
	void SendPacketSectorOne(int sectorX, int sectorY, unsigned int exceptSessionId, CPacket* packet);
	bool PacketProc(MessageData* messageData);
	bool NetPacketProcMoveStart(Character* target, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcMoveStop(Character* target, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcAttack1(Character* target, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcAttack2(Character* target, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcAttack3(Character* target, unsigned char direction, unsigned short x, unsigned short y);
	bool NetPacketProcEcho(Character* target, unsigned int time);
	bool SectorUpdateCharacter(Character* target);
	void SectorUpdate(Character* target);
	void HitCheck(Character* attackCharacter, int attackNumber);
	void GetSectorAroundForHitLeft(Character* target, int boundaryX, int boundaryY, SectorAround* around);
	void GetSectorAroundForHitRight(Character* target, int boundaryX, int boundaryY, SectorAround* around);
	void GetUpdateSectorAround(Character* target, SectorAround* removeSector, SectorAround* addSector);
	void Update();
	void GameRun(Character* target);
	void DeleteDisconnect();
	void DisconnectContents(Character* target);
	void MessageLoop();

	TLockFreeQueue<MessageData*> messageQueue_;
	TLSObjectFreeList<MessageData> messageDataFreeList_;
	HANDLE logicThreadHandle_;
	ObjectFreeList<Character> characterFreeList_;
	std::list<Character*> sector_[SectorMaxY][SectorMaxX];
	std::list<unsigned int> deleteList_;
	std::unordered_map<SessionId, Character*> characterMap_;
	unsigned int oldTick_;
	unsigned int oldTickForCheck_;
	int globalLoop_ = 0;
	int count_ = 0;
};

