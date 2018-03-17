
#include "regex_object.h"
#include "qmatch.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

typedef pretty_print::delimiters_values<char> Values;
struct MyDelims{ static Values values; };
Values MyDelims::values = { "[\"", "\", \"", "\"]" };

template <class Container>
inline pretty_print::custom_delims<char, MyDelims> make_format(const Container& cont)
{
    if (cont.empty())
    {
        Values vs = { "[", ", ", "]" };
        MyDelims::values = vs;
    }
    else
    {
        Values vs = { "[\"", "\", \"", "\"]" };
        MyDelims::values = vs;
    }
    return pretty_print::custom_delims<char, MyDelims>(cont);
}

namespace qmatch
{
    Slice test_expr_impl(RegexObject& expr, const Slice& context, BranchList* branch_list);
} // namespace regex

void get_keywords(const Slice& regstr)
{
    std::cout << "#" << regstr << std::endl;
    RegexObject expr;
    qmatch::BranchList branchs;
    Slice result = qmatch::test_expr_impl(expr, regstr, &branchs);
    std::cout << "#result:" << std::endl;
    if (result == "smart")
    {
        std::cout << "[" << std::endl;
        for (BOOST_AUTO(iter, branchs.begin()); iter != branchs.end(); ++iter)
        {
            std::cout << "   " << *iter << std::endl;
        }
        std::cout << "]" << std::endl;
    }
    else
    {
        std::cout << "-- failed" << std::endl;
    }
    std::cout << "\n" << std::endl;
}

