#include "SelectMMOTCPFighterPQ.h"
#include "Character.h"
#include "CPacket.h"
#include "Profiler.h"
#include "PacketDefine.h"
#include "ContentsCPacket.h"


SelectMMOTCPFighterPQ::SelectMMOTCPFighterPQ() : characterFreeList_(10000)
{

}

SelectMMOTCPFighterPQ::~SelectMMOTCPFighterPQ()
{

}

void SelectMMOTCPFighterPQ::OnAccept(SessionId sessionId)
{
    Character* newPlayer = CreateCharacter(sessionId);

    RegisterCharacter(newPlayer);
    SendNewCharacterCreate(newPlayer);
    SendExistingCharactersToNewCharacter(newPlayer);

   








}

void SelectMMOTCPFighterPQ::OnMessage(SessionId sessionId, unsigned char packetType, CPacket* packet)
{
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

void SelectMMOTCPFighterPQ::OnRelease(SessionId sessionId)
{
    Character* target = characterMap_.at(sessionId);

    CPacket* packetDeleteCharacter = CPacket::Alloc();
    MakePacketDeleteCharacter(target->sessionId_, packetDeleteCharacter, target->sessionId_);
    CPacket::Free(packetDeleteCharacter);

    sectorCharacterList_[target->characterSectorPos_.y_][target->characterSectorPos_.x_].remove(target);
    characterMap_.erase(sessionId);
    characterFreeList_.Free(target);
}

void SelectMMOTCPFighterPQ::OnUpdate()
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

void SelectMMOTCPFighterPQ::GameRun(Character* target)
{
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


void SelectMMOTCPFighterPQ::HitCheck(Character* attackCharacter, int attackNumber)
{
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

    if (attackCharacter->direction_ == PacketMoveDirectionLL)
    {
        GetSectorAroundForHitLeft(attackCharacter, boundaryX, boundaryY, &hitCheckSector);

        for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
        {
            std::list<Character*>::iterator iter;
            for (iter = sectorCharacterList_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].begin(); iter != sectorCharacterList_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].end(); ++iter)
            {
                Character* target = *iter;

                if ((attackCharacter == target) || (attackCharacter->x_ < target->x_))
                {
                    continue;
                }

                if (abs(attackCharacter->y_ - target->y_) <= boundaryY && abs(attackCharacter->x_ - target->x_) <= boundaryX)
                {
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
    }
    else
    {
        GetSectorAroundForHitRight(attackCharacter, boundaryX, boundaryY, &hitCheckSector);
        for (unsigned int i = 0; i < hitCheckSector.count_; ++i)
        {
            std::list<Character*>::iterator iter;
            for (iter = sectorCharacterList_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].begin(); iter != sectorCharacterList_[hitCheckSector.around_[i].y_][hitCheckSector.around_[i].x_].end(); ++iter)
            {
                Character* target = *iter;

                if ((attackCharacter == target) || (attackCharacter->x_ > target->x_))
                {
                    continue;
                }

                if (abs(attackCharacter->y_ - target->y_) <= boundaryY && abs(attackCharacter->x_ - target->x_) <= boundaryX)
                {
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
    }
}

void SelectMMOTCPFighterPQ::GetSectorAround(int sectorX, int sectorY, SectorAround* aroundSector)
{
    aroundSector->count_ = 0;

    aroundSector->around_[aroundSector->count_].x_ = sectorX;
    aroundSector->around_[aroundSector->count_].y_ = sectorY;
    aroundSector->count_++;

    if (sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        aroundSector->count_++;
    }
    if (sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        aroundSector->count_++;
    }

    if (sectorY + 1 < SectorMaxY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        aroundSector->count_++;
    }

    if (sectorY - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        aroundSector->count_++;
    }

    if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        aroundSector->count_++;
    }

    if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        aroundSector->count_++;
    }

    if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        aroundSector->count_++;
    }
    if (sectorY - 1 >= 0 && sectorX - 1 >= 0)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        aroundSector->count_++;
    }
}

void SelectMMOTCPFighterPQ::GetSectorAroundForHitLeft(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
{
    int sectorX = target->characterSectorPos_.x_;
    int sectorY = target->characterSectorPos_.y_;

    aroundSector->count_ = 0;

    aroundSector->around_[aroundSector->count_].x_ = sectorX;
    aroundSector->around_[aroundSector->count_].y_ = sectorY;
    aroundSector->count_++;

    int targetValidPosX = ((target->x_ - boundaryX) / SectorXSize);
    int targetValidPosYAbove = ((target->y_ - boundaryY) / SectorYSize);
    int targetValidPosYBelow = ((target->y_ + boundaryY) / SectorYSize);

    if (sectorX - 1 >= 0 && targetValidPosX != sectorX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        aroundSector->count_++;
    }

    if (sectorY + 1 < SectorMaxY && targetValidPosYBelow != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        aroundSector->count_++;
    }

    if (sectorY - 1 >= 0 && targetValidPosYAbove != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        aroundSector->count_++;
    }

    if (sectorY + 1 < SectorMaxY && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        aroundSector->count_++;
    }
    if (sectorY - 1 >= 0 && sectorX - 1 >= 0 && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX - 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        aroundSector->count_++;
    }
}

void SelectMMOTCPFighterPQ::GetSectorAroundForHitRight(Character* target, int boundaryX, int boundaryY, SectorAround* aroundSector)
{
    int sectorX = target->characterSectorPos_.x_;
    int sectorY = target->characterSectorPos_.y_;

    aroundSector->count_ = 0;

    aroundSector->around_[aroundSector->count_].x_ = sectorX;
    aroundSector->around_[aroundSector->count_].y_ = sectorY;
    aroundSector->count_++;

    int targetValidPosX = ((target->x_ + boundaryX) / SectorXSize);
    int targetValidPosYAbove = ((target->y_ - boundaryY) / SectorYSize);
    int targetValidPosYBelow = ((target->y_ + boundaryY) / SectorYSize);

    if (sectorX + 1 < SectorMaxX && targetValidPosX != sectorX)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY;
        aroundSector->count_++;
    }

    if (sectorY + 1 < SectorMaxY && targetValidPosYBelow != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        aroundSector->count_++;
    }

    if (sectorY - 1 >= 0 && targetValidPosYAbove != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        aroundSector->count_++;
    }

    if (sectorY + 1 < SectorMaxY && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYBelow != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY + 1;
        aroundSector->count_++;
    }

    if (sectorY - 1 >= 0 && sectorX + 1 < SectorMaxX && targetValidPosX != sectorX && targetValidPosYAbove != sectorY)
    {
        aroundSector->around_[aroundSector->count_].x_ = sectorX + 1;
        aroundSector->around_[aroundSector->count_].y_ = sectorY - 1;
        aroundSector->count_++;
    }
}

void SelectMMOTCPFighterPQ::GetUpdateSectorAround(Character* target, SectorAround* removeSector, SectorAround* addSector)
{
    removeSector->count_ = 0;
    addSector->count_ = 0;

    // ->
    if ((target->characterSectorPos_.y_ == target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ > target->oldSectorPos_.x_))
    {
        if (target->characterSectorPos_.y_ == 0)
        {
            if (target->oldSectorPos_.x_ != 0)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
                removeSector->count_++;
            }

            if (target->characterSectorPos_.x_ + 1 != SectorMaxX)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
                addSector->count_++;
            }

            return;
        }

        if (target->characterSectorPos_.y_ == SectorMaxY - 1)
        {
            if (target->oldSectorPos_.x_ != 0)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
                removeSector->count_++;
            }

            if (target->characterSectorPos_.x_ + 1 != SectorMaxX)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
                addSector->count_++;
            }

            return;
        }

        if (target->oldSectorPos_.x_ != 0)
        {
            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
            removeSector->count_++;
        }

        if (target->characterSectorPos_.x_ + 1 != SectorMaxX)
        {
            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
            addSector->count_++;
        }

        return;
    }

    //<-

    if ((target->characterSectorPos_.y_ == target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ < target->oldSectorPos_.x_))
    {
        if (target->characterSectorPos_.y_ == 0)
        {
            if (target->characterSectorPos_.x_ != 0)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
                addSector->count_++;
            }

            if (target->oldSectorPos_.x_ + 1 != SectorMaxX)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
                removeSector->count_++;
            }

            return;
        }

        if (target->characterSectorPos_.y_ == SectorMaxY - 1)
        {
            if (target->characterSectorPos_.x_ != 0)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
                addSector->count_++;
            }

            if (target->oldSectorPos_.x_ + 1 != SectorMaxX)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
                removeSector->count_++;
            }

            return;
        }

        if (target->characterSectorPos_.x_ != 0)
        {
            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
            addSector->count_++;
        }

        if (target->oldSectorPos_.x_ + 1 != SectorMaxX)
        {
            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
            removeSector->count_++;
        }

        return;
    }

    //..
    if ((target->characterSectorPos_.y_ < target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ == target->oldSectorPos_.x_))
    {
        if (target->characterSectorPos_.x_ == 0)
        {
            if (target->characterSectorPos_.y_ != 0)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
                addSector->count_++;
            }

            if (target->oldSectorPos_.y_ + 1 != SectorMaxY)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
                removeSector->count_++;
            }

            return;
        }

        if (target->characterSectorPos_.x_ == SectorMaxX - 1)
        {
            if (target->characterSectorPos_.y_ != 0)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
                addSector->count_++;
            }

            if (target->oldSectorPos_.y_ + 1 != SectorMaxY)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
                removeSector->count_++;
            }

            return;
        }

        if (target->characterSectorPos_.y_ != 0)
        {
            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ - 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
            addSector->count_++;
        }

        if (target->oldSectorPos_.y_ + 1 != SectorMaxY)
        {
            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ + 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
            removeSector->count_++;
        }

        return;
    }

    //..
    if ((target->characterSectorPos_.y_ > target->oldSectorPos_.y_) && (target->characterSectorPos_.x_ == target->oldSectorPos_.x_))
    {
        if (target->characterSectorPos_.x_ == 0)
        {
            if (target->oldSectorPos_.y_ != 0)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
                removeSector->count_++;
            }

            if (target->characterSectorPos_.y_ + 1 != SectorMaxY)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
                addSector->count_++;
            }

            return;
        }

        if (target->characterSectorPos_.x_ == SectorMaxX - 1)
        {
            if (target->oldSectorPos_.y_ != 0)
            {
                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
                removeSector->count_++;

                removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
                removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
                removeSector->count_++;
            }

            if (target->characterSectorPos_.y_ + 1 != SectorMaxY)
            {
                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
                addSector->count_++;

                addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
                addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
                addSector->count_++;
            }

            return;
        }

        if (target->oldSectorPos_.y_ != 0)
        {
            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ - 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_ + 1;
            removeSector->count_++;

            removeSector->around_[removeSector->count_].y_ = target->oldSectorPos_.y_ - 1;
            removeSector->around_[removeSector->count_].x_ = target->oldSectorPos_.x_;
            removeSector->count_++;
        }

        if (target->characterSectorPos_.y_ + 1 != SectorMaxY)
        {
            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ + 1;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_;
            addSector->count_++;

            addSector->around_[addSector->count_].y_ = target->characterSectorPos_.y_ + 1;
            addSector->around_[addSector->count_].x_ = target->characterSectorPos_.x_ - 1;
            addSector->count_++;
        }

        return;
    }

    SectorAround oldSectorAround;
    SectorAround curSectorAround;

    GetSectorAround(target->oldSectorPos_.x_, target->oldSectorPos_.y_, &oldSectorAround);
    GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &curSectorAround);

    unsigned int removeIndex;
    for (unsigned int i = 0; i < oldSectorAround.count_; ++i)
    {
        for (removeIndex = 0; removeIndex < curSectorAround.count_; removeIndex++)
        {
            if (oldSectorAround.around_[i].x_ == curSectorAround.around_[removeIndex].x_ && oldSectorAround.around_[i].y_ == curSectorAround.around_[removeIndex].y_)
            {
                break;
            }
        }
        if (removeIndex == curSectorAround.count_)
        {
            removeSector->around_[removeSector->count_].x_ = oldSectorAround.around_[i].x_;
            removeSector->around_[removeSector->count_].y_ = oldSectorAround.around_[i].y_;
            removeSector->count_++;
        }
    }

    addSector->count_ = 0;
    unsigned int j;
    for (unsigned int i = 0; i < curSectorAround.count_; ++i)
    {
        for (j = 0; j < oldSectorAround.count_; j++)
        {
            if (oldSectorAround.around_[j].x_ == curSectorAround.around_[i].x_ && oldSectorAround.around_[j].y_ == curSectorAround.around_[i].y_)
            {
                break;
            }
        }
        if (j == oldSectorAround.count_)
        {
            addSector->around_[addSector->count_].x_ = curSectorAround.around_[i].x_;
            addSector->around_[addSector->count_].y_ = curSectorAround.around_[i].y_;
            addSector->count_++;
        }
    }


}

