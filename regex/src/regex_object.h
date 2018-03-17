
#ifndef _REGEX_OBJECT_H_
#define _REGEX_OBJECT_H_

#include "common/string_algo.h"
#include "common/xstring.h"
#include "common/object_pool.h"
#include <set>
#include <string>
#include <vector>

// Reference: 
// (1). https://zh.wikipedia.org/wiki/%E6%AD%A3%E5%88%99%E8%A1%A8%E8%BE%BE%E5%BC%8F
// (2). http://www.pcre.org/current/doc/html/pcre2syntax.html#SEC8
class RegexObject
{
public:
    typedef XString<8> SimpleStr;
    typedef std::vector<SimpleStr>  SimpleStrList;
    typedef RegexObject* RegexObjectPtr;
    typedef RegexObject* NodePtr;
    typedef std::vector<Slice> SliceList;
    typedef std::vector<NodePtr>    NodeList;
    typedef std::vector<NodeList>   NodeLists;

    RegexObject()
      : mDisable(false)
      , mAndDisable(false)
      , mTarget(false)
      , mSplice(false)
      , mExpansion(true)
    {}

    RegexObject(const Slice& context)
      : mContext(context)
      , mDisable(false)
      , mAndDisable(false)
      , mTarget(false)
      , mSplice(false)
      , mExpansion(true)
    {}

    void clear();
    void reset(const Slice& context, bool expansion = true, bool splice = false);
    void swap(RegexObject& other);
    bool compile();
    bool adjust() { return (adjust_impl() != nullptr); }
    bool merge();
    bool unfold(bool all = true);
    void get_all(SliceList& results);
    void get_and(SliceList& results);
    void get_and(NodeList& results);
    void get_branchs(NodeLists& results);

public:
    std::string to_string() const;
    std::ostream& format(std::ostream& ss) const { return format(ss, "and", 0); }

    bool enable()    const { return !mDisable; }
    bool is_target() const { return mTarget;   }
    bool is_leaf()   const { return (mTarget || !mOr.empty()); }
    const Slice& text() const { return mText; }
    const Slice& context()  const { return mContext; }
    size_t number_of_and() const { return mAnd.size(); }
    size_t number_of_or()  const { return mOr.size();  }
    const NodeList& list_of_and() const { return mAnd; }
    const NodeList& list_of_or()  const { return mOr;  }
    int64_t get_branch_cnt();

private:
    std::ostream& format(std::ostream& ss, const Slice& logic, uint32_t depth) const;
    bool split(SliceList& store);
    const void* find_definition (const Slice& definition, bool* utf = nullptr);
    const void* find_subpattern (const Slice& subpattern);
    const void* find_quantifier (const Slice& quantifier, bool& disable);
    const void* new_startpattern(const Slice& subpattern, SimpleStr* &normal, const uint8_t* &mark);
    const void* new_special     (const Slice& backslash,  SimpleStr* &normal, const uint8_t* &mark, const Slice& key);
    const void* new_backslash   (const Slice& backslash,  SimpleStr* &normal, const uint8_t* &mark);
    const void* new_subpattern  (const Slice& subpattern, SimpleStr* &normal, const uint8_t* &mark);
    const void* new_definition  (const Slice& definition, SimpleStr* &normal, const uint8_t* &mark);
    const void* new_literal     (const Slice& current, SimpleStr* &normal, const uint8_t* &mark);

    Slice and_append(const Slice& view, const Slice& context, bool target, bool disable = false);
    Slice  or_append(const Slice& view, const Slice& context, bool target, bool disable = false);
    Slice and_append(RegexObject* node, bool target, bool disable = false);
    Slice  or_append(RegexObject* node, bool target, bool disable = false);

    NodePtr clone();
    NodePtr adjust_impl();
    NodePtr merge_impl();
    bool    mark_impl();
    bool    unfold_impl(NodeList& branchs, bool all);
    void    get_all_impl(SliceList& results);
    void    get_and_impl(SliceList& results);
    void    get_and_impl(NodeList& results);
    void    get_branchs_impl(NodeLists& results);
    void    find_fold_nodes(NodeList& nodes);
    void    add_branch(NodeList& branchs, NodePtr node);

private:
    typedef std::set<uint8_t> CharSet;

    void any_of_digit(CharSet& charset);
    void not_of_digit(CharSet& charset);
    void any_of_space(CharSet& charset);
    void not_of_space(CharSet& charset);
    void any_of_blank(CharSet& charset);
    void not_of_blank(CharSet& charset);
    void any_of_word (CharSet& charset);
    void not_of_word (CharSet& charset);
    void any_of_vertical(CharSet& charset);
    void not_of_vertical(CharSet& charset);

private:
    NodeList mAnd;
    NodeList mOr;
    Slice    mContext;
    Slice    mText;
    bool     mDisable;
    bool     mAndDisable;
    bool     mTarget;
    bool     mSplice;
    bool     mExpansion;
};

typedef RegexObject RegexObject;

inline std::string to_string(const RegexObject& expr)
{
    return expr.to_string();
}

inline std::ostream& operator<<(std::ostream& ss, const RegexObject& expr)
{
    ss << '\n';
    return expr.format(ss);
}

inline std::ostream& operator<<(std::ostream& ss, const RegexObject* expr)
{
    ss << '\n';
    return expr->format(ss);
}

namespace regex
{
    RegexObject::SimpleStr* new_simplestr();
    void shrink_to_fit();
    void clear();
    std::ostream& format(std::ostream& ss, const Slice& str);
} // namespace regex

#endif // _REGEX_OBJECT_H_
