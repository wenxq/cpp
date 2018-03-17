
#include "regex.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#define PCRE2_STATIC 1
#include "pcre2.h"

typedef PCRE2_SIZE* OVector;

namespace detail
{
    void regex_code_free(uintptr_t* ptr)
    {
        pcre2_code_free((pcre2_code *)ptr);
    }

    void regex_match_context_free(uintptr_t* ptr)
    {
        pcre2_match_context_free((pcre2_match_context *)ptr);
    }

    void regex_match_data_free(uintptr_t* ptr)
    {
        pcre2_match_data_free((pcre2_match_data *)ptr);
    }
} // namespace detail

MatchBlocks::MatchBlocks(void* ptr)
  : mBlocks((uintptr_t *)ptr, detail::regex_match_data_free)
{}

uint32_t MatchBlocks::size() const
{
    return pcre2_get_ovector_count((pcre2_match_data *)mBlocks.get());
}

MatchBlocks::pointer MatchBlocks::data() const
{
    return pcre2_get_ovector_pointer((pcre2_match_data *)mBlocks.get());
}

void* MatchBlocks::get() const
{
    return (void *)mBlocks.get();
}

MatchBlocks MatchBlocks::clone() const
{
    return MatchBlocks(pcre2_match_data_create(size(), NULL));
}

Regex::match_context_ptr Regex::ContextBlocks((uintptr_t *)pcre2_match_context_create(NULL),
                                                detail::regex_match_context_free);

int Regex::compile(const Slice& pattern)
{
    SMART_ASSERT(!mRegex)("pattern", mExpr);

    int err_code;
    PCRE2_SIZE err_offset;

    uint32_t options = (PCRE2_UTF | PCRE2_DOTALL | PCRE2_MULTILINE);

    mExpr  = pattern;
    mRegex.reset((uintptr_t *)pcre2_compile(
                    (PCRE2_SPTR8)pattern.data(), /* the pattern */
                    pattern.length(),            /* length */
                    options,                     /* options */
                    &err_code,                   /* for error code */
                    &err_offset,                 /* for error offset */
                    NULL                         /* use default character tables */
                 ), detail::regex_code_free);

    if (!mRegex)
    {
        SMART_ASSERT(mRegex).msg("PCRE2 compilation failed")
                    ("expr", pattern)("offset", (int)err_offset)("err_code", err_code);            
        return 1;
    }

    int ret = pcre2_jit_compile((pcre2_code *)(mRegex.get()), PCRE2_JIT_COMPLETE);
    if (ret != 0)
    {
        std::cerr << "PCRE2 JIT compilation failed, pattern:\"" << pattern << "\"\n";
        mJit = false;
    }
    else
    {
        mJit = true;
    }

    return 0;
}

MatchBlocks Regex::new_match_blocks() const
{
    if (!mRegex)
    {
        return MatchBlocks();
    }

    return MatchBlocks(pcre2_match_data_create_from_pattern((pcre2_code *)(mRegex.get()), NULL));
}

bool Regex::match(const Slice& subject, const MatchBlocks& match, bool anchored) const
{
    if (!mRegex) return false;

    const uint32_t options = anchored ? PCRE2_ANCHORED : 0;

    int err_code;

    pcre2_code* re = (pcre2_code *)(mRegex.get());
    pcre2_match_data* match_data = (pcre2_match_data *)match.get();
    pcre2_match_context* mcontext = (pcre2_match_context *)ContextBlocks.get();

    if (mJit)
    {
        err_code = pcre2_jit_match(
                      re,                          /* the compiled pattern */
                      (PCRE2_SPTR8)subject.data(), /* the subject string */
                      subject.length(),            /* the length of the subject */
                      0,                           /* start at offset 0 in the subject */
                      options,                     /* default options */
                      match_data,                  /* match data */
                      mcontext                     /* match context */
                   );

        if (err_code <= 0)
        {
            if (err_code != PCRE2_ERROR_NOMATCH)
            {
                std::cerr << "PCRE2 pcre2_jit_match failed with: " << err_code << ", pattern:" << mExpr << '\n';
            }
            return false;
        }
    }
    else
    {
        err_code = pcre2_match(
                      re,                          /* the compiled pattern */
                      (PCRE2_SPTR8)subject.data(), /* the subject string */
                      subject.length(),            /* the length of the subject */
                      0,                           /* start at offset 0 in the subject */
                      options,                     /* default options */
                      match_data,                  /* match data */
                      mcontext                     /* match context */
                   );

        if (err_code <= 0)
        {
            if (err_code != PCRE2_ERROR_NOMATCH)
            {
                std::cerr << "PCRE2 pcre2_match failed with: " << err_code << ", pattern:" << mExpr << '\n';
            }
            return false;
        }
    }

    return true;
}

