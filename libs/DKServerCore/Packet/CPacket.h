#pragma once

#define BUFFER_DEFAULT 500
#define LIBHEADERSIZE 5

#include "TLSObjectFreeList.h"




class CPacket
{
	CPacket();
	CPacket(int NewBufferSize);
	


public:
	friend class LanNetworkLibraryServer;
	friend class ContentsCPacket;
	friend class TLSObjectFreeList<CPacket>;
	friend class ContentsNetLibrary;
	friend class NetLibrary;


	/*---------------------------------------------------------------
	Packet Enum.

	----------------------------------------------------------------*/

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	

	virtual	~CPacket();


	//////////////////////////////////////////////////////////////////////////
	// 패킷 청소.
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);
	void InitLan(void);

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)패킷 버퍼 사이즈 얻기.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void) { return BufferSize; }
	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 데이타 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void) { return DataSize; }



	//////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터 얻기.
	//
	// Parameters: 없음.
	// Return: (char *)버퍼 포인터.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void) { return SerializeBuffer; }
	char* GetWriteBufferPtr(void) { return SerializeBuffer + WritePosition; }
	char* GetReadPosition(void) { return SerializeBuffer + LIBHEADERSIZE; }


	//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수 이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePosition(int Size);
	int		MoveReadPosition(int Size);

	int IncreaseRefCount();
	int DecreaseRefCount();







	/* ============================================================================= */
	// 연산자 오버로딩
	/* ============================================================================= */
	CPacket& operator = (CPacket& SourcePacket);

	//////////////////////////////////////////////////////////////////////////
	// 넣기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator << (unsigned char Value);
	CPacket& operator << (char Value);

	CPacket& operator << (short Value);
	CPacket& operator << (unsigned short Value);

	CPacket& operator << (int Value);
	CPacket& operator << (long Value);
	CPacket& operator << (unsigned int Value);
	CPacket& operator << (float Value);

	CPacket& operator << (__int64 Value);
	CPacket& operator << (double Value);


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator >> (unsigned char& Value);
	CPacket& operator >> (char& Value);

	CPacket& operator >> (short& Value);
	CPacket& operator >> (unsigned short& Value);

	CPacket& operator >> (int& Value);
	CPacket& operator >> (unsigned int& Value);
	CPacket& operator >> (float& Value);

	CPacket& operator >> (__int64& Value);
	CPacket& operator >> (double& Value);




	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* Destination, int DestinationSize);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* Source, int SourceSize);

	//void PrintOffset();
	
	BYTE Encode(char* Text, WORD Size, BYTE rk);

	bool Decode(char* Text, WORD Size, BYTE rk);

	static void Free(CPacket* packet_buffer);

	static CPacket* Alloc();

	static int GetPoolSize();

	static int GetUseSize();

	static int GetCapacity();

protected:

	char* SerializeBuffer;

	int	BufferSize;

	long ref_count;

	int	DataSize;
	//얘가 리어

	int WritePosition;
	//얘가 프론트

	int ReadPosition;

	int EncodingFlag;

	CRITICAL_SECTION EncodingLock;

	static TLSObjectFreeList<CPacket> serialize_list;
};



