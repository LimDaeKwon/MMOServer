#pragma once

struct Session;
struct Character;
struct SectorAround;
class CPacket;

using SessionId = unsigned int;


void GameRun(Character* Target);

void CreateCharacter(Session* NewSession);

void Update();

void HitCheck(Character* TargetSession, int AttackNumber);

void Disconnect(Session* TargetSession);

void GetSectorAround(int SectorX, int SectorY, SectorAround* Around);

void GetSectorAroundForHitLeft(Character* Target , int BoundaryX, int BoundaryY, SectorAround* Around);

void GetSectorAroundForHitRight(Character* Target, int BoundaryX, int BoundaryY, SectorAround* Around);

void GetUpdateSectorAround(Character* Target, SectorAround* RemoveSector, SectorAround* AddSector);

bool SectorUpdateCharacter(Character* Target);

void SectorUpdate(Character* Target);

void FreeCharacter(Character* Target);

void PrintUpdateSector(SectorAround* RemoveSector, SectorAround* AddSector);

void PrintHitCheckSector(SectorAround* HitCheckSector);

void SendPacketAround(SessionId Except, CPacket* Packet, bool SendMe = false);

void SendPacketSectorOne(int SectorX, int SectorY, SessionId Except, CPacket* Packet);

void SendPacketAroundRemoveSector(SessionId Target, CPacket* Packet, SectorAround* Around);

void SendPacketAroundAddSector(SessionId Target, CPacket* Packet, SectorAround* Around);
