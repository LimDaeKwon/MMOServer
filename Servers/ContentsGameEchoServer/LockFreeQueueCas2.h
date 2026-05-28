#pragma once

#include "TLSObjectFreeList.h"
//#define SCENARIO

#ifdef SCENARIO
#define MAKESCENARIO(ThreadID,What,Size,CurrentNode,OldNode,NewNode,Cookie) MakeScenario(ThreadID,What,Size,CurrentNode,OldNode,NewNode,Cookie)
#define MAXLOGBUFFERSIZE 65536


#else
#define MAKESCENARIO(ThreadID,What,Size,CurrentNode,OldNode,NewNode,Cookie)
#define MAXLOGBUFFERSIZE 1

#endif
//클래스 멤버에 넣는다면? 

#define ADRMASK 0x00007fffffffffff
#define TAGMASK 0xffff800000000000
#define MAKETAG 0x0000800000000000

#define MAXQSIZE 200


//unsigned long long logindexQ = 0;
struct LogData
{
	DWORD ThreadID;
	int Work = 0xcccccccc;//0xbbbbbbbb Enqueue 0xdddddddd Dequeue
	int Size = 0xcccccccc;
	int Cookie = 0xcccccccc;
	void* CurrentNode = nullptr;
	__int64 Cookie2 = 0xcccccccccccccccc;
	void* OldNode = nullptr;
	__int64 Cookie3 = 0xcccccccccccccccc;
	void* NewNode = nullptr;
};

//LogData LogBuffer[MAXLOGBUFFERSIZE];

template <class DATA>
class TLockFreeQueue
{
private:
	struct Node
	{
		DATA Data;
		Node* Next;
	};

public:
	TLockFreeQueue() //: NodeFreeList(0)
	{
		Node* DummyNode = NodeFreeList.Alloc();
		DummyNode->Next = nullptr;
		Head = DummyNode;
		Tail = DummyNode;
	}
	~TLockFreeQueue()
	{
		Node* DeleteNode;
		while (1)
		{
			DeleteNode = UnMaskTag((unsigned long long) Head);

			if (DeleteNode->Next == nullptr)
			{
				delete DeleteNode;
				break;
			}
			Head = DeleteNode->Next;
			delete DeleteNode;
		}
	}

	__int64 MaskNewTag(__int64 LocalTop, __int64 MaskNewNode)
	{
		__int64 Tag = LocalTop;
		Tag &= TAGMASK;
		Tag += MAKETAG;
		MaskNewNode |= Tag;

		return MaskNewNode;
	}


	Node* UnMaskTag(__int64 HeadNode)
	{
		HeadNode &= ADRMASK;
		return (Node*)HeadNode;
	}


	int Enqueue(DATA data)
	{

		//unsigned long QueueSize = InterlockedIncrement(&Size);
		//if (MAXQSIZE < QueueSize)
		//{
		//	InterlockedDecrement(&Size);
		//	return false;
		//}
		// 
		//이러면 인큐 실패 시 디큐에 묶여있음.
		
		//Or로 체크하는게 좋아보인다. 

		long QueueSize = InterlockedOr(&Size,0);
		
		if (MAXQSIZE < QueueSize)
		{
			//__debugbreak();
			return false;
		}



		Node* OldTail;
		Node* UnMaskTail;
		Node* NewNode = NodeFreeList.Alloc();
		NewNode->Data = data;
		NewNode->Next = nullptr;
		unsigned long long MaskNewNode;


		while (1)
		{
			OldTail = Tail;
			MaskNewNode = MaskNewTag((__int64)OldTail, (__int64)NewNode);
			UnMaskTail = UnMaskTag((unsigned long long)OldTail);

			if (UnMaskTail->Next != nullptr)
			{
				InterlockedCompareExchange64((__int64*)&Tail, (__int64)UnMaskTail->Next, (__int64)OldTail);
				continue;
			}
			//공용 노드 풀 사용과 큐의 결함을 막기 위한 CAS 순서 바꾸기..  맘에 들지는 않음. 
			if (InterlockedCompareExchange64((__int64*)&Tail, (_int64)MaskNewNode, (__int64)OldTail) == (__int64)OldTail)
			{
				if (InterlockedCompareExchange64((__int64*)&UnMaskTail->Next, (__int64)MaskNewNode, (__int64)nullptr) == (__int64)nullptr)
				{
					break;
				}
			}
		}
		__int64 CurrentTail = InterlockedCompareExchange64((__int64*)&Tail, (_int64)MaskNewNode, (__int64)OldTail);

		InterlockedIncrement(&Size);


		MAKESCENARIO(GetCurrentThreadId(), 0xeeeeeeee, QueueSize, (Node*)CurrentTail, OldTail, (Node*)MaskNewNode, 0xeeeeeeeeeeeeeeee);

		return true;

	}

	bool Dequeue(DATA* data)
	{
		long QueueSize = InterlockedDecrement(&Size);
		if (QueueSize < 0)
		{
			InterlockedIncrement(&Size);
			return false;
		}

		Node* OldHead = nullptr;
		Node* NewHead = nullptr;
		Node* UnMaskHead = nullptr;

		while (1)
		{
			AdvanceTailToNull();
			OldHead = Head;
			UnMaskHead = UnMaskTag((unsigned long long)OldHead);
			NewHead = UnMaskHead->Next;
			if (NewHead == nullptr)
			{
				continue;
			}

			Node* DataNode = UnMaskTag((unsigned long long)NewHead);
			*data = DataNode->Data;
			if (InterlockedCompareExchange64((__int64*)&Head, (__int64)NewHead, (__int64)OldHead) == (__int64)OldHead)
			{
				break;
			}
		}

		NodeFreeList.Free(UnMaskHead);
		MAKESCENARIO(GetCurrentThreadId(), 0xdddddddd, QueueSize, (Node*)nullptr, OldHead, NewHead, 0xdddddddddddddddd);

		return true;
	}

	void AdvanceTailToNull()
	{
		while (1)
		{
			Node* OldTail;
			Node* UnMaskTail;

			OldTail = Tail;
			UnMaskTail = UnMaskTag((unsigned long long)OldTail);
			if (UnMaskTail->Next == nullptr)
			{
				break;
			}
			InterlockedCompareExchange64((__int64*)&Tail, (__int64)UnMaskTail->Next, (__int64)OldTail);
		}
	}


	void MakeScenario(DWORD ThreadID, DWORD What, DWORD Size, Node* CurrentNode, Node* OldNode, Node* NewNode, __int64 Cookie)
	{
		//unsigned long long LogIndex = InterlockedIncrement(&logindexQ);
		//LogIndex %= MAXLOGBUFFERSIZE;

		//LogBuffer[LogIndex].ThreadID = ThreadID;
		//LogBuffer[LogIndex].Work = What;
		//LogBuffer[LogIndex].Size = What;
		//LogBuffer[LogIndex].CurrentNode = CurrentNode;
		//LogBuffer[LogIndex].OldNode = OldNode;
		//LogBuffer[LogIndex].NewNode = NewNode;
		//LogBuffer[LogIndex].Cookie2 = Cookie;
		//LogBuffer[LogIndex].Cookie3 = Cookie;
	}

	long GetSize()
	{
		return Size;
	}


	
private:

	static TLSObjectFreeList<Node> NodeFreeList;

	//TLSObjectFreeList<Node> NodeFreeList;
	Node* Head;
	Node* Tail;
	long Size = 0;
	
};


template<class DATA>
TLSObjectFreeList<typename TLockFreeQueue<DATA>::Node> TLockFreeQueue<DATA>::NodeFreeList(0);
