
#ifndef _REGEX_H_
#define _REGEX_H_

#include "common/string_algo.h"
#include "common/noncopyable.h"
#include <vector>

#ifdef _HAVE_CXX11_
#include <memory>
namespace detail { using std::shared_ptr; }
#else
#include <tr1/memory>
namespace detail { using std::tr1::shared_ptr; }
#endif

class MatchBlocks
{
public:
    typedef size_t        value_type;
    typedef size_t*       pointer;
    typedef const size_t* const_pointer;

    explicit MatchBlocks(void* ptr = nullptr);

    operator  bool() const { return mBlocks.get() != nullptr; }
    uint32_t  size() const;
    pointer   data() const;
    void*     get()  const;

    MatchBlocks clone() const;

private:
    typedef detail::shared_ptr<uintptr_t> match_blocks_ptr;

    match_blocks_ptr mBlocks;
};

class Regex : boost::noncopyable
{
public:
    typedef detail::shared_ptr<uintptr_t> regex_code_ptr;
    typedef detail::shared_ptr<uintptr_t> match_context_ptr;

    static match_context_ptr ContextBlocks;

public:
    Regex()
      : mJit(false)
    {}

    operator bool() const { return mRegex.get() != nullptr; }

    Slice expr() const { return mExpr; }

    MatchBlocks new_match_blocks() const;

    bool is_jit() const { return mJit; }
    int  compile(const Slice& expr);
    bool match(const Slice& subject, const MatchBlocks& match, bool anchored = true) const;
    bool search(const Slice& subject, Slice& result, const MatchBlocks& match) const;
    int  find_all(const Slice& subject, std::vector<Slice>& results, const MatchBlocks& match) const;

private:
    regex_code_ptr mRegex;
    Slice          mExpr;
    bool           mJit;
};

typedef detail::shared_ptr<Regex> RegexPtr;

#endif // _REGEX_H_
