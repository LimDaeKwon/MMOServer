#pragma once
#include "Windows.h"
#include <new.h>



extern unsigned int GlobalChecksum;
#define ADRMASK 0x00007fffffffffff
#define TAGMASK 0xffff800000000000
#define TAGOFFSET 47


#define DEBUG
#ifdef DEBUG



template <class DATA>
class LFObjectFreeList
{

public:

	struct BLOCK_NODE
	{
		unsigned int UnderflowCheckSum;
		DATA Data;
		unsigned int OverflowCheckSum;
		BLOCK_NODE* Next;
	};

	// IsPlacementNew == true : Alloc / Free 시점에 생성자 , 소멸자 호출
	LFObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), Checksum(GlobalChecksum++), TopNode(NULL), UseCount(0), TagIndex(0), Capacity(0)
	{

		// Data에서 BLOCK_NODE의 시작까지 오프셋 계산
		BLOCK_NODE* FirstAlloc = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
		ChecksumPosition = (__int64)&FirstAlloc->Data - (__int64)&FirstAlloc->UnderflowCheckSum;
		free(FirstAlloc);


		//초기 블록 할당
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

	virtual	~LFObjectFreeList()
	{
		__int64 TaggedTop = (__int64)TopNode;
		for (int i = 0; i < Capacity; ++i)
		{

			BLOCK_NODE* DeleteNode = UnMaskTag(TaggedTop);
			__int64     NextTagged = (__int64)DeleteNode->Next;

			if (!IsPlacementNew)
			{
				DeleteNode->Data.~DATA();
			}

			free(DeleteNode);
			TaggedTop = NextTagged;
		}
	}


	void Push(BLOCK_NODE* NewTop)
	{
		BLOCK_NODE* LocalTop;
		__int64 MaskNewTop;
		while (1)
		{
			LocalTop = TopNode;
			MaskNewTop = MaskNewTag((__int64)LocalTop, (__int64)NewTop);
			NewTop->Next = LocalTop;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)MaskNewTop, (__int64)LocalTop))
			{
				break;
			}
		}
	}

	BLOCK_NODE* AllocNewNode()
	{
		BLOCK_NODE* temp = new BLOCK_NODE;

		temp->OverflowCheckSum = Checksum;
		temp->UnderflowCheckSum = Checksum;

		return temp;
	}

	DATA* Alloc(void)
	{

		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;


		while (1)
		{
			LocalTop = TopNode;
			if (LocalTop == nullptr)
			{

				BLOCK_NODE* NewNode = AllocNewNode();

				InterlockedIncrement(&UseCount);
				InterlockedIncrement(&Capacity);

				return &NewNode->Data;

			}

			UnMaskTop = UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->Next;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}

		if (IsPlacementNew)
		{
			new (&UnMaskTop->Data) DATA;
		}

		InterlockedIncrement(&UseCount);

		return &UnMaskTop->Data;

	}


	bool	Free(DATA* Data)
	{

		BLOCK_NODE* ReturnNode = GetNodePosition(Data);
		CheckUnderOver(ReturnNode);


		if (IsPlacementNew)
		{
			ReturnNode->Data.~DATA();
		}

		Push(ReturnNode);

		InterlockedDecrement(&UseCount);
		return true;
	}

	BLOCK_NODE* GetNodePosition(DATA* Data)
	{
		char* MovePointer = (char*)Data;

		MovePointer -= ChecksumPosition;
		return (BLOCK_NODE*)MovePointer;
	}

	void CheckUnderOver(BLOCK_NODE* ReturnNode)
	{
		if (ReturnNode->OverflowCheckSum != Checksum)
		{
			//wprintf(L"Overflow \n");
			DebugBreak();
		}
		if (ReturnNode->UnderflowCheckSum != Checksum)
		{
			//wprintf(L"Underflow \n");
			DebugBreak();
		}
	}


	int		GetCapacityCount(void) { return Capacity; }

	int		GetUseCount(void) { return UseCount; }

	unsigned int Checksum;

private:
	__int64 MaskNewTag(__int64 LocalTop, __int64 MaskNewNode)
	{
		__int64 Tag = InterlockedIncrement(&TagIndex);
		MaskNewNode |= (Tag << TAGOFFSET);

		return MaskNewNode;
	}


	BLOCK_NODE* UnMaskTag(__int64 TaggedNode)
	{
		TaggedNode &= ADRMASK;
		return (BLOCK_NODE*)TaggedNode;
	}



private:
	BLOCK_NODE* TopNode;
	__int64 ChecksumPosition;
	bool IsPlacementNew;
	long Capacity;
	long UseCount;

	unsigned long long TagIndex;

};



#else

template <class DATA>
class TObjectFreeList
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
	TObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), TopNode(NULL), UseCount(0)
	{

		if (BlockNum == 0)
		{
			Capacity = BlockNum;
			return;
		}

		for (int i = 0; i < BlockNum; ++i)
		{
			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));;

			if (!IsPlacementNew)
			{
				new (&Temp->Data) DATA;
			}
			Push(Temp);
		}

		Capacity = BlockNum;
	}

	virtual	~TObjectFreeList()
	{
		BLOCK_NODE* DeleteNode;

		for (int i = 0; i < Capacity; ++i)
		{
			DeleteNode = TopNode;
			TopNode = TopNode->Next;
			delete DeleteNode;
		}
	}

	__int64 MaskNewTag(__int64 LocalTop, __int64 MaskNewNode)
	{
		__int64 Tag = InterlockedIncrement(&TagIndex);
		MaskNewNode |= (Tag << 47);
		return MaskNewNode;
	}


	BLOCK_NODE* UnMaskTag(__int64 HeadNode)
	{
		HeadNode &= ADRMASK;
		return (BLOCK_NODE*)HeadNode;
	}

	void Push(BLOCK_NODE* NewTop)
	{
		BLOCK_NODE* LocalTop;
		__int64 MaskNewTop;

		while (1)
		{
			LocalTop = TopNode;
			MaskNewTop = MaskNewTag((__int64)LocalTop, (__int64)NewTop);
			NewTop->Next = LocalTop;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)MaskNewTop, (__int64)LocalTop))
			{
				break;
			}
		}

	}


	BLOCK_NODE* Pop()
	{
		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;

		while (1)
		{

			LocalTop = TopNode;
			UnMaskTop = UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->Next;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}

		return UnMaskTop;
	}

	DATA* Alloc(void)
	{

		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;

		while (1)
		{
			LocalTop = TopNode;
			if (LocalTop == nullptr)
			{
				BLOCK_NODE* NewNode = new BLOCK_NODE;

				InterlockedIncrement(&UseCount);
				InterlockedIncrement(&Capacity);

				return &NewNode->Data;
			}

			UnMaskTop = UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->Next;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}

		if (IsPlacementNew)
		{
			new (&UnMaskTop->Data) DATA;
		}
		InterlockedIncrement(&UseCount);

		return &UnMaskTop->Data;

	}


	bool	Free(DATA* Data)
	{
		BLOCK_NODE* Temp = (BLOCK_NODE*)Data;

		if (IsPlacementNew)
		{
			Temp->Data.~DATA();
		}

		Push(Temp);

		InterlockedDecrement(&UseCount);
		return true;
	}


	int		GetCapacityCount(void) { return Capacity; }

	int		GetUseCount(void) { return UseCount; }

	unsigned int Checksum;

private:
	BLOCK_NODE* TopNode;
	__int64 ChecksumPosition;
	bool IsPlacementNew;
	long Capacity;
	long UseCount;

	unsigned long long TagIndex;

};


#endif

