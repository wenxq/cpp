
#include "common/xstring.h"
#include "defines.h"
#include "qmatch.h"
#include "regex_object.h"
#include "trie_tree.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <unordered_set>

#define MAX_BRANCH_CNT 100000

#ifndef NO_TR1
    using std::tr1::unordered_set;
#else
    using std::unordered_set;
#endif // NO_TR1

typedef XString<1024> CString;

namespace qmatch
{
    template <class StringType>
    struct hash
    {   
        size_t operator()(const StringType& str) const
        {   
            size_t key = 0;
            const typename StringType::const_iterator last = str.cend();
            for (typename StringType::const_iterator xpos = str.cbegin(); xpos != last; ++xpos)
            {   
                key = key * 31ul + static_cast<uint8_t>(*xpos);
            }
            return key;
        }
    };

    typedef unordered_set<Slice, hash<Slice>, lowcase_equal_to> KeyWordSet;

    ObjectPool<MatchPiece>        mp_pool(512u);
    ObjectPool<MatchFrame>        mf_pool(1024u);
    ObjectPool<MatchFramePtrList> mfptr_pool(256u);
    ObjectPool<CString>           string_pool(8u);
    std::vector<TrieTreePtr>      trie_list;
    KeyWordSet                    keywords;
    CString*                      last_string = nullptr;
    bool                          expansion = true;

    inline MatchPiecePtr new_matchpiece()
    {
        return &(mp_pool.new_object());
    }

    inline MatchFramePtr new_matchframe()
    {
        return &(mf_pool.new_object());
    }

    inline MatchFramePtrList* new_matchframe_list()
    {
        return &(mfptr_pool.new_object());
    }

    inline CString* new_cstring()
    {
        return &(string_pool.new_object());
    }

    inline Slice get_piece(const Slice& piece)
    {
        BOOST_AUTO(iter, keywords.find(piece));
        if (iter == keywords.end())
        {
            Slice str = qmatch::push_cstring(piece);
            keywords.insert(str);
            return str;
        }
        return *iter;
    }

    void set_expansion(bool exs)
    {
        expansion = exs;
    }

    void clear()
    {
        mp_pool.clear();
        mf_pool.clear();
        mfptr_pool.clear();

        last_string = nullptr;
        string_pool.clear();
        trie_list.clear();
        expansion = true;
    }

    Slice test_expr_impl(RegexObject& expr, const Slice& context, BranchList* branch_list = nullptr);
    bool  add_matchpiece(const size_t& index, const SliceList& results, void* data);
    TNID  add_callback(const Slice& keyword, const TNID& id, const TNID& endof, void* data);
} // namespace qmatch

std::ostream& operator<<(std::ostream& ss, const qmatch::MatchPiece& m)
{
    ss << "(mData: " << (uintptr_t)(m.mData);
    ss << ", mSize: "    << m.mSize
       << ", mCur: "     << m.mCur
       << ", mVersion: " << m.mVersion
       << ')';
    return ss;
}

std::ostream& operator<<(std::ostream& ss, const qmatch::MatchFrame& q)
{
    ss << "(mKey: "    << (q.mKey)
       << ", mMatch: " << *(q.mMatch)
       << ", mPos: "   << (q.mPos)
       << ", mUserData: " << (q.mUserData)
       << ')';
    return ss;
}

std::ostream& qmatch::format(std::ostream& ss, const Slice& str)
{
    return regex::format(ss, str);
}

void qmatch::clear_tmpbuf()
{
    regex::clear();
    regex::shrink_to_fit();
}

void qmatch::resize(const size_t& size)
{
    if (size > qmatch::trie_list.size())
    {
        size_t count = size - qmatch::trie_list.size();
        while (count-- != 0)
        {
            qmatch::trie_list.push_back(TrieTreePtr(new TrieTree(1024ul)));
        }
    }
}

