
#include "common/string-inl.h"
#include "defines.h"
#include "regex_object.h"
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

#define is_octal(o) ((o) >= '0' && (o) < '8')
#define is_hex(x)   ((((x) >= '0') && ((x) <= '9')) || (((x) >= 'a') && ((x) <= 'f')) || (((x) >= 'A') && ((x) <= 'F')))

#define NEW_NODE_LOG(text) NEW_BLOCK_LOG(">>>", text)

#define else_if_char_types(str, type, callback) \
    else if ((str).starts_with(type)) { \
        callback(charset); \
        xpos += detail::length_of(type); \
    }

#define else_if_named_charset(str, positive) \
    else if ((str).starts_with("[:"#positive":]") || (str).starts_with("[:^"#positive":]")) { \
        const bool opposite = (str).starts_with("[:^"); \
        for (uint8_t i = 0u; i < 128u; ++i) { \
            if (is##positive(i)) { \
                if (!opposite) { \
                    charset.insert(i); \
                } \
            } else if (opposite) { \
                charset.insert(i); \
            } \
        } \
        xpos += detail::length_of("[:" #positive ":]"); \
        if (opposite) ++xpos; \
    }

#define GET_UTF8LEN(beg, end) detail::get_utf8len(beg, end)
#define CHECK_UTF8(beg, end, count) do { \
    if (end - beg < count) return nullptr; \
    const uint8_t* b = beg + 1; \
    const uint8_t* e = beg + count; \
    while (b < e) { \
        if (((*b) & 0xc0) != 0x80) return nullptr; \
        ++b; \
    } \
} while (0)

namespace detail
{
    const uint8_t* get_utf8len(const uint8_t* beg, const uint8_t* end);
} // namespace detail

namespace regex
{
    typedef RegexObject::SimpleStr SimpleStr;

    ObjectPool<SimpleStr>  StringPool(512);
    ObjectPool<RegexObject>    NodePool(1024);

    const Slice MetaCharSet1 = "\\^$.|[]()?*+{}"; // Outside square brackets
    const Slice MetaCharSet2 = "\\^-[]";          // Inside square brackets

    const Slice SpecialCharSet[] = 
    {
        "\\b", "\\B", // matches at a word boundary or not
        "\\d", "\\D", // any decimal digit or not
        "\\h", "\\H", // any horizontal white space character or not
        "\\Q", "\\E", // remove the special meaning from a sequence of characters
        "\\s", "\\S", // any white space character or not
        "\\v", "\\V", // any vertical white space character or not
        "\\w", "\\W", // any "word" character or not
        "\\z", "\\Z", // matches at the end of the subject
        "\\A",        // matches at the start of the subject
        //"\\C",        // matches any one code unit, whether or not a UTF mode is set
        "\\G",        // matches at the first matching position in the subject
        "\\K"         // any previously matched characters not to be included in the final matched sequence
    };

    const Slice NonPrintCharSet[] =
    {
        "\\a",      // alarm, that is, the BEL character (hex 07)
        "\\c" ,     // "control-x", where x is any printable ASCII character
        "\\e",      // escape (hex 1B)
        "\\f",      // form feed (hex 0C)
        "\\n",      // linefeed (hex 0A)
        "\\r",      // carriage return (hex 0D)
        "\\t"       // tab (hex 09)
        // \0dd      character with octal code 0dd
        // \ddd      character with octal code ddd, or back reference
        // \o{ddd..} character with octal code ddd..
        // \xhh      character with hex code hh
        // \x{hhh..} character with hex code hhh.. (default mode)
        // \uhhhh    character with hex code hhhh (when PCRE2_ALT_BSUX is set)
    };

    const Slice ComplexPatterns[] =
    {
        "(?:",  // non-capturing group
        "(?=",  // positive look ahead
        "(?!",  // negative look ahead
        "(?<=", // positive look behind
        "(?<!", // negative look behind
        "(?>",  // atomic, non-capturing group
        "(?|",  // non-capturing group; reset group numbers for capturing groups in each alternative
        "(?#",  // comment (not nestable)
        "(?P<", // named capturing group (Python)
        "(?'",  // named capturing group (Perl)
        "(?<"   // named capturing group (Perl)
    };

    const Slice Options = 
                                "i" // caseless
                                "J" // allow duplicate names
                                "m" // multiline
                                "s" // single line (dotall)
                                "U" // default ungreedy (lazy)
                                "x" // extended (ignore white space)
                                "-";

    inline SimpleStr* new_cstring()
    {
        return &(StringPool.new_object());
    }

    SimpleStr* new_simplestr()
    {
        return new_cstring();
    }

    inline RegexObject* new_expr()
    {
        return &(NodePool.new_object());
    }

    void shrink_to_fit()
    {
        StringPool.shrink_to_fit();
        NodePool.shrink_to_fit();
    }

    void clear()
    {
        StringPool.clear();
        NodePool.clear();
    }

    template <size_t N>
    bool find_key(const Slice (&elems)[N], const Slice& str, Slice& key)
    {
        for (size_t i = 0; i < dimensionof(elems); ++i)
        {
            if (str.starts_with(elems[i]))
            {
                key = elems[i];
                return true;
            }
        }

        return false;
    }

    inline bool is_special(const Slice& str, Slice& key)
    {
        return find_key(SpecialCharSet, str, key);
    }

    inline bool is_complex_pattern(const Slice& str, Slice& key)
    {
        return find_key(ComplexPatterns, str, key);
    }

    inline bool is_option(uint8_t c)
    {
        return is_any_of(Options)(c);
    }

    inline bool is_metachar(uint8_t c)
    {
        return is_any_of(MetaCharSet1)(c) || is_any_of(MetaCharSet2)(c);
    }

    inline bool is_quantifier(uint8_t c)
    {
        return is_any_of(".*+?{")(c);
    }

    bool is_nonprint(const Slice& str, Slice& key, uint8_t& c);
} // namespace regex

bool regex::is_nonprint(const Slice& str, Slice& key, uint8_t& c)
{
    bool result = regex::find_key(regex::NonPrintCharSet, str, key);
    if (result)
    {
        if (key == "\\a")
        {
            c = 0x07;
        }
        else if (key == "\\e")
        {
            c = 0x1B;
        }
        else if (key == "\\f")
        {
            c = 0x0C;
        }
        else if (key == "\\n")
        {
            c = 0x0A;
        }
        else if (key == "\\r")
        {
            c = 0x0D;
        }
        else if (key == "\\t")
        {
            c = 0x09;
        }
        else if (key == "\\c")
        {
            c = static_cast<uint8_t>(str[key.length()]);
            bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_any_of("@[\\]^_?")(c);
            if (!valid)
            {
                SMART_ASSERT(valid)("str", str)("key", key);
                throw std::invalid_argument("invalid of \\cx");
            }

            if (c < 32u || c > 126u)
            {
                SMART_ASSERT(c >= 32u && c <= 126u);
                return false;
            }
            c = ToUpper[c] ^ 0x40;
        }
        else
        {
            return false;
        }
    }

    return result;
}

#define IS_PRINT(c) ((c) > 0 && isprint(c))

std::ostream& regex::format(std::ostream& ss, const Slice& str)
{
    for (const char* ptr = str.begin(); ptr != str.end(); ++ptr)
    {
        if (SPECIAL_CHAR != '\0' && *ptr > 0 && *ptr < 256 && (is_any_of("\n\r\t\f\a")(*ptr) || !IS_PRINT(*ptr)))
        {
            ss << static_cast<uint8_t>(SPECIAL_CHAR);
        }

        if (*ptr == '\n')
        {
            ss << "\\n";
        }
        else if (*ptr == '\r')
        {
            ss << "\\r";
        }
        else if (*ptr == '\t')
        {
            ss << "\\t";
        }
        else if (*ptr == '\f')
        {
            ss << "\\f";
        }
        else if (*ptr == '\a')
        {
            ss << "\\a";
        }
        else if (IS_PRINT(*ptr))
        {
            ss << *ptr;
        }
        else
        {
            ss << std::hex << std::showbase
               << integer_cast<uint16_t>(static_cast<uint8_t>(*ptr))
               << std::dec << std::noshowbase;
        }
    }

    return ss;
}

const uint8_t* detail::get_utf8len(const uint8_t* beg, const uint8_t* end)
{
    if ((*beg & 0xfe) == 0xfe)
    {
        return nullptr;
    }
    else if ((*beg & 0xfc) == 0xfc)
    {
        CHECK_UTF8(beg, end, 6);
        return beg + 6;
    }
    else if ((*beg & 0xf8) == 0xf8)
    {
        CHECK_UTF8(beg, end, 5);
        return beg + 5;
    }
    else if ((*beg & 0xf0) == 0xf0)
    {
        CHECK_UTF8(beg, end, 4);
        return beg + 4;
    }
    else if ((*beg & 0xe0) == 0xe0)
    {
        CHECK_UTF8(beg, end, 3);
        return beg + 3;
    }
    else if ((*beg & 0xc0) == 0xc0)
    {
        CHECK_UTF8(beg, end, 2);
        return beg + 2;
    }
    else if ((*beg & 0x80) == 0x80)
    {
        return nullptr;
    }
    else
    {
        return beg;
    }
}

void RegexObject::clear()
{
    reset(Slice());
}

void RegexObject::reset(const Slice& context, bool expansion, bool splice)
{
    mAnd.clear();
    mOr .clear();

    mText       = context;
    mContext    = context;
    mDisable    = false;
    mAndDisable = false;
    mTarget     = false;
    mSplice     = splice;
    mExpansion  = expansion;

    if (!expansion)
    {
        mSplice = false;
    }
}

void RegexObject::swap(RegexObject& other)
{
    mAnd.swap(other.mAnd);
    mOr .swap(other.mOr);

    std::swap(mText,       other.mText);
    std::swap(mContext,    other.mContext);
    std::swap(mAndDisable, other.mAndDisable);
    std::swap(mDisable,    other.mDisable);
    std::swap(mTarget,     other.mTarget);
    //std::swap(mSplice,     other.mSplice);
    std::swap(mExpansion,  other.mExpansion);
}

std::string RegexObject::to_string() const
{
    std::ostringstream ss;
    ss << '\n';
    format(ss, "and", 0);
    return ss.str();
}

std::ostream& RegexObject::format(std::ostream& ss, const Slice& logic, uint32_t depth) const
{
    XString<256> indent;
    for (uint32_t i = 0; i < depth; ++i)
    {
        indent.append("  ");
    }

    if (mTarget)
    {
        SMART_ASSERT(mOr.empty());

        ss << indent << logic << '\n';

        if (!mAnd.empty() && depth != 0)
        {
            ss << indent << "  and\n";
            ss << indent << (mDisable ? "    X\"" : "    \"");
        }
        else
        {
            ss << indent << (mDisable ? "  X\"" : "  \"");
        }

        regex::format(ss, mContext);
        ss << "\"\n";
    }

    if (!mOr.empty())
    {
        ss << indent << logic << '\n';

        uint32_t dd = depth + 1;
        if (!mAnd.empty() && depth != 0)
        {
            ss << indent << "  and\n";
            ++dd;
        }

        for (BOOST_AUTO(iter, mOr.begin()); iter != mOr.end(); ++iter)
        {
            (*iter)->format(ss, "or", dd);
        }
    }

    if (!is_leaf())
    {
        ss << indent << logic << '\n';
    }

    if (!mAnd.empty())
    {
        if (depth != 0) ++depth;

        for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
        {
            (*iter)->format(ss, "and", depth);
        }
    }

    return ss;
}

// decimal digits (same as \d)
void RegexObject::any_of_digit(CharSet& charset)
{
    for (uint8_t i = 0u; i <= 9u; ++i)
    {
        charset.insert(integer_cast<uint8_t>('0' + i));
    }
}

// not a decimal digits (same as \D)
void RegexObject::not_of_digit(CharSet& charset)
{
    for (uint8_t i = 0u; i < 128u; ++i)
    {
        if (is_none_of("1234567890")(i))
        {
            charset.insert(i);
        }
    }
}

// white space (the same as \s from PCRE2 8.34)
void RegexObject::any_of_space(CharSet& charset)
{
    charset.insert(' ');  // Space (32)
    charset.insert('\f'); // FF (12)
    charset.insert('\n'); // LF (10)
    charset.insert('\r'); // CR (13)
    charset.insert('\t'); // HT (9)
    charset.insert('\v'); // VT (11)
}

//not white space (the same as \S from PCRE2 8.34)
void RegexObject::not_of_space(CharSet& charset)
{
    for (uint8_t i = 0u; i < 128u; ++i)
    {
        if (is_none_of(" \f\n\r\t\v")(i))
        {
            charset.insert(i);
        }
    }
}

// any horizontal white space character: space or tab only
void RegexObject::any_of_blank(CharSet& charset)
{
    charset.insert(0x09); // Horizontal tab (HT)(9)
    charset.insert(0x20); // Space(32)
    charset.insert(0xA0); // Non-break space
}

// not a horizontal white space character
void RegexObject::not_of_blank(CharSet& charset)
{
    for (uint8_t i = 0u; i < 128u; ++i)
    {
        if (i != 0x09 && i != 0x20 && i != 0xA0)
        {
            charset.insert(i);
        }
    }
}

// "word" characters (same as \w), -> [A-Za-z0-9_]
void RegexObject::any_of_word(CharSet& charset)
{
    for (uint8_t i = 0u; i < 128u; ++i)
    {
        if (isalnum(i))
        {
            charset.insert(i);
        }
    }
    charset.insert('_');
}

// not "word" characters (same as \W), -> [^A-Za-z0-9_]
void RegexObject::not_of_word(CharSet& charset)
{
    
    for (uint8_t i = 0u; i < 128u; ++i)
    {
        if (i != '_' && !isalnum(i))
        {
            charset.insert(i);
        }
    }
}

// any vertical white space character
void RegexObject::any_of_vertical(CharSet& charset)
{
    charset.insert(0x0A); // Linefeed (LF)
    charset.insert(0x0B); // Vertical tab (VT)
    charset.insert(0x0C); // Form feed (FF)
    charset.insert(0x0D); // Carriage return (CR)
}

// not vertical white space character
void RegexObject::not_of_vertical(CharSet& charset)
{
    for (uint8_t i = 0; i < 128u; ++i)
    {
        if (i != 0x0A && i != 0x0B && i != 0x0C && i != 0x0D)
        {
            charset.insert(i);
        }
    }
}

Slice RegexObject::and_append(const Slice& view, const Slice& context, bool target, bool disable)
{
    RegexObject* node = regex::new_expr();
    node->mText      = view;
    node->mContext   = context;
    node->mDisable   = disable;
    node->mTarget    = target;
    node->mSplice    = mSplice;
    node->mExpansion = mExpansion;
    mAnd.push_back(node);
    return context;
}

Slice RegexObject::or_append(const Slice& view, const Slice& context, bool target, bool disable)
{
    RegexObject* node = regex::new_expr();
    node->mText      = view;
    node->mContext   = context;
    node->mDisable   = disable;
    node->mTarget    = target;
    node->mSplice    = mSplice;
    node->mExpansion = mExpansion;
    mOr.push_back(node);
    return context;
}

Slice RegexObject::and_append(RegexObject* node, bool target, bool disable)  
{
    node->mDisable   = disable;
    node->mTarget    = target;
    node->mExpansion = mExpansion;
    mAnd.push_back(node);
    return node->mContext;
}

Slice RegexObject::or_append(RegexObject* node, bool target, bool disable)  
{
    node->mDisable   = disable;
    node->mTarget    = target;
    node->mExpansion = mExpansion;
    mOr.push_back(node);
    return node->mContext;
}

bool RegexObject::split(SliceList& store)
{
    const uint8_t* xpos = (const uint8_t *)(mContext.begin());
    const uint8_t* end  = (const uint8_t *)(mContext.end());
    const uint8_t* prev = xpos;

    while (xpos < end)
    {
        if (*xpos == '\\' && (xpos + 1) < end)
        {
            xpos += (xpos[1] != 'c') \
                        ? detail::length_of("\\(") \
                        : detail::length_of("\\cx");
            continue;
        }

        if (*xpos == '[')
        {
            const void* last = find_definition(make_slice(xpos, end));
            if (last == nullptr) return nullptr;

            xpos = static_cast<const uint8_t *>(last);
        }
        else if (*xpos == '(')
        {
            const void* last = find_subpattern(make_slice(xpos, end));
            if (last == nullptr) return nullptr;

            xpos = static_cast<const uint8_t *>(last);
        }
        else if (*xpos == '|')
        {
            store.push_back(make_slice(prev, xpos));
            prev = xpos + 1;
        }
        else if (*xpos >= 128u)
        {
            const uint8_t* tmp = GET_UTF8LEN(xpos, end);
            if (tmp == nullptr)
            {
                SMART_ASSERT(GET_UTF8LEN(xpos, end) != nullptr);
                return nullptr;
            }
            xpos = tmp;
            continue;
        }
        
        ++xpos;
    }

    if (xpos == end && prev <= end)
    {
        store.push_back(make_slice(prev, end));
    }

    if (xpos != end || store.empty())
    {
        SMART_ASSERT(xpos != end || store.empty())
                    ("expr", mContext)("xpos", make_slice(xpos, end))("store", store);
        return false;
    }

    return true;
}

const void* RegexObject::find_subpattern(const Slice& subpattern)
{
    if (subpattern.front() != '(')
    {
        SMART_ASSERT(subpattern.front() == '(')("except", '(')("actual", subpattern);
        return nullptr;
    }

    const uint8_t* xpos = (const uint8_t *)(subpattern.begin());
    const uint8_t* end  = (const uint8_t *)(subpattern.end());

    uint32_t cnt = 0;
    while (xpos < end)
    {
        if (*xpos == '\\' && (xpos + 1) < end)
        {
            xpos += (xpos[1] != 'c') \
                        ? detail::length_of("\\(") \
                        : detail::length_of("\\cx");
            continue;
        }

        if (*xpos == '[')
        {
            const void* last = find_definition(make_slice(xpos, end));
            if (last == nullptr) return nullptr;

            xpos = static_cast<const uint8_t *>(last);
        }
        else if (*xpos == '(')
        {
            ++cnt;
        }
        else if (*xpos == ')')
        {
            if (cnt <= 0)
            {
                SMART_ASSERT(cnt > 0)("cnt", cnt)("expr", subpattern)("xpos", make_slice(xpos, end));
                return nullptr;
            }

            if (--cnt == 0)
                break;
        }
        else if (*xpos >= 128u)
        {
            const uint8_t* tmp = GET_UTF8LEN(xpos, end);
            if (tmp == nullptr)
            {
                SMART_ASSERT(GET_UTF8LEN(xpos, end) != nullptr);
                return nullptr;
            }
            xpos = tmp;
            continue;
        }
        
        ++xpos;
    }

    if ((xpos != end && *xpos != ')') || cnt != 0)
    {
        SMART_ASSERT((xpos == end || *xpos == ')') && cnt == 0)
                    ("expr", subpattern)("xpos", make_slice(xpos, end))("cnt", cnt);
        return nullptr;
    }

    return xpos;
}

const void* RegexObject::find_definition(const Slice& definition, bool* utf)
{
    if (definition.front() != '[')
    {
        SMART_ASSERT(definition.front() == '[')("except", '[')("actual", definition);
        return nullptr;
    }

    if (utf != nullptr) *utf = false;

    const uint8_t* beg  = (const uint8_t *)(definition.begin());
    const uint8_t* end  = (const uint8_t *)(definition.end());
    const uint8_t* xpos = (const uint8_t *)(definition.begin());
    while (xpos < end)
    {
        if (*xpos == '\\' && (xpos + 1) < end)
        {
            xpos += (xpos[1] != 'c') \
                        ? detail::length_of("\\(") \
                        : detail::length_of("\\cx");
            continue;
        }

        if (*xpos == '[')
        {
            if (xpos + 1 == end) return nullptr;

            if (xpos[1] == ':' && xpos != beg)
            {
                Slice str = make_slice(xpos, end);
                int pos = str.find(":]");
                if (pos == Slice::NPOS)
                {
                    return nullptr;
                }

                xpos += pos + detail::length_of(":]");
                continue;
            }

            // ignore all '['
        }
        else if (*xpos == ']')
        {
            //SMART_ASSERT(xpos[-1] != ':')("expr", definition)("xpos", make_slice(xpos, end));
            break;
        }
        else if (*xpos >= 128u)
        {
            if (utf != nullptr) *utf = true;
            const uint8_t* tmp = GET_UTF8LEN(xpos, end);
            if (tmp == nullptr)
            {
                SMART_ASSERT(GET_UTF8LEN(xpos, end) != nullptr);
                return nullptr;
            }
            xpos = tmp;
            continue;
        }
        
        ++xpos;
    }

    if (xpos == end || *xpos != ']')
    {
        SMART_ASSERT(xpos != end && *xpos == ']')("expr", definition)("xpos", make_slice(xpos, end));
        return nullptr;
    }

    return xpos;
}

const void* RegexObject::find_quantifier(const Slice& quantifier, bool& disable)
{
    disable = is_any_of("?*")(quantifier.front());

    const char* xpos = quantifier.begin();
    while (xpos < quantifier.end())
    {
        if (!regex::is_quantifier(*xpos))
            break;

        if (*xpos == '{')
        {
            Slice str = make_slice(xpos + detail::length_of('{'), quantifier.end());
            if (str.length() < detail::length_of("n}"))
            {
                SMART_ASSERT(str.length() >= detail::length_of("n}"))("quantifier", quantifier)("substr of {", str);
                return nullptr;
            }

            if (str.starts_with("0}") || str.starts_with("0,"))
            {
                disable = true;
            }

            if (atoi(str) < 0)
            {
                SMART_ASSERT(atoi(str) >= 0)("quantifier", quantifier)("substr of {", str);
                return nullptr;
            }

            int pos = str.find_first('}');
            if (pos == Slice::NPOS)
            {
                SMART_ASSERT(pos != Slice::NPOS)("quantifier", quantifier)("substr of {", str);
                return nullptr;
            }

            xpos = str.begin() + (pos + detail::length_of('}'));
        }
        else
        {
            ++xpos;
        }
    }

    return xpos;
}

const void* RegexObject::new_literal(const Slice& current, SimpleStr* &normal, const uint8_t* &mark)
{
    bool qstart = regex::is_quantifier(current.front());
    if (qstart || (current.length() >= detail::length_of("?+") && is_any_of("?*{")(current[1])))
    {
        Slice str = current;
        if (!qstart)
        {
            str.remove_prefix(detail::length_of('a'));
        }

        bool disable = false;
        const void* block = find_quantifier(str, disable);
        if (block > str.end())
        {
            SMART_ASSERT(block <= str.end())("pattern", current)("ptr", str);
            return nullptr;
        }
        
        Slice context = make_slice(current.begin(), block);
        if (!qstart && !disable)
        {
            normal->push_back(current.front());
            context.remove_prefix(detail::length_of('a'));
        }

        if (!normal->empty())
        {
            and_append(make_slice(mark, current.begin()), *normal, true);
            NEW_NODE_LOG(*normal);
            normal = regex::new_cstring();
            mark = (const uint8_t *)current.begin();
        }

        and_append(context, context, true, true);
        NEW_NODE_LOG(context);
        return static_cast<const uint8_t *>(block);
    }
    else if (static_cast<uint8_t>(current.front()) < 128u)
    {
        normal->push_back(current.front());
        return current.begin() + detail::length_of("a");
    }
    else
    {
        const uint8_t* xpos = (const uint8_t *)current.begin();
        const uint8_t* epos = GET_UTF8LEN(xpos, (const uint8_t *)current.end());
        if (epos == nullptr)
        {
            SMART_ASSERT(GET_UTF8LEN(xpos, (const uint8_t *)current.end()) != nullptr);
            return nullptr;
        }

        while (xpos != epos)
        {
            normal->push_back(*xpos);
            ++xpos;
        }
        return epos;
    }
}

const void* RegexObject::new_special(const Slice& backslash, SimpleStr* &normal, const uint8_t* &mark, const Slice& key)
{
    const char* xpos = backslash.begin();
    CharSet     charset;
    Slice enclosed;

    if (false) { /* nothing */ }
    else_if_char_types(backslash, "\\d", any_of_digit)
    else_if_char_types(backslash, "\\D", not_of_digit)
    else_if_char_types(backslash, "\\s", any_of_space)
    else_if_char_types(backslash, "\\S", not_of_space)
    else_if_char_types(backslash, "\\h", any_of_blank)
    else_if_char_types(backslash, "\\H", not_of_blank)
    else_if_char_types(backslash, "\\v", any_of_vertical)
    else_if_char_types(backslash, "\\V", not_of_vertical)
    else_if_char_types(backslash, "\\w", any_of_word)
    else_if_char_types(backslash, "\\W", not_of_word)
    else if (key == "\\Q")
    {
        int be = backslash.find_first("\\E");
        if (be == Slice::NPOS)
        {
            SMART_ASSERT(be != Slice::NPOS)("backslash", backslash);
            return nullptr;
        }

        enclosed = backslash.substr(0, be);
        enclosed.remove_prefix(detail::length_of("\\Q"));
        xpos = enclosed.end() + detail::length_of("\\E");
    }
    else
    { // disable
        xpos += key.length();
    }

    Slice quantifier = make_slice(xpos, backslash.end());
    if (!quantifier.empty())
    {
        bool disable = false;
        const void* next = find_quantifier(quantifier, disable);
        if (disable)
        {
            if (next > quantifier.end())
            {
                SMART_ASSERT(next <= quantifier.end())("quantifier", quantifier);
                return nullptr;
            }

            if (!normal->empty())
            {
                NEW_NODE_LOG(*normal);
                and_append(make_slice(mark, backslash.begin()), *normal, true);
                normal = regex::new_cstring();
                mark = (const uint8_t *)backslash.begin();
            }

            Slice text = make_slice(backslash.begin(), next);
            NEW_NODE_LOG(text);
            and_append(text, text, true, true);
            return next;
        }
    }

    if ((charset.empty() && enclosed.empty()) || !mExpansion)
    {
        if (!normal->empty())
        {
            NEW_NODE_LOG(*normal);
            and_append(make_slice(mark, backslash.begin()), *normal, true);
            normal = regex::new_cstring();
            mark = (const uint8_t *)backslash.begin();
        }

        NEW_NODE_LOG(key);
        and_append(make_slice(backslash.begin(), xpos), key, true, true);
        return xpos;
    }
    else if (!enclosed.empty())
    {
        normal->append(enclosed);
        return xpos;
    }
    else if (normal->empty() || !mSplice)
    {
        if (!normal->empty())
        {
            NEW_NODE_LOG(*normal);
            and_append(make_slice(mark, backslash.begin()), *normal, true);
            normal = regex::new_cstring();
            mark = (const uint8_t *)backslash.begin();
        }

        SimpleStr* str = regex::new_cstring();
        for (BOOST_AUTO(iter, charset.begin()); iter != charset.end(); ++iter)
        {
            str->push_back(*iter);
        }

        NEW_NODE_LOG(key);
        RegexObject* node = regex::new_expr();
        node->mText    = make_slice(backslash.begin(), xpos);
        node->mContext = key;
        node->mSplice  = mSplice;
        node->mExpansion = mExpansion;

        for (const char* ptr = str->begin(); ptr != str->end(); ++ptr)
        {
            node->or_append(node->mText, make_slice(ptr, 1), true);
            LOG_APPEND2(*ptr, ", ");
        }
        LOG_APPEND('\n');
        and_append(node, false);
    }
    else
    {
        SimpleStr* context = regex::new_cstring();
        context->append(*normal);
        context->append(key);

        NEW_NODE_LOG(*context);
        RegexObject* node = regex::new_expr();
        node->mText    = make_slice(mark, xpos);
        node->mContext = *context;
        node->mSplice  = mSplice;
        node->mExpansion = mExpansion;

        for (BOOST_AUTO(iter, charset.begin()); iter != charset.end(); ++iter)
        {
            SimpleStr* str = regex::new_cstring();
            str->append(*normal);
            str->push_back(*iter);

            node->or_append(node->mText, *str, true);
            LOG_APPEND2(*str, ", ");
        }
        LOG_APPEND('\n');
        and_append(node, false);
        normal = regex::new_cstring();
    }
    
    return xpos;
}

const void* RegexObject::new_backslash(const Slice& backslash, SimpleStr* &normal, const uint8_t* &mark)
{
    Slice key;
    if (regex::is_special(backslash, key))
    {
        return new_special(backslash, normal, mark, key);
    }

    const char* xpos = backslash.begin();
    uint8_t     c;
    uint8_t     ci = 128u;
    Slice context;

    if (backslash.starts_with("\\\\"))
    {
        ci = '\\';
        xpos += detail::length_of("\\\\");
    }
    else if (regex::is_nonprint(backslash, key, c))
    {
        ci = c;
        xpos += key.length();
    }
    else if (is_octal(backslash[1]))
    { // \0dd | \ddd: disable
        if (backslash.starts_with("\\0"))
        {
            if (backslash.length() < detail::length_of("\\0dd")
                || !is_octal(backslash[2])
                || !is_octal(backslash[3]))
            {
                SMART_ASSERT(backslash.length() >= detail::length_of("\\0dd")
                                && is_octal(backslash[2]) && is_octal(backslash[3]))
                            ("backslash", backslash);
                return nullptr;
            }

            int64_t dd = octtoi(backslash.substr(detail::length_of("\\0"), detail::length_of("dd")));
            if (dd < 0 || integer_cast<uint64_t>(dd) >= 128ul)
            {
                SMART_ASSERT(dd >= 0 && integer_cast<uint64_t>(dd) < 128ul)("backslash", backslash)("octal", dd);
                return nullptr;
            }

            ci = integer_cast<uint8_t>(dd);
            xpos += detail::length_of("\\0dd");
        }
        else
        { // might be a back reference
            Slice str = backslash.substr(detail::length_of('\\'));
            for (xpos = str.begin(); xpos != str.end(); ++xpos)
            {
                if (!is_octal(*xpos))
                    break;
            }

            context = make_slice(backslash.begin(), xpos);
            xpos = context.end();
        }
    }
    else if (isdigit(backslash[1]) || backslash.starts_with("\\g"))
    { // \n | \gn | \g{n} | \g{-n} | \g{name}
        Slice str = backslash;

        str.remove_prefix((str[1] == 'g') ? detail::length_of("\\g") : detail::length_of("\\"));
        if (str.empty())
        {
            SMART_ASSERT(!str.empty()).msg("invalid argument of [ \\g | \\n ]")("backslash", backslash);
            return nullptr;
        }
        else if (str.front() == '{')
        {
            int pos = str.find_first('}');
            if (pos == Slice::NPOS)
            {
                SMART_ASSERT(pos != Slice::NPOS).msg("invalid argument of [ \\g{n} | \\g{-n} | \\g{name} ]")("backslash", backslash);
                return nullptr;
            }

            context = make_slice(backslash.begin(), str.begin() + (pos + detail::length_of("}")));
            xpos = context.end();
        }
        else
        {
            for (xpos = str.begin(); xpos != str.end(); ++xpos)
            {
                if (!isdigit(*xpos))
                    break;
            }

            context = make_slice(backslash.begin(), xpos);
        }
    }
    else if (backslash.starts_with("\\o"))
    { // \o{ddd..}: disable
        if (backslash.length() < detail::length_of("\\o{ddd}"))
        {
            SMART_ASSERT(backslash.length() >= detail::length_of("\\o{ddd}"))("backslash", backslash);
            return nullptr;
        }

        Slice str = backslash.substr(detail::length_of("\\o"));
        if (str.front() != '{')
        {
            SMART_ASSERT(str.front() == '{');
            return nullptr;
        }

        for (xpos = str.begin(); xpos != str.end(); ++xpos)
        {
            if (*xpos != ',' && !is_octal(*xpos))
            {
                if (*xpos != '}')
                {
                    SMART_ASSERT(*xpos == '}')("str", str)("xpos", make_slice(xpos, str.end()));
                    return nullptr;
                }
                context = make_slice(backslash.begin(), xpos + detail::length_of("}"));
                xpos = context.end();
                break;
            }
        }

        if (context.empty())
        {
            SMART_ASSERT(!context.empty()).msg("invalid argument of \\o{ddd...}")("backslash", backslash);
            return nullptr;
        }
    }
    else if (backslash.starts_with("\\x"))
    { // \xhh | \x{hhh..}: disable
        if (backslash.length() < detail::length_of("\\xhh" /* or "\\x{}" */))
        {
            SMART_ASSERT(backslash.length() >= detail::length_of("\\xhh"))("backslash", backslash);
            return nullptr;
        }

        if (backslash[2] != '{')
        {
            if (!is_hex(backslash[2]) || !is_hex(backslash[3]))
            {
                SMART_ASSERT(is_hex(backslash[2]) && is_hex(backslash[3]))("backslash", backslash);
                return nullptr;
            }

            int64_t hh = hextoi(backslash.substr(detail::length_of("\\x"), detail::length_of("hh")));
            if (hh < 0 || integer_cast<uint64_t>(hh) >= 128ul)
            {
                SMART_ASSERT(hh >= 0 && integer_cast<uint64_t>(hh) < 128ul)("backslash", backslash)("hex", hh);
                return nullptr;
            }

            ci = integer_cast<uint8_t>(hh);
            xpos += detail::length_of("\\xhh");
        }
        else if (backslash.length() >= detail::length_of("\\x{hhh}"))
        {
            Slice str = backslash.substr(detail::length_of("\\x{"));
            for (xpos = str.begin(); xpos != str.end(); ++xpos)
            {
                if (*xpos != ',' && !is_hex(*xpos))
                {
                    if (*xpos != '}')
                    {
                        SMART_ASSERT(*xpos == '}')("str", str)("xpos", make_slice(xpos, str.end()));
                        return nullptr;
                    }
                    context = make_slice(backslash.begin(), xpos + + detail::length_of("}"));
                    xpos = context.end();
                    break;
                }
            }

            if (context.empty())
            {
                SMART_ASSERT(!context.empty()).msg("invalid argument of \\xhh or \\x{hhh...}")("backslash", backslash);
                return nullptr;
            }
        }
        else
        {
            SMART_ASSERT(backslash.length() >= detail::length_of("\\x{hhh}"))("backslash", backslash);
            return nullptr;
        }
    }
    else if (backslash.starts_with("\\k"))
    { // \k<name> | \k'name': all disable
        Slice str = backslash;

        str.remove_prefix(detail::length_of("\\k"));
        if (str.empty())
        {
            SMART_ASSERT(!str.empty()).msg("invalid argument of \\k")("backslash", backslash);
            return nullptr;
        }

        const uint8_t sep = (str.front() == '<') ? '>' \
                            : ((str.front() == '{') ? '}' : '\'');
        int pos = str.find_first(sep);
        if (pos == Slice::NPOS)
        {
            SMART_ASSERT(pos != Slice::NPOS).msg("invalid argument of \\k")("backslash", backslash);
            return nullptr;
        }

        context = make_slice(backslash.begin(), str.begin() + (pos + detail::length_of("}")));
        xpos = context.end();
    }
    //else if (regex::is_metachar(backslash[1]))
    else
    {
        ci = static_cast<uint8_t>(backslash[1]);
        xpos += detail::length_of("\\." /* "\\*", "\\{" .etc */);
    }
    //else if (!regex::is_metachar(backslash[1]))
    //{
    //    SMART_ASSERT(regex::is_metachar(backslash[1]))("backslash", backslash);
    //    return nullptr;
    //}

    if (!(ci <= 128u && (!context.empty() || ci != 128u)))
    {
        SMART_ASSERT(ci <= 128u && (!context.empty() || ci != 128u))("c", ci)("context", context);
        return nullptr;
    }

    Slice quantifier = make_slice(xpos, backslash.end());
    if (!quantifier.empty())
    {
        bool disable = false;
        const void* next = find_quantifier(quantifier, disable);
        if (disable)
        {
            if (next > quantifier.end())
            {
                SMART_ASSERT(next <= quantifier.end())("quantifier", quantifier);
                return nullptr;
            }

            if (!normal->empty())
            {
                NEW_NODE_LOG(*normal);
                and_append(make_slice(mark, backslash.begin()), *normal, true);
                normal = regex::new_cstring();
                mark = (const uint8_t *)backslash.begin();
            }

            Slice text = make_slice(backslash.begin(), next);
            NEW_NODE_LOG(text);
            and_append(text, text, true, true);
            return next;
        }
    }
    
    if (ci != 128u)
    {
        normal->push_back(static_cast<uint8_t>(ci));
    }
    else
    {
        if (!normal->empty())
        {
            NEW_NODE_LOG(*normal);
            and_append(make_slice(mark, backslash.begin()), *normal, true);
            normal = regex::new_cstring();
            mark = (const uint8_t *)backslash.begin();;
        }
        NEW_NODE_LOG(context);
        and_append(make_slice(backslash.begin(), xpos), context, true, true);
    }

    return xpos;
}

const void* RegexObject::new_startpattern(const Slice& subpattern, SimpleStr* &normal, const uint8_t* &mark)
{
    int pos = subpattern.find_first(')');
    if (pos == Slice::NPOS)
    {
        SMART_ASSERT(pos != Slice::NPOS).msg("Opening parenthesis without a corresponding closing parenthesis");
        return nullptr;
    }

    Slice str = subpattern.substr(0, pos + detail::length_of(')'));
    Slice quantifier = make_slice(str.end(), subpattern.end());
    if (!quantifier.empty())
    {
        bool disable = false;
        const void* next = find_quantifier(quantifier, disable);
        if (disable)
        {
            if (next > quantifier.end())
            {
                SMART_ASSERT(next <= quantifier.end())("quantifier", quantifier);
                return nullptr;
            }

            if (!normal->empty())
            {
                NEW_NODE_LOG(*normal);
                and_append(make_slice(mark, subpattern.begin()), *normal, true);
                normal = regex::new_cstring();
                mark = (const uint8_t *)subpattern.begin();
            }

            Slice text = make_slice(subpattern.begin(), next);
            NEW_NODE_LOG(text);
            and_append(text, text, true, true);
            return next;
        }
    }

    if (str == "(*CR)")
    { // carriage return
        normal->push_back('\r');
        return str.end();
    }
    else if (str == "(*LF)")
    { // linefeed
        normal->push_back('\n');
        return str.end();
    }
    else if (str == "(*CRLF)")
    { // carriage return, followed by linefeed
        normal->append("\r\n");
        return str.end();
    }
    else if (str == "(*ANYCRLF)")
    { // any of the three above
        const Slice anycrlf[] = { "\r", "\n", "\r\n" };

        RegexObject* node = regex::new_expr();
        node->mContext = str;
        node->mSplice  = mSplice;
        node->mExpansion = mExpansion;

        if (normal->empty() || !mSplice)
        {
            if (!normal->empty())
            {
                NEW_NODE_LOG(*normal);
                and_append(make_slice(mark, str.begin()), *normal, true);
                normal = regex::new_cstring();
                mark = (const uint8_t *)str.begin();
            }

            node->mText = str;
            NEW_NODE_LOG(node->mContext);
            for (size_t i = 0; i < dimensionof(anycrlf); ++i)
            {
                node->or_append(str, anycrlf[i], true);
                LOG_APPEND2(anycrlf[i], ", ");
            }
            LOG_APPEND('\n');
        }
        else
        {
            SimpleStr* context = regex::new_cstring();
            context->append(*normal);
            context->append(str);
            node->mText = make_slice(mark, str.end());
            node->mContext = *context;
            LOG_APPEND(node->mContext);

            for (size_t i = 0; i < dimensionof(anycrlf); ++i)
            {
                SimpleStr* line = regex::new_cstring();
                line->append(*normal);
                line->append(anycrlf[i]);
                node->or_append(node->mText, *line, true);
                LOG_APPEND2(*line, ", ");
            }
            LOG_APPEND('\n');
            normal = regex::new_cstring();
            mark = (const uint8_t *)str.end();
        }

        and_append(node, false);
        return str.end();
    }
    else if (str == "(*NOTEMPTY)" || str == "(*NOTEMPTY_ATSTART)"
          || str == "(*NO_START_OPT)" || str == "(*NO_DOTSTAR_ANCHOR)"
          || str.starts_with("(*LIMIT_MATCH=") || str.starts_with("(*LIMIT_RECURSION=)"))
    {
        NEW_NODE_LOG(str);
        and_append(str, str, true, true);
        return str.end();
    }
    else if (str == "(*UTF)" || str == "(*UCP)")
    {
        SMART_ASSERT(str != "(*UTF)" && str != "(*UCP)").msg("Unicode property support")
                    ("str", subpattern);
        return nullptr;
    }
    else if (str == "(*NO_AUTO_POSSESS)")
    {
        SMART_ASSERT(str != "(*NO_AUTO_POSSESS)").msg("Disabling auto-possessification ")
                    ("str", subpattern);
        return nullptr;
    }
    else if (str == "(*NO_JIT)")
    {
        SMART_ASSERT(0).msg("Disabling JIT compilation");
        return nullptr;
    }
    else if (str == "(*ACCEPT)")
    {
        SMART_ASSERT(0).msg("Verbs that act immediately");
        return nullptr;
    }
    else if (str == "(*COMMIT)" || str == "(*THEN)")
    {
        SMART_ASSERT(0).msg("Verbs that act after backtracking");
        return nullptr;
    }
    else if (str == "(*FAIL)" || str == "(*F)")
    {
        SMART_ASSERT(0).msg("Lookahead assertions");
        return nullptr;
    }
    else if (str.starts_with("(*VERB"))
    {
        SMART_ASSERT(0).msg("Backtracking control");
        return nullptr;
    }
    else if (str.starts_with("(*MARK") || str.starts_with("(*PRUNE")
            || str.starts_with("(*THEN") || str == "(*SKIP)")
    {
        SMART_ASSERT(0).msg("Recording which path was taken");
        return nullptr;
    }
    else if (str == "(*ANY)" || str == "(*BSR_ANYCRLF)" || str == "(*BSR_UNICODE)")
    { // all Unicode newline sequences
        SMART_ASSERT(0).msg("all Unicode newline sequences")("str", subpattern);
        return nullptr;
    }
    else
    {
        SMART_ASSERT(0).msg("Unknown special start-of-pattern items")("str", subpattern);
        return nullptr;
    }
    
    return nullptr;
}

const void* RegexObject::new_subpattern(const Slice& subpattern, SimpleStr* &normal, const uint8_t* &mark)
{
    if (!normal->empty())
    {
        NEW_NODE_LOG(*normal);
        and_append(make_slice(mark, subpattern.begin()), *normal, true);
        normal = regex::new_cstring();
        mark = (const uint8_t *)subpattern.begin();
    }

    const void* block = find_subpattern(subpattern);
    if (block == subpattern.end())
    {
        SMART_ASSERT(block != subpattern.end()).msg("Opening parenthesis without a corresponding closing parenthesis")
                    ("ptr", subpattern);
        return nullptr;
    }

    Slice quantifier = make_slice((const char *)(block) + detail::length_of(')'), subpattern.end());
    if (!quantifier.empty())
    {
        bool disable = false;
        const void* next = find_quantifier(quantifier, disable);
        if (disable)
        {
            if (next > quantifier.end())
            {
                SMART_ASSERT(next <= quantifier.end())("quantifier", quantifier);
                return nullptr;
            }
            Slice text = make_slice(subpattern.begin(), next);
            NEW_NODE_LOG(text);
            and_append(text, text, true, true);        
            return next;
        }
    }

    const Slice text = make_slice(subpattern.begin(), (const char *)(block) + 1);
    NEW_BLOCK_LOG("(subpattern begin)", text);

    RegexObject* node = regex::new_expr();
    node->mText    = text;
    node->mContext = make_slice(subpattern.begin(), block);
    node->mSplice  = mSplice;
    node->mExpansion = mExpansion;

    bool sepi = false;
    Slice key;
    if (regex::is_complex_pattern(node->mContext, key))
    { // look ahead or behind or comment: (?=...)
        sepi = true;
        if (key == "(?!" || key == "(?<!")
        {
            node->mDisable = true;
        }
        else if (key == "(?#")
        {
            node->mContext.clear();
        }
        else if (is_any_of("P'<")(key[2]) && is_none_of("!=")(key.back()))
        {
            const uint8_t sep = (key[2] == '\'') ? '\'' : '>';
            int pos = node->mContext.find_first(sep, detail::length_of(key));
            if (pos == Slice::NPOS)
            {
                SMART_ASSERT(pos != Slice::NPOS)("subpattern", node->mContext)("key", key)("sep", sep);
                return nullptr;
            }

            node->mContext.remove_prefix(pos + 1);
        }
        else
        {
            node->mContext.remove_prefix(detail::length_of(key));
        }
    }
    else if (node->mContext.starts_with("(?"))
    { // options: (?im-sx) or (?group) or (?P=group)
        sepi = true;
        bool option = true;
        const char* ptr = node->mContext.begin() + detail::length_of("(?");
        for ( ; ptr != block; ++ptr)
        {
            if (*ptr == ')')
            {
                SMART_ASSERT(*ptr != ')')("subpattern", subpattern);
                return nullptr;
            }
            else if (*ptr == ':')
            {
                ++ptr;
                break;
            }

            if (!regex::is_option(*ptr))
            {
                sepi = true;
            }
        }
        node->mDisable = !option;
        if (!node->mDisable)
        {
            node->mContext = make_slice(ptr, block);
        }
    }
    else
    { // simple subpattern: (...)
        node->mContext.remove_prefix(detail::length_of('('));
    }

    if (node->mDisable || node->mContext.empty())
    {
        node->mDisable = true;
        NEW_NODE_LOG(text);
        and_append(text, text, true, true);
        return static_cast<const uint8_t *>(block) + 1;
    }
    else if (!node->compile())
    {
        SMART_ASSERT(0).msg("subpattern compile failed")("subpattern", node->mContext);
        return nullptr;
    }
    else
    {
        NEW_BLOCK_LOG("(subpattern end) ->", node->mContext);
        NEW_NODE_LOG(text);

        // if (sepi)
        // {
        //     RegexObject* bnode = regex::new_expr();
        //     bnode->mText    = make_slice(subpattern.begin(), node->mContext.begin());
        //     bnode->mSplice  = mSplice;
        //     bnode->mExpansion = mExpansion;
        //     and_append(bnode, true, false);
        //     node->mText = node->mContext;
        // }

        and_append(node, node->mTarget, node->mDisable);

        // if (sepi)
        // {
        //     RegexObject* enode = regex::new_expr();
        //     enode->mText    = ")";
        //     enode->mSplice  = mSplice;
        //     enode->mExpansion = mExpansion;
        //     and_append(enode, true, false);
        // }
    }
    
    return static_cast<const uint8_t *>(block) + 1;
}

const void* RegexObject::new_definition(const Slice& definition, SimpleStr* &normal, const uint8_t* &mark)
{
    bool utf = false;
    const void* block = find_definition(definition, &utf);
    if (block > definition.end())
    {
        SMART_ASSERT(block != definition.end())("ptr", definition);
        return nullptr;
    }

    bool disable = false;
    const void* next = nullptr;
    if (utf)
    {
        disable = true;
        next = (const char *)(block) + detail::length_of(']');
    }
    else
    {
        Slice quantifier = make_slice((const char *)(block) + detail::length_of(']'), definition.end());
        if (!quantifier.empty())
        {
            next = find_quantifier(quantifier, disable);
            if (next > quantifier.end())
            {
                SMART_ASSERT(!disable || next <= quantifier.end())("quantifier", quantifier);
                return nullptr;
            }
        }
    }

    if (disable)
    {
        if (!normal->empty())
        {
            NEW_NODE_LOG(*normal);
            and_append(make_slice(mark, definition.begin()), *normal, true);
            normal = regex::new_cstring();
            mark = (const uint8_t *)definition.begin();
        }

        Slice text = make_slice(definition.begin(), next);
        NEW_NODE_LOG(text);
        and_append(text, text, true, true);
        return next;
    }

    bool opposite = false;
    RegexObject* node = regex::new_expr();
    node->mText    = make_slice(definition.begin(), static_cast<const uint8_t *>(block) + 1);
    node->mContext = make_slice(definition.begin() + 1, block);
    node->mSplice  = mSplice;
    node->mExpansion = mExpansion;
    if (node->mContext.empty())
    {
        SMART_ASSERT(!node->mContext.empty())("definition", definition)("context", node->mContext);
        return static_cast<const uint8_t *>(block) + 1;
    }
    else if (!mExpansion)
    {
        if (!normal->empty())
        {
            NEW_NODE_LOG(*normal);
            and_append(make_slice(mark, definition.begin()), *normal, true);
            normal = regex::new_cstring();
            mark = (const uint8_t *)definition.begin();
        }

        NEW_NODE_LOG(node->mContext);
        and_append(node, true, true);
        return static_cast<const uint8_t *>(block) + 1;
    }
    else if (node->mContext.front() == '^')
    {
        opposite = true;
    }

    CharSet charset;
    const uint8_t* xpos = (const uint8_t *)(node->mContext.begin());
    const uint8_t* end  = (const uint8_t *)(node->mContext.end());

    while (xpos != end)
    {
        if (is_none_of("\\[")(*xpos))
        {
            if (end - xpos >= detail::length_of("x-y") && xpos[1] == '-')
            { // range with characters: x-y
                uint8_t br = xpos[0]; // begin of range
                uint8_t er = xpos[2]; // end of range
                if (er == '\\' || er <= br)
                {
                    SMART_ASSERT(er == '\\' || er <= br)("pattern", node->mContext)("xpos", make_slice(xpos, end));
                    return nullptr;
                }

                for (uint8_t i = br; i <= er; ++i)
                {
                    charset.insert(i);
                }

                xpos += detail::length_of("x-y");
                continue;
            }

            charset.insert(*xpos++);
            continue;
        }

        Slice key;
        uint8_t     c;
        Slice special = make_slice(xpos, end);
        if (special.starts_with("\\\\"))
        { // '\'
            charset.insert('\\');
            xpos += detail::length_of("\\\\");
        }
        else if (special.starts_with("\\b"))
        {
            charset.insert(0x08);
            xpos += detail::length_of("\\b");
        }
        else if (regex::is_nonprint(special, key, c))
        {
            charset.insert(c);
            xpos += key.length();
            if (key == "\\c") ++xpos;
        }
        else if (is_octal(special[1]))
        { // \0dd | \ddd: disable
            if (special[1] != '0')
            {
                SMART_ASSERT(special[1] == '0').msg("invalid argument of \\ddd")("special", special);
                return nullptr;
            }

            if (special.length() < detail::length_of("\\0dd") || !is_octal(special[2]) || !is_octal(special[3]))
            {
                SMART_ASSERT(special.length() >= detail::length_of("\\0dd") && is_octal(special[2]) && is_octal(special[3]))
                            ("special", special);
                return nullptr;
            }

            int64_t dd = octtoi(special.substr(detail::length_of("\\0"), detail::length_of("dd")));
            if (dd < 0 || integer_cast<uint64_t>(dd) >= 128ul)
            {
                SMART_ASSERT(dd >= 0 && integer_cast<uint64_t>(dd) < 128ul)("special", special)("octal", dd);
                return nullptr;
            }

            charset.insert(integer_cast<uint8_t>(dd));
            xpos += detail::length_of("\\0dd");
        }
        else if (special.starts_with("\\o"))
        { // \o{ddd..}: disable
            SMART_ASSERT(0).msg("invalid argument of \\o{ddd...}")("special", special);
            return nullptr;
        }
        else if (special.starts_with("\\x"))
        { // \xhh or \x{hhh..}: disable
            if (special.length() < detail::length_of("\\xhh") || !is_hex(special[2]) || !is_hex(special[3]))
            {
                SMART_ASSERT(special.length() >= detail::length_of("\\xhh") && is_hex(special[2]) && is_hex(special[3]))
                            ("special", special);
                return nullptr;
            }

            int64_t hh = hextoi(special.substr(detail::length_of("\\x"), detail::length_of("hh")));
            if (hh < 0 || integer_cast<uint64_t>(hh) >= 128ul)
            {
                SMART_ASSERT(hh >= 0 && integer_cast<uint64_t>(hh) < 128ul)("special", special)("hex", hh);
                return nullptr;
            }

            xpos += detail::length_of("\\xhh");

            Slice str = make_slice(xpos, end);
            if (str.starts_with("-\\x"))
            { // range with hex characters: \xhh-\xhh
                if (str.length() < detail::length_of("-\\xhh") || !is_hex(str[3]) || !is_hex(str[4]))
                {
                    SMART_ASSERT(str.length() >= detail::length_of("-\\xhh") && is_hex(str[3]) && is_hex(str[4]))
                                ("str", str);
                    return nullptr;
                }

                int64_t hhe = hextoi(str.substr(detail::length_of("-\\x"), detail::length_of("hh")));
                if (hhe < 0 || integer_cast<uint64_t>(hhe) >= 128ul || hhe <= hh)
                {
                    SMART_ASSERT(hhe >= 0 && integer_cast<uint64_t>(hhe) < 128ul)
                                ("special", special)("str", str)("hex2", hhe)("hex1", hh);
                    return nullptr;
                }

                uint8_t br = integer_cast<uint8_t>(hh);  // begin of range
                uint8_t er = integer_cast<uint8_t>(hhe); // end of range
                for (uint8_t i = br; i <= er; ++i)
                {
                    charset.insert(i);
                }

                xpos += detail::length_of("-\\xhh");
                continue;
            }
            else
            {
                charset.insert(integer_cast<uint8_t>(hh));
            }
        }
        else if (special.starts_with("\\Q"))
        { // \Q...\E
            int be = special.find_first("\\E");
            if (be == Slice::NPOS)
            {
                SMART_ASSERT(be != Slice::NPOS)("special", special);
                return nullptr;
            }

            Slice context = special.substr(0, be);
            context.remove_prefix(detail::length_of("\\Q"));
            for (const char* ptr = context.begin(); ptr != context.end(); ++ptr)
            {
                charset.insert(static_cast<uint8_t>(*ptr));
            }

            xpos = (const uint8_t *)(context.end()) + detail::length_of("\\E");
        }
        else_if_char_types(special, "\\d", any_of_digit)
        else_if_char_types(special, "\\D", not_of_digit)
        else_if_char_types(special, "\\s", any_of_space)
        else_if_char_types(special, "\\S", not_of_space)
        else_if_char_types(special, "\\h", any_of_blank)
        else_if_char_types(special, "\\H", not_of_blank)
        else_if_char_types(special, "\\v", any_of_vertical)
        else_if_char_types(special, "\\V", not_of_vertical)
        else_if_char_types(special, "\\w", any_of_word)
        else_if_char_types(special, "\\W", not_of_word)
        else_if_char_types(special, "[:digit:]",  any_of_digit)
        else_if_char_types(special, "[:^digit:]", not_of_digit)
        else_if_char_types(special, "[:space:]",  any_of_space)
        else_if_char_types(special, "[:^space:]", not_of_space)
        else_if_char_types(special, "[:blank:]",  any_of_blank)
        else_if_char_types(special, "[:^blank:]", not_of_blank)
        else_if_char_types(special, "[:word:]",   any_of_word)
        else_if_char_types(special, "[:^word:]",  not_of_word)
        else_if_named_charset(special, alnum)  // letters and digits
        else_if_named_charset(special, alpha)  // letters
        else_if_named_charset(special, ascii)  // character codes 0 - 127
        else_if_named_charset(special, cntrl)  // control characters
        else_if_named_charset(special, graph)  // printing characters, excluding space
        else_if_named_charset(special, lower)  // lower case letters
        else_if_named_charset(special, upper)  // upper case letters
        else_if_named_charset(special, print)  // printing characters, including space
        else_if_named_charset(special, punct)  // printing characters, excluding letters and digits and space
        else_if_named_charset(special, xdigit) // hexadecimal digits
        else if (regex::is_metachar(special[1]))
        {
            charset.insert(static_cast<uint8_t>(special[1]));
            xpos += detail::length_of("\\." /* "\\*", "\\{" .etc */);
        }
        else
        {
            SMART_ASSERT(regex::is_metachar(special[1]))("special", special);
            return nullptr;
        }
    }

    //SMART_ASSERT(charset.size() < 128ul).msg("are you sure?")("definition", definition);
    if (charset.empty())
    {
        SMART_ASSERT(!charset.empty())("pattern", definition);
        return nullptr;
    }
    else if (opposite)
    {
        CharSet negative;
        for (uint8_t i = 0u; i < 128u; ++i)
        {
            if (charset.find(i) == charset.end())
            {
                negative.insert(i);
            }
        }
        charset.swap(negative);
    }

    if (charset.size() == 1)
    {
        normal->push_back(*charset.begin());
    }
    else if (normal->empty() || !mSplice)
    {
        if (!normal->empty())
        {
            NEW_NODE_LOG(*normal);
            and_append(make_slice(mark, definition.begin()), *normal, true);
            normal = regex::new_cstring();
            mark = (const uint8_t *)definition.begin();
        }

        NEW_NODE_LOG(node->mContext);

        SimpleStr* str = regex::new_cstring();
        for (BOOST_AUTO(iter, charset.begin()); iter != charset.end(); ++iter)
        {
            str->push_back(*iter);
        }

        for (const char* ptr = str->begin(); ptr != str->end(); ++ptr)
        {
            node->or_append(node->mText, make_slice(ptr, 1), true);
            LOG_APPEND2(*ptr, ", ");
        }
        LOG_APPEND('\n');
        and_append(node, false);
    }
    else
    {
        SimpleStr* context = regex::new_cstring();
        context->append(*normal);
        context->push_back('[');
        context->append(node->mContext);
        context->push_back(']');
        node->mText    = make_slice(mark, xpos);
        node->mContext = *context;
        NEW_NODE_LOG(node->mContext);

        for (BOOST_AUTO(iter, charset.begin()); iter != charset.end(); ++iter)
        {
            SimpleStr* str = regex::new_cstring();
            str->append(*normal);
            str->push_back(*iter);
            node->or_append(node->mText, *str, true);
            LOG_APPEND2(*str, ", ");
        }
        LOG_APPEND('\n');
        and_append(node, false);
        normal = regex::new_cstring();
        mark = (const uint8_t *)definition.begin();
    }

    return static_cast<const uint8_t *>(block) + 1;
}

// ---------------------------------------------
// | 优先权   |   符号                         |
// | 最高     |    \                           |
// | 高       |    ( )、(?: )、(?= )、[ ]      |
// | 中       |    *、+、?、{n}、{n,}、{m,n}   |
// | 低       |    ^、$、中介字符              |
// | 最低     |    |                           |
// ---------------------------------------------
bool RegexObject::compile()
{
    SliceList logic_or;

    if (!split(logic_or))
    {
        SMART_ASSERT(0).msg("split failed")("pattern", mContext);
        return false;
    }

    LOG_APPEND3("(logic_or){", logic_or.size(), "} -> [");
    for (BOOST_AUTO(iter, logic_or.begin()); iter != logic_or.end(); )
    {
        LOG_APPEND3('\"', *iter, '\"');
        if (++iter != logic_or.end())
        {
            LOG_APPEND(", ");
        }
    }
    LOG_APPEND3("] @", __LINE__, '\n');

    SimpleStr* normal = regex::new_cstring();
    for (BOOST_AUTO(iter, logic_or.begin()); iter != logic_or.end(); ++iter)
    {
        const Slice& condition = *iter;
        const uint8_t* ptr = (const uint8_t *)(condition.begin());
        const uint8_t* end = (const uint8_t *)(condition.end());

        RegexObject* node = regex::new_expr();
        node->mText    = *iter;
        node->mContext = condition;
        node->mSplice  = mSplice;
        node->mExpansion = mExpansion;

        const uint8_t* mark = nullptr;
        while (ptr < end)
        {
            Slice current = make_slice(ptr, end);
            if (current.empty())
            {
                SMART_ASSERT(!current.empty())("pattern", condition);
                return nullptr;
            }

            if (normal->empty())
            {
                mark = ptr;
            }

            if (*ptr == '\\')
            {
                const void* next = node->new_backslash(current, normal, mark);
                if (!next)
                {
                    SMART_ASSERT(next != nullptr)("pattern", condition)("ptr", current);
                    return false;
                }

                ptr = static_cast<const uint8_t *>(next);
            }
            else if (*ptr == '(')
            {
                const void* next = nullptr;

                if (ptr + 1 < end && ptr[1] == '*')
                {
                    next = node->new_startpattern(current, normal, mark);
                }
                else
                {
                    next = node->new_subpattern(current, normal, mark);
                }

                if (!next)
                {
                    SMART_ASSERT(next != nullptr)("pattern", condition)("ptr", current);
                    return false;
                }

                ptr = static_cast<const uint8_t *>(next);
            }
            else if (*ptr == '[')
            {
                const void* next = node->new_definition(current, normal, mark);
                if (!next)
                {
                    SMART_ASSERT(next != nullptr)("pattern", condition)("ptr", current);
                    return false;
                }

                ptr = static_cast<const uint8_t *>(next);
            }
            else if (is_any_of("^$")(*ptr))
            {
                if (!(*ptr == '$' || ((const char *)ptr == condition.begin() && condition.front() != '[')))
                {
                    SMART_ASSERT(*ptr == '$' || ((const char *)ptr == condition.begin() && condition.front() != '['))
                                ("pattern", condition)("ptr", current);
                    return nullptr;
                }

                if (!normal->empty())
                {
                    SMART_ASSERT(mark != nullptr)("pattern", condition)("ptr", current);
                    NEW_NODE_LOG(*normal);
                    node->and_append(make_slice(mark, ptr), *normal, true, false);
                    normal = regex::new_cstring();
                    mark = nullptr;
                }

                const uint8_t* xpos = ptr + detail::length_of('^');
                while (xpos != end)
                {
                    if (is_none_of("^$")(*xpos))
                        break;
                    ++xpos;
                }

                Slice text = make_slice(ptr, xpos);
                NEW_NODE_LOG(text);
                node->and_append(text, text, true, true);
                ptr = xpos;
            }
            else
            {
                if (static_cast<uint8_t>(*ptr) < 128u && *ptr != '-' && !regex::is_quantifier(*ptr) && regex::is_metachar(*ptr))
                {
                    SMART_ASSERT(*ptr != '-' && !regex::is_quantifier(*ptr) && regex::is_metachar(*ptr))
                                ("pattern", condition)("ptr", current);
                    return false;
                }

                const void* next = node->new_literal(current, normal, mark);
                if (!next)
                {
                    SMART_ASSERT(next != nullptr)("pattern", condition)("ptr", current);
                    return false;
                }

                ptr = static_cast<const uint8_t *>(next);
            }
        }

        if (ptr != end)
        {
            SMART_ASSERT(ptr == end)("pattern", condition);
            return nullptr;
        }

        if (!normal->empty())
        {
            NEW_NODE_LOG(*normal);
            node->and_append(make_slice(mark, ptr), *normal, true);
            normal = regex::new_cstring();
        }

        if (condition.empty())
        {
            node->mTarget = true;
        }

        or_append(node, node->mTarget, node->mDisable);
        //NEW_BLOCK_LOG("(pattern) ->", condition);
    }

    return true;
}

bool RegexObject::merge()
{
    if (adjust_impl() && !mark_impl())
    {
        return (merge_impl() != nullptr);
    }
    return false;
}

bool RegexObject::unfold(bool all)
{
    if (!adjust_impl())
    {
        return false;
    }

    if (!all && mark_impl())
    {
        return false;
    }

    NodeList branchs;
    if (!unfold_impl(branchs, all))
    {
        return false;
    }

    for (BOOST_AUTO(iter, branchs.begin()); iter != branchs.end(); ++iter)
    {
        if (!(*iter)->merge())
            return false;
    }

    clear();
    mOr.swap(branchs);
    return true;
}

void RegexObject::get_and(SliceList& results)
{
    if (!mark_impl())
    {
        get_and_impl(results);
    }
}

void RegexObject::get_and(NodeList& results)
{
    get_and_impl(results);
}

void RegexObject::get_all(SliceList& results)
{
    if (!mark_impl())
    {
        get_all_impl(results);
    }
}

void RegexObject::get_branchs(NodeLists& results)
{
    get_branchs_impl(results);
}

RegexObject::NodePtr RegexObject::adjust_impl()
{
    if (mTarget)
    {
        if (!mOr.empty())
        {
            SMART_ASSERT(mOr.empty()).msg("invalid regexpr")("expr", *this);
            return nullptr;
        }
    }
    else
    {
        for (BOOST_AUTO(iter, mOr.begin()); iter != mOr.end(); ++iter)
        {
            *iter = (*iter)->adjust_impl();
            if (*iter == nullptr) return nullptr;
        }
    }

    if (!mAnd.empty())
    {
        for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
        {
            *iter = (*iter)->adjust_impl();
            if (*iter == nullptr) return nullptr;
        }

        NodeList mergelist;
        for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
        {
            mergelist.push_back(*iter);

            if (!(*iter)->mAnd.empty())
            {
                std::copy((*iter)->mAnd.begin(), (*iter)->mAnd.end(), std::back_inserter(mergelist));
                (*iter)->mAnd.clear();
            }
        }

        if (mergelist.size() != mAnd.size())
        {
            SMART_ASSERT(mergelist.size() > mAnd.size());
            mergelist.swap(mAnd);
        }
    }

    if (mOr.size() == 1)
    {
        if (mTarget)
        {
            SMART_ASSERT(!mTarget).msg("invalid regexpr")("expr", *this);
            return nullptr;
        }

        NodePtr front = mOr.front();
        mOr.clear();

        NodeList tmplist = mAnd;
        mAnd.clear();
        mAnd.push_back(front);
        std::copy(tmplist.begin(), tmplist.end(), std::back_inserter(front->mAnd));
    }

    if (!is_leaf())
    {
        if (mAnd.empty())
        {
            SMART_ASSERT(!mAnd.empty()).msg("invalid regexpr")("expr", *this);
            return nullptr;
        }

        NodePtr front = mAnd.front();
        if (mAnd.size() == 1)
        {
            mAnd.clear();
            swap(*front);
            return this;
        }

        if (!front->mAnd.empty())
        {
            SMART_ASSERT(front->mAnd.empty()).msg("invalid regexpr")("front", front)("expr", *this);
            return nullptr;
        }

        NodeList tmplist(mAnd.begin() + 1, mAnd.end());
        tmplist.swap(front->mAnd);
        mAnd.clear();
        return front;
    }

    return this;
}

bool RegexObject::mark_impl()
{
    if (!mOr.empty())
    {
        if (mTarget)
        {
            SMART_ASSERT(!mTarget).msg("invalid regexpr")("expr", *this);
            return true;
        }

        for (BOOST_AUTO(iter, mOr.begin()); iter != mOr.end(); ++iter)
        {
            if ((*iter)->mark_impl())
            {
                mDisable = true;
                break;
            }
        }
    }

    size_t cnt = 0;

    for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
    {
        if ((*iter)->mark_impl())
        {
            ++cnt;
        }
    }

    if (is_leaf())
    {
        mAndDisable = mDisable && (cnt == mAnd.size());
    }
    else
    {
        mAndDisable = (cnt == mAnd.size());
    }

    return mAndDisable;
}

void RegexObject::get_all_impl(SliceList& results)
{
    if (!mAndDisable)
    {
        if (!mDisable)
        {
            if (mTarget)
            {
                if (!mContext.empty())
                {
                    results.push_back(mContext);
                }
            }
            else if (!mOr.empty())
            {
                for (BOOST_AUTO(iter, mOr.begin()); iter != mOr.end(); ++iter)
                {
                    (*iter)->get_all_impl(results);
                }
            }
        }

        for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
        {
            (*iter)->get_all_impl(results);
        }
    }
}

void RegexObject::get_and_impl(SliceList& results)
{
    if (!mAndDisable)
    {
        if (!mDisable)
        {
            if (mTarget && !mContext.empty())
            {
                results.push_back(mContext);
            }
        }

        for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
        {
            (*iter)->get_and_impl(results);
        }
    }
}

void RegexObject::get_and_impl(NodeList& results)
{
    results.push_back(this);

    for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
    {
        (*iter)->get_and_impl(results);
    }
}

void RegexObject::get_branchs_impl(NodeLists& results)
{
    if (mTarget)
    {
        if (!mContext.empty() || !mText.empty())
        {
            if (results.empty())
            {
                NodeList l;
                l.push_back(this);
                results.push_back(l);
            }
            else
            {
                for (BOOST_AUTO(iter, results.begin()); iter != results.end(); ++iter)
                {
                    iter->push_back(this);
                }
            }
        }
    }

    if (!mOr.empty())
    {
        NodeLists tmplist;
        tmplist.swap(results);

        NodeLists news;
        for (BOOST_AUTO(iter, mOr.begin()); iter != mOr.end(); ++iter)
        {
            news.clear();
            (*iter)->get_branchs_impl(news);

            for (BOOST_AUTO(s, news.begin()); s != news.end(); ++s)
            {
                if (tmplist.empty())
                {
                    results.push_back(*s);
                }
                else
                {
                    for (BOOST_AUTO(l, tmplist.begin()); l != tmplist.end(); ++l)
                    {
                        results.push_back(*l);
                        results.back().insert(results.back().end(), s->begin(), s->end());
                    }
                }
            }
        }
    }

    for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
    {
        (*iter)->get_branchs_impl(results);
    }
}

RegexObject::NodePtr RegexObject::merge_impl()
{
    if (mTarget)
    {
        if (!mOr.empty())
        {
            SMART_ASSERT(mOr.empty()).msg("invalid regexpr")("expr", *this);
            return nullptr;
        }
    }
    else
    {
        for (BOOST_AUTO(iter, mOr.begin()); iter != mOr.end(); ++iter)
        {
            *iter = (*iter)->merge_impl();
            if (*iter == nullptr) return nullptr;
        }
    }

    if (!mAnd.empty())
    {
        for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
        {
            *iter = (*iter)->merge_impl();
            if (*iter == nullptr) return nullptr;
        }

        NodeList tmplist = mAnd;
        mAnd.clear();

        SimpleStr* normal = nullptr;
        SimpleStr* text = nullptr;
        if (mTarget && !mDisable)
        {
            if (!mContext.empty())
            {
                normal = regex::new_cstring();
                normal->append(mContext);
                mContext.clear();
            }

            if (!mText.empty())
            {
                text = regex::new_cstring();
                text->append(mText);
                mText.clear();
            }
        }

        for (BOOST_AUTO(iter, tmplist.begin()); iter != tmplist.end(); ++iter)
        {
            const NodePtr node = *iter;
            if (node->mTarget && !node->mDisable)
            {
                if (!node->mContext.empty())
                {
                    if (!normal)
                    {
                        normal = regex::new_cstring();
                    }
                    normal->append(node->mContext);
                }

                if (!node->mText.empty())
                {
                    if (!text)
                    {
                        text = regex::new_cstring();
                    }
                    text->append(node->mText);
                }
            }
            else
            {
                if (normal != nullptr)
                {
                    SMART_ASSERT(text);
                    if (mText.empty())
                    {
                        mContext = *normal;
                        mText = *text;
                    }
                    else
                    {
                        and_append(*text, *normal, true);
                    }

                    normal = nullptr;
                }
                
                and_append(node, node->mTarget, node->mDisable);
            }
        }

        if (normal != nullptr)
        {
            SMART_ASSERT(text);
            if (mText.empty())
            {
                mContext = *normal;
                mText = *text;
            }
            else
            {
                and_append(*text, *normal, true);
            }

            normal = nullptr;
        }
    }

    return this;
}

RegexObject::NodePtr RegexObject::clone()
{
    NodePtr node = regex::new_expr();

    node->mDisable    = mDisable;
    node->mAndDisable = mAndDisable;
    node->mTarget     = mTarget;
    node->mSplice     = mSplice;
    node->mExpansion  = mExpansion;
    node->mContext    = mContext;
    node->mText       = mText;

    if (!mAnd.empty())
    {
        for (BOOST_AUTO(iter, mAnd.begin()); iter != mAnd.end(); ++iter)
        {
            node->mAnd.push_back((*iter)->clone());
        }
    }

    if (!mOr.empty())
    {
        for (BOOST_AUTO(iter, mOr.begin()); iter != mOr.end(); ++iter)
        {
            node->mOr.push_back((*iter)->clone());
        }
    }

    return node;
}

void RegexObject::add_branch(NodeList& branchs, NodePtr node)
{
    if (branchs.empty())
    {
        branchs.push_back(node->clone());
    }
    else
    {
        for (BOOST_AUTO(branch, branchs.begin()); branch != branchs.end(); ++branch)
        {
            (*branch)->mAnd.push_back(node->clone());
        }
    }
}

bool RegexObject::unfold_impl(NodeList& branchs, bool all)
{
    NodeList and_list;

    and_list.swap(mAnd);

    if (mTarget || (!all && mDisable))
    {
        if (!(mOr.empty() || (!all && mDisable)))
        {
            SMART_ASSERT(mOr.empty() || (!all && mDisable)).msg("invalid regexpr")("expr", *this);
            return false;
        }
        add_branch(branchs, this);
    }
    else
    {
        int64_t sum = 0;
        for (BOOST_AUTO(cond, mOr.begin()); cond != mOr.end(); ++cond)
        {
            sum += (*cond)->get_branch_cnt();
        }

        const int64_t base = branchs.empty() ? 1 : branchs.size();
        if (sum * base <= REGEX_MAX_NODES)
        {
            NodeList new_branchs;
            for (BOOST_AUTO(cond, mOr.begin()); cond != mOr.end(); ++cond)
            {
                NodeList tmplist;
                if (!(*cond)->unfold_impl(tmplist, all))
                    return false;

                std::copy(tmplist.begin(), tmplist.end(), std::back_inserter(new_branchs));
                const size_t count = branchs.empty() ? new_branchs.size() : (branchs.size() * new_branchs.size());
                if (count > REGEX_MAX_NODES)
                    return false;
            }
            mOr.clear();

            if (branchs.empty())
            {
                branchs.swap(new_branchs);
            }
            else
            {
                NodeList tmplist;
                for (BOOST_AUTO(branch, branchs.begin()); branch != branchs.end(); ++branch)
                {
                    for (BOOST_AUTO(iter, new_branchs.begin()); iter != new_branchs.end(); ++iter)
                    {
                        const bool last = (iter + 1 == new_branchs.end());

                        NodePtr node = last ? (*branch) : (*branch)->clone();
                        NodePtr cond = last ? (*iter)   : (*iter)->clone();

                        if (!cond->mOr.empty())
                        {
                            SMART_ASSERT(cond->mOr.empty()).msg("invalid expr")("expr", *cond);
                            return false;
                        }

                        if (cond->mTarget)
                        {
                            node->mAnd.push_back(cond);
                        }

                        std::copy(cond->mAnd.begin(), cond->mAnd.end(), std::back_inserter(node->mAnd));
                        cond->mAnd.clear();
                        tmplist.push_back(node);
                        if (tmplist.size() > REGEX_MAX_NODES)
                            return false;
                    }
                }
                branchs.swap(tmplist);
            }
        }
        else
        {
            add_branch(branchs, this);
        }
    }

    for (BOOST_AUTO(next, and_list.begin()); next != and_list.end(); ++next)
    {
        const int64_t base = branchs.empty() ? 1 : branchs.size();
        if ((*next)->get_branch_cnt() * base > REGEX_MAX_NODES)
        {
            add_branch(branchs, *next);
            continue;
        }

        if (!(*next)->unfold_impl(branchs, all))
            return false;

        if (branchs.size() > REGEX_MAX_NODES)
            return false;
    }

    return true;
}

int64_t RegexObject::get_branch_cnt()
{
    int64_t cnt = mOr.empty() ? 1 : 0;

    for (BOOST_AUTO(next, mOr.begin()); next != mOr.end(); ++next)
    {
        cnt += (*next)->get_branch_cnt();
    }

    for (BOOST_AUTO(next, mAnd.begin()); next != mAnd.end(); ++next)
    {
        cnt *= (*next)->get_branch_cnt();
    }

    return cnt;
}

void RegexObject::find_fold_nodes(NodeList& nodes)
{
    int64_t sum = 0;
    for (BOOST_AUTO(next, mOr.begin()); next != mOr.end(); ++next)
    {
        sum += (*next)->get_branch_cnt();
    }

    if (sum > REGEX_MAX_NODES)
    {
        std::copy(mOr.begin(), mOr.end(), std::back_inserter(nodes));
    }

    for (BOOST_AUTO(next, mAnd.begin()); next != mAnd.end(); ++next)
    {
        if ((*next)->get_branch_cnt() > REGEX_MAX_NODES)
        {
            continue;
        }
        (*next)->find_fold_nodes(nodes);
    }
}
