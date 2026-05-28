#include "CPacket.h"
#include "memory.h"

CPacket::CPacket()
{

	BufferSize = BUFFER_DEFAULT;
	DataSize = 0;
	SerializeBuffer = new char[BUFFER_DEFAULT];

	WritePosition = 0;
	ReadPosition = 0;
}

CPacket::CPacket(int NewBufferSize)
{
	DataSize = 0;
	BufferSize = NewBufferSize;
	SerializeBuffer = new char[NewBufferSize];

	WritePosition = 0;
	ReadPosition = 0;

}

CPacket::~CPacket()
{
	delete[] SerializeBuffer;
}

void CPacket::Clear(void)
{
	DataSize = 0;
	WritePosition = 0;
	ReadPosition = 0;
}

int CPacket::MoveWritePosition(int Size)
{


	if (DataSize + Size > BufferSize)
	{
		//고민. 최대한 넣어줄것인가.
		//끌까? 아니요. 최대한 넣어주고 로그를 남깁시다.
		//로그는 근데 자료구조에서 남기는게 아님. -1을 리턴 후 -1이라면 로그찍기를 컨텐츠단에서 진행.
		return -1;
	}

	WritePosition += Size;
	DataSize += Size;

	return Size;

}

int CPacket::MoveReadPosition(int Size)
{
	if (DataSize - Size < 0)
	{
		//고민. 최대한 빼 줄 것인가?
		//끌까? 아니요. 최대한 넣어주고 로그를 남깁시다.
		//로그는 근데 자료구조에서 남기는게 아님. -1을 리턴 후 -1이라면 로그찍기를 컨텐츠단에서 진행.
		return -1;
	}

	ReadPosition -= Size;
	DataSize -= Size;

	return Size;
}

