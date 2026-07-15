#pragma once

#include <Windows.h>

#include "CoreDefines.h"
#include "TLSObjectFreeList.h"

class CPacket
{
private:
    CPacket();
    CPacket(int newBufferSize);

public:
    friend class IOCPServer;
    friend class ContentsCPacket;
    friend class TLSObjectFreeList<CPacket>;
    friend class ContentsNetLibrary;
    friend class NetLibrary;
    friend class NetLibraryLock;

    virtual ~CPacket();

    void Clear();
    void InitLan();

    int GetBufferSize() const;
    int GetDataSize() const;

    char* GetBufferPtr();
    char* GetWriteBufferPtr();
    char* GetReadPosition();

    int MoveWritePosition(int size);
    int MoveReadPosition(int size);

    int IncreaseRefCount();
    int DecreaseRefCount();

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

    BYTE Encode(char* text, WORD size, BYTE randomKey);
    bool Decode(char* text, WORD size, BYTE randomKey);

    static void Free(CPacket* packet);
    static CPacket* Alloc();

    static int GetPoolSize();
    static int GetUseSize();
    static int GetCapacity();

protected:
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