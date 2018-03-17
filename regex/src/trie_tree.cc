
#include "common/smart_assert.h"
#include "common/string-inl.h"
#include "trie_tree.h"
#include <set>

#ifdef MINIMUM_LETTER_SET
    #undef  MIN_LETTER
    #undef  MAX_LETTER
    #define MIN_LETTER             (32)
    #define MAX_LETTER             (127)
    #define MAX_LETTER_NUM         ((MAX_LETTER) - (MIN_LETTER) + 1)
    #define LETTER_INSET(x)        ((x) >= 32 && (x) < 128)
    #define get_state(node, x)     ((node)->mState[(x) - 32])
    #define get_move(node, x)      ((node)->mMove [(x) - 32])
#else
    #define MAX_LETTER_NUM         ((MAX_LETTER) - (MIN_LETTER) + 1)
    #define LETTER_INSET(x)        ((x) < MAX_LETTER_NUM)
    #define get_state(node, x)     ((node)->mState[(x)])
    #define get_move(node, x)      ((node)->mMove [(x)])
#endif

#define NBSP                        static_cast<uint8_t>(160)
#define TOUPPER(x)                 (ToUpper[static_cast<uint8_t>(x)])
#define endof_state(node)          ((node)->output() != nullptr)
#define offsetof_tx(pn, e, s)      (integer_cast<int>((e) - (pn).begin() - (s)->mLength + 1))
#define whole_word(pn, sp, s)      (((sp) + 1 == (pn).end() || IsSpace[static_cast<uint8_t>((sp)[1])]) \
                                     && ((sp) == (pn).begin() + (s)->mLength -1 || IsSpace[static_cast<uint8_t>((sp)[-(s)->mLength])]))
#define output_state(pn, sp, s)    (endof_state(s) && ((s)->mMode == 0 || whole_word(pn, sp, s)))
#define get_storage()              ((ObjectPool<detail::TrieNode> *)(mStorage))
#define get_wordpool()             ((ObjectPool<detail::EndNode>  *)(mWStorage))

#if (MAX_LETTER_NUM < 256)
    #define go_state(node, x)      (LETTER_INSET(x) ? get_state((node), (x)) : nullptr)
    #define go_state_fail(node, x) (LETTER_INSET(x) ? (get_state((node), (x)) == nullptr) : true)
#else
    #define go_state(node, x)      (get_state((node), (x)))
    #define go_state_fail(node, x) (get_state((node), (x)) == nullptr)
#endif

#define queue_empty(q) ((q).head == nullptr)

#define queue_push(q, e) do { \
    (e)->mNext = nullptr; \
    if ((q).head == nullptr) {\
        (q).head = (q).tail = (e); \
    } else { \
        (q).tail->mNext = (e); \
        (q).tail = (e); \
    } \
} while (0)

#define queue_pop(q, e) do { \
    if ((q).head == nullptr) { \
        (e) = nullptr; \
    } else { \
        (e) = (q).head; \
        (q).head = (e)->mNext; \
    } \
} while (0)

#define ishex(x) VALID_HEX(x)

#define GET_LETTER(x, sp, e) x = ((decode == DecodeType::kNone) \
                                    ? TOUPPER(*(sp)) \
                                    : ((decode == DecodeType::kUrlDecodeUni) \
                                        ? detail::get_unicode(mark, sp, e) \
                                        : detail::get_htmlentry(mark, sp, e)))

namespace detail
{
    typedef Slice::pointer pointer;

    struct TrieNode
    {
    private:
        struct beeoutput
        {
            const TrieNode* mNode;
            const TrieNode* mNext;
        };

        void set_output(const TrieNode* out)
        {
            mOutput.mNode = out;
            SMART_ASSERT(output_next() == nullptr);
        }

        void set_output_next(const TrieNode* next)
        {
            SMART_ASSERT(output());
            mOutput.mNext = next;
        }

        TrieNode* output_next()
        {
            return const_cast<TrieNode *>(mOutput.mNext);
        }