Slice qmatch::test_expr_impl(RegexObject& expr, const Slice& context, qmatch::BranchList* branch_list)
{
    regex::clear();
    expr.reset(context, qmatch::expansion);
    LOG_APPEND3("#pattern: \"", expr.context(), "\"\n#compiling:\n");
    if (!expr.compile())
    {
        std::cerr << "compile1 failed: \"" << context << "\"" << std::endl;
        return Slice("none");
    }

    expr.merge();
    LOG_APPEND3("#result(merge1):\n", expr, "\n");

    int64_t cnt = expr.get_branch_cnt();
    if (cnt > MAX_BRANCH_CNT && qmatch::expansion)
    {
        expr.reset(context, false);
        if (!expr.compile())
        {
            std::cerr << "compile2 failed: \"" << context << "\"" << std::endl;
            return Slice("none");
        }
        expr.merge();
        LOG_APPEND3("#result(merge2):\n", expr, "\n");
        cnt = expr.get_branch_cnt();
    }

    if (cnt > MAX_BRANCH_CNT)
    {
        SliceList results;
        expr.get_all(results);
        for (BOOST_AUTO(iter, results.begin()); iter != results.end(); ++iter)
        {
            if (iter->length() <= 1)
            {
                LOG_APPEND2("expr.smart, not all branch\'s keystr length greater 1: ", context);
                return Slice("none");
            }
            
            if (branch_list)
            {
                SliceList l;
                l.push_back(*iter);
                branch_list->insert(l);
            }
        }
        return Slice("smart");
    }

    typedef RegexObject::SimpleStr   SimpleStr;
    typedef std::pair<size_t, Slice> Position;
    typedef std::vector<Position>    PositionList;

    RegexObject::NodeLists or_list;
    or_list.reserve(integer_cast<size_t>(cnt));
    expr.get_branchs(or_list);
    for (BOOST_AUTO(branch, or_list.begin()); branch != or_list.end(); ++branch)
    {
        if (branch->empty())
        {
            return Slice("none");
        }

        SliceList keywords;

        const size_t null_pos = static_cast<size_t>(-1);
        Position     prev(null_pos, Slice());
        SimpleStr*   normal = nullptr;
        size_t       pos = 0;
        for (BOOST_AUTO(elem, branch->begin()); elem != branch->end(); ++elem)
        {
            Slice text = (*elem)->context();
            if (!(*elem)->enable() || text.empty())
            {
                ++pos;
                continue;
            }

            if (prev.first == null_pos)
            {
                prev = Position(pos++, text);
                continue;
            }

            if (prev.first + 1 == pos)
            {
                if (!normal)
                {
                    normal = regex::new_simplestr();
                }
                normal->append(prev.second);
            }
            else if (normal)
            {
                normal->append(prev.second);
                if (normal->length() > 1)
                    keywords.push_back(*normal);
                normal = nullptr;
            }
            else if (prev.second.length() > 1)
            {
                keywords.push_back(prev.second);
            }

            prev = Position(pos++, text);
        }

        if (prev.first != null_pos)
        {
            if (normal)
            {
                normal->append(prev.second);
                if (normal->length() > 1)
                    keywords.push_back(*normal);
                normal = nullptr;
            }
            else if (prev.second.length() > 1)
            {
                keywords.push_back(prev.second);
            }
        }

        if (keywords.empty())
        {
            LOG_APPEND2("expr.smart, can not find keystr in regex: ", context);
            return Slice("none");
        }

        if (branch_list != nullptr)
        {
            branch_list->insert(keywords).second;
        }
    }

    return Slice("smart");
}

bool qmatch::test_expr(const Slice& context)
{
    RegexObject expr;
    LOG_APPEND3("# (test) pattern: \"", context, "\"\n");
    return (qmatch::test_expr_impl(expr, context) != "none");
}

bool qmatch::add_expr(const size_t& index, const Slice& context, void* data)
{
    SMART_ASSERT(index < qmatch::trie_list.size())("index", index)("size", qmatch::trie_list.size());

    LOG_APPEND3("# *** pattern: \"", context, "\"\n");
    RegexObject expr;
    qmatch::BranchList branchs;
    Slice action = qmatch::test_expr_impl(expr, context, &branchs);
    if (action == "none")
    {
        std::cerr << "compile failed: \"" << context << "\"" << std::endl;
        return false;
    }

    SMART_ASSERT(!branchs.empty() && (action == "and" || action == "or" || action == "smart"));
    LOG_APPEND3("# *** RESULTS(", branchs.size(), ") = [\n");
    for (BOOST_AUTO(iter, branchs.begin()); iter != branchs.end(); ++iter)
    {
        SMART_ASSERT(!iter->empty())("expr", context);
        LOG_APPEND3("    ", *iter, ",\n");
    }
    LOG_APPEND("]\n");

    for (BOOST_AUTO(iter, branchs.begin()); iter != branchs.end(); ++iter)
    {
        bool result = qmatch::add_matchpiece(index, *iter, data);
        SMART_ASSERT(result)("expr", context)("results", *iter);
    }

    return true;
}

bool qmatch::add_keyword(const size_t& index, const Slice& context, void* data)
{
    SMART_ASSERT(index < qmatch::trie_list.size())("index", index)("size", qmatch::trie_list.size());

    SliceList results;
    results.push_back(context);
    bool result = qmatch::add_matchpiece(index, results, data);
    SMART_ASSERT(result)("expr", context)("results", results);
    return true;
}

