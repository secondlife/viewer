/**
 * @file   llstreamqueue.h
 * @author Nat Goodspeed
 * @date   2012-01-04
 * @brief  Definition of LLStreamQueue
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLSTREAMQUEUE_H)
#define LL_LLSTREAMQUEUE_H

#include <string>
#include <list>
#include <iosfwd>                   // std::streamsize
#include <boost/iostreams/categories.hpp>

/**
 * This class is a growable buffer between a producer and consumer. It serves
 * as a queue usable with Boost.Iostreams -- hence, a "stream queue."
 *
 * This is especially useful for buffering nonblocking I/O. For instance, we
 * want application logic to be able to serialize LLSD to a std::ostream. We
 * may write more data than the destination pipe can handle all at once, but
 * it's imperative NOT to block the application-level serialization call. So
 * we buffer it instead. Successive frames can try nonblocking writes to the
 * destination pipe until all buffered data has been sent.
 *
 * Similarly, we want application logic be able to deserialize LLSD from a
 * std::istream. Again, we must not block that deserialize call waiting for
 * more data to arrive from the input pipe! Instead we build up a buffer over
 * a number of frames, using successive nonblocking reads, until we have
 * "enough" data to be able to present it through a std::istream.
 *
 * @note The use cases for this class overlap somewhat with those for the
 * LLIOPipe/LLPumpIO hierarchies, and indeed we considered using those. This
 * class has two virtues over the older machinery:
 *
 * # It's vastly simpler -- way fewer concepts. It's not clear to me whether
 *   there were ever LLIOPipe/etc. use cases that demanded all the fanciness
 *   rolled in, or whether they were simply overdesigned. In any case, no
 *   remaining Lindens will admit to familiarity with those classes -- and
 *   they're sufficiently obtuse that it would take considerable learning
 *   curve to figure out how to use them properly. The bottom line is that
 *   current management is not keen on any more engineers climbing that curve.
 * # This class is designed around available components such as std::string,
 *   std::list, Boost.Iostreams. There's less proprietary code.
 */
template <typename Ch>
class LLGenericStreamQueue
{
public:
    LLGenericStreamQueue():
        mSize(0),
        mClosed(false)
    {}

    /**
     * Boost.Iostreams Source Device facade for use with other Boost.Iostreams
     * functionality. LLGenericStreamQueue doesn't quite fit any of the Boost
     * 1.48 Iostreams concepts; instead it behaves as both a Sink and a
     * Source. This is its Source facade.
     */
    struct Source
    {
        typedef Ch char_type;
        typedef boost::iostreams::source_tag category;

        /// Bind the underlying LLGenericStreamQueue
        Source(LLGenericStreamQueue& sq):
            mStreamQueue(sq)
        {}

        // Read up to n characters from the underlying data source into the
        // buffer s, returning the number of characters read; return -1 to
        // indicate EOF
        std::streamsize read(Ch* s, std::streamsize n)
        {
            return mStreamQueue.read(s, n);
        }

        LLGenericStreamQueue& mStreamQueue;
    };

    /**
     * Boost.Iostreams Sink Device facade for use with other Boost.Iostreams
     * functionality. LLGenericStreamQueue doesn't quite fit any of the Boost
     * 1.48 Iostreams concepts; instead it behaves as both a Sink and a
     * Source. This is its Sink facade.
     */
    struct Sink
    {
        typedef Ch char_type;
        typedef boost::iostreams::sink_tag category;

        /// Bind the underlying LLGenericStreamQueue
        Sink(LLGenericStreamQueue& sq):
            mStreamQueue(sq)
        {}

        /// Write up to n characters from the buffer s to the output sequence,
        /// returning the number of characters written
        std::streamsize write(const Ch* s, std::streamsize n)
        {
            return mStreamQueue.write(s, n);
        }

        /// Send EOF to consumer
        void close()
        {
            mStreamQueue.close();
        }

        LLGenericStreamQueue& mStreamQueue;
    };

