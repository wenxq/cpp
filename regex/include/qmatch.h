
#ifndef _QMATCH_H_
#define _QMATCH_H_

#include "common/object_pool.h"
#include "common/slice.h"
#include "common/noncopyable.h"
#include "defines.h"

#include <limits>
#include <set>
#include <vector>

typedef bool(*QMatchCallBack)(const Slice& text, int32_t offset, TNID id, const Slice& keyword, void* data);

namespace qmatch
{
    struct MatchPiece : boost::noncopyable
    {
        void*    mData;
        uint64_t mVersion;
        uint32_t mSize;
        int32_t  mCur;
        bool     mDiscard;

        MatchPiece()
          : mData(nullptr)
          , mVersion(0)
          , mSize(0)
          , mCur(-1)
          , mDiscard(false)
        {}

        void*    data()    const { return mData;    }
        uint64_t version() const { return mVersion; }

        bool is_match_all() const
        {
            SMART_ASSERT(mSize > 0 && mCur >= 0)("mSize", mSize)("mCur", mCur);
            return integer_cast<uint32_t>(mCur + 1) == mSize;
        }

        bool is_next(const int32_t& pos) const
        {
            return !mDiscard && mCur + 1 == pos;
        }

        void forward()
        {
            ++mCur;
        }

        bool try_forward(const int32_t& pos)
        {
            if (is_next(pos))
            {
                forward();
                return true;
            }

            return false;
        }

        void clear()
        {
            mData    = nullptr;
            mVersion = 0;
            mSize    = 0;
            mCur     = -1;
            mDiscard = false;
        }

        bool discard() const { return mDiscard; }

        void set_discard()
        {
            mDiscard = true;
        }

        void reset(const uint64_t& version)
        {
            SMART_ASSERT(mData != nullptr)("mData", (uintptr_t)mData);
            SMART_ASSERT(mSize > 0 && mSize < integer_cast<uint32_t>(std::numeric_limits<int32_t>::max()))("mSize", mSize);

            mVersion = version;
            mCur     = -1;
            mDiscard = false;
        }

        void reset(const uint32_t& size, void* data)
        {
            SMART_ASSERT(size > 0 && size < integer_cast<uint32_t>(std::numeric_limits<int32_t>::max()))("size", size);
            SMART_ASSERT(mData == nullptr && mSize == 0 && mCur == -1 && mVersion == 0)
                        ("mData", (uintptr_t)mData)("mSize", mSize)("mCur", mCur)("mVersion", mVersion);

            mData    = data;
            mVersion = 0;
            mSize    = size;
            mCur     = -1;
            mDiscard = false;
        }
    };

    typedef MatchPiece* MatchPiecePtr;

    struct MatchFrame 
    {
        Slice       mKey;
        MatchPiecePtr     mMatch;
        const MatchFrame* mPrev;
        int32_t           mUserData[7];
        int32_t           mPos;

        MatchFrame()
          : mMatch(nullptr)
          , mPrev(nullptr)
          , mPos(-1)
        {
            bzero(mUserData, sizeof(mUserData));
        }

        bool     is_head() const { return mPos == 0;        }
        uint64_t version() const { return mMatch->mVersion; }
        void*    data()    const { return mMatch->mData;    }

        Slice key() const
        {
            return mKey;
        }

        int32_t pos() const
        {
            return mPos;
        }

        const MatchFrame* prev() const
        {
            return mPrev;
        }

        void clear()
        {
            mKey.clear();
            mMatch = nullptr;
            mPrev  = nullptr;
            mPos   = -1;
            bzero(mUserData, sizeof(mUserData));
        }

        void reset(const Slice& key, MatchPiece* match, const MatchFrame* prev, int32_t pos)
        {
            mKey   = key;
            mMatch = match;
            mPrev  = prev;
            mPos   = pos;
            bzero(mUserData, sizeof(mUserData));
        }

        void reset(const uint64_t& version)
        {
            mMatch->reset(version);
        }

        void set_userdata(size_t idx, int32_t offset)
        {
            SMART_ASSERT(idx < dimensionof(mUserData) && offset >= 0);
            mUserData[idx] = offset;
        }

        int32_t userdata(size_t idx) const
        {
            SMART_ASSERT(idx < dimensionof(mUserData));
            return mUserData[idx];
        }

        bool is_match_all() const
        {
            return mMatch->is_match_all();
        }

        bool is_next() const
        {
            return mMatch->is_next(mPos);
        }

        void forward()
        {
            mMatch->forward();
        }

        bool try_forward()
        {
            return mMatch->try_forward(mPos);
        }

        void set_discard()
        {
            mMatch->set_discard();
        }
    };

    typedef MatchFrame  MatchFrame;
    typedef MatchFrame* MatchFramePtr;
    typedef std::vector<MatchFramePtr> MatchFramePtrList;

    typedef void (*ListCallBack)(const size_t& index, const Slice& key, const TNID& endof);

    std::ostream& format(std::ostream& ss, const Slice& str);

    void clear();
    void clear_tmpbuf();
    Slice push_cstring(const Slice& str);
    void reset_version(const uint64_t& version);
    void set_expansion(bool expansion);
    void resize(const size_t& size);
    bool test_expr(const Slice& context);
    bool add_expr(const size_t& index, const Slice& context, void* data = nullptr);
    bool add_keyword(const size_t& index, const Slice& context, void* data = nullptr);
    bool compile(const size_t& index);
    void list_endof(const size_t& index, ListCallBack list);
    bool search(const size_t& index, const Slice& context, DecodeType decode, QMatchCallBack match, void* data = nullptr);
} // namespace qmatch

namespace qmatch
{
    struct lowcase_equal_to : std::binary_function<Slice, Slice, bool>
    { 
        bool operator()(const Slice& x, const Slice& y) const
        {
            return x.icompare(y) == 0;
        }
    };

    typedef std::vector<Slice> SliceList;
    struct BranchCompare
    {
        bool operator()(const SliceList& lhs, const SliceList& rhs) const
        {
            if (lhs.size() < rhs.size())
                return true;

            if (lhs.size() > rhs.size())
                return false;

            BOOST_AUTO(pair, std::mismatch(lhs.begin(), lhs.end(), rhs.begin(), lowcase_equal_to()));
            if (pair.first == lhs.end())
                return false;

            return *(pair.first) < *(pair.second);
        }
    };

    typedef std::set<SliceList, BranchCompare> BranchList;
} // namespace qmatch

std::ostream& operator<<(std::ostream& ss, const qmatch::MatchPiece& m);

inline std::ostream& operator<<(std::ostream& ss, const qmatch::MatchPiece* m)
{
    return (ss << *m);
}

inline std::string to_string(const qmatch::MatchPiece& m)
{
    std::ostringstream ss;
    ss << m;
    return ss.str();
}

std::ostream& operator<<(std::ostream& ss, const qmatch::MatchFrame& q);

inline std::ostream& operator<<(std::ostream& ss, const qmatch::MatchFrame* q)
{
    return (ss << *q);
}

inline std::string to_string(const qmatch::MatchFrame& q)
{
    std::ostringstream ss;
    ss << q;
    return ss.str();
}

#endif // _QMATCH_H_
