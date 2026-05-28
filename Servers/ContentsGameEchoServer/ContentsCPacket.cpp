#include "ContentsCPacket.h"

ContentsCPacket::ContentsCPacket()
{


}





ContentsCPacket::ContentsCPacket(CPacket* alloc_buffer)
{
	packet_buffer = alloc_buffer;

	//packet_buffer->IncreaseRefCount();

	
}

ContentsCPacket::ContentsCPacket(ContentsCPacket& copy)
{
	packet_buffer = copy.packet_buffer;

	packet_buffer->IncreaseRefCount();

}

ContentsCPacket::ContentsCPacket(ContentsCPacket* copy)
{
	packet_buffer = (CPacket*)copy;

}



//ContentsCPacket& ContentsCPacket::operator=(CPacket* alloc_buffer)
//{
//	packet_buffer = alloc_buffer;
//
//	packet_buffer->IncreaseRefCount();
//
//	
//
//	return *this; // 객체 자신의 참조 반환
//}

//ContentsCPacket& ContentsCPacket::operator=(ContentsCPacket* alloc_buffer)
//{
//	packet_buffer = alloc_buffer->packet_buffer;
//
//	packet_buffer->IncreaseRefCount();
//
//	
//
//	return *this; // 객체 자신의 참조 반환
//}

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
	
	return other->packet_buffer;
}


ContentsCPacket::~ContentsCPacket()
{
	//if (packet_buffer->DecreaseRefCount() == 0)
	//{
	CPacket::Free(packet_buffer);
	//}
}



ContentsCPacket& ContentsCPacket::operator<<(unsigned char Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

ContentsCPacket& ContentsCPacket::operator<<(char Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

ContentsCPacket& ContentsCPacket::operator<<(short Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

ContentsCPacket& ContentsCPacket::operator<<(unsigned short Value)
{
	*packet_buffer << Value;

	return *this;
	// TODO: 여기에 return 문을 삽입합니다.
}

ContentsCPacket& ContentsCPacket::operator<<(int Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(long Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(unsigned int Value)
{
	*packet_buffer << Value;
	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(float Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(__int64 Value)
{
	*packet_buffer << Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator<<(double Value)
{
	*packet_buffer << Value;

	return *this;
}


//-----------------------------------------------------------------
//(*(unsigned short*)(SerializeBuffer + WritePosition)) = Value;
ContentsCPacket& ContentsCPacket::operator>>(unsigned char& Value)
{
	*packet_buffer >> Value;

	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(char& Value)
{
	*packet_buffer >> Value;


	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(short& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(unsigned short& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(int& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(unsigned int& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(float& Value)
{
	*packet_buffer >> Value;


	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(__int64& Value)
{
	*packet_buffer >> Value;



	return *this;
}

ContentsCPacket& ContentsCPacket::operator>>(double& Value)
{
	*packet_buffer >> Value;

	return *this;
}

int ContentsCPacket::GetData(char* Destination, int DestinationSize)
{
	return packet_buffer->GetData(Destination, DestinationSize);

}

//풋데이터는 타입이 없으니까?

int ContentsCPacket::PutData(char* Source, int SourceSize)
{

	return packet_buffer->PutData(Source, SourceSize);;
}

int ContentsCPacket::GetDataSize()
{
	return packet_buffer->GetDataSize();
}