bool test_regexpr(const Slice& regstr)
{
    RegexObject expr;
    RegexObject::NodeList nodes;
    RegexObject::SliceList results;
    RegexObject::NodeLists branchs;
    regex::clear();
    expr.reset(regstr, false);
    bool result = expr.compile();
    std::cout << "#pattern: \"" << expr.context() << "\"\n#compiling:" << std::endl;
    std::cout << std::boolalpha << "#compile: " << result << std::endl;
    //std::cout << "#result:\n" << expr << std::endl;
    SMART_ASSERT(result);

    expr.adjust();
    std::cout << "#branchs: " << expr.get_branch_cnt() << std::endl;
    std::cout << "#result(adjust):\n" << expr << std::endl;

    expr.get_branchs(branchs);
    std::cout << "#result(branchs):\n" << branchs.size() << std::endl;
    RegexObject::SimpleStr str;
    for (BOOST_AUTO(iter, branchs.begin()); iter != branchs.end(); ++iter)
    {
        size_t pos = 0;
        std::cout << "[" << std::endl;
        for (BOOST_AUTO(elem, iter->begin()); elem != iter->end(); ++elem)
        {
            if ((*elem)->enable())
            {
                std::cout << "\t" << pos << " -> " << (*elem)->context() << std::endl;
                pos++;
            }
        }
        std::cout << "]" << std::endl;
    }

    /*results.clear();
    expr.get_and(results);
    std::cout << "#results(adjust and):\n" << make_format(results) << std::endl;

    expr.merge();
    std::cout << "#result(merge):\n" << expr << std::endl;

    std::cout << std::dec << "#nodes(merge or){" << "}:\n[" << std::endl;
    expr.get_and(nodes);
    std::cout << "  " << make_format(nodes) << std::endl;
    std::cout << "]" << std::endl;

    regex::clear();
    expr.reset(regstr);
    result = expr.compile() && expr.unfold(false);
    if (!result)
    {
        std::cout << "#result(unfold):" << std::endl;
        std::cerr << "[warning] branchs of expr: \"" << regstr << "\" greater than " << REGEX_MAX_NODES << ", or disabled" << std::endl;
    }
    else
    {
        std::cout << "#result(unfold):\n" << expr << std::endl;
        BOOST_AUTO(or_list, expr.list_of_or());
        std::cout << std::dec << "#results(unfold or){" << or_list.size() << "}:\n[" << std::endl;
        for (BOOST_AUTO(iter, or_list.begin()); iter != or_list.end(); ++iter)
        {
            results.clear();
            (*iter)->get_and(results);
            std::cout << "  " << make_format(results) << std::endl;
        }
        std::cout << "]" << std::endl;
    }

    regex::clear();
    expr.reset(regstr);
    result = expr.compile() && expr.unfold(true);
    if (!result)
    {
        std::cout << "#result(unfold with disable):" << std::endl;
        std::cerr << "[warning] branchs of expr: \"" << regstr << "\" greater than " << REGEX_MAX_NODES << std::endl;
    }
    else
    {
        std::cout << "#result(unfold with disable):\n" << expr << std::endl;
        BOOST_AUTO(or_list, expr.list_of_or());
        std::cout << std::dec << "#results(unfold or){" << or_list.size() << "}:\n[" << std::endl;
        for (BOOST_AUTO(iter, or_list.begin()); iter != or_list.end(); ++iter)
        {
            results.clear();
            (*iter)->get_and(results);
            std::cout << "  " << make_format(results) << std::endl;
        }
        std::cout << "]" << std::endl;

        std::cout << std::dec << "#nodes(unfold or){" << or_list.size() << "}:\n[" << std::endl;
        for (BOOST_AUTO(iter, or_list.begin()); iter != or_list.end(); ++iter)
        {
            nodes.clear();
            (*iter)->get_and(nodes);
            std::cout << "  " << make_format(nodes) << std::endl;
        }
        std::cout << "]" << std::endl;
    }

    regex::clear();
    expr.reset(regstr, false, true);
    expr.compile();
    expr.adjust();
    std::cout << "#result(adjust2):\n" << expr << std::endl;
    results.clear();
    expr.get_all(results);
    std::cout << "#results(adjust all):\n" << make_format(results) << std::endl;
    std::cout << std::endl;

    regex::clear();
    expr.reset(regstr, false, true);
    result = expr.compile() && expr.unfold(false);
    if (!result)
    {
        std::cout << "#result(unfold2):" << std::endl;
        std::cerr << "[warning] branchs of expr: \"" << regstr << "\" greater than " << REGEX_MAX_NODES << ", or disabled" << std::endl;
    }
    else
    {
        std::cout << "#result(unfold2):\n" << expr << std::endl;
        BOOST_AUTO(or_list, expr.list_of_or());
        std::cout << std::dec << "#results(unfold2 or){" << or_list.size() << "}:\n[" << std::endl;
        for (BOOST_AUTO(iter, or_list.begin()); iter != or_list.end(); ++iter)
        {
            results.clear();
            (*iter)->get_and(results);
            std::cout << "  " << make_format(results) << std::endl;
        }
        std::cout << "]" << std::endl;

        std::cout << std::dec << "#nodes(unfold2 or){" << or_list.size() << "}:\n[" << std::endl;
        for (BOOST_AUTO(iter, or_list.begin()); iter != or_list.end(); ++iter)
        {
            nodes.clear();
            (*iter)->get_and(nodes);
            std::cout << "  " << make_format(nodes) << std::endl;
        }
        std::cout << "]" << std::endl;
    }

    regex::clear();
    expr.reset(regstr, false, true);
    result = expr.compile() && expr.unfold(true);
    if (!result)
    {
        std::cout << "#result(unfold2 with disable):" << std::endl;
        std::cerr << "[warning] branchs of expr: \"" << regstr << "\" greater than " << REGEX_MAX_NODES << std::endl;
    }
    else
    {
        std::cout << "#result(unfold2 with disable):\n" << expr << std::endl;
        BOOST_AUTO(or_list, expr.list_of_or());
        std::cout << std::dec << "#results(unfold2 or){" << or_list.size() << "}:\n[" << std::endl;
        for (BOOST_AUTO(iter, or_list.begin()); iter != or_list.end(); ++iter)
        {
            results.clear();
            (*iter)->get_and(results);
            std::cout << "  " << make_format(results) << std::endl;
        }
        std::cout << "]" << std::endl;

        std::cout << std::dec << "#nodes(unfold2 or){" << or_list.size() << "}:\n[" << std::endl;
        for (BOOST_AUTO(iter, or_list.begin()); iter != or_list.end(); ++iter)
        {
            nodes.clear();
            (*iter)->get_and(nodes);
            std::cout << "  " << make_format(nodes) << std::endl;
        }
        std::cout << "]" << std::endl;
    }*/

    return true;
}