    public:
        void reset(TrieNode* parent, TNID id, pointer suffix, int32_t len, bool whole_word)
        {
			mOutput.mNext = nullptr;
			mOutput.mNode = nullptr;
            mParent  = parent;
            mFail    = nullptr;
            mNext    = nullptr;
            mPiece   = (suffix != nullptr) ? (suffix - (len - 1)) : nullptr;
            mEndof   = TK_INVAILD;
            mID      = id;
            mLength  = len;
            mMode    = whole_word ? 1 : 0;
            bzero(mState, sizeof(mState));
            bzero(mMove,  sizeof(mMove));
        }

        Slice key() const
        {
            return Slice(mPiece, mLength);
        }

        std::pair<Slice, TNID> data() const
        {
            return std::make_pair(key(), mEndof);
        }

        const TrieNode* output() const
        {
            return mOutput.mNode;
        }

        const TrieNode* output_next() const
        {
            return mOutput.mNext;
        }

        void add_output(const TrieNode* out)
        {
            if (output() == nullptr)
            {
                set_output(out);
            }
            else
            {
                SMART_ASSERT(out != output());

                TrieNode* node = this;
                while (node && node->output_next())
                {
                    SMART_ASSERT(node->output());
                    if (out == node->output())
                        break;
                    node = node->output_next();
                }

                SMART_ASSERT(node && node->output());
                if (out != node->output())
                {
                    node->set_output_next(out);
                }
            }
        }

        void merge_output(const TrieNode* out)
        {
            add_output(out);
        }

    public:
        TrieNode*  mState[MAX_LETTER_NUM];
        TrieNode*  mMove [MAX_LETTER_NUM];
        beeoutput mOutput;
        pointer   mPiece;
        TNID      mEndof;   // endof id
        TNID      mID;
        int32_t   mLength;
        uint16_t  mMode;
        uint16_t  mNotUsed;
        TrieNode*  mParent;
        TrieNode*  mFail;
        TrieNode*  mNext;    // used while construct the failure function
    };

    struct EndNode
    {
        const TrieNode* mCur;
        const EndNode*  mNext;
        int32_t         mLen;

        void reset(const TrieNode* cur, int32_t len, const EndNode* next)
        {
            SMART_ASSERT(cur && len > 0 && len <= cur->mLength);
            mCur  = cur;
            mLen  = len;
            mNext = next;
        }

        Slice key() const
        {
            return Slice(mCur->mPiece, mLen);
        }

        std::pair<Slice, TNID> data() const
        {
            return std::make_pair(key(), mCur->mEndof);
        }
    };

    uint8_t get_unicode(const char* &mark, const char* &sp, const char* ep);
    uint8_t get_htmlentry(const char* &mark, const char* &sp, const char* ep);
} // namespace detail

inline uint8_t detail::get_unicode(const char* &mark, const char* &sp, const char* ep)
{
    // if (mark && sp != mark)
    // {
    //     SMART_ASSERT(mark == sp + 2);
    //     uint8_t c = TOUPPER(detail::x2c((const uint8_t* )sp));
    //     mark = nullptr;
    //     sp += 2;
    //     return c;
    // }

    if (*sp == '+')
    {
        return ' ';
    }

    if (*sp == '%' && sp + 1 != ep)
    {
        if (TOUPPER(sp[1]) == 'U')
        {
            if (sp + 5 < ep && ishex(sp[2]) && ishex(sp[3]) && ishex(sp[4]) && ishex(sp[5]))
            {
                // We first make use of the lower byte here, ignoring the higher byte.
                uint8_t c = TOUPPER(detail::x2c((const uint8_t* )sp + 4));

                // Full width ASCII (ff01 - ff5e) needs 0x20 added
                if (c > 0x00 && c < 0x5f && TOUPPER(sp[2]) == 'F' && TOUPPER(sp[3]) == 'F')
                {
                    c = integer_cast<uint8_t>(c + 0x20);
                }

                //mark = sp + 6;
                sp += 5;
                return c;
            }
        }
        else if (sp + 2 < ep && ishex(sp[1]) && ishex(sp[2]))
        {
            uint8_t c = TOUPPER(detail::x2c((const uint8_t* )sp + 1));
            sp += 2;
            return c;
        }
    }

    return TOUPPER(*sp);
}

