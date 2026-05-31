#include "ContentsCPacket.h"

ContentsCPacket::ContentsCPacket()
    : packetBuffer_(nullptr)
{
}

ContentsCPacket::ContentsCPacket(CPacket* allocBuffer)
    : packetBuffer_(allocBuffer)
{
}

ContentsCPacket::ContentsCPacket(ContentsCPacket& copy)
    : packetBuffer_(copy.packetBuffer_)
{
    packetBuffer_->IncreaseRefCount();
}

ContentsCPacket::ContentsCPacket(ContentsCPacket* copy)
    : packetBuffer_(reinterpret_cast<CPacket*>(copy))
{
}

ContentsCPacket::~ContentsCPacket()
{
    CPacket::Free(packetBuffer_);
}

CPacket* ContentsCPacket::MakeContentsPacket()
{
    return CPacket::Alloc();
}

CPacket* ContentsCPacket::MakeContentsPacket(CPacket* other)
{
    return other;
}

CPacket* ContentsCPacket::MakeContentsPacket(ContentsCPacket* other)
{
    return other->packetBuffer_;
}

void ContentsCPacket::IncreaseRefCount()
{
    packetBuffer_->IncreaseRefCount();
}

ContentsCPacket& ContentsCPacket::operator<<(unsigned char value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(char value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(short value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(unsigned short value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(int value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(long value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(unsigned int value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(float value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(__int64 value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(double value)
{
    *packetBuffer_ << value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(unsigned char& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(char& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(short& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(unsigned short& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(int& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(unsigned int& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(float& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(__int64& value)
{
    *packetBuffer_ >> value;

    return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(double& value)
{
    *packetBuffer_ >> value;

    return *this;
}

int ContentsCPacket::GetData(char* destination, int destinationSize)
{
    return packetBuffer_->GetData(destination, destinationSize);
}

int ContentsCPacket::PutData(char* source, int sourceSize)
{
    return packetBuffer_->PutData(source, sourceSize);
}

int ContentsCPacket::GetDataSize() const
{
    return packetBuffer_->GetDataSize();
}