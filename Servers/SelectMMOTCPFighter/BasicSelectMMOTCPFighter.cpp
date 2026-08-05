#include "BasicSelectMMOTCPFighter.h"
#include "SelectMMOTCPFighter.h"
#include "Character.h"
#include "CPacket.h"
#include "Profiler.h"
#include "PacketDefine.h"
#include "ContentsCPacket.h"


BasicSelectMMOTCPFighter::BasicSelectMMOTCPFighter() : characterFreeList_(10000)
{
    InitializeSectorUpdateAround();
}

BasicSelectMMOTCPFighter::~BasicSelectMMOTCPFighter()
{

}

void BasicSelectMMOTCPFighter::OnAccept(SessionId sessionId)
{
	Profile profile(L"OnAccept");

    Character* newPlayer = CreateCharacter(sessionId);

    RegisterCharacter(newPlayer);
    SendNewCharacterCreate(newPlayer);
    SendExistingCharactersToNewCharacter(newPlayer);
}

void BasicSelectMMOTCPFighter::OnMessage(SessionId sessionId, unsigned char packetType, CPacket* packet)
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

void BasicSelectMMOTCPFighter::OnRelease(SessionId sessionId)
{
	Profile profile(L"OnRelease");
    Character* target = characterMap_.at(sessionId);

    SendCharacterDelete(target);
    ReleaseCharacter(target);
}

void BasicSelectMMOTCPFighter::OnUpdate()
{
    Profile profile(L"OnUpdate");
    std::unordered_map<SessionId, Character*>::iterator iter;
    for (iter = characterMap_.begin(); iter != characterMap_.end(); ++iter)
    {
        Character* target = iter->second;

        if (target->isMove_)
        {

            GameRun(target);
        }
    }
}

