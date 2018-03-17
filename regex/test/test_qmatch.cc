
#include "qmatch.h"
#include "regex_object.h"
#include <iostream>

struct MatchData
{
    uint64_t mVersion;
};

typedef MatchData* MatchDataPtr;

typedef RegexObject::SliceList SliceList;

namespace qmatch
{
    extern Slice test_expr_impl(RegexObject& expr, const Slice& context, BranchList* branch_list = nullptr);
} // endof namespace qmatch

bool match(const Slice& text, int32_t offset, TNID endof, const Slice& keyword, void* data)
{
    std::cout << "text: " << text
              << ", offset: " << offset
              << ", endof: " << endof
              << ", keyword: " << keyword
              << ", data: " << (uintptr_t)(data)
              << std::endl;

    SMART_ASSERT(data != nullptr).msg("invaild state pointer");
    SMART_ASSERT(endof != TK_INVAILD).msg("this is impossible");

    MatchDataPtr state = (MatchDataPtr)(data);
    qmatch::MatchFramePtrList* list = (qmatch::MatchFramePtrList *)(endof);

    std::cout << "list length: " << list->size() << std::endl;

    for (BOOST_AUTO(iter, list->begin()); iter != list->end(); ++iter)
    {
        qmatch::MatchFramePtr q = *iter;

        if (q->version() != state->mVersion)
        {
            if (!q->is_head()) continue;

            q->reset(state->mVersion);
        }

        if (q->try_forward() && q->is_match_all())
        {
            std::cout << "matched: {text: " << text
                      << ", offset: " << offset
                      << ", endof: " << endof
                      << ", matched"
                      << ", pos: " << q->mPos
                      << '}' << std::endl;
            q->set_discard();
            return true;
        }

        std::cout << *q << std::endl;
    }
    std::cout << "-----------------------------------" << std::endl;    
    return false;
}

int test(int id)
{
    MatchData data = { 0ul };

    qmatch::resize(3);

    std::cout << qmatch::test_expr("abc.*[a-z]{2,3} \\(.*\\)") << std::endl;

    qmatch::add_expr(1, "abc.*[a-z]{2,3} \\(.*\\)", (void *)1);
    qmatch::compile(1);

    qmatch::add_keyword(2, "abc", (void *)2);
    qmatch::compile(2);

    bool result = false;
    for (int i = 0; i < 3; ++i)
    {
        data.mVersion++;
        result = qmatch::search(1, "123456abc.*[a-z]{2,3} (.*)", DecodeType::kNone, match, &data);
        SMART_ASSERT(result);
        std::cout << "result1: " << result << std::endl;
    }

	Slice cookie = "5cbae404-211a-402f-a06d-09e099236e4f%2C%u7EFC%u5408%u529E%u4E13%u5458";
	data.mVersion++;
	result = qmatch::search(1, cookie, DecodeType::kUrlDecodeUni, match, &data);

    data.mVersion++;
    result = qmatch::search(1, "123456abc.*[a-z]{2,3} ", DecodeType::kNone, match, &data);
    SMART_ASSERT(!result);
    std::cout << "result2: " << result << std::endl;

    data.mVersion++;
    result = qmatch::search(2, "123456abc.*[a-z]{2,3} (.*)", DecodeType::kNone, match, &data);
    SMART_ASSERT(result);
    std::cout << "result3: " << result << std::endl;

    data.mVersion++;
    result = qmatch::search(2, "123456bc.*[a-z]{2,3} .*)", DecodeType::kNone, match, &data);
    SMART_ASSERT(!result);
    std::cout << "result4: " << result << std::endl;


    SliceList a;
    a.push_back("abc");
    a.push_back("123");

    SliceList b;
    b.push_back("123");
    b.push_back("abc");

    qmatch::BranchList list;

    list.insert(a);
    list.insert(b);
    list.insert(a);

    std::cout << list << std::endl;


    Slice context = "\\b(?:(?:create|drop)\\s+table|insert\\s+into)\\s+cmd\\b.*";

    std::cout << "# *** pattern: \"" << context << "\"\n";
    RegexObject expr;
    qmatch::BranchList branchs;
    Slice action = qmatch::test_expr_impl(expr, context, &branchs);
    if (action == "none")
    {
        std::cerr << "compile failed: \"" << context << "\"" << std::endl;
        return 1;
    }

    SMART_ASSERT(!branchs.empty() && (action == "and" || action == "or" || action == "smart"));
    std::cout << "# *** RESULTS(" << branchs.size() << ") = [\n";
    for (BOOST_AUTO(iter, branchs.begin()); iter != branchs.end(); ++iter)
    {
        SMART_ASSERT(!iter->empty())("expr", context);
        LOG_APPEND3("    ", *iter, ",\n");
    }
    LOG_APPEND("]\n");

    return 0;
}

int main(int argc, char* argv[])
{
    for (int i = 0; i < 50; ++i)
    {
        test(i);
        qmatch::clear_tmpbuf();
        qmatch::clear();
    }

    return 0;
}
