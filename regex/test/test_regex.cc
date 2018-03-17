
#include "test_config.h"
#include "regex_object.h"
#include "regex.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#define PCRE2_STATIC 1
#include "pcre2.h"
#include <algorithm>
#include <iostream>
#include <fstream>

CString spec = "\\b(?i:[\"'][,].*(((v|(\\\\\\\\u0076)|(\\\\166)|(\\\\x76))[^a-z0-9]*(a|(\\\\\\\\u0061)|(\\\\141)|(\\\\x61))[^a-z0-9]*(l|(\\\\\\\\u006C)|(\\\\154)|(\\\\x6C))[^a-z0-9]*(u|(\\\\\\\\u0075)|(\\\\165)|(\\\\x75))[^a-z0-9]*(e|(\\\\\\\\u0065)|(\\\\145)|(\\\\x65))[^a-z0-9]*(O|(\\\\\\\\u004F)|(\\\\117)|(\\\\x4F))[^a-z0-9]*(f|(\\\\\\\\u0066)|(\\\\146)|(\\\\x66)))|((t|(\\\\\\\\u0074)|(\\\\164)|(\\\\x74))[^a-z0-9]*(o|(\\\\\\\\u006F)|(\\\\157)|(\\\\x6F))[^a-z0-9]*(S|(\\\\\\\\u0053)|(\\\\123)|(\\\\x53))[^a-z0-9]*(t|(\\\\\\\\u0074)|(\\\\164)|(\\\\x74))[^a-z0-9]*(r|(\\\\\\\\u0072)|(\\\\162)|(\\\\x72))[^a-z0-9]*(i|(\\\\\\\\u0069)|(\\\\151)|(\\\\x69))[^a-z0-9]*(n|(\\\\\\\\u006E)|(\\\\156)|(\\\\x6E))[^a-z0-9]*(g|(\\\\\\\\u0067)|(\\\\147)|(\\\\x67)))).*?:)";

int main(int argc, char* argv[])
{
    CString str = ".*abc";

    Regex reg;

    ASSERT_EQ(sizeof(PCRE2_SIZE), sizeof(MatchBlocks::value_type));

    ASSERT_EQ(0, reg.compile(str));
    ASSERT_EQ(true, reg.match("123456abc", reg.new_match_blocks()));

    str.assign("hello, world");
    ASSERT_EQ(true, reg.match("123456abc", reg.new_match_blocks()));

    Regex tmp;
    ASSERT_EQ(0, tmp.compile("_pk_ref"));

    if (argc > 2)
    {
        Regex uni;
        char buffer[64 * 1048];

        std::ifstream in(argv[1], std::ios::in);
        in.getline(buffer, dimensionof(buffer));
        int len = integer_cast<int>(in.gcount()-1);
        in.close();
        ASSERT_TRUE(len < dimensionof(buffer));

        str.assign(make_slice(buffer, len));
        regex::format(std::cout, str) << std::endl;

        ASSERT_EQ(0, uni.compile(str));

        in.open(argv[2], std::ios::in);
        while (in.getline(buffer, dimensionof(buffer)))
        {
            Slice line = make_slice(buffer, integer_cast<int>(in.gcount()-1));
            regex::format(std::cout, line) << std::endl;
            ASSERT_EQ(true, uni.match(line, uni.new_match_blocks()));
        }
        in.close();
    }

    std::cout << std::endl;

    regex::format(std::cout, str) << std::endl;
    return report_errors();
}
