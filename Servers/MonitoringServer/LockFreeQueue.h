#pragma once
#include "LockFreeObjectFreeList.h"
#include "TLSObjectFreeList.h"

#define LFQTAGMASK 0x00007FFFFFFFFFFF
#define LFQUNMASK  0xFFFF800000000000
#define LFQADDTAG  0x0000800000000000




template<typename T>
class LFQ
{
private:
	long _size;

	struct Node
	{
		T data;
		Node* next;
	};

	//struct LogData
	//{
	//	DWORD _thread;
	//	DWORD _size;
	//	__int64 _work;
	//	Node* _current;
	//	__int64 _blank2 = 0;
	//	Node* _old;
	//	__int64 _blank3 = 0;
	//	Node* _new;
	//};
	long long _index = 0;

	static TLSObjectFreeList<Node> _nodepool;
	//LogData logdata[65536];


	Node* _head;
	Node* _tail;
	short _logindex;

public:
	LFQ()
	{
		_size = 0;
		Node* dummy = _nodepool.Alloc();
		dummy->next = nullptr;
		_head = dummy;
		_tail = dummy;

	}

	virtual ~LFQ()
	{

	}


	bool Enqueue(T t)
	{

		int localsize = InterlockedAdd(&_size, 0);
		if (localsize > 50000)
		{
			return false;//리턴 false;
		}



		Node* newnode = _nodepool.Alloc();
		newnode->data = t;
		newnode->next = nullptr;


		//unsigned short li1 = InterlockedIncrement16(&_logindex);
		//MakeScenario(GetCurrentThreadId(), 0xAAAAAAAAAAAAAAAA, localsize, (Node*)newnode, newnode, newnode, li1);


		Node* oldtail;
		Node* unmasktail;

		__int64 new_tag = InterlockedAdd64(&_index, LFQADDTAG);
		newnode = (Node*)((__int64)newnode | new_tag);


		while (1)
		{
			oldtail = _tail;
			unmasktail = (Node*)((__int64)oldtail & LFQTAGMASK); // 태그 제거
			//tag = (__int64)oldtail & LFQUNMASK; // 태그 추출
			//tag += LFQADDTAG; // 태그 갱신 

			//newnode = (Node*)((__int64)newnode & LFQTAGMASK); // newnode에 태그 밀어주기.
			//newnode = (Node*)((__int64)newnode | tag); //newnode에 태그 심기. 

			// 현재 테일의 넥스트 (지금 노드가 들어갈 곳)
			if (unmasktail->next == nullptr)
			{
				if (InterlockedCompareExchange64((__int64*)&unmasktail->next, (__int64)newnode, (__int64)nullptr) == (__int64)nullptr)
				{
					break;
				}
			}
			else
			{
				AdvencedTailToNullptr();
			}
		}

		InterlockedCompareExchange64((__int64*)&_tail, (__int64)newnode, (__int64)oldtail);
		// << 실패의 경우 그 이유 추적

		//InterlockedIncrement(&_size);
		int ls = InterlockedIncrement(&_size);
		/*unsigned short li = InterlockedIncrement16(&_logindex);
		MakeScenario(GetCurrentThreadId(), 0xEEEEEEEE22222222, ls, (Node*)current, oldtail, newnode, li);*/

		return true;

	}

	void AdvencedTailToNullptr()
	{

		Node* oldtail;
		Node* unmasktail;

		while (1)
		{
			oldtail = _tail;
			unmasktail = (Node*)((__int64)oldtail & LFQTAGMASK); // 태그 제거
			if (unmasktail->next == nullptr)
			{
				if ((__int64)oldtail == InterlockedOr64((__int64*)&_tail, 0))
				{
					break;
				}
				continue;
			}
			InterlockedCompareExchange64((__int64*)&_tail, (__int64)unmasktail->next, (__int64)oldtail);
		}

	}

	bool Dequeue(T* t)
	{
		int localsize = InterlockedDecrement(&_size);

		if (localsize < 0)
		{
			InterlockedIncrement(&_size);
			return false;
		}


		Node* oldhead;
		Node* unmaskhead;
		Node* next;
		while (true)
		{
			AdvencedTailToNullptr();

			oldhead = _head; // 헤드 저장
			unmaskhead = (Node*)((__int64)oldhead & LFQTAGMASK); // 헤드 언마스크 (넥스트 접근을 위해)
			next = unmaskhead->next;

			if (next == nullptr)
			{
				continue;
			}
			if (InterlockedCompareExchange64((__int64*)&_head, (__int64)next, (__int64)oldhead) == (__int64)oldhead)
			{
				break;
			}
		}

		/*	int ls = localsize;
			unsigned short li = InterlockedIncrement16(&_logindex);
			MakeScenario(GetCurrentThreadId(), 0xDDDDDDDDDDDDDDDD, ls, (Node*)oldhead, 0, next, li);*/


		Node* unmasknext = (Node*)((__int64)next & LFQTAGMASK);
		*t = unmasknext->data;


		_nodepool.Free(unmaskhead);

		return true;
	}

	/*void MakeScenario(DWORD _thread, __int64 _work, DWORD _size, Node* _current, Node* _old, Node* _new, int Index)
	{
		logdata[Index]._thread = _thread;
		logdata[Index]._work = _work;
		logdata[Index]._size = _size;
		logdata[Index]._current = _current;
		logdata[Index]._old = _old;
		logdata[Index]._new = _new;
	}*/


	int GetSize()
	{
		return _size;
	}


};


template<class DATA>
TLSObjectFreeList<typename LFQ<DATA>::Node> LFQ<DATA>::_nodepool(0);