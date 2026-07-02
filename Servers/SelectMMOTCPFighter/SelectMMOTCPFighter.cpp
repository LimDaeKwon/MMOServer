#include "SelectMMOTCPFighter.h"
#include "Character.h"
#include "CPacket.h"
#include "Profiler.h"
#include "PacketDefine.h"
#include "ContentsCPacket.h"


SelectMMOTCPFighter::SelectMMOTCPFighter() : characterFreeList_(10000)
{
    InitializeSectorUpdateAround();
}

SelectMMOTCPFighter::~SelectMMOTCPFighter()
{

}

void SelectMMOTCPFighter::OnAccept(SessionId sessionId)
{
    Profile profile(L"OnAccept");

    Character* newPlayer = CreateCharacter(sessionId);

    RegisterCharacter(newPlayer);
    SendNewCharacterCreate(newPlayer);
    SendExistingCharactersToNewCharacter(newPlayer);
}

void SelectMMOTCPFighter::OnMessage(SessionId sessionId, unsigned char packetType, CPacket* packet)
{
    Profile profile(L"OnMessage");
    switch (packetType)
    {
    case PacketCsMoveStart:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packet >> direction >> x >> y;
        NetPacketProcMoveStart(sessionId, direction, x, y);
        break;
    }

    case PacketCsMoveStop:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packet >> direction >> x >> y;
        NetPacketProcMoveStop(sessionId, direction, x, y);
        break;
    }

    case PacketCsAttack1:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packet >> direction >> x >> y;
        NetPacketProcAttack1(sessionId, direction, x, y);
        break;
    }

    case PacketCsAttack2:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packet >> direction >> x >> y;
        NetPacketProcAttack2(sessionId, direction, x, y);
        break;
    }

    case PacketCsAttack3:
    {
        unsigned char direction;
        unsigned short x;
        unsigned short y;

        *packet >> direction >> x >> y;
        NetPacketProcAttack3(sessionId, direction, x, y);
        break;
    }

    case PacketCsEcho:
    {
        unsigned int time;

        *packet >> time;
        NetPacketProcEcho(sessionId, time);
        break;
    }

    default:
    {
        break;
    }
    }


}

void SelectMMOTCPFighter::OnRelease(SessionId sessionId)
{
    Profile profile(L"OnRelease");
    Character* target = characterMap_.at(sessionId);

    SendCharacterDelete(target);
    ReleaseCharacter(target);
}

void SelectMMOTCPFighter::OnUpdate()
{
    Profile profile(L"OnUpdate");
    for (unsigned int i = 0; i < movingCharacters_.size();)
    {
        Character* target = movingCharacters_[i];

        GameRun(target);

		//안쪽에서 스왑으로 삭제되면 타겟의 Index를 삭제시킴. 즉 -1이면 지금 위치는 스왑된 맨 끝에 있던 유저이다.
        //다시 돌려줘야 함 .

        if (target->movingIndex_ != -1)
        {
            ++i;
        }
    }
}

