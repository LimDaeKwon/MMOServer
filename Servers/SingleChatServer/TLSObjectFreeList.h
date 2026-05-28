#pragma once
#include "Windows.h"
#include <new.h>
#include <mutex>


static unsigned int GlobalChecksum1;
#define ADRMASK 0x00007fffffffffff
#define TAGMASK 0xffff800000000000


#define TAGOFFSET 47

#define MAXNODECOUNT 1000

template <class DATA>
class TLSObjectFreeList // 얘가 노드 풀. 
{

public:

	struct BLOCK_NODE
	{
		unsigned int UnderflowCheckSum;
		DATA Data;
		unsigned int OverflowCheckSum;
		BLOCK_NODE* Next;
		BLOCK_NODE* PoolNext;
	};




	TLSObjectFreeList(int BlockNum, bool PlacementNew = false) : IsPlacementNew(PlacementNew), Checksum(GlobalChecksum1++), TopNode(NULL), PoolSize(0)
	{
		TLSIndex = TlsAlloc();
		if (TLSIndex == TLS_OUT_OF_INDEXES)
		{
			__debugbreak();
		}

		InitBlockNum = BlockNum;

		BLOCK_NODE* FirstAlloc = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
		ChecksumPosition = (__int64)&FirstAlloc->Data - (__int64)&FirstAlloc->UnderflowCheckSum;
		free(FirstAlloc);


	}

	virtual	~TLSObjectFreeList()
	{
		//TLS 정리.
		TLSRegistryNode* cur = TLSRegistryHead;
		while (cur != nullptr)
		{
			ThreadLocalMember* p = cur->Ptr;
			if (p != NULL)
			{
				FreeList(p->TopNode);
				FreeList(p->FreeNode);
				delete p;
			}
			TLSRegistryNode* next = cur->Next;
			free(cur);
			cur = next;
		}
		TLSRegistryHead = nullptr;

		//풀 정리
		__int64 Tagged = (__int64)TopNode;
		while (Tagged != 0)
		{
			BLOCK_NODE* Bucket = (BLOCK_NODE*)UnMaskTag(Tagged);
			if (Bucket == nullptr)
			{
				break;
			}
			
			Tagged = (__int64)Bucket->PoolNext;
			FreeList(Bucket);
			
		}

		if (TLSIndex != TLS_OUT_OF_INDEXES)
		{
			TlsFree(TLSIndex);
		}

	}

	void FreeList(BLOCK_NODE* Node)
	{
		while (Node != NULL)
		{
			BLOCK_NODE* Next = Node->Next;
			if (IsPlacementNew)
			{
				Node->Data.~DATA();
			}
			free(Node);
			Node = Next;
		}
	}

	void PoolPush(BLOCK_NODE* NewTop)
	{

		BLOCK_NODE* NewPoolNode = NewTop;

		BLOCK_NODE* LocalTop;
		__int64 MaskNewTop = MaskNewTag((__int64)NewPoolNode);
		while (1)
		{
			LocalTop = TopNode;
			NewPoolNode->PoolNext = LocalTop;
			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)MaskNewTop, (__int64)LocalTop))
			{
				InterlockedIncrement(&PoolSize);
				break;
			}
		}
	}

	BLOCK_NODE* AllocNewBucket()
	{
		BLOCK_NODE* TempTop = nullptr;
		//버킷 단위로 할당해서 넘겨주기. 
		//성능을 위해 생각해볼 수 있는것은 버킷단위로 한 방에 할당받아서 이어서 넘겨주고
		//그 포인터를 저장해놓고 있다가 일괄 정리.

		for (int i = 0; i < MAXNODECOUNT; ++i)
		{

			BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));;

			Temp->Next = TempTop;


			if (!IsPlacementNew)
			{
				new (&Temp->Data) DATA;
			}

			Temp->OverflowCheckSum = Checksum;
			Temp->UnderflowCheckSum = Checksum;

			TempTop = Temp;
			InterlockedIncrement(&Capacity);
		}
		InterlockedIncrement(&PoolSize);
		return TempTop;
	}

	BLOCK_NODE* PoolAlloc(void)
	{

		BLOCK_NODE* LocalTop;
		BLOCK_NODE* UnMaskTop;
		BLOCK_NODE* NewTop;

		long size = InterlockedDecrement(&PoolSize);
		if (size < 0)
		{
			return AllocNewBucket();
		}

		while (1)
		{
			LocalTop = TopNode;

			UnMaskTop = (BLOCK_NODE*)UnMaskTag((__int64)LocalTop);
			NewTop = UnMaskTop->PoolNext;

			if ((__int64)LocalTop == InterlockedCompareExchange64((__int64*)&TopNode, (__int64)NewTop, (__int64)LocalTop))
			{
				break;
			}
		}
		return (BLOCK_NODE*)UnMaskTop;

	}