inline uint8_t detail::get_htmlentry(const char* &mark, const char* &sp, const char* ep)
{
    if (*sp == '&' && sp + 1 < ep)
    {
        if (sp + 2 < ep && sp[1] == '#')
        {
            if (ISDIGIT(sp[2]))
            {
                sp += 2;
                for (mark = sp + 1; mark != ep && ISDIGIT(*mark); ++mark)
                    ;
                const uint8_t c = (uint8_t)(atoi(make_slice(sp, mark)));
                if (mark == ep || *mark != ';')
                    --mark;
                sp = mark;
                return c;
            }
            else if (sp + 3 < ep && TOUPPER(sp[2]) == 'X' && ishex(sp[3]))
            {
                sp += 3;
                for (mark = sp + 1; mark != ep && ishex(*mark); ++mark)
                    ;
                const uint8_t c = (uint8_t)(hextoi(make_slice(sp, mark)));
                if (mark == ep || *mark != ';')
                    --mark;
                sp = mark;
                return c;
            }
        }
        else if (sp + 1 < ep && isalnum(sp[1]))
        {
            uint8_t c;
            Slice prefix = make_slice(sp, ep);
            if (prefix.istarts_with("&quot"))
            {
                c = '"';
                mark = sp + detail::length_of("&quot");
            }
            else if (prefix.istarts_with("&amp"))
            {
                c = '&';
                mark = sp + detail::length_of("&amp");
            }
            else if (prefix.istarts_with("&lt"))
            {
                c = '<';
                mark = sp + detail::length_of("&lt");
            }
            else if (prefix.istarts_with("&gt"))
            {
                c = '>';
                mark = sp + detail::length_of("&gt");
            }
            else if (prefix.istarts_with("&nbsp"))
            {
                c = NBSP;
                mark = sp + detail::length_of("&nbsp");
            }
            else
            {
                return TOUPPER(*sp);
            }
            
            if (mark == ep || *mark != ';')
                --mark;
            sp = mark;
            return c;
        }
    }

    return TOUPPER(*sp);
}

typedef detail::TrieNode* TrieNodePtr;
typedef detail::EndNode*  EndNodePtr;

typedef const detail::TrieNode* TrieNodeConstPtr;
typedef const detail::EndNode*  EndNodeConstPtr;

struct TrieNodeQueue
{
    TrieNodePtr head;
    TrieNodePtr tail;
};

TrieTree::iterator::iterator(uintptr_t pos)
  : mPos(pos)
  , mValue((pos != 0) ? ((EndNodeConstPtr)pos)->data() : value_type(Slice(), TK_INVAILD))
{

}

TrieTree::iterator& TrieTree::iterator::operator++()
{
    EndNodeConstPtr node = (EndNodeConstPtr)mPos;

    if (node != nullptr)
    {
        node = node->mNext;
        mPos = (uintptr_t)node;
        mValue = (node != nullptr) ? node->data() : value_type(Slice(), TK_INVAILD);
    }

    return *this;
}

TrieTree::TrieTree(uint32_t unit, bool lowercase)
  : mStorage((uintptr_t)(new ObjectPool<detail::TrieNode>(unit > 0 ? unit : 128)))
  , mWStorage((uintptr_t)(new ObjectPool<detail::EndNode>(unit > 0 ? unit : 256)))
  , mRoot((uintptr_t)(nullptr))
  , mWord((uintptr_t)(nullptr))
  , mNodes(1)
  , mWords(0)
  , mMinLength(0)
  , mLowercase(lowercase ? 1 : 0)
{
    detail::TrieNode& node = get_storage()->new_object();
    node.reset(nullptr, TK_INVAILD, nullptr, 0, false);
    mRoot = (uintptr_t)(&node);
}

TrieTree::~TrieTree()
{
    delete get_storage();
    delete get_wordpool();
}

size_t TrieTree::memory_size() const
{
    return mNodes * sizeof(detail::TrieNode);
}

