#pragma once
#include "Windows.h"
#include <new.h>






#define DEBUGOBJECTFREELIST
#ifdef DEBUGOBJECTFREELIST
extern unsigned int GlobalChecksum;

template <class DATA>
class ObjectFreeList
{

public:

	struct BLOCK_NODE
	{
		unsigned int UnderflowCheckSum;
		DATA Data;
		unsigned int OverflowCheckSum;
		BLOCK_NODE* Next;
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	ObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), Checksum(GlobalChecksum++), FreeNode(NULL), UseCount(0) , Capacity(0)
	{

		// Data에서 BLOCK_NODE의 시작까지 오프셋 계산
		BLOCK_NODE* FirstAlloc = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
		ChecksumPosition = (__int64)&FirstAlloc->Data - (__int64)&FirstAlloc->UnderflowCheckSum;
		free(FirstAlloc);


		for (int i = 0; i < BlockNum; ++i)
		{
			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));

			if (IsPlacementNew)
			{
				new (&Temp->Data) DATA;
			}
			Temp->OverflowCheckSum = Checksum;
			Temp->UnderflowCheckSum = Checksum;
			Push(Temp);
			++Capacity;
		}


	}

	virtual	~ObjectFreeList()
	{
		for (int i = 0; i < Capacity; ++i)
		{
			delete Pop();
		}
	}



	void Push(BLOCK_NODE* Data)
	{
		BLOCK_NODE* Temp = FreeNode;
		FreeNode = Data;
		FreeNode->Next = Temp;

	}


	BLOCK_NODE* Pop()
	{


		BLOCK_NODE* Temp = FreeNode;

		FreeNode = FreeNode->Next;

		return Temp;

	}

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{

		BLOCK_NODE* Temp;

		if (UseCount == Capacity)
		{

			Temp = new BLOCK_NODE;

			Temp->OverflowCheckSum = Checksum;
			Temp->UnderflowCheckSum = Checksum;


			++UseCount;
			++Capacity;

			return &Temp->Data;

		}


		Temp = Pop();

		if (IsPlacementNew)
		{
			new (&Temp->Data) DATA;
		}

		++UseCount;

		return &Temp->Data;


	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* Data)
	{

		char* MovePointer = (char*)Data;

		MovePointer -= ChecksumPosition;

		BLOCK_NODE* Temp = (BLOCK_NODE*)MovePointer;

		if (Temp->OverflowCheckSum != Checksum)
		{
			//wprintf(L"Overflow \n");
			DebugBreak();
		}
		if (Temp->UnderflowCheckSum != Checksum)
		{
			//wprintf(L"Underflow \n");
			DebugBreak();
		}

		if (IsPlacementNew)
		{
			Temp->Data.~DATA();
		}

		Push(Temp);

		--UseCount;


		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return Capacity; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return UseCount; }


	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.

	

private:

	BLOCK_NODE* FreeNode;
	unsigned int Checksum;
	__int64 ChecksumPosition;

	bool IsPlacementNew;
	int Capacity;
	int UseCount;
};


#else


template <class DATA>
class ObjectFreeList
{

public:

	struct BLOCK_NODE
	{
		DATA Data;
		BLOCK_NODE* Next;
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	ObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), FreeNode(NULL), UseCount(0), Capacity(0)
	{

		for (int i = 0; i < BlockNum; ++i)
		{
			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));

			if (IsPlacementNew)
			{
				new (&Temp->Data) DATA;
			}
			Push(Temp);
			++Capacity;
		}


	}

	virtual	~ObjectFreeList()
	{
		for (int i = 0; i < Capacity; ++i)
		{
			delete Pop();
		}
	}



	void Push(BLOCK_NODE* Data)
	{
		BLOCK_NODE* Temp = FreeNode;
		FreeNode = Data;
		FreeNode->Next = Temp;

	}


	BLOCK_NODE* Pop()
	{


		BLOCK_NODE* Temp = FreeNode;

		FreeNode = FreeNode->Next;

		return Temp;

	}

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{

		char* MovePointer;
		BLOCK_NODE* Temp;

		if (UseCount == Capacity)
		{

			Temp = new BLOCK_NODE;

			++UseCount;
			++Capacity;

			return &Temp->Data;

		}


		Temp = Pop();

		if (IsPlacementNew)
		{
			new (&Temp->Data) DATA;
		}

		++UseCount;

		return &Temp->Data;


	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* Data)
	{

		BLOCK_NODE* Temp = (BLOCK_NODE*)Data;


		if (IsPlacementNew)
		{
			Temp->Data.~DATA();
		}

		Push(Temp);

		--UseCount;


		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return Capacity; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return UseCount; }


	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.



private:

	BLOCK_NODE* FreeNode;
	bool IsPlacementNew;
	int Capacity;
	int UseCount;
};


#endif