private:
	BLOCK_NODE* TopNode;

	__int64 ChecksumPosition;
	unsigned int Checksum;
	bool IsPlacementNew;
	long InitBlockNum;
	DWORD TLSIndex;

	unsigned long long TagIndex;
	

	long Capacity;
	long UseCount;

	long PoolSize;
	long PoolUseCount;


public:


	struct ThreadLocalMember
	{
		BLOCK_NODE* TopNode;
		long TopCount;

		BLOCK_NODE* FreeNode;
		long FreeCount;

	};

	//종료시 모든 TLS 해제를 위해 등록시에 관리하기.
	struct TLSRegistryNode
	{
		ThreadLocalMember* Ptr;
		TLSRegistryNode* Next;
		DWORD              ThreadId;
	};

	TLSRegistryNode* TLSRegistryHead = nullptr;
	std::mutex       TLSRegistryLock;

	void RegisterTLS(ThreadLocalMember* NewTLS)
	{
		TLSRegistryNode* NewTLSNode = (TLSRegistryNode*)malloc(sizeof(TLSRegistryNode));
		NewTLSNode->Ptr = NewTLS;
		NewTLSNode->ThreadId = GetCurrentThreadId();

		std::lock_guard<std::mutex> guard(TLSRegistryLock);
		NewTLSNode->Next = TLSRegistryHead;
		TLSRegistryHead = NewTLSNode;
	}


	ThreadLocalMember* GetTLS()
	{

		ThreadLocalMember* LocalMember = (ThreadLocalMember*)TlsGetValue(TLSIndex);
		if (LocalMember == NULL)
		{
			LocalMember = new ThreadLocalMember;
			memset(LocalMember, 0, sizeof(ThreadLocalMember));


			for (int i = 0; i < InitBlockNum; ++i)
			{
				BLOCK_NODE* Temp = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));;

				if (!IsPlacementNew)
				{
					new (&Temp->Data) DATA;
				}

				Temp->OverflowCheckSum = Checksum;
				Temp->UnderflowCheckSum = Checksum;


				Push(Temp, LocalMember);
				InterlockedIncrement(&Capacity);
			}


			TlsSetValue(TLSIndex, LocalMember);
			RegisterTLS(LocalMember);
		}

		return LocalMember;
	}


	__int64 MaskNewTag(__int64 MaskNewNode)
	{
		__int64 Tag = InterlockedIncrement(&TagIndex);
		MaskNewNode |= (Tag << TAGOFFSET);

		return MaskNewNode;
	}

	__int64 UnMaskTag(__int64 HeadNode)
	{
		HeadNode &= ADRMASK;
		return HeadNode;
	}


	void Push(BLOCK_NODE* NewTop, ThreadLocalMember* ThreadLocal)
	{
		//메인 풀 다 찼는지

		if (ThreadLocal->TopCount == MAXNODECOUNT)
		{

			NewTop->Next = ThreadLocal->FreeNode;
			ThreadLocal->FreeNode = NewTop;
			ThreadLocal->FreeCount++;

			//반환용 풀 다 찼는지
			if (ThreadLocal->FreeCount == MAXNODECOUNT)
			{
				//반환
				PoolPush(ThreadLocal->FreeNode);
				ThreadLocal->FreeNode = nullptr;
				ThreadLocal->FreeCount = 0;
			}
			return;
		}
		NewTop->Next = ThreadLocal->TopNode;
		ThreadLocal->TopNode = NewTop;
		ThreadLocal->TopCount++;

	}


	DATA* Alloc(void)
	{

		BLOCK_NODE* NewData;

		ThreadLocalMember* ThreadLocal = GetTLS();

		if (ThreadLocal->FreeNode == nullptr)
		{
			if (ThreadLocal->TopNode == nullptr)
			{
				ThreadLocal->TopNode = PoolAlloc();
				ThreadLocal->TopCount = MAXNODECOUNT;
			}

			NewData = ThreadLocal->TopNode;
			ThreadLocal->TopNode = NewData->Next;
			ThreadLocal->TopCount--;

			if (IsPlacementNew)
			{
				new (&NewData->Data) DATA;
			}
			InterlockedIncrement(&UseCount);

			return &NewData->Data;

		}

		NewData = ThreadLocal->FreeNode;
		ThreadLocal->FreeNode = NewData->Next;
		ThreadLocal->FreeCount--;

		if (IsPlacementNew)
		{
			new (&NewData->Data) DATA;
		}

		InterlockedIncrement(&UseCount);
		return &NewData->Data;

	}

	bool	Free(DATA* Data)
	{
		ThreadLocalMember* ThreadLocal = GetTLS();

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

		Push(Temp, ThreadLocal);
		InterlockedDecrement(&UseCount);
		return true;
	}


	int		GetCapacityCount(void) { return Capacity; }

	int		GetUseCount(void) { return UseCount; }

	int		GetPoolSize(void) { return PoolSize; }




};