void TrieTree::clear()
{
    get_storage()->clear();
    get_wordpool()->clear();

    detail::TrieNode& node = get_storage()->new_object();
    node.reset(nullptr, TK_INVAILD, nullptr, 0, false);
    mRoot = (uintptr_t)(&node);

    mWord  = (uintptr_t)(nullptr);
    mNodes = 1;
    mWords = 0;
    mMinLength = 0;
}

bool TrieTree::add(const Slice& word, TNID id, bool whole_word, AddCallBack callback, void* data)
{
    if (id == TK_INVAILD || word.empty())
    {
        SMART_ASSERT(id != TK_INVAILD && !word.empty());
        return false;
    }

    TrieNodePtr parent = (TrieNodePtr)mRoot;
    const detail::pointer last = word.end();
    for (detail::pointer xpos = word.begin(); xpos != last; ++xpos)
    {
        if (!LETTER_INSET(TOUPPER(*xpos)))
        {
            SMART_ASSERT(LETTER_INSET(TOUPPER(*xpos)))("c", static_cast<uint32_t>(TOUPPER(*xpos)));
            return false;
        }

        TrieNodePtr* state = &get_state(parent, TOUPPER(*xpos));
        if (*state == nullptr)
        {
            *state = &(get_storage()->new_object());
            (*state)->reset(parent, id, xpos, integer_cast<int>(xpos - word.begin() + 1), whole_word);
            ++mNodes;
            (*state)->mFail = (TrieNodePtr)mRoot;
        }

        if (xpos + 1 == last)
        {
            if ((*state)->mEndof == TK_INVAILD)
            {
                EndNodePtr endof = &(get_wordpool()->new_object());
                endof->reset(*state, word.length(), (EndNodePtr)mWord);
                mWord = (uintptr_t)(endof);
                (*state)->add_output(*state);
            }

            if (callback == nullptr)
            {
                (*state)->mEndof = id;
            }
            else
            {
                (*state)->mEndof = callback(word, (*state)->mID, (*state)->mEndof, data);
            }

            SMART_ASSERT((*state)->mEndof != TK_INVAILD);
        }

        parent = *state;
    }

    if (mMinLength == 0 || mMinLength > word.length())
        mMinLength = word.length();

    ++mWords;

    return true;
}

TNID TrieTree::find_key(const Slice& key) const
{
    if (key.length() < mMinLength)
        return TK_INVAILD;

    TrieNodePtr node = (TrieNodePtr)(mRoot);
    detail::pointer last = key.end();

    for (detail::pointer xpos = key.begin(); xpos != last; ++xpos)
    {
        node = go_state(node, TOUPPER(*xpos));
        if (node == nullptr)
            return TK_INVAILD;
    }

    return node->mID;
}

TNID TrieTree::find_subkey(const Slice& key, MatchCallBack match, void* data) const
{
    if (key.length() < mMinLength)
        return TK_INVAILD;

    TrieNodePtr node = (TrieNodePtr)(mRoot);
    detail::pointer last = key.end();

    for (detail::pointer xpos = key.begin(); xpos != last; ++xpos)
    {
        node = go_state(node, TOUPPER(*xpos));
        if (node == nullptr)
            return TK_INVAILD;

        if (output_state(key, xpos, node))
        {
            int32_t offset = offsetof_tx(key, xpos, node);
            if (!match || match(key, offset, node->mEndof, node->key(), data))
                return node->mID;
        }
    }

    return TK_INVAILD;
}

TNID TrieTree::find_first(const Slice& text, DecodeType decode) const
{
    const TrieNodePtr root = (const TrieNodePtr)(mRoot);
    TrieNodePtr node = (TrieNodePtr)(mRoot);
    detail::pointer last = text.end();
    detail::pointer mark = nullptr;

    uint8_t c = 0;
    for (detail::pointer xpos = text.begin(); xpos != last; ++xpos)
    {
        GET_LETTER(c, xpos, last);
        SMART_ASSERT(xpos < last)("xpos", xpos - last)("text", text);

        while (go_state_fail(node, c))
        {
            if (node == root)
            {
                if (xpos - text.begin() < mMinLength)
                    return TK_INVAILD;

                break;
            }

            node = node->mFail;
            if (output_state(text, xpos, node))
                return node->mEndof;
        }

        const TrieNodePtr state = go_state(node, c);
        if (state == nullptr)
            continue;

        if (output_state(text, xpos, state))
            return state->mEndof;

        node = state;
    }

    return TK_INVAILD;
}