void SelectMMOTCPFighter::GameRun(Character* target)
{
    Profile profile(L"GameRun");
    switch (target->action_)
    {
    case PacketMoveDirectionLL:
        if (target->x_ - 6 < RangeMoveLeft)
        {
            RemoveMovingCharacter(target);
            return;
        }

        target->x_ -= 6;

        break;

    case PacketMoveDirectionLU:
        if (target->y_ - 4 < RangeMoveTop || target->x_ - 6 < RangeMoveLeft)
        {
            RemoveMovingCharacter(target);
            return;
        }

        target->x_ -= 6;
        target->y_ -= 4;

        break;

    case PacketMoveDirectionUU:
        if (target->y_ - 4 < RangeMoveTop)
        {
            RemoveMovingCharacter(target);
            return;
        }

        target->y_ -= 4;

        break;

    case PacketMoveDirectionRU:

        if ((target->y_ - 4 < RangeMoveTop) || (target->x_ + 6 >= RangeMoveRight))
        {
            RemoveMovingCharacter(target);
            return;
        }

        target->x_ += 6;
        target->y_ -= 4;

        break;

    case PacketMoveDirectionRR:

        if (target->x_ + 6 >= RangeMoveRight)
        {
            RemoveMovingCharacter(target);
            return;
        }
        target->x_ += 6;

        break;

    case PacketMoveDirectionRD:

        if (target->y_ + 4 >= RangeMoveBottom || target->x_ + 6 >= RangeMoveRight)
        {
            RemoveMovingCharacter(target);
            return;
        }

        target->x_ += 6;
        target->y_ += 4;

        break;

    case PacketMoveDirectionDD:
        if (target->y_ + 4 >= RangeMoveBottom)
        {
            RemoveMovingCharacter(target);
            return;
        }

        target->y_ += 4;

        break;

    case PacketMoveDirectionLD:
        if (target->y_ + 4 >= RangeMoveBottom || target->x_ - 6 < RangeMoveLeft)
        {
            RemoveMovingCharacter(target);
            return;
        }

        target->x_ -= 6;
        target->y_ += 4;

        break;
    }

    if (SectorUpdateCharacter(target))
    {
        SectorUpdate(target);
    }
}


void SelectMMOTCPFighter::HitCheck(Character* attackCharacter, int attackNumber)
{
    Profile profile(L"HitCheck");
    int boundaryX = 0;
    int boundaryY = 0;

    int damage = 0;

    switch (attackNumber)
    {
    case 1:
        boundaryX = Attack1RangeX;
        boundaryY = Attack1RangeY;
        damage = Attack1Damage;
        break;

    case 2:
        boundaryX = Attack2RangeX;
        boundaryY = Attack2RangeY;
        damage = Attack2Damage;
        break;

    case 3:
        boundaryX = Attack3RangeX;
        boundaryY = Attack3RangeY;
        damage = Attack3Damage;
        break;

    default:
        DebugBreak();
    }

    SectorAround hitCheckSector;

    GetSectorAroundForHit(attackCharacter, boundaryX, boundaryY, &hitCheckSector);

    for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
    {
        int sectorX = hitCheckSector.around_[i].x_;
        int sectorY = hitCheckSector.around_[i].y_;

        std::list<Character*>::iterator iter;

        for (iter = sectorCharacterList_[sectorY][sectorX].begin(); iter != sectorCharacterList_[sectorY][sectorX].end(); ++iter)
        {
            Character* target = *iter;

            if (CanHitTarget(attackCharacter, target, boundaryX, boundaryY) == false)
            {
                continue;
            }

            if (damage >= target->hp_)
            {
                target->hp_ = 0;
            }
            else
            {
                target->hp_ -= damage;
            }

            CPacket* packetDamage = CPacket::Alloc();
            MakePacketDamage(target->sessionId_, packetDamage, attackCharacter->sessionId_, target->sessionId_, target->hp_);
            CPacket::Free(packetDamage);

            if (target->hp_ == 0)
            {
                Disconnect(target->sessionId_);
            }

            return;
        }
    }
}

void SelectMMOTCPFighter::GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
{
    aroundSector->count_ = 0;

    AddSectorPosition(aroundSector, sectorX, sectorY);

    if (sectorX + 1 < SectorMaxX)
    {
        AddSectorPosition(aroundSector, sectorX + 1, sectorY);
    }

    if (sectorX - 1 >= 0)
    {
        AddSectorPosition(aroundSector, sectorX - 1, sectorY);
    }

    if (sectorY + 1 < SectorMaxY)
    {
        AddSectorPosition(aroundSector, sectorX, sectorY + 1);
    }

    if (sectorY - 1 >= 0)
    {
        AddSectorPosition(aroundSector, sectorX, sectorY - 1);
    }

    if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX)
    {
        AddSectorPosition(aroundSector, sectorX + 1, sectorY + 1);
    }

    if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX)
    {
        AddSectorPosition(aroundSector, sectorX + 1, sectorY - 1);
    }

    if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0)
    {
        AddSectorPosition(aroundSector, sectorX - 1, sectorY + 1);
    }

    if (sectorY - 1 >= 0 && sectorX - 1 >= 0)
    {
        AddSectorPosition(aroundSector, sectorX - 1, sectorY - 1);
    }
}

