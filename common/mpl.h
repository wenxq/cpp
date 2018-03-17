
#ifndef _MPL_H_
#define _MPL_H_

namespace mpl {

template <bool Y, typename S, typename D>
struct if_
{
    typedef D type;
};

template <typename S, typename D>
struct if_<true, S, D>
{
    typedef S type;
};

} // namespace mpl 

#endif // _MPL_H_

