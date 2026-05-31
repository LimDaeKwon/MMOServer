#pragma once

#include <Windows.h>

#include "CoreDefines.h"
#include "TLSObjectFreeList.h"

template <typename DataType>
class LockFreeQueue
{
private:
    struct Node
    {
        DataType data_;
        Node* next_;
    };

public:
    LockFreeQueue()
        : size_(0),
        index_(0),
        head_(nullptr),
        tail_(nullptr)
    {
        Node* dummy = nodePool_.Alloc();

        dummy->next_ = nullptr;

        head_ = dummy;
        tail_ = dummy;
    }

    virtual ~LockFreeQueue()
    {
    }

    bool Enqueue(DataType data)
    {
        int localSize = InterlockedAdd(&size_, 0);

        if (localSize > 50000)
        {
            return false;
        }

        Node* newNode = nodePool_.Alloc();

        newNode->data_ = data;
        newNode->next_ = nullptr;

        __int64 newTag = InterlockedAdd64(
            &index_,
            static_cast<__int64>(1) << DKServerCore::TagOffset);

        newNode = reinterpret_cast<Node*>(
            reinterpret_cast<__int64>(newNode) | newTag);

        Node* oldTail = nullptr;
        Node* unmaskedTail = nullptr;

        while (true)
        {
            oldTail = tail_;
            unmaskedTail = UnmaskNode(oldTail);

            if (unmaskedTail->next_ == nullptr)
            {
                if (InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&unmaskedTail->next_),
                    reinterpret_cast<__int64>(newNode),
                    reinterpret_cast<__int64>(nullptr)) == reinterpret_cast<__int64>(nullptr))
                {
                    break;
                }
            }
            else
            {
                AdvanceTailToNullptr();
            }
        }

        InterlockedCompareExchange64(
            reinterpret_cast<volatile __int64*>(&tail_),
            reinterpret_cast<__int64>(newNode),
            reinterpret_cast<__int64>(oldTail));

        InterlockedIncrement(&size_);

        return true;
    }

    bool Dequeue(DataType* data)
    {
        int localSize = InterlockedDecrement(&size_);

        if (localSize < 0)
        {
            InterlockedIncrement(&size_);
            return false;
        }

        Node* oldHead = nullptr;
        Node* unmaskedHead = nullptr;
        Node* next = nullptr;

        while (true)
        {
            AdvanceTailToNullptr();

            oldHead = head_;
            unmaskedHead = UnmaskNode(oldHead);
            next = unmaskedHead->next_;

            if (next == nullptr)
            {
                continue;
            }

            if (InterlockedCompareExchange64(
                reinterpret_cast<volatile __int64*>(&head_),
                reinterpret_cast<__int64>(next),
                reinterpret_cast<__int64>(oldHead)) == reinterpret_cast<__int64>(oldHead))
            {
                break;
            }
        }

        Node* unmaskedNext = UnmaskNode(next);

        *data = unmaskedNext->data_;

        nodePool_.Free(unmaskedHead);

        return true;
    }

    int GetSize()
    {
        return InterlockedAdd(&size_, 0);
    }

private:
    void AdvanceTailToNullptr()
    {
        Node* oldTail = nullptr;
        Node* unmaskedTail = nullptr;

        while (true)
        {
            oldTail = tail_;
            unmaskedTail = UnmaskNode(oldTail);

            if (unmaskedTail->next_ == nullptr)
            {
                if (reinterpret_cast<__int64>(oldTail) ==
                    InterlockedOr64(reinterpret_cast<volatile __int64*>(&tail_), 0))
                {
                    break;
                }

                continue;
            }

            InterlockedCompareExchange64(
                reinterpret_cast<volatile __int64*>(&tail_),
                reinterpret_cast<__int64>(unmaskedTail->next_),
                reinterpret_cast<__int64>(oldTail));
        }
    }

    Node* UnmaskNode(Node* node)
    {
        return reinterpret_cast<Node*>(
            reinterpret_cast<__int64>(node) & DKServerCore::AddressMask);
    }

private:
    volatile long size_;
    volatile __int64 index_;

    Node* head_;
    Node* tail_;

    static TLSObjectFreeList<Node> nodePool_;
};

template <class DataType>
TLSObjectFreeList<typename LockFreeQueue<DataType>::Node> LockFreeQueue<DataType>::nodePool_(0);