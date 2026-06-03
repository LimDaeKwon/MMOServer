#pragma once

struct SESSION;
struct CHARACTER;
struct SectorAround;
class CPacket;


void GameRun(CHARACTER* Target);

void CreateCharater(SESSION* NewSession);

void Update();

void HitCheck(CHARACTER* TargetSession, int AttackNumber);

void Disconnect(SESSION* TargetSession);

void GetSectorAround(int SectorX, int SectorY, SectorAround* Around);

void GetSectorAroundForHitLeft(CHARACTER* Target , int BoundaryX, int BoundaryY, SectorAround* Around);

void GetSectorAroundForHitRight(CHARACTER* Target, int BoundaryX, int BoundaryY, SectorAround* Around);

void GetUpdateSectorAround(CHARACTER* Target, SectorAround* RemoveSector, SectorAround* AddSector);

bool SectorUpdateCharacter(CHARACTER* Target);

void SectorUpdate(CHARACTER* Target);

void FreeCharacter(CHARACTER* Target);

void PrintUpdateSector(SectorAround* RemoveSector, SectorAround* AddSector);

void PrintHitCheckSector(SectorAround* HitCheckSector);
