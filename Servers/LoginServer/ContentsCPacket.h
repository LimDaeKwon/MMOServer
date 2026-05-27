#pragma once
#include"CPacket.h"

class ContentsCPacket
{
private:
	ContentsCPacket();
	
	

public:

	ContentsCPacket(ContentsCPacket& copy);
	ContentsCPacket(ContentsCPacket* copy);
	ContentsCPacket(CPacket* alloc_buffer);

	
	//ContentsCPacket& operator=(CPacket* alloc_buffer);

	static CPacket* MakeContentsPacket();
	static CPacket* MakeContentsPacket(CPacket* other);
	static CPacket* MakeContentsPacket(ContentsCPacket* other);

	//ContentsCPacket& operator=(ContentsCPacket* alloc_buffer);

	~ContentsCPacket();
	 
	void IncreaseRefCount()
	{
		packet_buffer->IncreaseRefCount();
	}

	CPacket* packet_buffer;


	//////////////////////////////////////////////////////////////////////////
	// 넣기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	ContentsCPacket& operator << (unsigned char Value);
	ContentsCPacket& operator << (char Value);

	ContentsCPacket& operator << (short Value);
	ContentsCPacket& operator << (unsigned short Value);

	ContentsCPacket& operator << (int Value);
	ContentsCPacket& operator << (long Value);
	ContentsCPacket& operator << (unsigned int Value);
	ContentsCPacket& operator << (float Value);

	ContentsCPacket& operator << (__int64 Value);
	ContentsCPacket& operator << (double Value);


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	ContentsCPacket& operator >> (unsigned char& Value);
	ContentsCPacket& operator >> (char& Value);

	ContentsCPacket& operator >> (short& Value);
	ContentsCPacket& operator >> (unsigned short& Value);

	ContentsCPacket& operator >> (int& Value);
	ContentsCPacket& operator >> (unsigned int& Value);
	ContentsCPacket& operator >> (float& Value);

	ContentsCPacket& operator >> (__int64& Value);
	ContentsCPacket& operator >> (double& Value);



	int		GetData(char* Destination, int DestinationSize);

	int		PutData(char* Source, int SourceSize);
	int		GetDataSize();

};

