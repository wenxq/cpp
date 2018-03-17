
#include <cassert>
#include <vector>

#include "common/windows/string_utils.h"

// static
std::wstring WindowsStringUtils::GetBaseName(const std::wstring& filename)
{
    std::wstring base_name(filename);
    size_t slash_pos = base_name.find_last_of(L"/\\");
    if (slash_pos != std::wstring::npos)
    {
        base_name.erase(0, slash_pos + 1);
    }
    return base_name;
}

// static
bool WindowsStringUtils::safe_mbstowcs(const std::string& mbs,
                                       std::wstring* wcs)
{
    assert(wcs);

    // First, determine the length of the destination buffer.
    size_t wcs_length;

#if _MSC_VER >= 1400  // MSVC 2005/8
    errno_t err;
    if ((err = mbstowcs_s(&wcs_length, NULL, 0, mbs.c_str(), _TRUNCATE)) != 0)
    {
        return false;
    }
    assert(wcs_length > 0);
#else  // _MSC_VER >= 1400
    if ((wcs_length = mbstowcs(NULL, mbs.c_str(), mbs.length())) == (size_t) - 1)
    {
        return false;
    }

    // Leave space for the 0-terminator.
    ++wcs_length;
#endif  // _MSC_VER >= 1400

    std::vector<wchar_t> wcs_v(wcs_length);

    // Now, convert.
#if _MSC_VER >= 1400  // MSVC 2005/8
    if ((err = mbstowcs_s(NULL, &wcs_v[0], wcs_length, mbs.c_str(),
                          _TRUNCATE)) != 0)
    {
        return false;
    }
#else  // _MSC_VER >= 1400
    if (mbstowcs(&wcs_v[0], mbs.c_str(), mbs.length()) == (size_t) - 1)
    {
        return false;
    }

    // Ensure presence of 0-terminator.
    wcs_v[wcs_length - 1] = '\0';
#endif  // _MSC_VER >= 1400

    *wcs = &wcs_v[0];
    return true;
}

// static
bool WindowsStringUtils::safe_wcstombs(const std::wstring& wcs,
                                       std::string* mbs)
{
    assert(mbs);

    // First, determine the length of the destination buffer.
    size_t mbs_length;

#if _MSC_VER >= 1400  // MSVC 2005/8
    errno_t err;
    if ((err = wcstombs_s(&mbs_length, NULL, 0, wcs.c_str(), _TRUNCATE)) != 0)
    {
        return false;
    }
    assert(mbs_length > 0);
#else  // _MSC_VER >= 1400
    if ((mbs_length = wcstombs(NULL, wcs.c_str(), wcs.length())) == (size_t) - 1)
    {
        return false;
    }

    // Leave space for the 0-terminator.
    ++mbs_length;
#endif  // _MSC_VER >= 1400

    std::vector<char> mbs_v(mbs_length);

    // Now, convert.
#if _MSC_VER >= 1400  // MSVC 2005/8
    if ((err = wcstombs_s(NULL, &mbs_v[0], mbs_length, wcs.c_str(),
                          _TRUNCATE)) != 0)
    {
        return false;
    }
#else  // _MSC_VER >= 1400
    if (wcstombs(&mbs_v[0], wcs.c_str(), wcs.length()) == (size_t) - 1)
    {
        return false;
    }

    // Ensure presence of 0-terminator.
    mbs_v[mbs_length - 1] = '\0';
#endif  // _MSC_VER >= 1400

    *mbs = &mbs_v[0];
    return true;
}

}  // namespace google_breakpad
