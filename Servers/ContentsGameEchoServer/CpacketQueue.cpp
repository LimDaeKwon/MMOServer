#include "CPacketQueue.h"
#include "stdio.h"
#include "memory.h"
#include "Windows.h"
#include "CPacket.h"

CPacketQueue::CPacketQueue(void) : front(0),rear(0),size(DEFAULTBUFFERSIZE)
{

	packet_queue = (CPacket**)malloc(sizeof(CPacket*) * DEFAULTBUFFERSIZE);

}

CPacketQueue::CPacketQueue(int BufferSize) : front(0), rear(0), size(BufferSize)
{

	packet_queue = (CPacket**)malloc(sizeof(CPacket*) * BufferSize);

}

CPacketQueue::~CPacketQueue()
{
	delete packet_queue;
}



void CPacketQueue::Resize(int NewSize)
{
	//더 작은 크기로 옮기고 싶은거면?
	//지금 사용중인 버퍼의 크기가 NewSize보다 크다면 불가능. 아니면 가능.
	//if (GetUseSize() > NewSize)
	//{
	//	//지금 사용중인 버퍼의 크기가 NewSize보다 크다면 불가능. 

	//	return;
	//}

	//더 큰 크기로 옮기고 싶은거면?
	//그냥 냅다 옮겨줘. // 바껴야합니다.
	//들어있는 크기만큼 싹 디큐를 해줘서 하면 되겠네.



	/*char* Temp = new char[NewSize];
	int UseSize = GetUseSize();
	if (Dequeue(Temp, UseSize) != UseSize)
	{

		int Error = GetLastError();
		wprintf(L"Dequeue Error %d \n", Error);
		DebugBreak();


	}
	delete[] RingBuffer;

	RingBuffer = Temp;
	Front = 0;
	Rear = UseSize;
	Size = NewSize;*/


}

int CPacketQueue::GetBufferSize(void)
{
	return size;
}

//리어가 3이고 프론트가 0이면 012 즉 3개있는것.,
int CPacketQueue::GetUseSize(void)
{
	int LocalFront;
	int LocalRear;

	LocalFront = front;
	LocalRear = rear;

	if (LocalFront > LocalRear)
	{


		return size - LocalFront + LocalRear;
	}


	return LocalRear - LocalFront;
}
int CPacketQueue::GetFreeSize(void)
{
	//쓰고 있는 사이즈가 8 . 버퍼는 10. 그럼 1 

	return size - GetUseSize() - 1;
}

int CPacketQueue::IsEmpty(void)
{
	return rear == front;
}

bool CPacketQueue::Enqueue(CPacket* Data)
{

	if (GetFreeSize() == 0)
	{

		return false;
	}

	packet_queue[rear] = Data;


	if (rear == size - 1)
	{
		rear = 0;
	}
	else
	{
		rear++;
	}

	return true;
}

bool CPacketQueue::Dequeue(CPacket** Out)
{

	if (IsEmpty())
	{

		return false;

	}
	*Out = packet_queue[front];
 	

	if (front == size -1)
	{
		front = 0;
	}
	else
	{
		front++;
	}

	return true;
}

void CPacketQueue::ClearBuffer(void)
{
	front = 0;
	rear = 0;
}


