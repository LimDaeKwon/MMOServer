#pragma once

#include <Windows.h>
#include <new>

#include "CoreDefines.h"

extern unsigned int GlobalChecksum;

#if DK_ENABLE_MEMORY_CHECK

template <class DataType>
class LFObjectFreeList
{
public:
    struct BlockNode
    {
        unsigned int underflowChecksum_;
        DataType data_;
        unsigned int overflowChecksum_;
        BlockNode* next_;
    };

public:
    LFObjectFreeList(int blockNum, bool placementNew = false)
        : topNode_(nullptr),
        checksumPosition_(0),
        isPlacementNew_(placementNew),
        capacity_(0),
        useCount_(0),
        tagIndex_(0),
        checksum_(GlobalChecksum++)
    {
        BlockNode* findOffset = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

        checksumPosition_ =
            reinterpret_cast<__int64>(&findOffset->data_) -
            reinterpret_cast<__int64>(&findOffset->underflowChecksum_);

        free(findOffset);

        for (int i = 0; i < blockNum; ++i)
        {
            BlockNode* tempNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

            tempNode->overflowChecksum_ = checksum_;
            tempNode->underflowChecksum_ = checksum_;

            Push(tempNode);
            ++capacity_;
        }
    }

    virtual ~LFObjectFreeList()
    {
        __int64 taggedTop = reinterpret_cast<__int64>(topNode_);

        for (int i = 0; i < capacity_; ++i)
        {
            BlockNode* deleteNode = UnmaskTag(taggedTop);
            __int64 nextTagged = reinterpret_cast<__int64>(deleteNode->next_);

            free(deleteNode);
            taggedTop = nextTagged;
        }
    }

    void Push(BlockNode* newTop)
    {
        BlockNode* localTop = nullptr;
        __int64 maskedNewTop = 0;

        while (true)
        {
            localTop = topNode_;
            maskedNewTop = MaskNewTag(reinterpret_cast<__int64>(localTop), reinterpret_cast<__int64>(newTop));

            newTop->next_ = localTop;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    maskedNewTop,
                    reinterpret_cast<__int64>(localTop)))
            {
                break;
            }
        }
    }

    DataType* Alloc()
    {
        BlockNode* localTop = nullptr;
        BlockNode* unmaskedTop = nullptr;
        BlockNode* newTop = nullptr;

        while (true)
        {
            localTop = topNode_;

            if (localTop == nullptr)
            {
                BlockNode* newNode = AllocNewNode();

                InterlockedIncrement(&useCount_);
                InterlockedIncrement(&capacity_);

                return &newNode->data_;
            }

            unmaskedTop = UnmaskTag(reinterpret_cast<__int64>(localTop));
            newTop = unmaskedTop->next_;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    reinterpret_cast<__int64>(newTop),
                    reinterpret_cast<__int64>(localTop)))
            {
                break;
            }
        }

        if (isPlacementNew_)
        {
            new (&unmaskedTop->data_) DataType;
        }

        InterlockedIncrement(&useCount_);

        return &unmaskedTop->data_;
    }

    bool Free(DataType* data)
    {
        BlockNode* returnNode = GetNodePosition(data);

        CheckUnderOver(returnNode);

        if (isPlacementNew_)
        {
            returnNode->data_.~DataType();
        }

        Push(returnNode);

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

private:
    BlockNode* AllocNewNode()
    {
        BlockNode* newNode = new BlockNode;

        newNode->overflowChecksum_ = checksum_;
        newNode->underflowChecksum_ = checksum_;

        return newNode;
    }

    BlockNode* GetNodePosition(DataType* data) const
    {
        char* movePointer = reinterpret_cast<char*>(data);
        movePointer -= checksumPosition_;

        return reinterpret_cast<BlockNode*>(movePointer);
    }

    void CheckUnderOver(BlockNode* returnNode) const
    {
        if (returnNode->overflowChecksum_ != checksum_)
        {
            DebugBreak();
        }

        if (returnNode->underflowChecksum_ != checksum_)
        {
            DebugBreak();
        }
    }

    __int64 MaskNewTag(__int64 localTop, __int64 newNode)
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

private:
    BlockNode* topNode_;
    __int64 checksumPosition_;
    bool isPlacementNew_;
    long capacity_;
    long useCount_;
    volatile __int64 tagIndex_;
    unsigned int checksum_;
};

#else

template <class DataType>
class LFObjectFreeList
{
public:
    struct BlockNode
    {
        DataType data_;
        BlockNode* next_;
    };

public:
    LFObjectFreeList(int blockNum, bool placementNew = false)
        : topNode_(nullptr),
        isPlacementNew_(placementNew),
        capacity_(0),
        useCount_(0),
        tagIndex_(0)
    {
        for (int i = 0; i < blockNum; ++i)
        {
            BlockNode* tempNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

            Push(tempNode);
            ++capacity_;
        }
    }

    virtual ~LFObjectFreeList()
    {
        __int64 taggedTop = reinterpret_cast<__int64>(topNode_);

        for (int i = 0; i < capacity_; ++i)
        {
            BlockNode* deleteNode = UnmaskTag(taggedTop);
            __int64 nextTagged = reinterpret_cast<__int64>(deleteNode->next_);

            delete deleteNode;
            taggedTop = nextTagged;
        }
    }

    void Push(BlockNode* newTop)
    {
        BlockNode* localTop = nullptr;
        __int64 maskedNewTop = 0;

        while (true)
        {
            localTop = topNode_;
            maskedNewTop = MaskNewTag(reinterpret_cast<__int64>(localTop), reinterpret_cast<__int64>(newTop));

            newTop->next_ = localTop;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    maskedNewTop,
                    reinterpret_cast<__int64>(localTop)))
            {
                break;
            }
        }
    }

    DataType* Alloc()
    {
        BlockNode* localTop = nullptr;
        BlockNode* unmaskedTop = nullptr;
        BlockNode* newTop = nullptr;

        while (true)
        {
            localTop = topNode_;

            if (localTop == nullptr)
            {
                BlockNode* newNode = AllocNewNode();

                InterlockedIncrement(&useCount_);
                InterlockedIncrement(&capacity_);

                return &newNode->data_;
            }

            unmaskedTop = UnmaskTag(reinterpret_cast<__int64>(localTop));
            newTop = unmaskedTop->next_;

            if (reinterpret_cast<__int64>(localTop) ==
                InterlockedCompareExchange64(
                    reinterpret_cast<volatile __int64*>(&topNode_),
                    reinterpret_cast<__int64>(newTop),
                    reinterpret_cast<__int64>(localTop)))
            {
                break;
            }
        }

        if (isPlacementNew_)
        {
            new (&unmaskedTop->data_) DataType;
        }

        InterlockedIncrement(&useCount_);

        return &unmaskedTop->data_;
    }

    bool Free(DataType* data)
    {
        BlockNode* returnNode = reinterpret_cast<BlockNode*>(data);

        if (isPlacementNew_)
        {
            returnNode->data_.~DataType();
        }

        Push(returnNode);

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

private:
    BlockNode* AllocNewNode()
    {
        BlockNode* newNode = new BlockNode;

        return newNode;
    }

    __int64 MaskNewTag(__int64 localTop, __int64 newNode)
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

private:
    BlockNode* topNode_;
    bool isPlacementNew_;
    long capacity_;
    long useCount_;
    volatile __int64 tagIndex_;
};

#endif