void SelectMMOTCPFighter::GetSectorAroundForHit(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
{
    int sectorX = target->characterSectorPos_.x_;
    int sectorY = target->characterSectorPos_.y_;

    aroundSector->count_ = 0;

    AddSectorPosition(aroundSector, sectorX, sectorY);

    int targetValidPosX;
    int targetValidPosYAbove = (target->y_ - boundaryY) / SectorYSize;
    int targetValidPosYBelow = (target->y_ + boundaryY) / SectorYSize;

    if (target->direction_ == PacketMoveDirectionLL)
    {
        targetValidPosX = (target->x_ - boundaryX) / SectorXSize;

        if (sectorX - 1 >= 0 && targetValidPosX != sectorX)
        {
            AddSectorPosition(aroundSector, sectorX - 1, sectorY);
        }
    }
    else
    {
        targetValidPosX = (target->x_ + boundaryX) / SectorXSize;

        if (sectorX + 1 < SectorMaxX && targetValidPosX != sectorX)
        {
            AddSectorPosition(aroundSector, sectorX + 1, sectorY);
        }
    }

    if (sectorY + 1 < SectorMaxY && targetValidPosYBelow != sectorY)
    {
        AddSectorPosition(aroundSector, sectorX, sectorY + 1);
    }

    if (sectorY - 1 >= 0 && targetValidPosYAbove != sectorY)
    {
        AddSectorPosition(aroundSector, sectorX, sectorY - 1);
    }

    if (target->direction_ == PacketMoveDirectionLL)
    {
        if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
        {
            AddSectorPosition(aroundSector, sectorX - 1, sectorY + 1);
        }

        if (sectorY - 1 >= 0 && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
        {
            AddSectorPosition(aroundSector, sectorX - 1, sectorY - 1);
        }
    }
    else
    {
        if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
        {
            AddSectorPosition(aroundSector, sectorX + 1, sectorY + 1);
        }

        if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
        {
            AddSectorPosition(aroundSector, sectorX + 1, sectorY - 1);
        }
    }

}

SectorUpdateAround* SelectMMOTCPFighter::GetUpdateSectorAround(Character* target)
{
    Profile profile(L"GetUpdateSectorAround");

    int moveIndexX = static_cast<int>(target->characterSectorPos_.x_) - static_cast<int>(target->oldSectorPos_.x_) + 1;
    int moveIndexY = static_cast<int>(target->characterSectorPos_.y_) - static_cast<int>(target->oldSectorPos_.y_) + 1;

    return &sectorUpdateAround_[target->oldSectorPos_.y_][target->oldSectorPos_.x_][moveIndexY][moveIndexX];



}

bool SelectMMOTCPFighter::SectorUpdateCharacter(Character* target)
{
    int targetCurSectorAroundPosX = (target->x_ / SectorXSize);
    int targetCurSectorAroundPosY = (target->y_ / SectorYSize);

    if ((target->characterSectorPos_.x_ != targetCurSectorAroundPosX) || (target->characterSectorPos_.y_ != targetCurSectorAroundPosY))
    {
        target->oldSectorPos_.x_ = target->characterSectorPos_.x_;
        target->oldSectorPos_.y_ = target->characterSectorPos_.y_;
        target->characterSectorPos_.x_ = targetCurSectorAroundPosX;
        target->characterSectorPos_.y_ = targetCurSectorAroundPosY;

        sectorCharacterList_[target->oldSectorPos_.y_][target->oldSectorPos_.x_].remove(target);
        sectorCharacterList_[target->characterSectorPos_.y_][target->characterSectorPos_.x_].push_back(target);

        return true;
    }

    return false;
}

void SelectMMOTCPFighter::SectorUpdate(Character* target)
{
    Profile profile(L"SectorUpdate");

    SectorUpdateAround* updateAround = GetUpdateSectorAround(target);

    SendRemoveSectorUpdate(target, &updateAround->removeSector_);
    SendAddSectorUpdate(target, &updateAround->addSector_);
}

void SelectMMOTCPFighter::MakePacketMoveStart(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketMoveStartForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketMoveStop(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStop) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketCreateMyCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateMyCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketCreateOtherCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketCreateOtherCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketDeleteCharacter(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketDamage(SessionId sessionId, CPacket* packet, SessionId attackId, SessionId damageId, unsigned char damageHp)
{
    *packet << static_cast<unsigned char>(PacketScDamage) << static_cast<unsigned int>(attackId) << static_cast<unsigned int>(damageId) << damageHp;

    SendPacketAround(sessionId, packet, true);
}

void SelectMMOTCPFighter::MakePacketAttack1(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack1) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketAttack2(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack2) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketAttack3(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack3) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketEcho(SessionId sessionId, CPacket* packet, unsigned int time)
{
    *packet << static_cast<unsigned char>(PacketScEcho) << time;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketDeleteCharacterRemoveSector(CPacket* packet, SectorAround* around, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacketToSectors(packet, around);
}

void SelectMMOTCPFighter::MakePacketDeleteCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighter::MakePacketCreateCharacterAddSector(CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacketToSectors(packet, around);
}

void SelectMMOTCPFighter::MakePacketMoveStartAddSector(CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketToSectors(packet, around);
}

void SelectMMOTCPFighter::MakePacketSync(SessionId sessionId, CPacket* packet, SessionId id, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScSync) << static_cast<unsigned int>(id) << x << y;

    SendPacket(sessionId, packet);
}



bool SelectMMOTCPFighter::NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcMoveStart");

    Character* target = characterMap_.at(sessionId);

	SyncOrApplyClientPosition(target, x, y);


    AddMovingCharacter(target);
    target->action_ = direction;

    UpdateCharacterFacingDirection(target, direction);

    CPacket* packetMoveStart = CPacket::Alloc();
    MakePacketMoveStart(target->sessionId_, packetMoveStart, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetMoveStart);

    return true;
}

bool SelectMMOTCPFighter::NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcMoveStop");
    Character* target = characterMap_.at(sessionId);

    SyncOrApplyClientPosition(target, x, y);

    RemoveMovingCharacter(target);

    target->action_ = direction;

    UpdateCharacterFacingDirection(target, direction);

    CPacket* packetMoveStop = CPacket::Alloc();
    MakePacketMoveStop(target->sessionId_, packetMoveStop, target->sessionId_, target->direction_, target->x_, target->y_);
    CPacket::Free(packetMoveStop);

    return true;
}

bool SelectMMOTCPFighter::NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcAttack");
    Character* target = characterMap_.at(sessionId);

    SyncOrApplyClientPosition(target, x, y);


    target->direction_ = direction;

    CPacket* packetAttack = CPacket::Alloc();
    MakePacketAttack1(target->sessionId_, packetAttack, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetAttack);
    HitCheck(target, 1);

    return true;
}

bool SelectMMOTCPFighter::NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcAttack");
    Character* target = characterMap_.at(sessionId);

    SyncOrApplyClientPosition(target, x, y);

    target->direction_ = direction;

    CPacket* packetAttack = CPacket::Alloc();
    MakePacketAttack2(target->sessionId_, packetAttack, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetAttack);
    HitCheck(target, 2);

    return true;
}

