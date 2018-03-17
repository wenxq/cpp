
#include "test_config.h"
#include "trie_tree.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef IS_UNIX
#include <unistd.h>
#endif // IS_UNIX

int loadword(std::vector<std::string>& words, const char* filename)
{
    FILE *fd = fopen(filename, "r");
    if (NULL == fd)
    {
        printf("fopen fail\n");
        return 1;
    }

    char buf[1024] = {0};
    while (fgets(buf, sizeof(buf)-1, fd) != NULL)
    {
        buf[strlen(buf)-1] = 0;

        words.push_back(buf);
        memset(buf, 0, sizeof(buf));
    }

    fclose(fd);
    return 0;
}

int loadtxt(std::string &txt, const char* filename)
{
    FILE *fd = fopen(filename, "r");
    if (NULL == fd)
    {
        printf("fopen fail\n");
        return 1;
    }

    char buf[1024] = {0};
    while (fgets(buf, sizeof(buf)-1, fd) != NULL)
    {
        txt.append(buf);
        memset(buf, 0, sizeof(buf));
    }

    fclose(fd);
    return 0;
}

typedef std::map<int32_t, Slice> CMatch;
typedef std::map<size_t, CMatch>       CResult; // id -> match pair

struct CData
{
    const CResult* mResult;
    size_t         mCount;

    CData()
      : mResult(nullptr)
      , mCount(0)
    {}
};

bool match_func(const Slice& text, int32_t offset, TNID id, const Slice& keyword, void* data)
{
    CData* ptr = (CData *)data;
    ptr->mCount++;

    BOOST_AUTO(match, ptr->mResult->find(id));
    ASSERT_EQ(true, match != ptr->mResult->end());

    BOOST_AUTO(ta, match->second.find(offset));
    ASSERT_EQ(true, ta != match->second.end());
    ASSERT_EQ(ta->second, keyword);

    ASSERT_EQ(true, text.substr(offset).istarts_with(keyword));
    std::cout << "[" << ptr->mCount << "] " << keyword << "[" << id << "] -> @" << offset << std::endl;
    return false;
}

bool match_break_func(const Slice& text, int32_t offset, TNID id, const Slice& keyword, void* data)
{
    CData* ptr = (CData *)data;
    ptr->mCount++;
    BOOST_AUTO(match, ptr->mResult->find(id));
    ASSERT_EQ(true, match != ptr->mResult->end());

    BOOST_AUTO(ta, match->second.find(offset));
    ASSERT_EQ(true, ta != match->second.end());
    ASSERT_EQ(ta->second, keyword);

    ASSERT_EQ(true, text.substr(offset).istarts_with(keyword));
    //std::cout << "[" << (*ptr)++ << "] " << keyword << "[" << id << "] -> @" << offset << std::endl;
    return true;
}

void get_results(const std::string& txt, const std::vector<std::string>& words, CResult& expect, size_t& count)
{
    CString text(make_slice(txt));
    text.tolower();

    for (size_t i = 0; i < words.size(); ++i)
    {
        Slice s = make_slice(words[i]);
        s.trim_right();

        CString w(s);
        w.tolower();

        int cnt = 0;
        int j = 0;
        do
        {
            int pos = make_slice(text).find_first(w, j);
            if (pos == Slice::NPOS)
                continue;

            if (j == 0)
            {
                std::cout << s << "(" << i << "): " << std::flush;
            }
            std::cout << "@" << pos << ", " << std::flush;
            expect[i][pos] = s;

            ++cnt;
            j = pos;
        } while (++j < text.length());

        if (cnt != 0)
        {
            std::cout << " -> {" << cnt << "}" << std::endl;
        }
    }

    for (BOOST_AUTO(iter, expect.begin()); iter != expect.end(); ++iter)
    {
        count += iter->second.size();
    }
}

int main(int argc, char* argv[])
{
    TrieTree trie;
    trie.add("_SS_DATA_", 0);
    trie.add("_LI_DATA_", 1);
    std::cout << trie << std::endl;
    trie.clear();

    std::string txt;
    std::vector<std::string> words;

    if (argc >= 2)
    {
        loadword(words, argv[1]);
    }

    if (argc >= 3)
    {
        loadtxt(txt, argv[2]);
    }

    CResult expect;
    size_t count = 0;
    get_results(txt, words, expect, count);

    std::cout << expect << " -> {" << count << "}" << std::endl;

    std::map<size_t, Slice> targets;
    for (size_t i = 0; i < words.size(); i++)
    {
        Slice s = make_slice(words[i]);
        s.trim_right();
        targets.insert(std::make_pair(i, s));
        ASSERT_EQ(true, trie.add(s, i, false));
    }
    trie.compile();
    ASSERT_EQ(true, trie.check());
    std::cout << "memory_size: " << trie.memory_size() << ", nodes: " << trie.nodes() << std::endl;

    for (TrieTree::iterator iter = trie.begin(); iter != trie.end(); ++iter)
    {
        BOOST_AUTO(ta, targets.find(iter->second));
        ASSERT_EQ(ta->second, iter->first);
    }

    if (!txt.empty())
    {
        Slice content = make_slice(txt);
        content.trim();

        std::vector<TrieResult> result;
        trie.find_all(content, DecodeType::kNone, result);
        ASSERT_EQ(false, result.empty());
        //std::cout << "\ntrie find: " << result.size() << ", " << txt.substr(0, 20) << std::endl;

        ASSERT_EQ(count, result.size());
        for (BOOST_AUTO(r, result.begin()); r != result.end(); ++r)
        {
            BOOST_AUTO(m, expect.find(r->mID));
            ASSERT_EQ(true, (m != expect.end()));

            BOOST_AUTO(ta, m->second.find(r->mOffset));
            ASSERT_EQ(true, (ta != m->second.end()));
            ASSERT_EQ(ta->second, r->mKey);

            std::string piece = txt.substr(r->mOffset);
            ASSERT_EQ(true, make_slice(piece).istarts_with(r->mKey));
        }

        CData data;

        data.mResult = &expect;
        data.mCount = 0;
        bool ret = trie.search(content, DecodeType::kNone, match_func, &data);
        ASSERT_EQ(false, ret);
        ASSERT_EQ(count, data.mCount);

        data.mCount = 0;
        ret = trie.search(content, DecodeType::kNone, match_break_func, &data);
        ASSERT_EQ(true, ret);
        ASSERT_EQ(1u, data.mCount);
    }

    return report_errors();
}
