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


//지금은 사용하지 않지만 메모리를 최대한으로 아끼는 구조에서는 사용할 법 하겠다.
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
	int LocalFront;
	int LocalRear;

	LocalFront = Front;
	LocalRear = Rear;

	if (LocalFront > LocalRear)
	{

		return Size - LocalFront + LocalRear;
	}


	return LocalRear - LocalFront;
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

		return 0;
	}


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

	if (GetUseSize() < DequeueSize)
	{

		return 0;

	}

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

	int LocalFront;
	int LocalRear;

	LocalFront = Front;
	LocalRear = Rear;



	if (LocalRear >= LocalFront)
	{
		if (LocalFront == 0)
		{
			return Size - LocalRear - 1;
		}
		else
		{
			return Size - LocalRear;
		}

	}


	return GetFreeSize();
}

int MyRingBuffer::DirectDequeueSize(void)
{

	int LocalFront;
	int LocalRear;
	int UseSize = 0;

	LocalFront = Front;
	LocalRear = Rear;


	if (LocalFront > LocalRear)
	{
		UseSize = Size - LocalFront + LocalRear;
	}
	else
	{
		UseSize = LocalRear - LocalFront;


		if (LocalRear < LocalFront)
		{
			if (UseSize <= Size - LocalFront)
			{
				return UseSize;
			}
			else
			{


				return Size - LocalFront;
			}
		}
	}


	return UseSize;

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

char* MyRingBuffer::GetStartBufferPtr(void)
{
	return RingBuffer;
}