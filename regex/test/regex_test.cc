
#include "common/xstring.h"
#include "regex_object.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>

bool test_regexpr(const Slice& regstr)
{
    RegexObject expr;
    RegexObject::SliceList results;
    regex::clear();
    expr.reset(regstr, false);
    bool result = expr.compile();
    std::cout << "#pattern: \"" << expr.context() << "\"\n#compiling:" << std::endl;
    std::cout << std::boolalpha << "#compile: " << result << std::endl;
    //std::cout << "#result:\n" << expr << std::endl;

    results.clear();
    expr.adjust();
    std::cout << "#result(adjust):\n" << expr << std::endl;

    return true;
}

bool test_file(const char* filename)
{
    std::ifstream infile(filename, std::ios::in);
    if (!infile)
    {
        //const char* msg = strerror(errno);
		SMART_ASSERT(0).msg("open file failed")("file", filename);// ("errmsg", msg);
        return false;
    }

    int  lineno = 0;
    char line[1024];

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

        XString<512> prefix;
        prefix.push_back(lineno);
        std::cout << "#[" << prefix << "] -> \"" << regstr << '\"' << std::endl;
        test_regexpr(regstr);
        std::cout << "-------------------------------------------------------------------" << std::endl;
    }

    regex::shrink_to_fit();

    return true;
}

int main(int argc, char* argv[])
{
    //std::clog.rdbuf(std::cout.rdbuf());

    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            struct stat buf;
            if (::stat(argv[i], &buf) == -1)
            {
                test_regexpr(make_slice(argv[i], integer_cast<int>(strlen(argv[i]))));
            }
            else
            {
                test_file(argv[i]);
            }
        }
    }

    return 0;
}
