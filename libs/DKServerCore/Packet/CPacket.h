#pragma once

#include <Windows.h>

#include "CoreDefines.h"
#include "TLSObjectFreeList.h"

class CPacket
{
private:
    friend class TLSObjectFreeList<CPacket>;
    friend class ContentsNetLibrary;
    friend class NetLibrary;
    friend class NetLibraryLock;

    CPacket();
    CPacket(int newBufferSize);

    bool CanWrite(int size) const;
    bool CanRead(int size) const;

    char* GetReadPosition();

    int MoveReadPosition(int size);
    int DecreaseRefCount();

    BYTE Encode(char* text, WORD size, BYTE randomKey);
    bool Decode(char* text, WORD size, BYTE randomKey);

public:
    ~CPacket();

    void Clear();
    void InitLan();

    int GetBufferSize() const;
    int GetDataSize() const;

    char* GetBufferPtr();
    char* GetWriteBufferPtr();

    int MoveWritePosition(int size);
    int IncreaseRefCount();

    CPacket& operator=(const CPacket& sourcePacket);

    CPacket& operator<<(unsigned char value);
    CPacket& operator<<(char value);

    CPacket& operator<<(short value);
    CPacket& operator<<(unsigned short value);

    CPacket& operator<<(int value);
    CPacket& operator<<(long value);
    CPacket& operator<<(unsigned int value);
    CPacket& operator<<(float value);

    CPacket& operator<<(__int64 value);
    CPacket& operator<<(double value);

    CPacket& operator>>(unsigned char& value);
    CPacket& operator>>(char& value);

    CPacket& operator>>(short& value);
    CPacket& operator>>(unsigned short& value);

    CPacket& operator>>(int& value);
    CPacket& operator>>(unsigned int& value);
    CPacket& operator>>(float& value);

    CPacket& operator>>(__int64& value);
    CPacket& operator>>(double& value);

    int GetData(char* destination, int destinationSize);
    int PutData(char* source, int sourceSize);

    static void Free(CPacket* packet);
    static CPacket* Alloc();

    static int GetPoolSize();
    static int GetUseSize();
    static int GetCapacity();

private:
    char* serializeBuffer_;

    int bufferSize_;
    long refCount_;

    int dataSize_;
    int writePosition_;
    int readPosition_;

    int encodingFlag_;
    CRITICAL_SECTION encodingLock_;

    static TLSObjectFreeList<CPacket> serializeList_;
};