void test1()
{
    const Slice str = "\\.(asa|asax|ascx|axd|backup|bak|bat|cdx|cer|cfg|cmd|com|config|conf|cs|csproj|csr|dat|db|dbf|dll|dos|htr|htw|ida|idc|idq|inc|ini|key|licx|lnk|log|mdb|old|pass|pdb|pol|printer|pwd|resources|resx|sql|sys|vbli|vbs|vbproj|vsdisco|webinfo|xsd|xsx|sqlite|bac|save|sav|old2|bk|~bk|orig|lst|arc|tmp|temp|data|cfm)";
    const Slice reg = "(?:\\b(?:(?:i(?:nterplay|hdr|d3)|m(?:ovi|thd)|r(?:ar!|iff)|(?:ex|jf)if|f(?:lv|ws)|varg|cws)\\b|gif)|B(?:%pdf|\\.ra)\\b)";
    const Slice test = "abc.*123{1,}(?=dos)[\\d\\s\\w](abc[0-9()])\\?\\[\\)[[:alpha:]](?:a|b|c){2,3}";
    const Slice re = "abc.*123(a|b|c){2,3}";
    const Slice test_crlf = "(*LF)-a.b";
    const Slice test_sep = "[\\b^\\-a]";
    const Slice test_l = "(?| (?=[\\x00-\\x7f])|(?=[\\x80-\\x{7ff}])|(?=[\\x{800}-\\x{ffff}])|(?=[\\x{10000}-\\x{1fffff}]))";
    const Slice subpatts = "  cat(aract|erpillar|)the1 ((red|white) (king|queen))the2 ((?:red2|white2) (king2|queen2))";
    const Slice option1 = "(?i:saturday|sunday)";
    const Slice option2 = "(?ix-m:(?ix)saturday|sunday)";
    const Slice dup1 = "(?|(Sat)ur|(Sun))day";
    const Slice dup2 = "/(?|(abc)|(def))\\1/";
    const Slice dup3 = "/(?|(abc)|(def))(?1)/";
    const Slice dup4 = "(?|(abc)|(def))";
    const Slice dup5 = ".abc*a?*b?+c??9-(?:1|2|3)*(?|4|5)?[A-F]*[A-F]?(*CR)?*+.\\^we are family\\**.?\\*?\\x01+\\x02*\\007*\\001+\\g1*\\k{abc}?";
    const Slice g1 = "(ring), \\1(ring), \\g1(ring), \\g{1}";
    const Slice g2 = "(?<p1>(?i)rah)\\s+\\g{p1}";
    const Slice k1 = "(?<p1>(?i)rah)\\s+\\k<p1>";
    const Slice k2 = "(?'p1'(?i)rah)\\s+\\k{p1}";
    const Slice k3 = "(?P<p1>(?i)rah)\\s+(?P=p1)";
    const Slice dollar = "\\b(net\\s+user)\\b$";
    const Slice merge1 = ">\\[abc\\]</[Aa]><br>";
    const Slice merge2 = "(?:<(?:TITLE>Index of.*?<H|title>Index of.*?<h)1>Index of|>\\[abc\\]</[Aa]><br>|(?:TITLE>Directory Listing For)|title>Directory Listing For)";
    const Slice splice = "<b>Version Information:</b>(?:&nbsp;|\\s)Microsoft \\.NET Framework Version:";
    const Slice unicode = "(?i:[\"'].*?[,].*(((v|(\\\\\\\\u0076)|(\\\\166)|(\\\\x76))[^a-z0-9]*(a|(\\\\\\\\u0061)|(\\\\141)|(\\\\x61))[^a-z0-9]*(l|(\\\\\\\\u006C)|(\\\\154)|(\\\\x6C))[^a-z0-9]*(u|(\\\\\\\\u0075)|(\\\\165)|(\\\\x75))[^a-z0-9]*(e|(\\\\\\\\u0065)|(\\\\145)|(\\\\x65))[^a-z0-9]*(O|(\\\\\\\\u004F)|(\\\\117)|(\\\\x4F))[^a-z0-9]*(f|(\\\\\\\\u0066)|(\\\\146)|(\\\\x66)))|((t|(\\\\\\\\u0074)|(\\\\164)|(\\\\x74))[^a-z0-9]*(o|(\\\\\\\\u006F)|(\\\\157)|(\\\\x6F))[^a-z0-9]*(S|(\\\\\\\\u0053)|(\\\\123)|(\\\\x53))[^a-z0-9]*(t|(\\\\\\\\u0074)|(\\\\164)|(\\\\x74))[^a-z0-9]*(r|(\\\\\\\\u0072)|(\\\\162)|(\\\\x72))[^a-z0-9]*(i|(\\\\\\\\u0069)|(\\\\151)|(\\\\x69))[^a-z0-9]*(n|(\\\\\\\\u006E)|(\\\\156)|(\\\\x6E))[^a-z0-9]*(g|(\\\\\\\\u0067)|(\\\\147)|(\\\\x67)))).*?:)";
    const Slice u2 = "(v|(\\\\\\\\u0076)|(\\\\166)|(\\\\x76))[^a-z0-9]*(a|(\\\\\\\\u0061)|(\\\\141)|(\\\\x61))[^a-z0-9]*";
    const Slice sql = "(?i)([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(\\band\\b|\\bor\\b|&&|\\|\\|)(?:[\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b)*(?:(?:(?:[\'\"]\\w*.*[\'\"])|(?:\\d.*))([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:\\bbetween\\b|(?:\\bnot\\b([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*\\bbbetween\\b))([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:(?:[\'\"]\\w*.*[\'\"])|(\\d.*))([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*\\band\\b(?:[\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n)(?:(?:[\'\"]\\w*.*)|(?:\\d.*))|(?:\\bcase\\b([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*\\bwhen\\b([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:[\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:(?:[\'\"]\\w*.*[\'\"])|(?:\\d.*))([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:=|>|<|&|~|\\||\\^|/|>=|<<|>>|!=|<=>|<>|\\+|\\-|\\*|%|(?:\\bnot\\b(?:[\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n)*)?\\blike\\b|\\brlike\\b|(?:\\bnot\\b(?:[\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n)*)?\\bregexp\\b|\\bdiv\\b|\\bmod\\b|\\bnot\\b|\\bxor\\b|(?:\\bnot\\b(?:[\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n)*)?\\bin\\b)([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:(?:[\'\"]\\w*.*[\'\"])|(?:\\d.*))([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n)*\\bthen\\b(?:[\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:(?:[\'\"]\\w*.*[\'\"])|(\\d.*)|\\(*?true\\)*?|\\(*?false\\)*)([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*\\belse\\b([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*(?:(?:[\'\"]\\w*.*[\'\"])|(\\d.*))([\\s\\+\\-~!@\\(\\)]|/\\*[\\s\\S]*?\\*/|#+\\n|\\bbinary\\b|\\bnot\\b)*\\bend\\b))\n";

    const Slice tests[] =
    {
        str, reg, test, re, test_crlf,
        test_sep, subpatts, option1, option2,
        dup1, dup2, dup3, dup4, dup5,
        g1, g2, k1, k2, k3, dollar, merge1, merge2,
        splice, unicode, sql
    };

    for (size_t i = 0; i < dimensionof(tests); ++i)
    {
        test_regexpr(tests[i]);
    }

    regex::shrink_to_fit();
}

