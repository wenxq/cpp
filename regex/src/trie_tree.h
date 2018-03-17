
#ifndef _TRIE_TREE_H_
#define _TRIE_TREE_H_

#include "common/string_algo.h"
#include "common/object_pool.h"
#include "defines.h"
#include <sstream>
#include <string>
#include <vector>

#ifdef _HAVE_CXX11_
#include <memory>
#else
#include <tr1/memory>
#endif

struct TrieResult
{
    Slice   mKey;
    TNID    mID;
    int32_t mOffset;
};

typedef TNID (*AddCallBack)(const Slice& keyword, const TNID& id, const TNID& endof, void* data);
typedef bool (*MatchCallBack)(const Slice& text, int32_t offset, TNID id, const Slice& keyword, void* data);
typedef std::vector<TrieResult> TrieResultSet;

class TrieTree : boost::noncopyable
{
public:
    class iterator
    {
    public:
        typedef std::pair<Slice, TNID> value_type;

        iterator(uintptr_t pos = 0);

        iterator& operator++();

        bool operator==(const iterator& other) { return mPos == other.mPos; }
        bool operator!=(const iterator& other) { return mPos != other.mPos; }

        const value_type& operator*()  const { return mValue;  }
        const value_type* operator->() const { return &mValue; }

    private:
        uintptr_t  mPos;
        value_type mValue;
    };

public:
    explicit TrieTree(uint32_t unit = 128, bool lowercase = false);
    ~TrieTree();

    bool     add(const Slice& key, TNID id, bool whole_word = true, AddCallBack endof = nullptr, void* data = nullptr);
    bool     compile() { return compile_f() && compile_m(); }
    bool     check() const { return check_sibling() && check_f(mRoot) && check_m(mRoot); }
    TNID     find_key(const Slice& key) const;
    TNID     find_subkey(const Slice& key, MatchCallBack match, void* data = nullptr) const;
    void     clear();

    TNID     find_first(const Slice& text, DecodeType decode) const;
    uint32_t find_all(const Slice& text, DecodeType decode, TrieResultSet& result) const;
    bool     search(const Slice& text, DecodeType decode, MatchCallBack match, void* data = nullptr) const;

    bool     empty() const { return mWords == 0u; }
    uint32_t nodes() const { return mNodes; }
    uint32_t words() const { return mWords; }

    int32_t  min_length() const { return mMinLength; }
    size_t   memory_size() const;

    std::string to_string() const;

    std::ostream& format(std::ostream& ss) const
    {
        return format(ss, mRoot, 0);
    }

    TrieTree::iterator begin() const
    {
        return TrieTree::iterator(mWord);
    }

    TrieTree::iterator end() const
    {
        return TrieTree::iterator();
    }

private:
    bool compile_f();
    bool compile_m();
    bool check_sibling() const;
    bool check_f(uintptr_t r) const;
    bool check_m(uintptr_t r) const;
    std::ostream& format(std::ostream& ss, uintptr_t parent, int depth) const;

    uintptr_t mStorage;
    uintptr_t mWStorage;
    uintptr_t mRoot;
    uintptr_t mWord;
    uint32_t  mNodes;
    uint32_t  mWords;
    int32_t   mMinLength;
    int32_t   mLowercase;
};

#ifdef _HAVE_CXX11_
typedef std::shared_ptr<TrieTree> TrieTreePtr;
#else
typedef std::tr1::shared_ptr<TrieTree> TrieTreePtr;
#endif

inline std::string to_string(const TrieTree& trie)
{
    return trie.to_string();
}

inline std::ostream& operator<<(std::ostream& ss, const TrieTree& trie)
{
    return trie.format(ss);
}

inline std::ostream& operator<<(std::ostream& ss, const TrieTree* trie)
{
    return trie->format(ss);
}

#endif // _TRIE_TREE_H_
