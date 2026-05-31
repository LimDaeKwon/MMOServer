#include "CPacket.h"

#include <cstring>

TLSObjectFreeList<CPacket> CPacket::serializeList_(0);

CPacket::CPacket()
    : serializeBuffer_(nullptr),
    bufferSize_(DKServerCore::PacketDefaultBufferSize),
    refCount_(0),
    dataSize_(0),
    writePosition_(DKServerCore::PacketLibHeaderSize),
    readPosition_(DKServerCore::PacketLibHeaderSize),
    encodingFlag_(0)
{
    InitializeCriticalSection(&encodingLock_);
    serializeBuffer_ = new char[DKServerCore::PacketDefaultBufferSize];
}

CPacket::CPacket(int newBufferSize)
    : serializeBuffer_(nullptr),
    bufferSize_(newBufferSize),
    refCount_(0),
    dataSize_(0),
    writePosition_(DKServerCore::PacketLibHeaderSize),
    readPosition_(DKServerCore::PacketLibHeaderSize),
    encodingFlag_(0)
{
    InitializeCriticalSection(&encodingLock_);
    serializeBuffer_ = new char[newBufferSize];
}

CPacket::~CPacket()
{
    DeleteCriticalSection(&encodingLock_);
    delete[] serializeBuffer_;
}

void CPacket::Clear()
{
    dataSize_ = 0;
    writePosition_ = DKServerCore::PacketLibHeaderSize;
    readPosition_ = DKServerCore::PacketLibHeaderSize;
    encodingFlag_ = 0;
}

void CPacket::InitLan()
{
    dataSize_ = 0;
    writePosition_ = 2;
    readPosition_ = 2;
    encodingFlag_ = 0;
}

int CPacket::GetBufferSize() const
{
    return bufferSize_;
}

int CPacket::GetDataSize() const
{
    return dataSize_;
}

char* CPacket::GetBufferPtr()
{
    return serializeBuffer_;
}

char* CPacket::GetWriteBufferPtr()
{
    return serializeBuffer_ + writePosition_;
}

char* CPacket::GetReadPosition()
{
    return serializeBuffer_ + DKServerCore::PacketLibHeaderSize;
}

int CPacket::MoveWritePosition(int size)
{
    if (dataSize_ + size > bufferSize_)
    {
        return -1;
    }

    writePosition_ += size;
    dataSize_ += size;

    return size;
}

int CPacket::MoveReadPosition(int size)
{
    if (dataSize_ - size < 0)
    {
        return -1;
    }

    readPosition_ += size;
    dataSize_ -= size;

    return size;
}

int CPacket::IncreaseRefCount()
{
    return InterlockedIncrement(&refCount_);
}

int CPacket::DecreaseRefCount()
{
    return InterlockedDecrement(&refCount_);
}

CPacket* CPacket::Alloc()
{
    CPacket* packet = serializeList_.Alloc();

    packet->IncreaseRefCount();

    return packet;
}

void CPacket::Free(CPacket* packet)
{
    if (packet->DecreaseRefCount() == 0)
    {
        packet->Clear();
        serializeList_.Free(packet);
    }
}

int CPacket::GetPoolSize()
{
    return serializeList_.GetPoolSize();
}

int CPacket::GetUseSize()
{
    return serializeList_.GetUseCount();
}

int CPacket::GetCapacity()
{
    return serializeList_.GetCapacityCount();
}

CPacket& CPacket::operator=(const CPacket& sourcePacket)
{
    if (bufferSize_ < sourcePacket.dataSize_)
    {
        bufferSize_ = sourcePacket.dataSize_;

        delete[] serializeBuffer_;
        serializeBuffer_ = new char[bufferSize_];
    }

    if (memcpy_s(
        serializeBuffer_,
        bufferSize_,
        sourcePacket.serializeBuffer_,
        sourcePacket.dataSize_) != 0)
    {
        return *this;
    }

    bufferSize_ = sourcePacket.bufferSize_;
    dataSize_ = sourcePacket.dataSize_;
    writePosition_ = sourcePacket.writePosition_;
    readPosition_ = sourcePacket.readPosition_;
    encodingFlag_ = sourcePacket.encodingFlag_;

    return *this;
}