bool SelectMMOTCPFighterPQ::SectorUpdateCharacter(Character* target)
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

void SelectMMOTCPFighterPQ::SectorUpdate(Character* target)
{
    Profile profile(L"SectorUpdate");

    SectorAround removeSector;
    SectorAround addSector;

    GetUpdateSectorAround(target, &removeSector, &addSector);

    CPacket* packetDeleteCharacterRemoveSector = CPacket::Alloc();
    MakePacketDeleteCharacterRemoveSector(target->sessionId_, packetDeleteCharacterRemoveSector, &removeSector, target->sessionId_);
    CPacket::Free(packetDeleteCharacterRemoveSector);

    //removeSector에 있는 애들의 삭제를 나에게 보냄.
    for (unsigned int i = 0; i < removeSector.count_; ++i)
    {
        std::list<Character*>::iterator iter;
        for (iter = sectorCharacterList_[removeSector.around_[i].y_][removeSector.around_[i].x_].begin(); iter != sectorCharacterList_[removeSector.around_[i].y_][removeSector.around_[i].x_].end(); ++iter)
        {
            CPacket* packetDeleteCharacterForMe = CPacket::Alloc();
            MakePacketDeleteCharacterForMe(target->sessionId_, packetDeleteCharacterForMe, (*iter)->sessionId_);
            CPacket::Free(packetDeleteCharacterForMe);
        }
    }

    //add에 있는 애들에게 나의 생성을 보냄.
    CPacket* packetCreateCharacterAddSector = CPacket::Alloc();
    MakePacketCreateCharacterAddSector(target->sessionId_, packetCreateCharacterAddSector, &addSector, target->sessionId_, target->direction_, target->x_, target->y_, target->hp_);
    CPacket::Free(packetCreateCharacterAddSector);
    //이동 정보도 보내줘야함.

    CPacket* packetMoveStartAddSector = CPacket::Alloc();
    MakePacketMoveStartAddSector(target->sessionId_, packetMoveStartAddSector, &addSector, target->sessionId_, target->action_, target->x_, target->y_);
    CPacket::Free(packetMoveStartAddSector);

    for (unsigned int i = 0; i < addSector.count_; ++i)
    {
        std::list<Character*>::iterator iterCreate;
        for (iterCreate = sectorCharacterList_[addSector.around_[i].y_][addSector.around_[i].x_].begin();
            iterCreate != sectorCharacterList_[addSector.around_[i].y_][addSector.around_[i].x_].end(); ++iterCreate)
        {
            Character* createCharacter = *iterCreate;
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

void SelectMMOTCPFighterPQ::MakePacketMoveStart(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketMoveStartForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketMoveStop(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStop) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketCreateMyCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateMyCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketCreateOtherCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketCreateOtherCharacter(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketDeleteCharacter(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketDamage(SessionId sessionId, CPacket* packet, SessionId attackId, SessionId damageId, unsigned char damageHp)
{
    *packet << static_cast<unsigned char>(PacketScDamage) << static_cast<unsigned int>(attackId) << static_cast<unsigned int>(damageId) << damageHp;

    SendPacketAround(sessionId, packet, true);
}

void SelectMMOTCPFighterPQ::MakePacketAttack1(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack1) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketAttack2(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack2) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketAttack3(SessionId sessionId, CPacket* packet, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScAttack3) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAround(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketEcho(SessionId sessionId, CPacket* packet, unsigned int time)
{
    *packet << static_cast<unsigned char>(PacketScEcho) << time;

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketDeleteCharacterRemoveSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacketAroundRemoveSector(sessionId, packet, around);
}

void SelectMMOTCPFighterPQ::MakePacketDeleteCharacterForMe(SessionId sessionId, CPacket* packet, SessionId id)
{
    *packet << static_cast<unsigned char>(PacketScDeleteCharacter) << static_cast<unsigned int>(id);

    SendPacket(sessionId, packet);
}

void SelectMMOTCPFighterPQ::MakePacketCreateCharacterAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y, unsigned char hp)
{
    *packet << static_cast<unsigned char>(PacketScCreateOtherCharacter) << static_cast<unsigned int>(id) << direction << x << y << hp;

    SendPacketAroundAddSector(sessionId, packet, around);
}

void SelectMMOTCPFighterPQ::MakePacketMoveStartAddSector(SessionId sessionId, CPacket* packet, SectorAround* around, SessionId id, unsigned char direction, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScMoveStart) << static_cast<unsigned int>(id) << direction << x << y;

    SendPacketAroundAddSector(sessionId, packet, around);
}

void SelectMMOTCPFighterPQ::MakePacketSync(SessionId sessionId, CPacket* packet, SessionId id, unsigned short x, unsigned short y)
{
    *packet << static_cast<unsigned char>(PacketScSync) << static_cast<unsigned int>(id) << x << y;

    SendPacket(sessionId, packet);
}



bool SelectMMOTCPFighterPQ::NetPacketProcMoveStart(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcMoveStart");

    Character* target = characterMap_.at(sessionId);

    if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
    {
        CPacket* packetSync = CPacket::Alloc();
        MakePacketSync(target->sessionId_, packetSync, target->sessionId_, target->x_, target->y_);
        CPacket::Free(packetSync);
    }
    else
    {
        target->x_ = x;
        target->y_ = y;

        if (SectorUpdateCharacter(target))
        {
            SectorUpdate(target);
        }
    }

    target->isMove_ = true;
    target->action_ = direction;

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

    CPacket* packetMoveStart = CPacket::Alloc();
    MakePacketMoveStart(target->sessionId_, packetMoveStart, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetMoveStart);

    return true;
}

bool SelectMMOTCPFighterPQ::NetPacketProcMoveStop(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcMoveStop");
    Character* target = characterMap_.at(sessionId);

    if ((abs(target->x_ - x) > ErrorRange) || (abs(target->y_ - y) > ErrorRange))
    {
        CPacket* packetSync = CPacket::Alloc();
        MakePacketSync(target->sessionId_, packetSync, target->sessionId_, target->x_, target->y_);
        CPacket::Free(packetSync);
    }
    else
    {
        target->x_ = x;
        target->y_ = y;

        if (SectorUpdateCharacter(target))
        {
            SectorUpdate(target);
        }
    }
    target->isMove_ = false;

    target->action_ = direction;

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

    CPacket* packetMoveStop = CPacket::Alloc();
    MakePacketMoveStop(target->sessionId_, packetMoveStop, target->sessionId_, target->direction_, target->x_, target->y_);
    CPacket::Free(packetMoveStop);

    return true;
}

bool SelectMMOTCPFighterPQ::NetPacketProcAttack1(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcAttack");
    Character* target = characterMap_.at(sessionId);

    if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
    {
        CPacket* packetSync = CPacket::Alloc();
        MakePacketSync(target->sessionId_, packetSync, target->sessionId_, target->x_, target->y_);
        CPacket::Free(packetSync);
    }
    else
    {
        target->x_ = x;
        target->y_ = y;

        if (SectorUpdateCharacter(target))
        {
            SectorUpdate(target);
        }
    }

    target->direction_ = direction;

    CPacket* packetAttack = CPacket::Alloc();
    MakePacketAttack1(target->sessionId_, packetAttack, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetAttack);
    HitCheck(target, 1);

    return true;
}

bool SelectMMOTCPFighterPQ::NetPacketProcAttack2(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcAttack");
    Character* target = characterMap_.at(sessionId);

    if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
    {
        CPacket* packetSync = CPacket::Alloc();
        MakePacketSync(target->sessionId_, packetSync, target->sessionId_, target->x_, target->y_);
        CPacket::Free(packetSync);
    }
    else
    {
        target->x_ = x;
        target->y_ = y;

        if (SectorUpdateCharacter(target))
        {
            SectorUpdate(target);
        }

    }

    target->direction_ = direction;
    CPacket* packetAttack = CPacket::Alloc();
    MakePacketAttack2(target->sessionId_, packetAttack, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetAttack);
    HitCheck(target, 2);

    return true;
}

bool SelectMMOTCPFighterPQ::NetPacketProcAttack3(SessionId sessionId, unsigned char direction, unsigned short x, unsigned short y)
{
    Profile profile(L"NetPacketProcAttack");
    Character* target = characterMap_.at(sessionId);

    if (abs(target->x_ - x) > ErrorRange || abs(target->y_ - y) > ErrorRange)
    {
        CPacket* packetSync = CPacket::Alloc();
        MakePacketSync(target->sessionId_, packetSync, target->sessionId_, target->x_, target->y_);
        CPacket::Free(packetSync);
    }
    else
    {
        target->x_ = x;
        target->y_ = y;

        if (SectorUpdateCharacter(target))
        {
            SectorUpdate(target);
        }

    }

    target->direction_ = direction;

    CPacket* packetAttack = CPacket::Alloc();
    MakePacketAttack3(target->sessionId_, packetAttack, target->sessionId_, direction, target->x_, target->y_);
    CPacket::Free(packetAttack);
    HitCheck(target, 3);
    return true;
}

bool SelectMMOTCPFighterPQ::NetPacketProcEcho(SessionId sessionId, unsigned int time)
{
    Profile profile(L"NetPacketEcho");
    Character* target = characterMap_.at(sessionId);

    CPacket* packetEcho = CPacket::Alloc();
    MakePacketEcho(target->sessionId_, packetEcho, time);
    CPacket::Free(packetEcho);

    return true;
}

Character* SelectMMOTCPFighterPQ::CreateCharacter(SessionId sessionId)
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

void SelectMMOTCPFighterPQ::RegisterCharacter(Character* newPlayer)
{
    sectorCharacterList_[newPlayer->characterSectorPos_.y_][newPlayer->characterSectorPos_.x_].push_back(newPlayer);
    characterMap_.insert(std::unordered_map<SessionId, Character*>::value_type(newPlayer->sessionId_, newPlayer));
}

void SelectMMOTCPFighterPQ::SendNewCharacterCreate(Character* newPlayer)
{
    CPacket* packetCreateMyCharacter = CPacket::Alloc();
    MakePacketCreateMyCharacter(newPlayer->sessionId_, packetCreateMyCharacter, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
    CPacket::Free(packetCreateMyCharacter);

    CPacket* packetCreateOtherCharacter = CPacket::Alloc();
    MakePacketCreateOtherCharacter(newPlayer->sessionId_, packetCreateOtherCharacter, newPlayer->sessionId_, newPlayer->direction_, newPlayer->x_, newPlayer->y_, newPlayer->hp_);
    CPacket::Free(packetCreateOtherCharacter);
}

void SelectMMOTCPFighterPQ::SendExistingCharactersToNewCharacter(Character* newPlayer)
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

void SelectMMOTCPFighterPQ::SendPacketSectorOne(int sectorX, int sectorY, SessionId exceptSessionId, CPacket* packet)
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

void SelectMMOTCPFighterPQ::SendPacketAroundRemoveSector(SessionId sessionId, CPacket* packet, SectorAround* around)
{
    for (unsigned int index = 0; index < around->count_; ++index)
    {
        SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, InvalidSessionId, packet);
    }
}

void SelectMMOTCPFighterPQ::SendPacketAroundAddSector(SessionId sessionId, CPacket* packet, SectorAround* around)
{
    for (unsigned int index = 0; index < around->count_; ++index)
    {
        SendPacketSectorOne(around->around_[index].x_, around->around_[index].y_, InvalidSessionId, packet);
    }
}

void SelectMMOTCPFighterPQ::SendPacketAround(SessionId sessionId, CPacket* packet, bool sendMe)
{
    Profile profile(L"SendPacketAround");

    Character* target = characterMap_.at(sessionId);
    SectorAround around;

    GetSectorAround(target->characterSectorPos_.x_, target->characterSectorPos_.y_, &around);

    if (sendMe)
    {
        for (unsigned int index = 0; index < around.count_; ++index)
        {
            SendPacketSectorOne(around.around_[index].x_, around.around_[index].y_, InvalidSessionId, packet);
        }
    }
    else
    {
        for (unsigned int index = 0; index < around.count_; ++index)
        {
            SendPacketSectorOne(around.around_[index].x_, around.around_[index].y_, target->sessionId_, packet);
        }
    }
}
