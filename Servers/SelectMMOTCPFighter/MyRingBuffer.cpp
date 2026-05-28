#include "MyRingBuffer.h"
#include "stdio.h"
#include "memory.h"
#include "Windows.h"

MyRingBuffer::MyRingBuffer(void)
{
	Front = 0;
	Rear = 0;
	RingBuffer = new char[DEFAULTBUFFERSIZE];
	Size = DEFAULTBUFFERSIZE;

}

MyRingBuffer::MyRingBuffer(int BufferSize)
{
	Front = 0;
	Rear = 0;
	RingBuffer = new char[BufferSize];
	Size = BufferSize;
}

MyRingBuffer::~MyRingBuffer()
{
	delete RingBuffer;
}



void MyRingBuffer::Resize(int NewSize)
{
	//더 작은 크기로 옮기고 싶은거면?
	//지금 사용중인 버퍼의 크기가 NewSize보다 크다면 불가능. 아니면 가능.
	if (GetUseSize() > NewSize)
	{
		//지금 사용중인 버퍼의 크기가 NewSize보다 크다면 불가능. 

		return;
	}

	//더 큰 크기로 옮기고 싶은거면?
	//그냥 냅다 옮겨줘. // 바껴야합니다.
	//들어있는 크기만큼 싹 디큐를 해줘서 하면 되겠네.



	char* Temp = new char[NewSize];
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
	Size = NewSize;


}

int MyRingBuffer::GetBufferSize(void)
{
	return Size;
}
//리어가 3이고 프론트가 0이면 012 즉 3개있는것.,
int MyRingBuffer::GetUseSize(void)
{


	if (Front > Rear)
	{
		return Size - Front + Rear;
	}


	return Rear - Front; //프론트가 더 크면? 이거도 가능. 프론트가 99고. 리어가 2라면.
	//같으면? 0 맞아. 리어가 3 , 프론트가 1이면 사용중인 사이즈는? 
	//2. 

}
int MyRingBuffer::GetFreeSize(void)
{
	//쓰고 있는 사이즈가 8 . 버퍼는 10. 그럼 1 

	return Size - GetUseSize() - 1;
}