CPacket& CPacket::operator<<(unsigned char value)
{
    if (dataSize_ + sizeof(unsigned char) >= bufferSize_)
    {
        return *this;
    }

    *(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(unsigned char);
    dataSize_ += sizeof(unsigned char);

    return *this;
}

CPacket& CPacket::operator<<(char value)
{
    if (dataSize_ + sizeof(char) >= bufferSize_)
    {
        return *this;
    }

    *(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(char);
    dataSize_ += sizeof(char);

    return *this;
}

CPacket& CPacket::operator<<(short value)
{
    if (dataSize_ + sizeof(short) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<short*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(short);
    dataSize_ += sizeof(short);

    return *this;
}

CPacket& CPacket::operator<<(unsigned short value)
{
    if (dataSize_ + sizeof(unsigned short) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<unsigned short*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(unsigned short);
    dataSize_ += sizeof(unsigned short);

    return *this;
}

CPacket& CPacket::operator<<(int value)
{
    if (dataSize_ + sizeof(int) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<int*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(int);
    dataSize_ += sizeof(int);

    return *this;
}

CPacket& CPacket::operator<<(long value)
{
    if (dataSize_ + sizeof(long) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<long*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(long);
    dataSize_ += sizeof(long);

    return *this;
}

CPacket& CPacket::operator<<(unsigned int value)
{
    if (dataSize_ + sizeof(unsigned int) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<unsigned int*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(unsigned int);
    dataSize_ += sizeof(unsigned int);

    return *this;
}

CPacket& CPacket::operator<<(float value)
{
    if (dataSize_ + sizeof(float) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<float*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(float);
    dataSize_ += sizeof(float);

    return *this;
}

CPacket& CPacket::operator<<(__int64 value)
{
    if (dataSize_ + sizeof(__int64) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<__int64*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(__int64);
    dataSize_ += sizeof(__int64);

    return *this;
}

CPacket& CPacket::operator<<(double value)
{
    if (dataSize_ + sizeof(double) >= bufferSize_)
    {
        return *this;
    }

    *reinterpret_cast<double*>(serializeBuffer_ + writePosition_) = value;

    writePosition_ += sizeof(double);
    dataSize_ += sizeof(double);

    return *this;
}

CPacket& CPacket::operator>>(unsigned char& value)
{
    if (dataSize_ < sizeof(unsigned char))
    {
        return *this;
    }

    value = *reinterpret_cast<unsigned char*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(unsigned char);
    dataSize_ -= sizeof(unsigned char);

    return *this;
}

CPacket& CPacket::operator>>(char& value)
{
    if (dataSize_ < sizeof(char))
    {
        return *this;
    }

    value = *reinterpret_cast<char*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(char);
    dataSize_ -= sizeof(char);

    return *this;
}

CPacket& CPacket::operator>>(short& value)
{
    if (dataSize_ < sizeof(short))
    {
        return *this;
    }

    value = *reinterpret_cast<short*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(short);
    dataSize_ -= sizeof(short);

    return *this;
}

CPacket& CPacket::operator>>(unsigned short& value)
{
    if (dataSize_ < sizeof(unsigned short))
    {
        return *this;
    }

    value = *reinterpret_cast<unsigned short*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(unsigned short);
    dataSize_ -= sizeof(unsigned short);

    return *this;
}

CPacket& CPacket::operator>>(int& value)
{
    if (dataSize_ < sizeof(int))
    {
        return *this;
    }

    value = *reinterpret_cast<int*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(int);
    dataSize_ -= sizeof(int);

    return *this;
}

CPacket& CPacket::operator>>(unsigned int& value)
{
    if (dataSize_ < sizeof(unsigned int))
    {
        return *this;
    }

    value = *reinterpret_cast<unsigned int*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(unsigned int);
    dataSize_ -= sizeof(unsigned int);

    return *this;
}

CPacket& CPacket::operator>>(float& value)
{
    if (dataSize_ < sizeof(float))
    {
        return *this;
    }

    value = *reinterpret_cast<float*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(float);
    dataSize_ -= sizeof(float);

    return *this;
}

CPacket& CPacket::operator>>(__int64& value)
{
    if (dataSize_ < sizeof(__int64))
    {
        return *this;
    }

    value = *reinterpret_cast<__int64*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(__int64);
    dataSize_ -= sizeof(__int64);

    return *this;
}

CPacket& CPacket::operator>>(double& value)
{
    if (dataSize_ < sizeof(double))
    {
        return *this;
    }

    value = *reinterpret_cast<double*>(serializeBuffer_ + readPosition_);

    readPosition_ += sizeof(double);
    dataSize_ -= sizeof(double);

    return *this;
}

int CPacket::GetData(char* destination, int destinationSize)
{
    if (dataSize_ < destinationSize)
    {
        return -1;
    }

    if (memcpy_s(
        destination,
        destinationSize,
        serializeBuffer_ + readPosition_,
        destinationSize) != 0)
    {
        return -1;
    }

    readPosition_ += destinationSize;
    dataSize_ -= destinationSize;

    return destinationSize;
}

int CPacket::PutData(char* source, int sourceSize)
{
    if (dataSize_ + sourceSize > bufferSize_)
    {
        return -1;
    }

    if (memcpy_s(
        serializeBuffer_ + writePosition_,
        sourceSize,
        source,
        sourceSize) != 0)
    {
        return -1;
    }

    writePosition_ += sourceSize;
    dataSize_ += sourceSize;

    return 0;
}

BYTE CPacket::Encode(char* text, WORD size, BYTE randomKey)
{
    unsigned char checksum = 0;

    for (WORD i = 0; i < size; ++i)
    {
        checksum += text[i];
    }

    checksum %= 256;

    unsigned char beforePlain = 0;
    int counter = 1;

    beforePlain = checksum ^ (randomKey + counter + beforePlain);
    checksum = beforePlain ^ (DKServerCore::PacketServerKey + counter);
    ++counter;

    beforePlain = text[0] ^ (randomKey + counter + beforePlain);
    text[0] = beforePlain ^ (checksum + DKServerCore::PacketServerKey + counter);

    for (WORD i = 1; i < size; ++i)
    {
        ++counter;

        beforePlain = text[i] ^ (randomKey + counter + beforePlain);
        text[i] = beforePlain ^ (text[i - 1] + DKServerCore::PacketServerKey + counter);
    }

    return checksum;
}

bool CPacket::Decode(char* text, WORD size, BYTE randomKey)
{
    int counter = 1;

    BYTE beforePlain = text[0] ^ (DKServerCore::PacketServerKey + counter);
    BYTE beforeEncoded = text[0];

    text[0] = beforeEncoded ^ (DKServerCore::PacketServerKey + counter) ^ (randomKey + counter);

    for (WORD i = 1; i < size; ++i)
    {
        ++counter;

        BYTE nowPlain = text[i] ^ (beforeEncoded + DKServerCore::PacketServerKey + counter);

        beforeEncoded = text[i];
        text[i] = nowPlain ^ (beforePlain + randomKey + counter);
        beforePlain = nowPlain;
    }

    BYTE checksum = 0;

    for (unsigned int i = 1; i < size; ++i)
    {
        checksum += text[i];
    }

    if (static_cast<BYTE>(text[0]) != checksum)
    {
        return false;
    }

    return true;
}