uint32_t TrieTree::find_all(const Slice& text, DecodeType decode, TrieResultSet& result) const
{
    const TrieNodePtr root = (const TrieNodePtr)(mRoot);
    TrieNodePtr node = root;
    detail::pointer last = text.end();
    detail::pointer mark = nullptr;
    TrieResult one;

    uint8_t c = 0;
    for (detail::pointer xpos = text.begin(); xpos != last; ++xpos)
    {
        GET_LETTER(c, xpos, last);
        SMART_ASSERT(xpos < last)("xpos", xpos - last)("text", text);

        node = get_move(node, c);
        if (node == root)
            continue;
        SMART_ASSERT(node != nullptr);

        if (output_state(text, xpos, node))
        {
            TrieNodeConstPtr out = node;
            while (out != nullptr && out->output() != nullptr)
            {
                TrieNodeConstPtr temp = out->output();
                one.mID = temp->mEndof;
                one.mKey = temp->key();
                one.mOffset = offsetof_tx(text, xpos, temp);
                result.push_back(one);
                out = out->output_next();
            }
        }
    }

    return integer_cast<uint32_t>(result.size());
}

bool TrieTree::search(const Slice& text, DecodeType decode, MatchCallBack match, void* data) const
{
    const TrieNodePtr root = (const TrieNodePtr)(mRoot);
    TrieNodePtr node = root;
    detail::pointer last = text.end();
    detail::pointer mark = nullptr;

    uint8_t c = 0;
    for (detail::pointer xpos = text.begin(); xpos != last; ++xpos)
    {
        GET_LETTER(c, xpos, last);
        SMART_ASSERT(xpos < last)("xpos", xpos - last)("text", text);

        node = get_move(node, c);
        if (node == root)
            continue;
        SMART_ASSERT(node != nullptr);

        if (output_state(text, xpos, node))
        {
            TrieNodeConstPtr out = node;
            while (out != nullptr && out->output() != nullptr)
            {
                TrieNodeConstPtr temp = out->output();
                int32_t offset = offsetof_tx(text, xpos, temp);
                if (match(text, offset, temp->mEndof, temp->key(), data))
                    return true;
                out = out->output_next();
            }
        }
    }

    return false;
}

#define compile_fail compile_f
bool TrieTree::compile_fail()
{
    const TrieNodePtr root = (const TrieNodePtr)(mRoot);
    TrieNodeQueue q = { nullptr, nullptr };

    for (size_t i = 0; i < dimensionof(root->mState); ++i)
    {
        SMART_ASSERT(i + MIN_LETTER <= 256)("uint8_t", i + MIN_LETTER);
        const uint8_t c = static_cast<uint8_t>(i + MIN_LETTER);

        if (go_state_fail(root, c))
            continue;

        get_state(root, c)->mFail = root;
        queue_push(q, go_state(root, c));
    }

    while (!queue_empty(q))
    {
        TrieNodePtr r = nullptr;
        queue_pop(q, r);
        SMART_ASSERT(r != nullptr);

        for (size_t i = 0; i < dimensionof(r->mState); ++i)
        {
            SMART_ASSERT(i + MIN_LETTER <= 256)("uint8_t", i + MIN_LETTER);
            const uint8_t c = static_cast<uint8_t>(i + MIN_LETTER);

            if (go_state_fail(r, c))
                continue;

            TrieNodePtr s = go_state(r, c);
            queue_push(q, s);
            
            TrieNodePtr state = r->mFail;
            while (state != root && go_state_fail(state, c))
                state = state->mFail;

            if (go_state(state, c))
            {
                s->mFail = go_state(state, c);
                s->merge_output(s->mFail->output());
            }
        }
    }

    return true;
}

