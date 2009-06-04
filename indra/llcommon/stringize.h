/**
 * @file   stringize.h
 * @author Nat Goodspeed
 * @date   2008-12-17
 * @brief  stringize(item) template function and STRINGIZE(expression) macro
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * Copyright (c) 2008, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_STRINGIZE_H)
#define LL_STRINGIZE_H

#include <sstream>

/**
 * stringize(item) encapsulates an idiom we use constantly, using
 * operator<<(std::ostringstream&, TYPE) followed by std::ostringstream::str()
 * to render a string expressing some item.
 */
template <typename T>
std::string stringize(const T& item)
{
    std::ostringstream out;
    out << item;
    return out.str();
}

/**
 * STRINGIZE(item1 << item2 << item3 ...) effectively expands to the
 * following:
 * @code
 * std::ostringstream out;
 * out << item1 << item2 << item3 ... ;
 * return out.str();
 * @endcode
 */
#define STRINGIZE(EXPRESSION) (static_cast<std::ostringstream&>(Stringize() << EXPRESSION).str())

/**
 * Helper class for STRINGIZE() macro. Ideally the body of
 * STRINGIZE(EXPRESSION) would look something like this:
 * @code
 * (std::ostringstream() << EXPRESSION).str()
 * @endcode
 * That doesn't work because each of the relevant operator<<() functions
 * accepts a non-const std::ostream&, to which you can't pass a temp instance
 * of std::ostringstream. Stringize plays the necessary const tricks to make
 * the whole thing work.
 */
class Stringize
{
public:
    /**
     * This is the essence of Stringize. The leftmost << operator (the one
     * coded in the STRINGIZE() macro) engages this operator<<() const method
     * on the temp Stringize instance. Every other << operator (ones embedded
     * in EXPRESSION) simply sees the std::ostream& returned by the first one.
     *
     * Finally, the STRINGIZE() macro downcasts that std::ostream& to
     * std::ostringstream&.
     */
    template <typename T>
    std::ostream& operator<<(const T& item) const
    {
        mOut << item;
        return mOut;
    }

private:
    mutable std::ostringstream mOut;
};

#endif /* ! defined(LL_STRINGIZE_H) */