bool SelectMMOTCPFighter::NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcAttack");
    Character* target = characterMap_.at(sessionId);

    SyncOrApplyClientPosition(target, x, y);

    target->direction_ = direction;

    CPacket* packetAttack = CPacket::Alloc();
    MakePacketAttack3(target->sessionId_, packetAttack, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetAttack);
    HitCheck(target, 3);
    return true;
}

bool SelectMMOTCPFighter::NetPacketProcEcho(SessionId sessionId, unsigned int time)
{
    Profile profile(L"NetPacketEcho");
    Character* target = characterMap_.at(sessionId);

    CPacket* packetEcho = CPacket::Alloc();
    MakePacketEcho(target->sessionId_, packetEcho, time);
    CPacket::Free(packetEcho);

    return true;
}

Character* SelectMMOTCPFighter::CreateCharacter(SessionId sessionId)
{
    Character* newPlayer = characterFreeList_.Alloc();
    newPlayer->sessionId_ = sessionId;

    newPlayer->direction_ = PacketMoveDirectionRR;
    newPlayer->action_ = PacketMoveDirectionRR;

    newPlayer->x_ = rand() % 6399;
    newPlayer->y_ = rand() % 6399;

    newPlayer->hp_ = DefaultHp;

    newPlayer->characterSectorPos_.x_ = newPlayer->x_ / SectorXSize;
    newPlayer->characterSectorPos_.y_ = newPlayer->y_ / SectorYSize;
    newPlayer->oldSectorPos_.x_ = SectorMaxX;
    newPlayer->oldSectorPos_.y_ = SectorMaxY;

    newPlayer->isMove_ = false;
    newPlayer->movingIndex_ = -1;


    return newPlayer;
}