bool Regex::search(const Slice& subject, Slice& result, const MatchBlocks& match) const
{
    if (!mRegex) return false;

    pcre2_code* re = (pcre2_code *)(mRegex.get());
    pcre2_match_data* match_data = (pcre2_match_data *)match.get();
    pcre2_match_context* mcontext = (pcre2_match_context *)ContextBlocks.get();

    int err_code;

    if (mJit)
    {
        err_code = pcre2_jit_match(
                      re,                          /* the compiled pattern */
                      (PCRE2_SPTR8)subject.data(), /* the subject string */
                      subject.length(),            /* the length of the subject */
                      0,                           /* start at offset 0 in the subject */
                      0,                           /* default options */
                      match_data,                  /* match data */
                      mcontext                     /* match context */
                   );

        if (err_code <= 0)
        {
            if (err_code != PCRE2_ERROR_NOMATCH)
            {
                std::cerr << "PCRE2 pcre2_jit_match failed with: " << err_code << ", pattern:" << mExpr << '\n';
            }
            return false;
        }
    }
    else
    {
        err_code = pcre2_match(
                      re,                          /* the compiled pattern */
                      (PCRE2_SPTR8)subject.data(), /* the subject string */
                      subject.length(),            /* the length of the subject */
                      0,                           /* start at offset 0 in the subject */
                      0,                           /* default options */
                      match_data,                  /* match data */
                      mcontext                     /* match context */
                   );

        if (err_code <= 0)
        {
            if (err_code != PCRE2_ERROR_NOMATCH)
            {
                std::cerr << "PCRE2 pcre2_match failed with: " << err_code << ", pattern:" << mExpr << '\n';
            }
            return false;
        }
    }

    MatchBlocks::pointer ovector = match.data();
    SMART_ASSERT(ovector != nullptr).msg("PCRE2 get ovector pointer failed");
    if (err_code > 1)
    {
        result = subject.substr(ovector[0], ovector[1] - ovector[0]);
    }
    else
    {
        result = subject.substr(ovector[0]);
    }

    return true;
}

int Regex::find_all(const Slice& subject, std::vector<Slice>& results, const MatchBlocks& match) const
{
    if (!mRegex) return 0;

    pcre2_code* re = (pcre2_code *)(mRegex.get());
    pcre2_match_data* match_data = (pcre2_match_data *)match.get();
    pcre2_match_context* mcontext = (pcre2_match_context *)ContextBlocks.get();

    const char* ptr = subject.data();
    PCRE2_SIZE len = integer_cast<PCRE2_SIZE>(subject.length());
    int err_code;
    int found = 0;

    MatchBlocks::pointer ovector = match.data();
    SMART_ASSERT(ovector != nullptr).msg("PCRE2 get ovector pointer failed");

    if (mJit)
    {
        while (ptr < subject.end())
        {
            err_code = pcre2_jit_match(
                          re,                 /* the compiled pattern */
                          (PCRE2_SPTR8)ptr,   /* the subject string */
                          len,                /* the length of the subject */
                          0,                  /* start at offset 0 in the subject */
                          0,                  /* default options */
                          match_data,         /* match data */
                          mcontext            /* match context */
                       );

            if (err_code <= 0)
            {
                if (err_code != PCRE2_ERROR_NOMATCH)
                {
                    std::cerr << "PCRE2 pcre2_jit_match failed with: " << err_code << ", pattern:" << mExpr << '\n';
                }
                break;
            }

            results.push_back(make_slice(ptr + ovector[0], integer_cast<int>(ovector[1] - ovector[0])));
            ptr += ovector[1];
            len -= ovector[1];
            ++found;
        }
    }
    else
    {
        while (ptr < subject.end())
        {
            err_code = pcre2_match(
                          re,                 /* the compiled pattern */
                          (PCRE2_SPTR8)ptr,   /* the subject string */
                          len,                /* the length of the subject */
                          0,                  /* start at offset 0 in the subject */
                          0,                  /* default options */
                          match_data,         /* match data */
                          mcontext            /* match context */
                       );

            if (err_code <= 0)
            {
                if (err_code != PCRE2_ERROR_NOMATCH)
                {
                    std::cerr << "PCRE2 pcre2_match failed with: " << err_code << '\n';
                }
                break;
            }

            results.push_back(make_slice(ptr + ovector[0], integer_cast<int>(ovector[1] - ovector[0])));
            ptr += ovector[1];
            len -= ovector[1];
            ++found;
        }
    }

    return found;
}