#define compile_move compile_m
bool TrieTree::compile_move()
{
    const TrieNodePtr root = (const TrieNodePtr)(mRoot);
    TrieNodeQueue q = { nullptr, nullptr };

    for (size_t i = 0; i < dimensionof(root->mState); ++i)
    {
        SMART_ASSERT(i + MIN_LETTER <= 256)("uint8_t", i + MIN_LETTER);
        const uint8_t c = static_cast<uint8_t>(i + MIN_LETTER);

        TrieNodePtr s = go_state(root, c);
        if (s != nullptr)
        {
            get_move(root, c) = s;
            queue_push(q, s);
        }
        else
        {
            get_move(root, c) = root;
        }
    }

    while (!queue_empty(q))
    {
        TrieNodePtr r = nullptr;
        queue_pop(q, r);
        SMART_ASSERT(r != nullptr);

        for (size_t i = 0; i < dimensionof(r->mState); ++i)
        {
            SMART_ASSERT(i + MIN_LETTER <= 256)("uint8_t", i + MIN_LETTER);
            const uint8_t c = static_cast<uint8_t>(i + MIN_LETTER);

            if (go_state_fail(r, c))
            {
                SMART_ASSERT(r->mFail != nullptr);
                get_move(r, c) = get_move(r->mFail, c);
            }
            else
            {
                TrieNodePtr s = go_state(r, c);
                get_move(r, c) = s;
                queue_push(q, s);
            }

            SMART_ASSERT(get_move(r, c));
        }
    }

    return true;
}

#define check_failure check_f
bool TrieTree::check_failure(uintptr_t parent) const
{
    const TrieNodeConstPtr root = (TrieNodeConstPtr)mRoot;
    TrieNodeConstPtr r = (TrieNodeConstPtr)parent;

    for (size_t i = 0; i < dimensionof(r->mState); ++i)
    {
        SMART_ASSERT(i + MIN_LETTER <= 256)("uint8_t", i + MIN_LETTER);
        const uint8_t c = static_cast<uint8_t>(i + MIN_LETTER);

        if (go_state_fail(r, c))
            continue;

        TrieNodeConstPtr s = go_state(r, c);
        if (s->mFail == nullptr)
        {
            SMART_ASSERT(s->mFail != nullptr);
            return false;
        }

        if (s->mEndof != TK_INVAILD && s->output() != s)
        {
            //std::cout << s->key() << std::endl;
            SMART_ASSERT(s->output() == s);
            return false;
        }

        if (s->output() && (s->output() == (s)->output_next()))
        {
            SMART_ASSERT(s->output() != s->output_next());
            return false;
        }

        uint8_t cc = TOUPPER(c);
        if (s->mFail != root && go_state_fail(s->mFail->mParent, cc))
        {
            SMART_ASSERT(s->mFail == root || go_state(s->mFail->mParent, cc));
            return false;
        }

        if (s->mFail == root)
        {
            if (s->mParent != root && go_state(root, cc))
            {
                SMART_ASSERT(s->mParent == root || go_state_fail(root, cc));
                return false;
            }
        }
        else
        {
            TrieNodeConstPtr fail = s->mFail;
            TrieNodeConstPtr sp = s;
            while (sp != root && fail != root)
            {
                const uint8_t c1 = sp->key().back();
                const uint8_t c2 = fail->key().back();
                if (TOUPPER(c1) != TOUPPER(c2))
                {
                    SMART_ASSERT(TOUPPER(c1) == TOUPPER(c2));
                    return false;
                }

                sp = sp->mParent;
                fail = fail->mParent;
            }
        }

        std::set<TrieNodeConstPtr> fails;
        TrieNodeConstPtr sp = s;
        while (sp->mFail != root)
        {
            bool not_cycle = fails.insert(sp->mFail).second;
            if (!not_cycle)
            {
                SMART_ASSERT(not_cycle);
                return false;
            }

            sp = sp->mFail;
        }

        if (!check_failure((uintptr_t)s))
            return false;
    }

    return true;
}