void SelectMMOTCPFighter::RegisterCharacter(Character* newPlayer)
{
    sectorCharacterList_[newPlayer->characterSectorPos_.y_][newPlayer->characterSectorPos_.x_].push_back(newPlayer);
    characterMap_.insert(std::unordered_map<SessionId, Character*>::value_type(newPlayer->sessionId_, newPlayer));
}

void SelectMMOTCPFighter::SendNewCharacterCreate(Character* newPlayer)
{
    CPacket* packetCreateMyCharacter = CPacket::Alloc();
    MakePacketCreateMyCharacter(newPlayer->sessionId_, packetCreateMyCharacter, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
    CPacket::Free(packetCreateMyCharacter);

    CPacket* packetCreateOtherCharacter = CPacket::Alloc();
    MakePacketCreateOtherCharacter(newPlayer->sessionId_, packetCreateOtherCharacter, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
    CPacket::Free(packetCreateOtherCharacter);
}

void SelectMMOTCPFighter::SendExistingCharactersToNewCharacter(Character* newPlayer)
{
    SectorAround around;
    GetSectorAround(newPlayer->characterSectorPos_.x_, newPlayer->characterSectorPos_.y_, &around);

    for (unsigned int i = 0; i < around.count_; ++i)
    {
        std::list<Character*>::iterator iter;
        for (iter = sectorCharacterList_[around.around_[i].y_][around.around_[i].x_].begin();
            iter != sectorCharacterList_[around.around_[i].y_][around.around_[i].x_].end(); ++iter)
        {
            Character* target = *iter;
            if ((target->sessionId_ == newPlayer->sessionId_))
            {
                continue;
            }

            CPacket* packetCreateOtherCharacterForMe = CPacket::Alloc();
            MakePacketCreateOtherCharacterForMe(newPlayer->sessionId_, packetCreateOtherCharacterForMe, target->sessionId_, target->direction_, target->x_, target->y_, target->hp_);
            CPacket::Free(packetCreateOtherCharacterForMe);

            if (target->isMove_ == true)
            {
                CPacket* packetMoveStartForMe = CPacket::Alloc();
                MakePacketMoveStartForMe(newPlayer->sessionId_, packetMoveStartForMe, target->sessionId_, target->action_, target->x_, target->y_);
                CPacket::Free(packetMoveStartForMe);
            }
        }
    }
}

void SelectMMOTCPFighter::SendCharacterDelete(Character* character)
{
    CPacket* packetDeleteCharacter = CPacket::Alloc();

    MakePacketDeleteCharacter(character->sessionId_, packetDeleteCharacter, character->sessionId_);

    CPacket::Free(packetDeleteCharacter);
}

void SelectMMOTCPFighter::UnregisterCharacter(Character* character)
{
    sectorCharacterList_[character->characterSectorPos_.y_][character->characterSectorPos_.x_].remove(character);
    characterMap_.erase(character->sessionId_);
}

void SelectMMOTCPFighter::ReleaseCharacter(Character* target)
{
    RemoveMovingCharacter(target);
    UnregisterCharacter(target);
    characterFreeList_.Free(target);
    
}

bool SelectMMOTCPFighter::IsClientPositionValid(Character* target, unsigned short x, unsigned short y)
{
    return !((abs(target->x_ - x) > ErrorRange) || (abs(target->y_ - y) > ErrorRange));
}

void SelectMMOTCPFighter::SendSync(Character* target)
{
    CPacket* packetSync = CPacket::Alloc();
    MakePacketSync(target->sessionId_, packetSync, target->sessionId_, target->x_, target->y_);
    CPacket::Free(packetSync);
}

void SelectMMOTCPFighter::ApplyClientPosition(Character* target, unsigned short x, unsigned short y)
{
    target->x_ = x;
    target->y_ = y;

    if (SectorUpdateCharacter(target))
    {
        SectorUpdate(target);
    }
}

void SelectMMOTCPFighter::SyncOrApplyClientPosition(Character* target, unsigned short x, unsigned short y)
{
    if (IsClientPositionValid(target, x, y) == false)
    {
        SendSync(target);
        return;
    }

    ApplyClientPosition(target, x, y);
}

void SelectMMOTCPFighter::UpdateCharacterFacingDirection(Character* target, unsigned char direction)
{
    switch (direction)
    {
    case PacketMoveDirectionRR:
    case PacketMoveDirectionRU:
    case PacketMoveDirectionRD:
        target->direction_ = PacketMoveDirectionRR;
        break;

    case PacketMoveDirectionLU:
    case PacketMoveDirectionLL:
    case PacketMoveDirectionLD:
        target->direction_ = PacketMoveDirectionLL;
        break;
    }
}

bool SelectMMOTCPFighter::CanHitTarget(const Character* attackCharacter, const Character* target, int boundaryX, int boundaryY)
{
    if (attackCharacter == target)
    {
        return false;
    }

    if (attackCharacter->direction_ == PacketMoveDirectionLL)
    {
        if (attackCharacter->x_ < target->x_)
        {
            return false;
        }
    }
    else
    {
        if (attackCharacter->x_ > target->x_)
        {
            return false;
        }
    }

    if (abs(attackCharacter->x_ - target->x_) > boundaryX)
    {
        return false;
    }

    if (abs(attackCharacter->y_ - target->y_) > boundaryY)
    {
        return false;
    }

    return true;
}

void SelectMMOTCPFighter::AddSectorPosition(SectorAround* aroundSector, int sectorX, int sectorY)
{
    aroundSector->around_[aroundSector->count_].x_ = sectorX;
    aroundSector->around_[aroundSector->count_].y_ = sectorY;
    aroundSector->count_++;
}

void SelectMMOTCPFighter::SendRemoveSectorUpdate(Character* target, SectorAround* removeSector)
{
    CPacket* packetDeleteCharacterRemoveSector = CPacket::Alloc();
    MakePacketDeleteCharacterRemoveSector(packetDeleteCharacterRemoveSector, removeSector, target->sessionId_);
    CPacket::Free(packetDeleteCharacterRemoveSector);

    for (unsigned int i = 0; i < removeSector->count_; ++i)
    {
        std::list<Character*>::iterator iter;

        for (iter = sectorCharacterList_[removeSector->around_[i].y_][removeSector->around_[i].x_].begin(); iter != sectorCharacterList_[removeSector->around_[i].y_][removeSector->around_[i].x_].end(); ++iter)
        {
            CPacket* packetDeleteCharacterForMe = CPacket::Alloc();
            MakePacketDeleteCharacterForMe(target->sessionId_, packetDeleteCharacterForMe, (*iter)->sessionId_);
            CPacket::Free(packetDeleteCharacterForMe);
        }
    }

}

void SelectMMOTCPFighter::SendAddSectorUpdate(Character* target, SectorAround* addSector)
{
    CPacket* packetCreateCharacterAddSector = CPacket::Alloc();
    MakePacketCreateCharacterAddSector(packetCreateCharacterAddSector, addSector, target->sessionId_, target->direction_, target->x_, target->y_, target->hp_);
    CPacket::Free(packetCreateCharacterAddSector);

    CPacket* packetMoveStartAddSector = CPacket::Alloc();
    MakePacketMoveStartAddSector(packetMoveStartAddSector, addSector, target->sessionId_, target->action_, target->x_, target->y_);
    CPacket::Free(packetMoveStartAddSector);

    for (unsigned int i = 0; i < addSector->count_; ++i)
    {
        std::list<Character*>::iterator iter;

        for (iter = sectorCharacterList_[addSector->around_[i].y_][addSector->around_[i].x_].begin(); iter != sectorCharacterList_[addSector->around_[i].y_][addSector->around_[i].x_].end(); ++iter)
        {
            Character* createCharacter = *iter;

            if (createCharacter->sessionId_ == target->sessionId_)
            {
                continue;
            }

            CPacket* packetCreateOtherCharacterForMe = CPacket::Alloc();
            MakePacketCreateOtherCharacterForMe(target->sessionId_, packetCreateOtherCharacterForMe, createCharacter->sessionId_, createCharacter->direction_, createCharacter->x_, createCharacter->y_, createCharacter->hp_);
            CPacket::Free(packetCreateOtherCharacterForMe);

            if (createCharacter->isMove_ == true)
            {
                CPacket* packetMoveStartForMe = CPacket::Alloc();
                MakePacketMoveStartForMe(target->sessionId_, packetMoveStartForMe, createCharacter->sessionId_, createCharacter->action_, createCharacter->x_, createCharacter->y_);
                CPacket::Free(packetMoveStartForMe);
            }
        }
    }
}

void SelectMMOTCPFighter::InitializeSectorUpdateAround()
{
    for (int oldSectorY = 0; oldSectorY < SectorMaxY; ++oldSectorY)
    {
        for (int oldSectorX = 0; oldSectorX < SectorMaxX; ++oldSectorX)
        {
            for (int moveSectorY = -1; moveSectorY <= 1; ++moveSectorY)
            {
                for (int moveSectorX = -1; moveSectorX <= 1; ++moveSectorX)
                {
                    int moveIndexX = moveSectorX + 1;
                    int moveIndexY = moveSectorY + 1;

                    SectorUpdateAround* sectorUpdateAround = &sectorUpdateAround_[oldSectorY][oldSectorX][moveIndexY][moveIndexX];

                    sectorUpdateAround->removeSector_.count_ = 0;
                    sectorUpdateAround->addSector_.count_ = 0;

                    if (moveSectorX == 0 && moveSectorY == 0)
                    {
                        continue;
                    }

                    int curSectorX = oldSectorX + moveSectorX;
                    int curSectorY = oldSectorY + moveSectorY;

                    if (curSectorX < 0 || curSectorX >= SectorMaxX)
                    {
                        continue;
                    }

                    if (curSectorY < 0 || curSectorY >= SectorMaxY)
                    {
                        continue;
                    }

                    BuildSectorUpdateAround(oldSectorX, oldSectorY, curSectorX, curSectorY, sectorUpdateAround);
                }
            }
        }
    }

}

void SelectMMOTCPFighter::BuildSectorUpdateAround(int oldSectorX, int oldSectorY, int curSectorX, int curSectorY, SectorUpdateAround* sectorUpdateAround)
{
    SectorAround oldSectorAround;
    SectorAround curSectorAround;

    GetSectorAround(oldSectorX, oldSectorY, &oldSectorAround);
    GetSectorAround(curSectorX, curSectorY, &curSectorAround);

    sectorUpdateAround->removeSector_.count_ = 0;
    sectorUpdateAround->addSector_.count_ = 0;

    unsigned int removeIndex;

    for (unsigned int i = 0; i < oldSectorAround.count_; ++i)
    {
        for (removeIndex = 0; removeIndex < curSectorAround.count_; ++removeIndex)
        {
            if (oldSectorAround.around_[i].x_ == curSectorAround.around_[removeIndex].x_ && oldSectorAround.around_[i].y_ == curSectorAround.around_[removeIndex].y_)
            {
                break;
            }
        }

        if (removeIndex == curSectorAround.count_)
        {
            AddSectorPosition(&sectorUpdateAround->removeSector_, oldSectorAround.around_[i].x_, oldSectorAround.around_[i].y_);
        }
    }

    unsigned int addIndex;

    for (unsigned int i = 0; i < curSectorAround.count_; ++i)
    {
        for (addIndex = 0; addIndex < oldSectorAround.count_; ++addIndex)
        {
            if (curSectorAround.around_[i].x_ == oldSectorAround.around_[addIndex].x_ && curSectorAround.around_[i].y_ == oldSectorAround.around_[addIndex].y_)
            {
                break;
            }
        }

        if (addIndex == oldSectorAround.count_)
        {
            AddSectorPosition(&sectorUpdateAround->addSector_, curSectorAround.around_[i].x_, curSectorAround.around_[i].y_);
        }
    }
}

void SelectMMOTCPFighter::AddMovingCharacter(Character* target)
{
    if (target->movingIndex_ != -1)
    {
        target->isMove_ = true;
        return;
    }

    target->movingIndex_ = static_cast<int>(movingCharacters_.size());
    movingCharacters_.push_back(target);
    target->isMove_ = true;
}

void SelectMMOTCPFighter::RemoveMovingCharacter(Character* target)
{

    if (target->movingIndex_ == -1)
    {
        target->isMove_ = false;
        return;
    }

    int removeIndex = target->movingIndex_;
    int lastIndex = static_cast<int>(movingCharacters_.size()) - 1;

    //스왑 후 뒤를 지워버리기
    if (removeIndex != lastIndex)
    {
        Character* lastCharacter = movingCharacters_[lastIndex];
        movingCharacters_[removeIndex] = lastCharacter;
        lastCharacter->movingIndex_ = removeIndex;
    }

    movingCharacters_.pop_back();

    target->movingIndex_ = -1;
    target->isMove_ = false;
}



void SelectMMOTCPFighter::SendPacketSectorOne(int sectorX, int sectorY, SessionId exceptSessionId, CPacket* packet)
{
    Character* target;
    std::list<Character*>::iterator iter;

    for (iter = sectorCharacterList_[sectorY][sectorX].begin(); iter != sectorCharacterList_[sectorY][sectorX].end(); ++iter)
    {
        target = *iter;

        if (target->sessionId_ == exceptSessionId)
        {
            continue;
        }
        SendPacket(target->sessionId_, packet);
    }
}

void SelectMMOTCPFighter::SendPacketToSectors(CPacket* packet, SectorAround* around, SessionId exceptSessionId)
{
    Profile profile(L"SendPacketToSectors");
    for (unsigned int index = 0; index < around->count_; ++index)
    {
        SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, exceptSessionId, packet);
    }
}


void SelectMMOTCPFighter::SendPacketAround(SessionId sessionId, CPacket* packet, bool sendMe)
{
    Profile profile(L"SendPacketAround");

    Character* target = characterMap_.at(sessionId);
    SectorAround around;

    GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &around);

    SessionId exceptSessionId;

    if (sendMe == true)
    {
        exceptSessionId = InvalidSessionId;
    }
    else
    {
        exceptSessionId = target->sessionId_;
    }

    SendPacketToSectors(packet, &around, exceptSessionId);
}
