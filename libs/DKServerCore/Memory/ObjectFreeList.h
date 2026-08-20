#pragma once
#include "Windows.h"
#include <new>
#include "CoreDefines.h"

#if DK_ENABLE_MEMORY_CHECK

extern unsigned int GlobalChecksum;

template <class DataType>
class ObjectFreeList
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
    ObjectFreeList(int blockNum, bool placementNew = false)
        : freeNode_(nullptr),
        checksum_(GlobalChecksum++),
        checksumPosition_(0),
        isPlacementNew_(placementNew),
        capacity_(0),
        useCount_(0)
    {
        BlockNode* findOffset = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

        checksumPosition_ =
            reinterpret_cast<__int64>(&findOffset->data_) -
            reinterpret_cast<__int64>(&findOffset->underflowChecksum_);

        free(findOffset);

        for (int i = 0; i < blockNum; ++i)
        {
            BlockNode* tempNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

            if (!isPlacementNew_)
            {
                new (&tempNode->data_) DataType;
            }

            tempNode->overflowChecksum_ = checksum_;
            tempNode->underflowChecksum_ = checksum_;

            Push(tempNode);
            ++capacity_;
        }
    }

    virtual ~ObjectFreeList()
    {
        while (freeNode_ != nullptr)
        {
            BlockNode* deleteNode = Pop();

            if (!isPlacementNew_)
            {
                deleteNode->data_.~DataType();
            }

            free(deleteNode);
        }
    }

    void Push(BlockNode* node)
    {
        BlockNode* tempNode = freeNode_;
        freeNode_ = node;
        freeNode_->next_ = tempNode;
    }

    BlockNode* Pop()
    {
        BlockNode* tempNode = freeNode_;
        freeNode_ = freeNode_->next_;

        return tempNode;
    }

    DataType* Alloc()
    {
        BlockNode* newNode = nullptr;

        if (useCount_ == capacity_)
        {
            newNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

            if (newNode == nullptr)
            {
                throw std::bad_alloc();
            }

            if (!isPlacementNew_)
            {
                new (&newNode->data_) DataType;
            }

            newNode->overflowChecksum_ = checksum_;
            newNode->underflowChecksum_ = checksum_;

            ++capacity_;
        }
        else
        {
            newNode = Pop();
        }

        if (isPlacementNew_)
        {
            new (&newNode->data_) DataType;
        }

        ++useCount_;

        return &newNode->data_;
    }

    bool Free(DataType* data)
    {
        char* movePointer = reinterpret_cast<char*>(data);
        movePointer -= checksumPosition_;

        BlockNode* tempNode = reinterpret_cast<BlockNode*>(movePointer);

        if (tempNode->overflowChecksum_ != checksum_)
        {
            DebugBreak();
        }

        if (tempNode->underflowChecksum_ != checksum_)
        {
            DebugBreak();
        }

        if (isPlacementNew_)
        {
            tempNode->data_.~DataType();
        }

        Push(tempNode);
        --useCount_;

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
    BlockNode* freeNode_;
    unsigned int checksum_;
    __int64 checksumPosition_;
    bool isPlacementNew_;
    int capacity_;
    int useCount_;
};

#else


template <class DataType>
class ObjectFreeList
{
public:
	struct BlockNode
	{
		DataType data_;
		BlockNode* next_;
	};

public:
	ObjectFreeList(int blockNum, bool placementNew = false)
		: freeNode_(nullptr),
		isPlacementNew_(placementNew),
		capacity_(0),
		useCount_(0)
	{
		for (int i = 0; i < blockNum; ++i)
		{
			BlockNode* temp = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

			if (!isPlacementNew_)
			{
				new (&temp->data_) DataType;
			}

			Push(temp);
			++capacity_;
		}
	}

	virtual ~ObjectFreeList()
	{
        while (freeNode_ != nullptr)
        {
            BlockNode* deleteNode = Pop();

            if (!isPlacementNew_)
            {
                deleteNode->data_.~DataType();
            }

            free(deleteNode);
        }
	}

	void Push(BlockNode* node)
	{
		BlockNode* temp = freeNode_;
		freeNode_ = node;
		freeNode_->next_ = temp;
	}

	BlockNode* Pop()
	{
		BlockNode* temp = freeNode_;
		freeNode_ = freeNode_->next_;

		return temp;
	}

	DataType* Alloc()
	{
		BlockNode* newNode = nullptr;

        if (useCount_ == capacity_)
        {
            newNode = static_cast<BlockNode*>(malloc(sizeof(BlockNode)));

            if (newNode == nullptr)
            {
                throw std::bad_alloc();
            }

            if (!isPlacementNew_)
            {
                new (&newNode->data_) DataType;
            }

            ++capacity_;
        }
        else
        {
            newNode = Pop();
        }

		if (isPlacementNew_)
		{
			new (&newNode->data_) DataType;
		}

		++useCount_;

		return &newNode->data_;
	}

	bool Free(DataType* data)
	{
		BlockNode* newNode = reinterpret_cast<BlockNode*>(data);

		if (isPlacementNew_)
		{
			newNode->data_.~DataType();
		}

		Push(newNode);

		--useCount_;

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
	BlockNode* freeNode_;
	bool isPlacementNew_;
	int capacity_;
	int useCount_;
};

#endif