#define check_move check_m
bool TrieTree::check_move(uintptr_t parent) const
{
    const TrieNodeConstPtr root = (TrieNodeConstPtr)mRoot;
    TrieNodeConstPtr r = (TrieNodeConstPtr)parent;

    for (size_t i = 0; i < dimensionof(r->mState); ++i)
    {
        SMART_ASSERT(i + MIN_LETTER <= 256)("uint8_t", i + MIN_LETTER);
        const uint8_t c = static_cast<uint8_t>(i + MIN_LETTER);

        TrieNodeConstPtr s = go_state(r, c);
        TrieNodeConstPtr m = get_move(r, c);

        if (m == nullptr)
        {
            SMART_ASSERT(m != nullptr);
            return false;
        }

        if (s != nullptr)
        {
            if (s != m)
            {
                SMART_ASSERT(s == m);
                return false;
            }

            if (!check_move((uintptr_t)s))
                return false;

            continue;
        }
        else if (m == root)
        {
            if (go_state(m, c))
            {
                SMART_ASSERT(go_state_fail(m, c));
                return false;
            }
            continue;
        }

        uint8_t cc = TOUPPER(c);
        TrieNodeConstPtr mp = m->mParent;

        if (go_state_fail(mp, cc))
        {
            SMART_ASSERT(go_state(mp, cc) == m);
            return false;
        }

        if (mp == root) continue;

        mp = mp->mParent;

        cc = TOUPPER(r->key().back());
        TrieNodeConstPtr sp = r->mParent;
        while (sp != root && mp != root)
        {
            if (go_state_fail(sp, cc))
            {
                SMART_ASSERT(go_state(sp, cc));
                return false;
            }

            if (go_state_fail(mp, cc))
            {
                SMART_ASSERT(go_state(mp, cc));
                return false;
            }

            if (sp != root)
            {
                if (sp->key().empty())
                {
                    SMART_ASSERT(!(sp->key().empty()));
                    return false;
                }

                cc = TOUPPER(sp->key().back());
            }

            sp = sp->mParent;
            mp = mp->mParent;
        }
    }

    return true;
}

bool TrieTree::check_sibling() const
{
    std::set<std::pair<TrieNodeConstPtr, int32_t> > list;
    EndNodeConstPtr node = (EndNodeConstPtr)mWord;
    while (node != nullptr)
    {
        bool result = list.insert(std::make_pair(node->mCur, node->mLen)).second;
        SMART_ASSERT(result)("text", node->key())("node", (uintptr_t)node);
        if (!result)
        {
            return false;
        }

        node = node->mNext;
    }

    return true;
}

std::ostream& TrieTree::format(std::ostream& ss, uintptr_t parent, int depth) const
{
    const TrieNodePtr root = (TrieNodePtr)mRoot;
    TrieNodePtr r = (TrieNodePtr)parent;

    for (size_t i = 0; i < dimensionof(r->mState); ++i)
    {
        SMART_ASSERT(i + MIN_LETTER <= 256)("uint8_t", i + MIN_LETTER);
        const uint8_t c = static_cast<uint8_t>(i + MIN_LETTER);

        if (go_state_fail(r, c))
            continue;

        TrieNodePtr s = go_state(r, c);
        for (int j = 0; j < depth; ++j)
        {
            ss << "  ";
        }

        ss << '\'' << s->mPiece[s->mLength - 1]
           << "\' [" << (uintptr_t)(s)
           << "] --> f[" << (uintptr_t)(s->mFail == root ? 0 : s->mFail) << "]";

        if (s->mEndof != TK_INVAILD)
        {
            ss << " -> #" << s->mID << " |";
            for (int j = 0; j < s->mLength; ++j)
            {
                ss << s->mPiece[j];
            }
            ss << '|';
        }
        ss << '\n';

        format(ss, (uintptr_t)s, depth + 1);
    }

    return ss;
}

std::string TrieTree::to_string() const
{
    std::ostringstream ss;
    format(ss);
    return ss.str();
}
