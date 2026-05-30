#pragma once

#include <Windows.h>

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>

#include "CoreDefines.h"

static unsigned int GlobalChecksum1 = 0;

constexpr int MaxNodeCount = 1000;

#ifdef DK_ENABLE_MEMORY_CHECK

template <class DataType>
class TLSObjectFreeList
{
public:
    struct BlockNode
    {
        unsigned int underflowChecksum_;
        DataType data_;
        unsigned int overflowChecksum_;
        BlockNode* next_;
        BlockNode* poolNext_;
    };

    struct ThreadLocalMember
    {
        BlockNode* topNode_;
        long topCount_;

        BlockNode* freeNode_;
        long freeCount_;
    };

    struct TLSRegistryNode
    {
        ThreadLocalMember* ptr_;
        TLSRegistryNode* next_;
        DWORD threadId_;
    };

public:
    TLSObjectFreeList(int blockNum, bool placementNew = false)
        : topNode_(nullptr),
        checksumPosition_(0),
        checksum_(GlobalChecksum1++),
        isPlacementNew_(placementNew),
        initBlockNum_(blockNum),
        tlsIndex_(TLS_OUT_OF_INDEXES),
        tagIndex_(0),
        capacity_(0),
        useCount_(0),
        poolSize_(0),
        poolUseCount_(0),
        tlsRegistryHead_(nullptr)
    {
        tlsIndex_ = TlsAlloc();

        if (tlsIndex_ == TLS_OUT_OF_INDEXES)
        {
            DebugBreak();
        }

        BlockNode* findOffset = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

        checksumPosition_ =
            reinterpret_cast<__int64>(&findOffset->data_) -
            reinterpret_cast<__int64>(&findOffset->underflowChecksum_);

        free(findOffset);
    }

    virtual ~TLSObjectFreeList()
    {
        TLSRegistryNode* currentNode = tlsRegistryHead_;

        while (currentNode != nullptr)
        {
            ThreadLocalMember* threadLocal = currentNode->ptr_;

            if (threadLocal != nullptr)
            {
                FreeList(threadLocal->topNode_);
                FreeList(threadLocal->freeNode_);

                delete threadLocal;
            }

            TLSRegistryNode* nextNode = currentNode->next_;
            free(currentNode);
            currentNode = nextNode;
        }

        tlsRegistryHead_ = nullptr;

        __int64 taggedNode = reinterpret_cast<__int64>(topNode_);

        while (taggedNode != 0)
        {
            BlockNode* bucket = UnmaskTag(taggedNode);

            if (bucket == nullptr)
            {
                break;
            }

            taggedNode = reinterpret_cast<__int64>(bucket->poolNext_);
            FreeList(bucket);
        }

        if (tlsIndex_ != TLS_OUT_OF_INDEXES)
        {
            TlsFree(tlsIndex_);
        }
    }

    DataType* Alloc()
    {
        BlockNode* newData = nullptr;
        ThreadLocalMember* threadLocal = GetTLS();

        if (threadLocal->freeNode_ == nullptr)
        {
            if (threadLocal->topNode_ == nullptr)
            {
                threadLocal->topNode_ = PoolAlloc();
                threadLocal->topCount_ = MaxNodeCount;
            }

            newData = threadLocal->topNode_;
            threadLocal->topNode_ = newData->next_;
            --threadLocal->topCount_;

            if (isPlacementNew_)
            {
                new (&newData->data_) DataType;
            }

            InterlockedIncrement(&useCount_);

            return &newData->data_;
        }

        newData = threadLocal->freeNode_;
        threadLocal->freeNode_ = newData->next_;
        --threadLocal->freeCount_;

        if (isPlacementNew_)
        {
            new (&newData->data_) DataType;
        }

        InterlockedIncrement(&useCount_);

        return &newData->data_;
    }

    bool Free(DataType* data)
    {
        ThreadLocalMember* threadLocal = GetTLS();

        BlockNode* tempNode = GetNodePosition(data);
        CheckUnderOver(tempNode);

        if (isPlacementNew_)
        {
            tempNode->data_.~DataType();
        }

        Push(tempNode, threadLocal);
        InterlockedDecrement(&useCount_);

        return true;
    }

    int GetCapacityCount() const
    {
        return capacity_;
    }

    int GetUseCount() const
    {
        return useCount_;
    }

    int GetPoolSize() const
    {
        return poolSize_;
    }

private:
    void FreeList(BlockNode* node)
    {
        while (node != nullptr)
        {
            BlockNode* nextNode = node->next_;

            if (!isPlacementNew_)
            {
                node->data_.~DataType();
            }

            free(node);
            node = nextNode;
        }
    }

