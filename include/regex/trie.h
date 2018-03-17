
#ifndef _TRIE_H_
#define _TRIE_H_

#include "common/string_algo.h"
#include "common/noncopyable.h"
#include <vector>

#ifdef _HAVE_CXX11_
#  include <memory>
#else
#  include <tr1/memory>
#endif

class Trie : boost::noncopyable
{
public:
	typedef uintptr_t               TUID;
	typedef std::pair<Slice, TUID>  value_type;
	typedef std::vector<value_type> value_set;

public:
    explicit Trie(uint32_t unit = 128, bool lowercase = false);
    ~Trie();

    bool   add(const Slice& key, TUID id, bool whole_word = true);
    bool   compile();
    bool   check() const;
    size_t wordset(value_set& store);
    TUID   find_key(const Slice& key) const;
    TUID   find_first(const Slice& text) const;
    void   clear();

    bool   empty() const;
    size_t words() const;
    size_t memory_size() const;

private:
    uintptr_t mData;
};

#ifdef _HAVE_CXX11_
typedef std::shared_ptr<Trie> TriePtr;
#else
typedef std::tr1::shared_ptr<Trie> TriePtr;
#endif

#endif // _TRIE_H_