bool test2(const char* filename)
{
    std::ifstream infile(filename, std::ios::in);
    if (!infile)
    {
        //const char* msg = strerror(errno);
        SMART_ASSERT(0).msg("open file failed")("file", filename);// ("errmsg", msg);
        return false;
    }

    int  lineno = 0;
    char line[8 * 1024];

    while (!infile.eof())
    {
        infile.getline(line, sizeof(line));
        if (infile.eof())
            break;

        lineno++;
        Slice regstr = make_slice(line, integer_cast<int>(infile.gcount()));

        if (regstr.front() == '#') continue;

        if (is_any_of("\r\n\0")(regstr.back()))
        {
            regstr.remove_suffix(1);
        }
        else if (regstr.ends_with("\r\n"))
        {
            regstr.remove_suffix(2);
        }

        RegexObject::SimpleStr prefix;
        prefix.push_back(lineno);
        std::cout << "#[" << prefix << "] -> \"" << regstr << '\"' << std::endl;
        get_keywords(regstr);
        std::cout << "-------------------------------------------------------------------" << std::endl;
    }

    regex::shrink_to_fit();

    return true;
}

int main(int argc, char* argv[])
{
    //std::ofstream output("D:\\expr.txt", std::ios::out | std::ios::trunc);
    //std::cout.rdbuf(output.rdbuf());
    //std::clog.rdbuf(output.rdbuf());

    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            std::ifstream input(argv[i], std::ios::in);
            if (!input)
            {
                get_keywords(make_slice(argv[i], strlen(argv[i])));
            }
            else
            {
                input.close();
                test2(argv[i]);
            }
        }
    }
    else
    {
        test1();
    }

    //output.close();
    return 0;
}