void BasicSelectMMOTCPFighter::GameRun(Character* target)
{
    Profile profile(L"GameRun");
    switch (target->action_)
    {
    case PacketMoveDirectionLL:
        if (target->x_ - 6 < RangeMoveLeft)
        {
            target->isMove_ = false;
            return;
        }

        target->x_ -= 6;

        break;

    case PacketMoveDirectionLU:
        if (target->y_ - 4 < RangeMoveTop || target->x_ - 6 < RangeMoveLeft)
        {
            target->isMove_ = false;
            return;
        }

        target->x_ -= 6;
        target->y_ -= 4;

        break;

    case PacketMoveDirectionUU:
        if (target->y_ - 4 < RangeMoveTop)
        {
            target->isMove_ = false;
            return;
        }

        target->y_ -= 4;

        break;

    case PacketMoveDirectionRU:

        if ((target->y_ - 4 < RangeMoveTop) || (target->x_ + 6 >= RangeMoveRight))
        {
            target->isMove_ = false;
            return;
        }

        target->x_ += 6;
        target->y_ -= 4;

        break;

    case PacketMoveDirectionRR:

        if (target->x_ + 6 >= RangeMoveRight)
        {
            target->isMove_ = false;
            return;
        }
        target->x_ += 6;

        break;

    case PacketMoveDirectionRD:

        if (target->y_ + 4 >= RangeMoveBottom || target->x_ + 6 >= RangeMoveRight)
        {
            target->isMove_ = false;
            return;
        }

        target->x_ += 6;
        target->y_ += 4;

        break;

    case PacketMoveDirectionDD:
        if (target->y_ + 4 >= RangeMoveBottom)
        {
            target->isMove_ = false;
            return;
        }

        target->y_ += 4;

        break;

    case PacketMoveDirectionLD:
        if (target->y_ + 4 >= RangeMoveBottom || target->x_ - 6 < RangeMoveLeft)
        {
            target->isMove_ = false;
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


void BasicSelectMMOTCPFighter::HitCheck(Character* attackCharacter, int attackNumber)
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

void BasicSelectMMOTCPFighter::GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
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

void BasicSelectMMOTCPFighter::GetSectorAroundForHit(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
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

void BasicSelectMMOTCPFighter::GetUpdateSectorAround(Character* target, SectorUpdateAround* updateAround)
{
    Profile profile(L"GetUpdateSectorAround");

    SectorAround oldSectorAround;
    SectorAround curSectorAround;
    SectorAround* removeSector = &updateAround->removeSector_;
    SectorAround* addSector = &updateAround->addSector_;

    removeSector->count_ = 0;
    addSector->count_ = 0;

    GetSectorAround(target->oldSectorPos_.x_, target->oldSectorPos_.y_, &oldSectorAround);
    GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &curSectorAround);

    for (unsigned int oldIndex = 0; oldIndex < oldSectorAround.count_; ++oldIndex)
    {
        bool isRemoveSector = true;

        for (unsigned int curIndex = 0; curIndex < curSectorAround.count_; ++curIndex)
        {
            if (oldSectorAround.around_[oldIndex].x_ == curSectorAround.around_[curIndex].x_ &&  oldSectorAround.around_[oldIndex].y_ == curSectorAround.around_[curIndex].y_)
            {
                isRemoveSector = false;
                break;
            }
        }

        if (isRemoveSector)
        {
            AddSectorPosition(removeSector, oldSectorAround.around_[oldIndex].x_, oldSectorAround.around_[oldIndex].y_);
        }
    }

    for (unsigned int curIndex = 0; curIndex < curSectorAround.count_; ++curIndex)
    {
        bool isAddSector = true;

        for (unsigned int oldIndex = 0; oldIndex < oldSectorAround.count_; ++oldIndex)
        {
            if (curSectorAround.around_[curIndex].x_ == oldSectorAround.around_[oldIndex].x_ && curSectorAround.around_[curIndex].y_ == oldSectorAround.around_[oldIndex].y_)
            {
                isAddSector = false;
                break;
            }
        }

        if (isAddSector)
        {
            AddSectorPosition(addSector, curSectorAround.around_[curIndex].x_, curSectorAround.around_[curIndex].y_);
        }
    }



}

bool BasicSelectMMOTCPFighter::SectorUpdateCharacter(Character* target)
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

void BasicSelectMMOTCPFighter::SectorUpdate(Character* target)
{
    Profile profile(L"SectorUpdate");
    SectorUpdateAround updateAround;
    GetUpdateSectorAround(target, &updateAround);

    SendRemoveSectorUpdate(target, &updateAround.removeSector_);
    SendAddSectorUpdate(target, &updateAround.addSector_);
}

void BasicSelectMMOTCPFighter::MakePacketMoveStart(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketMoveStartForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacket(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketMoveStop(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStop) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketCreateMyCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateMyCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacket(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketCreateOtherCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacket(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketCreateOtherCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacketAround(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketDeleteCharacter(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacketAround(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketDamage(SessionId sessionId, CPacket* packet, SessionId attackId, SessionId damageId, unsigned char damageHp)
{
    *packet << static_cast<unsigned char>(PacketScDamage) << static_cast<unsigned int>(attackId) << static_cast<unsigned int>(damageId) << damageHp;

    SendPacketAround(sessionId, packet, true);
}

void BasicSelectMMOTCPFighter::MakePacketAttack1(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack1) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketAttack2(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack2) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketAttack3(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack3) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketEcho(SessionId sessionId, CPacket* packet, unsigned int time)
{
    *packet << static_cast<unsigned char>(PacketScEcho) << time;

    SendPacket(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketDeleteCharacterRemoveSector(CPacket* packet, SectorAround* around, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacketToSectors(packet, around);
}

void BasicSelectMMOTCPFighter::MakePacketDeleteCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacket(sessionId, packet);
}

void BasicSelectMMOTCPFighter::MakePacketCreateCharacterAddSector(CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacketToSectors(packet, around);
}

void BasicSelectMMOTCPFighter::MakePacketMoveStartAddSector(CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketToSectors(packet, around);
}

void BasicSelectMMOTCPFighter::MakePacketSync(SessionId sessionId, CPacket* packet, SessionId id, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScSync) << static_cast<unsigned int>(id) << x << y;

    SendPacket(sessionId, packet);
}


bool BasicSelectMMOTCPFighter::NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcMoveStart");

    Character* target = characterMap_.at(sessionId);

    SyncOrApplyClientPosition(target, x, y);


    target->isMove_ = true;
    target->action_ = direction;

    UpdateCharacterFacingDirection(target, direction);

    CPacket* packetMoveStart = CPacket::Alloc();
    MakePacketMoveStart(target->sessionId_, packetMoveStart, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetMoveStart);

    return true;
}

bool BasicSelectMMOTCPFighter::NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcMoveStop");
    Character* target = characterMap_.at(sessionId);

    SyncOrApplyClientPosition(target, x, y);

    target->isMove_ = false;

    target->action_ = direction;

    UpdateCharacterFacingDirection(target, direction);

    CPacket* packetMoveStop = CPacket::Alloc();
    MakePacketMoveStop(target->sessionId_, packetMoveStop, target->sessionId_, target->direction_, target->x_, target->y_);
    CPacket::Free(packetMoveStop);

    return true;
}

bool BasicSelectMMOTCPFighter::NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
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

bool BasicSelectMMOTCPFighter::NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
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

bool BasicSelectMMOTCPFighter::NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
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

bool BasicSelectMMOTCPFighter::NetPacketProcEcho(SessionId sessionId, unsigned int time)
{
    Profile profile(L"NetPacketEcho");
    Character* target = characterMap_.at(sessionId);

    CPacket* packetEcho = CPacket::Alloc();
    MakePacketEcho(target->sessionId_, packetEcho, time);
    CPacket::Free(packetEcho);

    return true;
}

Character* BasicSelectMMOTCPFighter::CreateCharacter(SessionId sessionId)
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


    return newPlayer;
}

void BasicSelectMMOTCPFighter::RegisterCharacter(Character* newPlayer)
{
    sectorCharacterList_[newPlayer->characterSectorPos_.y_][newPlayer->characterSectorPos_.x_].push_back(newPlayer);
    characterMap_.insert(std::unordered_map<SessionId, Character*>::value_type(newPlayer->sessionId_, newPlayer));
}

void BasicSelectMMOTCPFighter::SendNewCharacterCreate(Character* newPlayer)
{
    CPacket* packetCreateMyCharacter = CPacket::Alloc();
    MakePacketCreateMyCharacter(newPlayer->sessionId_, packetCreateMyCharacter, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
    CPacket::Free(packetCreateMyCharacter);

    CPacket* packetCreateOtherCharacter = CPacket::Alloc();
    MakePacketCreateOtherCharacter(newPlayer->sessionId_, packetCreateOtherCharacter, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
    CPacket::Free(packetCreateOtherCharacter);
}

void BasicSelectMMOTCPFighter::SendExistingCharactersToNewCharacter(Character* newPlayer)
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

void BasicSelectMMOTCPFighter::SendCharacterDelete(Character* character)
{
    CPacket* packetDeleteCharacter = CPacket::Alloc();

    MakePacketDeleteCharacter(character->sessionId_, packetDeleteCharacter, character->sessionId_);

    CPacket::Free(packetDeleteCharacter);
}

void BasicSelectMMOTCPFighter::UnregisterCharacter(Character* character)
{
    sectorCharacterList_[character->characterSectorPos_.y_][character->characterSectorPos_.x_].remove(character);
    characterMap_.erase(character->sessionId_);
}

void BasicSelectMMOTCPFighter::ReleaseCharacter(Character* character)
{
    UnregisterCharacter(character);
    characterFreeList_.Free(character);
}

bool BasicSelectMMOTCPFighter::IsClientPositionValid(Character* target, unsigned short x, unsigned short y)
{
    return !((abs(target->x_ - x) > ErrorRange) || (abs(target->y_ - y) > ErrorRange));
}

void BasicSelectMMOTCPFighter::SendSync(Character* target)
{
    CPacket* packetSync = CPacket::Alloc();
    MakePacketSync(target->sessionId_, packetSync, target->sessionId_, target->x_, target->y_);
    CPacket::Free(packetSync);
}

void BasicSelectMMOTCPFighter::ApplyClientPosition(Character* target, unsigned short x, unsigned short y)
{
    target->x_ = x;
    target->y_ = y;

    if (SectorUpdateCharacter(target))
    {
        SectorUpdate(target);
    }
}

void BasicSelectMMOTCPFighter::SyncOrApplyClientPosition(Character* target, unsigned short x, unsigned short y)
{
    if (IsClientPositionValid(target, x, y) == false)
    {
        SendSync(target);
        return;
    }

    ApplyClientPosition(target, x, y);
}

void BasicSelectMMOTCPFighter::UpdateCharacterFacingDirection(Character* target, unsigned char direction)
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

bool BasicSelectMMOTCPFighter::CanHitTarget(const Character* attackCharacter, const Character* target, int boundaryX, int boundaryY)
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

void BasicSelectMMOTCPFighter::AddSectorPosition(SectorAround* aroundSector, int sectorX, int sectorY)
{
    aroundSector->around_[aroundSector->count_].x_ = sectorX;
    aroundSector->around_[aroundSector->count_].y_ = sectorY;
    aroundSector->count_++;
}

void BasicSelectMMOTCPFighter::SendRemoveSectorUpdate(Character* target, SectorAround* removeSector)
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

void BasicSelectMMOTCPFighter::SendAddSectorUpdate(Character* target, SectorAround* addSector)
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

void BasicSelectMMOTCPFighter::InitializeSectorUpdateAround()
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

void BasicSelectMMOTCPFighter::BuildSectorUpdateAround(int oldSectorX, int oldSectorY, int curSectorX, int curSectorY, SectorUpdateAround* sectorUpdateAround)
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



void BasicSelectMMOTCPFighter::SendPacketSectorOne(int sectorX, int sectorY, SessionId exceptSessionId, CPacket* packet)
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

void BasicSelectMMOTCPFighter::SendPacketToSectors(CPacket* packet, SectorAround* around, SessionId exceptSessionId)
{
    Profile profile(L"SendPacketToSectors");
    for (unsigned int index = 0; index < around->count_; ++index)
    {
        SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, exceptSessionId, packet);
    }
}


void BasicSelectMMOTCPFighter::SendPacketAround(SessionId sessionId, CPacket* packet, bool sendMe)
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
