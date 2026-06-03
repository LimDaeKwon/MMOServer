#pragma once

struct Session;
struct Character;
struct SectorAround;
class CPacket;


void GameRun(Character* Target);

void CreateCharater(Session* NewSession);

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