bool qmatch::add_matchpiece(const size_t& index, const SliceList& results, void* data)
{
    if (results.size() >= integer_cast<size_t>(std::numeric_limits<int32_t>::max()))
        return false;

    qmatch::MatchPiecePtr mpl = qmatch::new_matchpiece();
    mpl->reset(integer_cast<uint32_t>(results.size()), data);

    std::vector<qmatch::MatchFramePtr> framelist;
    for (size_t i = 0; i < results.size(); ++i)
    {
        framelist.push_back(qmatch::new_matchframe());
    }

    for (BOOST_AUTO(row, results.rbegin()); row != results.rend(); ++row)
    {
        const int32_t pos = integer_cast<int32_t>(row.base() - 1 - results.begin());
        SMART_ASSERT((row == results.rbegin())
                       ? (integer_cast<size_t>(pos) == results.size() - 1)
                       : (pos >= 0 && integer_cast<size_t>(pos) < framelist.size()))
                    ("pos", pos);

        const qmatch::MatchFramePtr prev = ((pos == 0) ? nullptr : framelist[pos - 1]);

        qmatch::MatchFramePtr frame = framelist[pos];
        frame->reset(qmatch::get_piece(*row), mpl, prev, pos);

        const TNID id = qmatch::trie_list[index]->words();
        const bool result = qmatch::trie_list[index]->add(frame->mKey, id, false, qmatch::add_callback, frame);
        SMART_ASSERT(result)("key", frame->mKey);
    }

    return true;
}

TNID qmatch::add_callback(const Slice& keyword, const TNID& id, const TNID& endof, void* data)
{
    SMART_ASSERT(data != nullptr).msg("invaild state pointer");

    qmatch::MatchFramePtr frame = static_cast<qmatch::MatchFramePtr>(data);

    qmatch::MatchFramePtrList* list = nullptr;

    if (endof == TK_INVAILD)
    {
        list = qmatch::new_matchframe_list();
        SMART_ASSERT((TNID)(list) != TK_INVAILD).msg("oh, my god, tell me why ...");
        SMART_ASSERT(frame->mMatch->mSize > 0ul)("count", frame->mMatch->mSize);
        list->reserve(frame->mMatch->mSize);
    }
    else
    {
        list = (qmatch::MatchFramePtrList *)(endof);
    }

    if (list->size() >= integer_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw std::bad_alloc();

    list->push_back(frame);
    return (TNID)(list);
}

void qmatch::list_endof(const size_t& index, ListCallBack list_it)
{
    SMART_ASSERT(index < qmatch::trie_list.size())("index", index)("size", qmatch::trie_list.size());

    for (BOOST_AUTO(iter, qmatch::trie_list[index]->begin()); iter != qmatch::trie_list[index]->end(); ++iter)
    {
        list_it(index, iter->first, iter->second);
    }
}

bool qmatch::compile(const size_t& index)
{
    SMART_ASSERT(index < qmatch::trie_list.size())("index", index)("size", qmatch::trie_list.size());
    bool result = qmatch::trie_list[index]->compile();
    SMART_ASSERT(result && qmatch::trie_list[index]->check());
    return result;
}

bool qmatch::search(const size_t& index, const Slice& context, DecodeType decode, MatchCallBack match, void* data)
{
    SMART_ASSERT(index < qmatch::trie_list.size())("index", index)("size", qmatch::trie_list.size());
    return !qmatch::trie_list[index]->empty() && qmatch::trie_list[index]->search(context, decode, match, data);
}

void qmatch::reset_version(const uint64_t& version)
{
    LOG_APPEND3("qmatch reset_version: ", qmatch::mp_pool.capacity(), '\n');

    BOOST_AUTO(node, qmatch::mp_pool.data());
    
    while (node != nullptr)
    {
        for (uint32_t i = 0; i < node->mNum; ++i)
        {
            (node->mObjs[i]).reset(version);
        }

        node = node->mNext;
    }
}

#define strbuf_remain(str) ((str)->capacity() - (str)->size())

Slice qmatch::push_cstring(const Slice& piece)
{
    CString* str = qmatch::last_string;

    const char* beg = nullptr;
    if ((str == nullptr) || (strbuf_remain(str) < piece.size()))
    {
        str = qmatch::new_cstring();
        str->reserve(piece.size() + 1);
        if ((qmatch::last_string == nullptr)
            || (strbuf_remain(qmatch::last_string) < str->capacity() - piece.size()))
        {
            qmatch::last_string = str;
        }

        beg = str->begin();
    }
    else
    {
        beg = str->end();
    }

    str->append(piece);
    return make_slice(beg, str->end());
}