int MyRingBuffer::Enqueue(const char* Data, int EnqueueSize)
{

	if (GetFreeSize() < EnqueueSize)
	{
		//넣을 수 없어.. 이 때 어떻게 해줄거냐는거지. 리사이즈 만들었으니까 리사이즈 해서 넣어줄까?
		//일단 리턴 0 해주고 최대한 받고 싶을 때 생각해보자.
		//미완성된 메시지 조차 남기고 싶다면?
		return 0;
	}

	// 0일때랑 아닐떄.
	//총 사이즈 30.
	//29 29 
	//잠깐. 이러면 뚫어버리는데.
	//똥그랗게 어떻게 하지? 일단 끝까지.
	//조건은? 사이즈 - 리어 가 Enqueue 사이즈보다 작을 때. 잠깐만. 리어에는 데이터가 없네.
	//사이즈는 100 리어는 70 인큐사이즈는 40일때. 총 데이터는 지금 30개는 이 30개는? 사이즈 - 리어.
	//리어에 바로 넣기 가능.

	if (Size - Rear < EnqueueSize)
	{
		int FirstEnqueueSize = Size - Rear;
		if (memcpy_s(RingBuffer + Rear, FirstEnqueueSize, Data, FirstEnqueueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		int SecondEnqueueSize = EnqueueSize - FirstEnqueueSize;
		//그리고 남은 만큼.
		if (memcpy_s(RingBuffer, SecondEnqueueSize, Data + FirstEnqueueSize, SecondEnqueueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		Rear = SecondEnqueueSize;

		return EnqueueSize;
	}

	//리어 프리사이즈에 넣어주는 작업?



	//이건 그냥.
	if (memcpy_s(RingBuffer + Rear, EnqueueSize, Data, EnqueueSize) != 0)
	{
		int Error = GetLastError();
		wprintf(L"memcpy_s Error %d \n", Error);
		DebugBreak();

	}
	//리어가 0. 데이터를 넣는다면. 10바. 0~9 리어는 10을 가리켜야함. 
	//맞잖아.

	if (Rear + EnqueueSize == GetBufferSize())
	{
		Rear = 0;
	}
	else
	{
		Rear += EnqueueSize;
	}

	return EnqueueSize;
}

int MyRingBuffer::Dequeue(char* Dest, int DequeueSize)
{

	//일단 원하는 사이즈만큼 있는지 확인해야지.
	if (GetUseSize() < DequeueSize)
	{
		//뺄 수 없어.. 이 때 어떻게 해줄거냐는거지. 리사이즈 만들었으니까 리사이즈 해서 넣어줄까?
		//일단 리턴 0 해주고 최대한 빼고 싶을 때 생각해보자.
		return 0;

	}

	//만약 프론트 0, 리어 10 

	if (Size - Front < DequeueSize)
	{
		int FirstDequeueSize = Size - Front;
		if (memcpy_s(Dest, FirstDequeueSize, RingBuffer + Front, FirstDequeueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		int SecondDequeueSize = DequeueSize - FirstDequeueSize;

		if (memcpy_s(Dest + FirstDequeueSize, SecondDequeueSize, RingBuffer, SecondDequeueSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		Front = SecondDequeueSize;

		return DequeueSize;
	}
	//이건 그냥.
	if (memcpy_s(Dest, DequeueSize, RingBuffer + Front, DequeueSize) != 0)
	{
		int Error = GetLastError();
		wprintf(L"memcpy_s Error %d \n", Error);
		DebugBreak();

	}
	//프론트가 0이었음. 10을 뺴. 다음 프론트는 10이어야지.


	if (Front + DequeueSize == GetBufferSize())
	{
		Front = 0;
	}
	else
	{
		Front += DequeueSize;
	}

	return DequeueSize;
}

int MyRingBuffer::Peek(char* Dest, int PeekSize)
{
	//그저 보기만.
	//일단 원하는 사이즈만큼 있는지 확인해야지.
	if (GetUseSize() < PeekSize)
	{
		printf("공간이 충분히 없어..\n");
		return 0;

	}

	//똥그랗게 어떻게 하지?
	//조건은? 그저 프론트가 증가하는거. 한방에 못빼면의 조건은? 사이즈 - 프론트가 디큐사이즈보다 작을 때.
	//만약 프론트가 70이고 디큐사이즈는 40이야. 그럼 30까지는 빼기 가능. 그 후 0부터 남은사이즈만큼 해주고 프론트 세팅해주기.
	// 30에도 넣기 가능임. 프론트가 0이야. 디큐를 해 . 
	//근데 70에는 데이터가 있는데? 0부터니까. 99까지 가능. 71~99 까지. 29개. 
	// 0이고 35를 뺐어. 0~34까지는 데이터가 있던거. 35로 가야겠네. 
	if (Size - Front < PeekSize)
	{
		int FirstPeekSize = Size - Front;
		if (memcpy_s(Dest, FirstPeekSize, RingBuffer + Front, FirstPeekSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		int SecondPeekSize = PeekSize - FirstPeekSize;

		if (memcpy_s(Dest + FirstPeekSize, SecondPeekSize, RingBuffer, SecondPeekSize) != 0)
		{
			int Error = GetLastError();
			wprintf(L"memcpy_s Error %d \n", Error);
			DebugBreak();
		}
		return PeekSize;
	}
	//이건 그냥.
	if (memcpy_s(Dest, PeekSize, RingBuffer + Front, PeekSize) != 0)
	{
		int Error = GetLastError();
		wprintf(L"memcpy_s Error %d \n", Error);
		DebugBreak();

	}




	return PeekSize;
}

void MyRingBuffer::ClearBuffer(void)
{
	Front = 0;
	Rear = 0;

}


int MyRingBuffer::DirectEnqueueSize(void)
{



	if (Rear >= Front)
	{
		if (Front == 0)
		{
			return Size - Rear - 1;
		}
		else
		{
			return Size - Rear;
		}

	}



	return GetFreeSize();


}

int MyRingBuffer::DirectDequeueSize(void)
{
	//리어0 프론트 9 -> 1개. size - 프론트 하면 1.
	//
	// 
	// 리어 8 프론트 3 이라면 그냥 34567 5개. 리어 - 프론트. wmr d
	//
	//프론트0 리어 9 9개. 사이즈 - 리어 
	if (Rear < Front)
	{

		return Size - Front;
	}

	return GetUseSize();

}

int MyRingBuffer::MoveRear(int MoveSize)
{
	if (MoveSize <= 0)
	{
		return 0;
	}


	if (Size - Rear < MoveSize)
	{
		int FirstEnqueueSize = Size - Rear;
		int SecondEnqueueSize = MoveSize - FirstEnqueueSize;
		//그리고 남은 만큼.
		Rear = SecondEnqueueSize;
		return MoveSize;
	}

	if (Rear + MoveSize == Size)
	{
		Rear = 0;
	}
	else
	{
		Rear += MoveSize;
	}

	return MoveSize;

}

int MyRingBuffer::MoveFront(int MoveSize)
{
	if (MoveSize <= 0)
	{
		return 0;
	}


	if (Size - Front < MoveSize)
	{
		int FirstDequeueSize = Size - Front;

		int SecondDequeueSize = MoveSize - FirstDequeueSize;

		Front = SecondDequeueSize;

		return MoveSize;
	}

	//프론트가 0이었음. 10을 뺴. 다음 프론트는 10이어야지.


	if (Front + MoveSize == Size)
	{
		Front = 0;
	}
	else
	{
		Front += MoveSize;
	}

	return MoveSize;
}

char* MyRingBuffer::GetFrontBufferPtr(void)
{
	return RingBuffer + Front;
}

char* MyRingBuffer::GetRearBufferPtr(void)
{
	return RingBuffer + Rear;
}