    void PoolPush(BlockNode* newTop)
    {
        BlockNode* newPoolNode = newTop;
        BlockNode* localTop = nullptr;
        __int64 maskedNewTop = MaskNewTag(reinterpret_cast<__int64>(newPoolNode));

        while (true)
        {
            localTop = topNode_;
            newPoolNode->poolNext_ = localTop;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    maskedNewTop,
                    reinterpret_cast<__int64>(localTop)))
            {
                InterlockedIncrement(&poolSize_);
                break;
            }
        }
    }

    BlockNode* AllocNewBucket()
    {
        BlockNode* tempTop = nullptr;

        for (int i = 0; i < MaxNodeCount; ++i)
        {
            BlockNode* tempNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

            tempNode->next_ = tempTop;

            if (!isPlacementNew_)
            {
                new (&tempNode->data_) DataType;
            }

            tempNode->overflowChecksum_ = checksum_;
            tempNode->underflowChecksum_ = checksum_;

            tempTop = tempNode;
            InterlockedIncrement(&capacity_);
        }

        InterlockedIncrement(&poolSize_);

        return tempTop;
    }

    BlockNode* PoolAlloc()
    {
        BlockNode* localTop = nullptr;
        BlockNode* unmaskedTop = nullptr;
        BlockNode* newTop = nullptr;

        long size = InterlockedDecrement(&poolSize_);

        if (size < 0)
        {
            return AllocNewBucket();
        }

        while (true)
        {
            localTop = topNode_;
            unmaskedTop = UnmaskTag(reinterpret_cast<__int64>(localTop));
            newTop = unmaskedTop->poolNext_;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    reinterpret_cast<__int64>(newTop),
                    reinterpret_cast<__int64>(localTop)))
            {
                break;
            }
        }

        return unmaskedTop;
    }

    void RegisterTLS(ThreadLocalMember* newTLS)
    {
        TLSRegistryNode* newTLSNode = static_cast<TLSRegistryNode*>(malloc(sizeof(TLSRegistryNode)));

        newTLSNode->ptr_ = newTLS;
        newTLSNode->threadId_ = GetCurrentThreadId();

        std::lock_guard<std::mutex> guard(tlsRegistryLock_);

        newTLSNode->next_ = tlsRegistryHead_;
        tlsRegistryHead_ = newTLSNode;
    }

    ThreadLocalMember* GetTLS()
    {
        ThreadLocalMember* localMember = static_cast<ThreadLocalMember*>(TlsGetValue(tlsIndex_));

        if (localMember == nullptr)
        {
            localMember = new ThreadLocalMember;
            memset(localMember, 0, sizeof(ThreadLocalMember));

            for (int i = 0; i < initBlockNum_; ++i)
            {
                BlockNode* tempNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

                if (!isPlacementNew_)
                {
                    new (&tempNode->data_) DataType;
                }

                tempNode->overflowChecksum_ = checksum_;
                tempNode->underflowChecksum_ = checksum_;

                Push(tempNode, localMember);
                InterlockedIncrement(&capacity_);
            }

            TlsSetValue(tlsIndex_, localMember);
            RegisterTLS(localMember);
        }

        return localMember;
    }

    __int64 MaskNewTag(__int64 newNode)
    {
        __int64 tag = InterlockedIncrement64(&tagIndex_);
        newNode |= tag << DKServerCore::TagOffset;

        return newNode;
    }

    BlockNode* UnmaskTag(__int64 taggedNode) const
    {
        taggedNode &= DKServerCore::AddressMask;

        return reinterpret_cast<BlockNode*>(taggedNode);
    }

    void Push(BlockNode* newTop, ThreadLocalMember* threadLocal)
    {
        if (threadLocal->topCount_ == MaxNodeCount)
        {
            newTop->next_ = threadLocal->freeNode_;
            threadLocal->freeNode_ = newTop;
            ++threadLocal->freeCount_;

            if (threadLocal->freeCount_ == MaxNodeCount)
            {
                PoolPush(threadLocal->freeNode_);
                threadLocal->freeNode_ = nullptr;
                threadLocal->freeCount_ = 0;
            }

            return;
        }

        newTop->next_ = threadLocal->topNode_;
        threadLocal->topNode_ = newTop;
        ++threadLocal->topCount_;
    }

    BlockNode* GetNodePosition(DataType* data) const
    {
        char* movePointer = reinterpret_cast<char*>(data);
        movePointer -= checksumPosition_;

        return reinterpret_cast<BlockNode*>(movePointer);
    }

    void CheckUnderOver(BlockNode* node) const
    {
        if (node->overflowChecksum_ != checksum_)
        {
            DebugBreak();
        }

        if (node->underflowChecksum_ != checksum_)
        {
            DebugBreak();
        }
    }

