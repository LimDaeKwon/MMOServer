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
		unsigned int underflowChecksum_;
		DATA data_;
		unsigned int overflowChecksum_;
		BLOCK_NODE* next_;
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	ObjectFreeList(int BlockNum, bool PlacementNew = false) : isPlacementNew_(PlacementNew), checksum_(GlobalChecksum++), freeNode_(NULL), useCount_(0) , capacity_(0)
	{

		// Data에서 BLOCK_NODE의 시작까지 오프셋 계산
		BLOCK_NODE* findOffset = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
		checksumPosition_ = (__int64)&findOffset->data_ - (__int64)&findOffset->underflowChecksum_;
		free(findOffset);


		for (int i = 0; i < BlockNum; ++i)
		{
			BLOCK_NODE* tempNode = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));

			if (isPlacementNew_)
			{
				new (&tempNode->data_) DATA;
			}
			tempNode->overflowChecksum_ = checksum_;
			tempNode->underflowChecksum_ = checksum_;
			Push(tempNode);
			++capacity_;
		}


	}

	virtual	~ObjectFreeList()
	{
		for (int i = 0; i < capacity_; ++i)
		{
			delete Pop();
		}
	}



	void Push(BLOCK_NODE* Data)
	{
		BLOCK_NODE* tempNode = freeNode_;
		freeNode_ = Data;
		freeNode_->next_ = tempNode;

	}


	BLOCK_NODE* Pop()
	{


		BLOCK_NODE* tempNode = freeNode_;

		freeNode_ = freeNode_->next_;

		return tempNode;

	}

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{

		BLOCK_NODE* newNode;

		if (useCount_ == capacity_)
		{

			newNode = new BLOCK_NODE;

			newNode->overflowChecksum_ = checksum_;
			newNode->underflowChecksum_ = checksum_;


			++useCount_;
			++capacity_;

			return &newNode->data_;

		}


		newNode = Pop();

		if (isPlacementNew_)
		{
			new (&newNode->data_) DATA;
		}

		++useCount_;

		return &newNode->data_;


	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* Data)
	{

		char* movePointer = (char*)Data;

		movePointer -= checksumPosition_;

		BLOCK_NODE* tempNode = (BLOCK_NODE*)movePointer;

		if (tempNode->overflowChecksum_ != checksum_)
		{
			//wprintf(L"Overflow \n");
			DebugBreak();
		}
		if (tempNode->underflowChecksum_ != checksum_)
		{
			//wprintf(L"Underflow \n");
			DebugBreak();
		}

		if (isPlacementNew_)
		{
			tempNode->data_.~DATA();
		}

		Push(tempNode);

		--useCount_;


		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return capacity_; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return useCount_; }


	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.

	

private:

	BLOCK_NODE* freeNode_;
	unsigned int checksum_;
	__int64 checksumPosition_;

	bool isPlacementNew_;
	int capacity_;
	int useCount_;
};


#else


//template <class DATA>
//class ObjectFreeList
//{
//
//public:
//
//	struct BLOCK_NODE
//	{
//		DATA data_;
//		BLOCK_NODE* next_;
//	};
//
//	//////////////////////////////////////////////////////////////////////////
//	// 생성자, 파괴자.
//	//
//	// Parameters:	(int) 초기 블럭 개수.
//	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
//	// Return:
//	//////////////////////////////////////////////////////////////////////////
//	ObjectFreeList(int blockNum, bool placementNew = false) : IsPlacementNew(placementNew), FreeNode(NULL), UseCount(0), Capacity(0)
//	{
//
//		for (int i = 0; i < blockNum; ++i)
//		{
//			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
//
//			if (IsPlacementNew)
//			{
//				new (&Temp->Data) DATA;
//			}
//			Push(Temp);
//			++Capacity;
//		}
//
//
//	}
//
//	virtual	~ObjectFreeList()
//	{
//		for (int i = 0; i < Capacity; ++i)
//		{
//			delete Pop();
//		}
//	}
//
//
//
//	void Push(BLOCK_NODE* Data)
//	{
//		BLOCK_NODE* Temp = FreeNode;
//		FreeNode = Data;
//		FreeNode->Next = Temp;
//
//	}
//
//
//	BLOCK_NODE* Pop()
//	{
//
//
//		BLOCK_NODE* Temp = FreeNode;
//
//		FreeNode = FreeNode->Next;
//
//		return Temp;
//
//	}
//
//	//////////////////////////////////////////////////////////////////////////
//	// 블럭 하나를 할당받는다.  
//	//
//	// Parameters: 없음.
//	// Return: (DATA *) 데이타 블럭 포인터.
//	//////////////////////////////////////////////////////////////////////////
//	DATA* Alloc(void)
//	{
//
//		char* MovePointer;
//		BLOCK_NODE* Temp;
//
//		if (UseCount == Capacity)
//		{
//
//			Temp = new BLOCK_NODE;
//
//			++UseCount;
//			++Capacity;
//
//			return &Temp->Data;
//
//		}
//
//
//		Temp = Pop();
//
//		if (IsPlacementNew)
//		{
//			new (&Temp->Data) DATA;
//		}
//
//		++UseCount;
//
//		return &Temp->Data;
//
//
//	}
//
//	//////////////////////////////////////////////////////////////////////////
//	// 사용중이던 블럭을 해제한다.
//	//
//	// Parameters: (DATA *) 블럭 포인터.
//	// Return: (BOOL) TRUE, FALSE.
//	//////////////////////////////////////////////////////////////////////////
//	bool	Free(DATA* Data)
//	{
//
//		BLOCK_NODE* Temp = (BLOCK_NODE*)Data;
//
//
//		if (IsPlacementNew)
//		{
//			Temp->Data.~DATA();
//		}
//
//		Push(Temp);
//
//		--UseCount;
//
//
//		return true;
//	}
//
//
//	//////////////////////////////////////////////////////////////////////////
//	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
//	//
//	// Parameters: 없음.
//	// Return: (int) 메모리 풀 내부 전체 개수
//	//////////////////////////////////////////////////////////////////////////
//	int		GetCapacityCount(void) { return Capacity; }
//
//	//////////////////////////////////////////////////////////////////////////
//	// 현재 사용중인 블럭 개수를 얻는다.
//	//
//	// Parameters: 없음.
//	// Return: (int) 사용중인 블럭 개수.
//	//////////////////////////////////////////////////////////////////////////
//	int		GetUseCount(void) { return UseCount; }
//
//
//	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
//
//
//
//private:
//
//	BLOCK_NODE* FreeNode;
//	bool IsPlacementNew;
//	int Capacity;
//	int UseCount;
//};


#endif

