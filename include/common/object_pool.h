
#ifndef _OBJECT_POOL_H_
#define _OBJECT_POOL_H_

#include "preprocessor.h"
#include "noncopyable.h"
#include "smart_assert.h"
#include <new>
#include <typeinfo>
#include <type_traits>

namespace detail {

    template <class T>
    inline void Construct(T* p, const std::false_type*)
    {
        new(p) T();
    }

    template <class T>
    inline void Construct(T* p, const std::true_type* /* is_pod */)
    {
        *p = T();
    }

    template <class T>
    inline void Destory(T* p, const std::false_type*)
    {
        p->~T();
    }

    template <class T>
    inline void Destory(T*, const std::true_type* /* is_pod */)
    {
        // do nothing
    }

    template <class T>
    inline void Clear(T* p, const std::false_type*)
    {
        p->clear();
    }

    template <class T>
    inline void Clear(T* p, const std::true_type* /* is_pod */)
    {
        *p = T();
    }

    template <class T>
    void ClearObjs(T* objs, uint32_t num, const std::false_type*)
    {   
        const T* end = objs + num;
        for (T* p = objs; p != end; ++p)
        {   
            p->clear();
        }
    }

    template <class T>
    void ClearObjs(T* objs, uint32_t num, const std::true_type* /* is_pod */)
    {   
        bzero(objs, sizeof(T) * num);
    }
} // namespace detail

template <class T>
class ObjectPool : boost::noncopyable
{
public:
    struct BlockNode
    {
        BlockNode* mNext;
        T*         mObjs;
        uint32_t   mNum;
    };

    explicit ObjectPool(uint32_t num = 512u)
      : mHead(nullptr)
      , mPos(nullptr)
      , mCapacity(0u)
      , mNum(num)
    {
        SMART_ASSERT(num != std::numeric_limits<uint32_t>::max());
        mHead = mPos = reserve();
    }

    ~ObjectPool()
    {
        BlockNode* node = mHead;
        while (node != nullptr)
        {
            BlockNode* p = node;
            node = node->mNext;
            destory(p);
            free(p);
        }
    }

    bool empty() const
    {
        return mHead->mNum == 0u;
    }

    uint64_t capacity() const
    {
        return mCapacity;
    }

    uint64_t blocks() const
    {
        return mCapacity / mNum;
    }

    uint64_t memory_size() const
    {
        return mCapacity * sizeof(T) + sizeof(BlockNode) * blocks();
    }

    void clear()
    {
        const typename std::is_pod<T>::type* ptype = nullptr;
        BlockNode* node = mHead;
        while (node != nullptr)
        {
            detail::ClearObjs(node->mObjs, node->mNum, ptype);
            node->mNum = 0;
            node = node->mNext;
        }
        mPos = mHead;
    }

    void swap(ObjectPool& other)
    {
        std::swap(other.mHead, mHead);
        std::swap(other.mPos,  mPos);
        std::swap(other.mNum,  mNum);
        std::swap(other.mCapacity, mCapacity);
    }

    void shrink_to_fit()
    {
        BlockNode* prev = nullptr;
        BlockNode* node = mHead;
        while (node != nullptr)
        {
            if (node->mNum != 0u)
            {
                prev = node;
                node = node->mNext;
            }
            else
            {
                if (prev != nullptr)
                {
                    prev->mNext = nullptr;
                }
                break;
            }
        }

        if (node == mHead)
        {
            SMART_ASSERT(prev == nullptr)("head", mHead)("node", node);
            mHead = mPos = nullptr;
        }
        else
        {
            SMART_ASSERT(prev != nullptr)("prev", prev)("head", mHead)("node", node);
            mPos = prev;
        }

        while (node != nullptr)
        {
            SMART_ASSERT(node->mNum == 0u);
            BlockNode* p = node;
            node = p->mNext;
            destory(p);
            free(p);
            mCapacity -= mNum;
        }

        SMART_ASSERT(mCapacity == 0u || (mCapacity >= mNum && mCapacity % mNum == 0u))("mCapacity", mCapacity)("mNum", mNum);
    }

    T& new_object()
    {
        if (mPos == nullptr)
        {
            SMART_ASSERT(mHead == nullptr);
            BlockNode* node = reserve();
            mHead = mPos = node;
        }
        else if (mPos->mNum == mNum)
        {
            if (mPos->mNext == nullptr)
            {
                BlockNode* node = reserve();
                mPos->mNext = node;
                mPos = node;
            }
            else
            {
                mPos = mPos->mNext;
            }
        }

        SMART_ASSERT(mPos != nullptr && mPos->mNum <= mNum)("mPos->mNum", mPos->mNum)("mNum", mNum);
        return mPos->mObjs[mPos->mNum++];
    }

    BlockNode* data() { return mHead; }

private:
    void destory(BlockNode* node)
    {
        const typename std::is_pod<T>::type* ptype = nullptr;
        const T* end = node->mObjs + mNum;
        for (T* p = node->mObjs; p != end; ++p)
        {
            detail::Destory(p, ptype);
        }
    }

    void construct(BlockNode* node)
    {
        const typename std::is_pod<T>::type* ptype = nullptr;
        const T* end = node->mObjs + mNum;
        for (T* p = node->mObjs; p != end; ++p)
        {
            detail::Construct(p, ptype);
        }
    }

    BlockNode* reserve()
    {
        BlockNode* node = (BlockNode *)malloc(sizeof(BlockNode) + sizeof(T) * mNum);
        if (node == nullptr)
        {
            throw std::bad_alloc();
        }

        node->mNext = nullptr;
        node->mNum  = 0u;
        node->mObjs = reinterpret_cast<T *>(node + 1);
        construct(node);

        mCapacity += mNum;
        return node;
    }

    BlockNode* mHead;
    BlockNode* mPos;
    uint64_t   mCapacity;
    uint32_t   mNum;
};

#endif // _OBJECT_POOL_H_
