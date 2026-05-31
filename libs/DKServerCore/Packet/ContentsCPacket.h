#pragma once

#include "CPacket.h"

class ContentsCPacket
{
private:
    ContentsCPacket();

public:
    ContentsCPacket(ContentsCPacket& copy);
    ContentsCPacket(ContentsCPacket* copy);
    ContentsCPacket(CPacket* allocBuffer);

    ~ContentsCPacket();

    static CPacket* MakeContentsPacket();
    static CPacket* MakeContentsPacket(CPacket* other);
    static CPacket* MakeContentsPacket(ContentsCPacket* other);

    void IncreaseRefCount();

    ContentsCPacket& operator<<(unsigned char value);
    ContentsCPacket& operator<<(char value);

    ContentsCPacket& operator<<(short value);
    ContentsCPacket& operator<<(unsigned short value);

    ContentsCPacket& operator<<(int value);
    ContentsCPacket& operator<<(long value);
    ContentsCPacket& operator<<(unsigned int value);
    ContentsCPacket& operator<<(float value);

    ContentsCPacket& operator<<(__int64 value);
    ContentsCPacket& operator<<(double value);

    ContentsCPacket& operator>>(unsigned char& value);
    ContentsCPacket& operator>>(char& value);

    ContentsCPacket& operator>>(short& value);
    ContentsCPacket& operator>>(unsigned short& value);

    ContentsCPacket& operator>>(int& value);
    ContentsCPacket& operator>>(unsigned int& value);
    ContentsCPacket& operator>>(float& value);

    ContentsCPacket& operator>>(__int64& value);
    ContentsCPacket& operator>>(double& value);

    int GetData(char* destination, int destinationSize);
    int PutData(char* source, int sourceSize);
    int GetDataSize() const;

public:
    CPacket* packetBuffer_;
};