    /// Present Boost.Iostreams Source facade
    Source asSource() { return Source(*this); }
    /// Present Boost.Iostreams Sink facade
    Sink   asSink()   { return Sink(*this); }

    /// append data to buffer
    std::streamsize write(const Ch* s, std::streamsize n)
    {
        // Unclear how often we might be asked to write 0 bytes -- perhaps a
        // naive caller responding to an unready nonblocking read. But if we
        // do get such a call, don't add a completely empty BufferList entry.
        if (n == 0)
            return n;
        // We could implement this using a single std::string object, a la
        // ostringstream. But the trouble with appending to a string is that
        // you might have to recopy all previous contents to grow its size. If
        // we want this to scale to large data volumes, better to allocate
        // individual pieces.
        mBuffer.push_back(string(s, n));
        mSize += n;
        return n;
    }

    /**
     * Inform this LLGenericStreamQueue that no further data are forthcoming.
     * For our purposes, close() is strictly a producer-side operation;
     * there's little point in closing the consumer side.
     */
    void close()
    {
        mClosed = true;
    }

    /// consume data from buffer
    std::streamsize read(Ch* s, std::streamsize n)
    {
        // read() is actually a convenience method for peek() followed by
        // skip().
        std::streamsize got(peek(s, n));
        // We can only skip() as many characters as we can peek(); ignore
        // skip() return here.
        skip(n);
        return got;
    }

    /// Retrieve data from buffer without consuming. Like read(), return -1 on
    /// EOF.
    std::streamsize peek(Ch* s, std::streamsize n) const;

    /// Consume data from buffer without retrieving. Unlike read() and peek(),
    /// at EOF we simply skip 0 characters.
    std::streamsize skip(std::streamsize n);

    /// How many characters do we currently have buffered?
    std::streamsize size() const
    {
        return mSize;
    }

private:
    typedef std::basic_string<Ch> string;
    typedef std::list<string> BufferList;
    BufferList mBuffer;
    std::streamsize mSize;
    bool mClosed;
};

template <typename Ch>
std::streamsize LLGenericStreamQueue<Ch>::peek(Ch* s, std::streamsize n) const
{
    // Here we may have to build up 'n' characters from an arbitrary
    // number of individual BufferList entries.
    typename BufferList::const_iterator bli(mBuffer.begin()), blend(mBuffer.end());
    // Indicate EOF if producer has closed the pipe AND we've exhausted
    // all previously-buffered data.
    if (mClosed && bli == blend)
    {
        return -1;
    }
    // Here either producer hasn't yet closed, or we haven't yet exhausted
    // remaining data.
    std::streamsize needed(n), got(0);
    // Loop until either we run out of BufferList entries or we've
    // completely satisfied the request.
    for ( ; bli != blend && needed; ++bli)
    {
        std::streamsize chunk(std::min(needed, std::streamsize(bli->length())));
        std::copy(bli->begin(), bli->begin() + chunk, s);
        needed -= chunk;
        s      += chunk;
        got    += chunk;
    }
    return got;
}

template <typename Ch>
std::streamsize LLGenericStreamQueue<Ch>::skip(std::streamsize n)
{
    typename BufferList::iterator bli(mBuffer.begin()), blend(mBuffer.end());
    std::streamsize toskip(n), skipped(0);
    while (bli != blend && toskip >= bli->length())
    {
        std::streamsize chunk(bli->length());
        typename BufferList::iterator zap(bli++);
        mBuffer.erase(zap);
        mSize   -= chunk;
        toskip  -= chunk;
        skipped += chunk;
    }
    if (bli != blend && toskip)
    {
        bli->erase(bli->begin(), bli->begin() + toskip);
        mSize   -= toskip;
        skipped += toskip;
    }
    return skipped;
}

typedef LLGenericStreamQueue<char>    LLStreamQueue;
typedef LLGenericStreamQueue<wchar_t> LLWStreamQueue;

#endif /* ! defined(LL_LLSTREAMQUEUE_H) */