private:
    BlockNode* topNode_;
    __int64 checksumPosition_;
    unsigned int checksum_;
    bool isPlacementNew_;
    long initBlockNum_;
    DWORD tlsIndex_;
    volatile __int64 tagIndex_;
    long capacity_;
    long useCount_;
    long poolSize_;
    long poolUseCount_;
    TLSRegistryNode* tlsRegistryHead_;
    std::mutex tlsRegistryLock_;
};

#else

template <class DataType>
class TLSObjectFreeList
{
public:
    struct BlockNode
    {
        DataType data_;
        BlockNode* next_;
        BlockNode* poolNext_;
    };

    struct ThreadLocalMember
    {
        BlockNode* topNode_;
        long topCount_;

        BlockNode* freeNode_;
        long freeCount_;
    };

    struct TLSRegistryNode
    {
        ThreadLocalMember* ptr_;
        TLSRegistryNode* next_;
        DWORD threadId_;
    };

public:
    TLSObjectFreeList(int blockNum, bool placementNew = false)
        : topNode_(nullptr),
        isPlacementNew_(placementNew),
        initBlockNum_(blockNum),
        tlsIndex_(TLS_OUT_OF_INDEXES),
        tagIndex_(0),
        capacity_(0),
        useCount_(0),
        poolSize_(0),
        poolUseCount_(0),
        tlsRegistryHead_(nullptr)
    {
        tlsIndex_ = TlsAlloc();

        if (tlsIndex_ == TLS_OUT_OF_INDEXES)
        {
            DebugBreak();
        }
    }

    virtual ~TLSObjectFreeList()
    {
        TLSRegistryNode* currentNode = tlsRegistryHead_;

        while (currentNode != nullptr)
        {
            ThreadLocalMember* threadLocal = currentNode->ptr_;

            if (threadLocal != nullptr)
            {
                FreeList(threadLocal->topNode_);
                FreeList(threadLocal->freeNode_);

                delete threadLocal;
            }

            TLSRegistryNode* nextNode = currentNode->next_;
            free(currentNode);
            currentNode = nextNode;
        }

        tlsRegistryHead_ = nullptr;

        __int64 taggedNode = reinterpret_cast<__int64>(topNode_);

        while (taggedNode != 0)
        {
            BlockNode* bucket = UnmaskTag(taggedNode);

            if (bucket == nullptr)
            {
                break;
            }

            taggedNode = reinterpret_cast<__int64>(bucket->poolNext_);
            FreeList(bucket);
        }

        if (tlsIndex_ != TLS_OUT_OF_INDEXES)
        {
            TlsFree(tlsIndex_);
        }
    }

    DataType* Alloc()
    {
        BlockNode* newData = nullptr;
        ThreadLocalMember* threadLocal = GetTLS();

        if (threadLocal->freeNode_ == nullptr)
        {
            if (threadLocal->topNode_ == nullptr)
            {
                threadLocal->topNode_ = PoolAlloc();
                threadLocal->topCount_ = MaxNodeCount;
            }

            newData = threadLocal->topNode_;
            threadLocal->topNode_ = newData->next_;
            --threadLocal->topCount_;

            if (isPlacementNew_)
            {
                new (&newData->data_) DataType;
            }

            InterlockedIncrement(&useCount_);

            return &newData->data_;
        }

        newData = threadLocal->freeNode_;
        threadLocal->freeNode_ = newData->next_;
        --threadLocal->freeCount_;

        if (isPlacementNew_)
        {
            new (&newData->data_) DataType;
        }

        InterlockedIncrement(&useCount_);

        return &newData->data_;
    }

    bool Free(DataType* data)
    {
        ThreadLocalMember* threadLocal = GetTLS();
        BlockNode* tempNode = reinterpret_cast<BlockNode*>(data);

        if (isPlacementNew_)
        {
            tempNode->data_.~DataType();
        }

        Push(tempNode, threadLocal);
        InterlockedDecrement(&useCount_);

        return true;
    }

    int GetCapacityCount() const
    {
        return capacity_;
    }

    int GetUseCount() const
    {
        return useCount_;
    }

    int GetPoolSize() const
    {
        return poolSize_;
    }

private:
    void FreeList(BlockNode* node)
    {
        while (node != nullptr)
        {
            BlockNode* nextNode = node->next_;

            if (!isPlacementNew_)
            {
                node->data_.~DataType();
            }

            free(node);
            node = nextNode;
        }
    }

