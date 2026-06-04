#pragma once

struct Session;
struct Character;
struct SectorAround;
class CPacket;

using SessionId = unsigned int;

void GameRun(Character* target);

void CreateCharacter(Session* newSession);

void Update();

void HitCheck(Character* target, int attackNumber);

void ReleaseInContents(SessionId sessionId);

void GetSectorAround(int sectorX, int sectorY, SectorAround* around);

void GetSectorAroundForHitLeft(Character* target, int boundaryX, int boundaryY, SectorAround* around);

void GetSectorAroundForHitRight(Character* target, int boundaryX, int boundaryY, SectorAround* around);

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