CPacket& CPacket::operator=(CPacket& SourcePacket)
{
	if (BufferSize < SourcePacket.DataSize)
	{
		//이건 일단 사이즈를 늘려줘야겠지?
		BufferSize = SourcePacket.DataSize;
		delete[] SerializeBuffer;

		SerializeBuffer = new char[BufferSize];
	}



	if (memcpy_s(SerializeBuffer, SourcePacket.DataSize, SourcePacket.SerializeBuffer, SourcePacket.DataSize) != 0)
	{

	}
	BufferSize = SourcePacket.BufferSize;
	DataSize = SourcePacket.DataSize;
	WritePosition = SourcePacket.WritePosition;
	ReadPosition = SourcePacket.ReadPosition;


	return *this;

	// TODO: 여기에 return 문을 삽입합니다.
}
//이걸 성능 좋게 어떻게 하지.
CPacket& CPacket::operator<<(unsigned char Value)
{
	if (DataSize + sizeof(unsigned char) >= BufferSize)
	{
		return *this;
	}

	*(SerializeBuffer + WritePosition) = Value;

	WritePosition += sizeof(unsigned char);
	DataSize += sizeof(unsigned char);

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(char Value)
{
	if (DataSize + sizeof(char) >= BufferSize)
	{
		return *this;
	}

	*(SerializeBuffer + WritePosition) = Value;

	WritePosition += sizeof(char);
	DataSize += sizeof(char);

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(short Value)
{
	if (DataSize + sizeof(short) >= BufferSize)
	{
		return *this;
	}

	(*(short*)(SerializeBuffer + WritePosition)) = Value;

	WritePosition += sizeof(short);
	DataSize += sizeof(short);

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(unsigned short Value)
{
	if (DataSize + sizeof(unsigned short) >= BufferSize)
	{
		return *this;
	}
	(*(unsigned short*)(SerializeBuffer + WritePosition)) = Value;


	WritePosition += sizeof(unsigned short);
	DataSize += sizeof(unsigned short);

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

CPacket& CPacket::operator<<(int Value)
{
	if (DataSize + sizeof(int) >= BufferSize)
	{
		return *this;
	}
	(*(int*)(SerializeBuffer + WritePosition)) = Value;


	WritePosition += sizeof(int);
	DataSize += sizeof(int);

	return *this;
}

CPacket& CPacket::operator<<(long Value)
{
	if (DataSize + sizeof(long) >= BufferSize)
	{
		return *this;
	}
	(*(long*)(SerializeBuffer + WritePosition)) = Value;


	WritePosition += sizeof(long);
	DataSize += sizeof(long);

	return *this;
}

CPacket& CPacket::operator<<(unsigned int Value)
{
	if (DataSize + sizeof(unsigned int) >= BufferSize)
	{
		return *this;
	}
	(*(unsigned int*)(SerializeBuffer + WritePosition)) = Value;


	WritePosition += sizeof(unsigned int);
	DataSize += sizeof(unsigned int);

	return *this;
}

CPacket& CPacket::operator<<(float Value)
{
	if (DataSize + sizeof(float) >= BufferSize)
	{
		return *this;
	}
	(*(float*)(SerializeBuffer + WritePosition)) = Value;


	WritePosition += sizeof(float);
	DataSize += sizeof(float);

	return *this;
}

CPacket& CPacket::operator<<(__int64 Value)
{
	if (DataSize + sizeof(__int64) >= BufferSize)
	{
		return *this;
	}
	(*(__int64*)(SerializeBuffer + WritePosition)) = Value;


	WritePosition += sizeof(__int64);
	DataSize += sizeof(__int64);

	return *this;
}

CPacket& CPacket::operator<<(double Value)
{
	if (DataSize + sizeof(double) >= BufferSize)
	{
		return *this;
	}
	(*(double*)(SerializeBuffer + WritePosition)) = Value;


	WritePosition += sizeof(double);
	DataSize += sizeof(double);

	return *this;
}


//-----------------------------------------------------------------
//(*(unsigned short*)(SerializeBuffer + WritePosition)) = Value;
CPacket& CPacket::operator>>(unsigned char& Value)
{
	if (DataSize < sizeof(unsigned char))
	{
		return *this;
	}

	Value = (*(unsigned char*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(unsigned char);
	DataSize -= sizeof(unsigned char);


	return *this;
}

CPacket& CPacket::operator>>(char& Value)
{
	if (DataSize < sizeof(char))
	{
		return *this;
	}

	Value = (*(char*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(char);
	DataSize -= sizeof(char);


	return *this;
}

CPacket& CPacket::operator>>(short& Value)
{
	if (DataSize < sizeof(short))
	{
		return *this;
	}

	Value = (*(short*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(short);
	DataSize -= sizeof(short);


	return *this;
}

CPacket& CPacket::operator>>(unsigned short& Value)
{
	if (DataSize < sizeof(unsigned short))
	{
		return *this;
	}

	Value = (*(unsigned short*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(unsigned short);
	DataSize -= sizeof(unsigned short);


	return *this;
}

CPacket& CPacket::operator>>(int& Value)
{
	if (DataSize < sizeof(int))
	{
		return *this;
	}

	Value = (*(int*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(int);
	DataSize -= sizeof(int);


	return *this;
}

CPacket& CPacket::operator>>(unsigned int& Value)
{
	if (DataSize < sizeof(unsigned int))
	{
		return *this;
	}

	Value = (*(unsigned int*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(unsigned int);
	DataSize -= sizeof(unsigned int);


	return *this;
}

CPacket& CPacket::operator>>(float& Value)
{
	if (DataSize < sizeof(float))
	{
		return *this;
	}

	Value = (*(float*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(float);
	DataSize -= sizeof(float);


	return *this;
}

CPacket& CPacket::operator>>(__int64& Value)
{
	if (DataSize < sizeof(__int64))
	{
		return *this;
	}

	Value = (*(__int64*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(__int64);
	DataSize -= sizeof(__int64);


	return *this;
}

CPacket& CPacket::operator>>(double& Value)
{
	if (DataSize < sizeof(double))
	{
		return *this;
	}

	Value = (*(double*)(SerializeBuffer + ReadPosition));
	ReadPosition += sizeof(double);
	DataSize -= sizeof(double);


	return *this;
}

int CPacket::GetData(char* Destination, int DestinationSize)
{
	if (DataSize < DestinationSize)
	{
		return -1;
	}


	if (memcpy_s(Destination, DestinationSize, SerializeBuffer + ReadPosition, DestinationSize) != 0)
	{

		//에러.
		return -1;

	}

	ReadPosition += DestinationSize;
	DataSize -= DestinationSize;


	return DestinationSize;

}

//풋데이터는 타입이 없으니까?

int CPacket::PutData(char* Source, int SourceSize)
{
	if (DataSize + SourceSize > BufferSize)
	{
		//고민. 최대한 넣어줄것인가.
		//끌까? 아니요. 최대한 넣어주고 로그를 남깁시다.
		//로그는 근데 자료구조에서 남기는게 아님. -1을 리턴 후 -1이라면 로그찍기를 컨텐츠단에서 진행.
		return -1;
	}


	if (memcpy_s(SerializeBuffer + WritePosition, SourceSize, Source, SourceSize) != 0)
	{

		//에러.
		return -1;

	}
	WritePosition += SourceSize;
	DataSize += SourceSize;

	return 0;
}
//
//void CPacket::PrintOffset()
//{
//	__int64 OffsetBuffer = (__int64)(&(((CPacket*)0)->SerializeBuffer));
//	__int64 OffsetBufferSize = (__int64)(&(((CPacket*)0)->BufferSize));
//	__int64 OffsetDataSize = (__int64)(&(((CPacket*)0)->DataSize));
//	__int64 OffsetWritePosition = (__int64)(&(((CPacket*)0)->WritePosition));
//	__int64 OffsetReadPosition = (__int64)(&(((CPacket*)0)->ReadPosition));
//
//	wprintf(L"OffsetBuffer : %lld \n", OffsetBuffer);
//	wprintf(L"OffsetBufferSize : %lld \n", OffsetBufferSize);
//	wprintf(L"OffsetDataSize : %lld \n", OffsetDataSize);
//	wprintf(L"OffsetWritePosition : %lld \n", OffsetWritePosition);
//	wprintf(L"OffsetReadPosition : %lld \n", OffsetReadPosition);
//
//}