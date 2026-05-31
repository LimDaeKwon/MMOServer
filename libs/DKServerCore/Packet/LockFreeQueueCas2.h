#pragma once

#include <Windows.h>

#include "CoreDefines.h"
#include "TLSObjectFreeList.h"

template <class DataType>
class LockFreeQueueCas2
{
private:
    struct Node
    {
        DataType data_;
        Node* next_;
    };

public:
    LockFreeQueueCas2()
        : head_(nullptr),
        tail_(nullptr),
        size_(0)
    {
        Node* dummyNode = nodeFreeList_.Alloc();

        dummyNode->next_ = nullptr;

        head_ = dummyNode;
        tail_ = dummyNode;
    }

    ~LockFreeQueueCas2()
    {
        Node* deleteNode = nullptr;

        while (true)
        {
            deleteNode = UnmaskTag(reinterpret_cast<__int64>(head_));

            if (deleteNode->next_ == nullptr)
            {
                nodeFreeList_.Free(deleteNode);
                break;
            }

            head_ = deleteNode->next_;
            nodeFreeList_.Free(deleteNode);
        }
    }

    int Enqueue(DataType data)
    {
        long queueSize = InterlockedOr(&size_, 0);

        if (DKServerCore::LockFreeQueueCas2MaxSize < queueSize)
        {
            return false;
        }

        Node* oldTail = nullptr;
        Node* unmaskedTail = nullptr;

        Node* newNode = nodeFreeList_.Alloc();

        newNode->data_ = data;
        newNode->next_ = nullptr;

        unsigned long long maskedNewNode = 0;

        while (true)
        {
            oldTail = tail_;
            maskedNewNode = MaskNewTag(
                reinterpret_cast<__int64>(oldTail),
                reinterpret_cast<__int64>(newNode));

            unmaskedTail = UnmaskTag(reinterpret_cast<__int64>(oldTail));

            if (unmaskedTail->next_ != nullptr)
            {
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&tail_),
                    reinterpret_cast<__int64>(unmaskedTail->next_),
                    reinterpret_cast<__int64>(oldTail));

                continue;
            }

            if (InterlockedCompareExchange64(
                reinterpret_cast<volatile __int64*>(&tail_),
                static_cast<__int64>(maskedNewNode),
                reinterpret_cast<__int64>(oldTail)) == reinterpret_cast<__int64>(oldTail))
            {
                if (InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&unmaskedTail->next_),
                    static_cast<__int64>(maskedNewNode),
                    reinterpret_cast<__int64>(nullptr)) == reinterpret_cast<__int64>(nullptr))
                {
                    break;
                }
            }
        }

        InterlockedCompareExchange64(
            reinterpret_cast<volatile __int64*>(&tail_),
            static_cast<__int64>(maskedNewNode),
            reinterpret_cast<__int64>(oldTail));

        InterlockedIncrement(&size_);

        return true;
    }

    bool Dequeue(DataType* data)
    {
        long queueSize = InterlockedDecrement(&size_);

        if (queueSize < 0)
        {
            InterlockedIncrement(&size_);
            return false;
        }

        Node* oldHead = nullptr;
        Node* newHead = nullptr;
        Node* unmaskedHead = nullptr;

        while (true)
        {
            AdvanceTailToNull();

            oldHead = head_;
            unmaskedHead = UnmaskTag(reinterpret_cast<__int64>(oldHead));
            newHead = unmaskedHead->next_;

            if (newHead == nullptr)
            {
                continue;
            }

            Node* dataNode = UnmaskTag(reinterpret_cast<__int64>(newHead));

            *data = dataNode->data_;

            if (InterlockedCompareExchange64(
                reinterpret_cast<volatile __int64*>(&head_),
                reinterpret_cast<__int64>(newHead),
                reinterpret_cast<__int64>(oldHead)) == reinterpret_cast<__int64>(oldHead))
            {
                break;
            }
        }

        nodeFreeList_.Free(unmaskedHead);

        return true;
    }

    long GetSize()
    {
        return InterlockedOr(&size_, 0);
    }

private:
    __int64 MaskNewTag(__int64 localTop, __int64 maskedNewNode)
    {
        __int64 tag = localTop;

        tag &= DKServerCore::TagMask;
        tag += static_cast<__int64>(1) << DKServerCore::TagOffset;

        maskedNewNode |= tag;

        return maskedNewNode;
    }

    Node* UnmaskTag(__int64 node)
    {
        node &= DKServerCore::AddressMask;

        return reinterpret_cast<Node*>(node);
    }

    void AdvanceTailToNull()
    {
        while (true)
        {
            Node* oldTail = tail_;
            Node* unmaskedTail = UnmaskTag(reinterpret_cast<__int64>(oldTail));

            if (unmaskedTail->next_ == nullptr)
            {
                break;
            }

            InterlockedCompareExchange64(
                reinterpret_cast<volatile __int64*>(&tail_),
                reinterpret_cast<__int64>(unmaskedTail->next_),
                reinterpret_cast<__int64>(oldTail));
        }
    }

private:
    static TLSObjectFreeList<Node> nodeFreeList_;

    Node* head_;
    Node* tail_;
    volatile long size_;
};

template <class DataType>
TLSObjectFreeList<typename LockFreeQueueCas2<DataType>::Node> LockFreeQueueCas2<DataType>::nodeFreeList_(0);

template <class DataType>
using TLockFreeQueue = LockFreeQueueCas2<DataType>;