    void PoolPush(BlockNode* newTop)
    {
        BlockNode* newPoolNode = newTop;
        BlockNode* localTop = nullptr;
        __int64 maskedNewTop = MaskNewTag(reinterpret_cast<__int64>(newPoolNode));

        while (true)
        {
            localTop = topNode_;
            newPoolNode->poolNext_ = localTop;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    maskedNewTop,
                    reinterpret_cast<__int64>(localTop)))
            {
                InterlockedIncrement(&poolSize_);
                break;
            }
        }
    }

    BlockNode* AllocNewBucket()
    {
        BlockNode* tempTop = nullptr;

        for (int i = 0; i < MaxNodeCount; ++i)
        {
            BlockNode* tempNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

            tempNode->next_ = tempTop;

            if (!isPlacementNew_)
            {
                new (&tempNode->data_) DataType;
            }

            tempTop = tempNode;
            InterlockedIncrement(&capacity_);
        }

        InterlockedIncrement(&poolSize_);

        return tempTop;
    }

    BlockNode* PoolAlloc()
    {
        BlockNode* localTop = nullptr;
        BlockNode* unmaskedTop = nullptr;
        BlockNode* newTop = nullptr;

        long size = InterlockedDecrement(&poolSize_);

        if (size < 0)
        {
            return AllocNewBucket();
        }

        while (true)
        {
            localTop = topNode_;
            unmaskedTop = UnmaskTag(reinterpret_cast<__int64>(localTop));
            newTop = unmaskedTop->poolNext_;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    reinterpret_cast<__int64>(newTop),
                    reinterpret_cast<__int64>(localTop)))
            {
                break;
            }
        }

        return unmaskedTop;
    }

    void RegisterTLS(ThreadLocalMember* newTLS)
    {
        TLSRegistryNode* newTLSNode = static_cast<TLSRegistryNode*>(malloc(sizeof(TLSRegistryNode)));

        newTLSNode->ptr_ = newTLS;
        newTLSNode->threadId_ = GetCurrentThreadId();

        std::lock_guard<std::mutex> guard(tlsRegistryLock_);

        newTLSNode->next_ = tlsRegistryHead_;
        tlsRegistryHead_ = newTLSNode;
    }

    ThreadLocalMember* GetTLS()
    {
        ThreadLocalMember* localMember = static_cast<ThreadLocalMember*>(TlsGetValue(tlsIndex_));

        if (localMember == nullptr)
        {
            localMember = new ThreadLocalMember;
            memset(localMember, 0, sizeof(ThreadLocalMember));

            for (int i = 0; i < initBlockNum_; ++i)
            {
                BlockNode* tempNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

                if (!isPlacementNew_)
                {
                    new (&tempNode->data_) DataType;
                }

                Push(tempNode, localMember);
                InterlockedIncrement(&capacity_);
            }

            TlsSetValue(tlsIndex_, localMember);
            RegisterTLS(localMember);
        }

        return localMember;
    }

    __int64 MaskNewTag(__int64 newNode)
    {
        __int64 tag = InterlockedIncrement64(&tagIndex_);
        newNode |= tag << DKServerCore::TagOffset;

        return newNode;
    }

    BlockNode* UnmaskTag(__int64 taggedNode) const
    {
        taggedNode &= DKServerCore::AddressMask;

        return reinterpret_cast<BlockNode*>(taggedNode);
    }

    void Push(BlockNode* newTop, ThreadLocalMember* threadLocal)
    {
        if (threadLocal->topCount_ == MaxNodeCount)
        {
            newTop->next_ = threadLocal->freeNode_;
            threadLocal->freeNode_ = newTop;
            ++threadLocal->freeCount_;

            if (threadLocal->freeCount_ == MaxNodeCount)
            {
                PoolPush(threadLocal->freeNode_);
                threadLocal->freeNode_ = nullptr;
                threadLocal->freeCount_ = 0;
            }

            return;
        }

        newTop->next_ = threadLocal->topNode_;
        threadLocal->topNode_ = newTop;
        ++threadLocal->topCount_;
    }

private:
    BlockNode* topNode_;
    bool isPlacementNew_;
    long initBlockNum_;
    DWORD tlsIndex_;
    volatile __int64 tagIndex_;
    long capacity_;
    long useCount_;
    long poolSize_;
    long poolUseCount_;
    TLSRegistryNode* tlsRegistryHead_;
    std::mutex tlsRegistryLock_;
};

#endif