/**
 * @file hbxxh.h
 * @brief High performances vectorized hashing based on xxHash.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (c) 2023, Henri Beauchamp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_HBXXH_H
#define LL_HBXXH_H

#include "lluuid.h"

// HBXXH* classes are to be used where speed matters and cryptographic quality
// is not required (no "one-way" guarantee, though they are likely not worst in
// this respect than MD5 which got busted and is now considered too weak). The
// xxHash code they are built upon is vectorized and about 50 times faster than
// MD5. A 64 bits hash class is also provided for when 128 bits of entropy are
// not needed. The hashes collision rate is similar to MD5's.
// See https://github.com/Cyan4973/xxHash#readme for details.

// 64 bits hashing class

class HBXXH64
{
    friend std::ostream& operator<<(std::ostream&, HBXXH64);

protected:
    LOG_CLASS(HBXXH64);

public:
    inline HBXXH64()                            { init(); }

    // Constructors for special circumstances; they all digest the first passed
    // parameter. Set 'do_finalize' to false if you do not want to finalize the
    // context, which is useful/needed when you want to update() it afterwards.
    // Ideally, the compiler should be smart enough to get our clue and
    // optimize out the const bool test during inlining...

    inline HBXXH64(const void* buffer, size_t len,
                   const bool do_finalize = true)
    {
        init();
        update(buffer, len);
        if (do_finalize)
        {
            finalize();
        }
    }

    inline HBXXH64(const std::string& str, const bool do_finalize = true)
    {
        init();
        update(str);
        if (do_finalize)
        {
            finalize();
        }
    }

    inline HBXXH64(std::istream& s, const bool do_finalize = true)
    {
        init();
        update(s);
        if (do_finalize)
        {
            finalize();
        }
    }

    inline HBXXH64(FILE* file, const bool do_finalize = true)
    {
        init();
        update(file);
        if (do_finalize)
        {
            finalize();
        }
    }

    // Make this class no-copy (it would be possible, with custom copy
    // operators, but it is not trivially copyable, because of the mState
    // pointer): it does not really make sense to allow copying it anyway,
    // since all we care about is the resulting digest (so you should only
    // need and care about storing/copying the digest and not a class
    // instance).
    HBXXH64(const HBXXH64&) noexcept = delete;
    HBXXH64& operator=(const HBXXH64&) noexcept = delete;

    ~HBXXH64();

    void update(const void* buffer, size_t len);
    void update(const std::string& str);
    void update(std::istream& s);
    void update(FILE* file);

    // Note that unlike what happens with LLMD5, you do not need to finalize()
    // HBXXH64 before using digest(), and you may keep updating() it even after
    // you got a first digest() (the next digest would of course change after
    // any update). It is still useful to use finalize() when you do not want
    // to store a final digest() result in a separate U64; after this method
    // has been called, digest() simply returns mDigest value.
    void finalize();

    U64 digest() const;

    // Fast static methods. Use them when hashing just one contiguous block of
    // data.
    static U64 digest(const void* buffer, size_t len);
    static U64 digest(const char* str);    // str must be NUL-terminated
    static U64 digest(const std::string& str);

private:
    void init();

private:
    // We use a void pointer to avoid including xxhash.h here for XXH3_state_t
    // (which cannot either be trivially forward-declared, due to complex API
    // related pre-processor macros in xxhash.h).
    void*   mState;
    U64     mDigest;
};

inline bool operator==(const HBXXH64& a, const HBXXH64& b)
{
    return a.digest() == b.digest();
}

inline bool operator!=(const HBXXH64& a, const HBXXH64& b)
{
    return a.digest() != b.digest();
}

// 128 bits hashing class

class HBXXH128
{
    friend std::ostream& operator<<(std::ostream&, HBXXH128);

protected:
    LOG_CLASS(HBXXH128);

public:
    inline HBXXH128()                           { init(); }

    // Constructors for special circumstances; they all digest the first passed
    // parameter. Set 'do_finalize' to false if you do not want to finalize the
    // context, which is useful/needed when you want to update() it afterwards.
    // Ideally, the compiler should be smart enough to get our clue and
    // optimize out the const bool test during inlining...

    inline HBXXH128(const void* buffer, size_t len,
                    const bool do_finalize = true)
    {
        init();
        update(buffer, len);
        if (do_finalize)
        {
            finalize();
        }
    }

    inline HBXXH128(const std::string& str, const bool do_finalize = true)
    {
        init();
        update(str);
        if (do_finalize)
        {
            finalize();
        }
    }

    inline HBXXH128(std::istream& s, const bool do_finalize = true)
    {
        init();
        update(s);
        if (do_finalize)
        {
            finalize();
        }
    }

    inline HBXXH128(FILE* file, const bool do_finalize = true)
    {
        init();
        update(file);
        if (do_finalize)
        {
            finalize();
        }
    }

    // Make this class no-copy (it would be possible, with custom copy
    // operators, but it is not trivially copyable, because of the mState
    // pointer): it does not really make sense to allow copying it anyway,
    // since all we care about is the resulting digest (so you should only
    // need and care about storing/copying the digest and not a class
    // instance).
    HBXXH128(const HBXXH128&) noexcept = delete;
    HBXXH128& operator=(const HBXXH128&) noexcept = delete;

    ~HBXXH128();

    void update(const void* buffer, size_t len);
    void update(const std::string& str);
    void update(std::istream& s);
    void update(FILE* file);

    // Note that unlike what happens with LLMD5, you do not need to finalize()
    // HBXXH128 before using digest(), and you may keep updating() it even
    // after you got a first digest() (the next digest would of course change
    // after any update). It is still useful to use finalize() when you do not
    // want to store a final digest() result in a separate LLUUID; after this
    // method has been called, digest() simply returns a reference on mDigest.
    void finalize();

    // We use an LLUUID for the digest, since this is a 128 bits wide native
    // type available in the viewer code, making it easy to manipulate. It also
    // allows to use HBXXH128 efficiently in LLUUID generate() and combine()
    // methods.
    const LLUUID& digest() const;

    // Here, we avoid an LLUUID copy whenever we already got one to store the
    // result *and* we did not yet call finalize().
    void digest(LLUUID& result) const;

    // Fast static methods. Use them when hashing just one contiguous block of
    // data.
    static LLUUID digest(const void* buffer, size_t len);
    static LLUUID digest(const char* str);    // str must be NUL-terminated
    static LLUUID digest(const std::string& str);
    // Same as above, but saves you from an LLUUID copy when you already got
    // one for storage use.
    static void digest(LLUUID& result, const void* buffer, size_t len);
    static void digest(LLUUID& result, const char* str); // str NUL-terminated
    static void digest(LLUUID& result, const std::string& str);

private:
    void init();

private:
    // We use a void pointer to avoid including xxhash.h here for XXH3_state_t
    // (which cannot either be trivially forward-declared, due to complex API
    // related pre-processor macros in xxhash.h).
    void*   mState;
    LLUUID  mDigest;
};

inline bool operator==(const HBXXH128& a, const HBXXH128& b)
{
    return a.digest() == b.digest();
}

inline bool operator!=(const HBXXH128& a, const HBXXH128& b)
{
    return a.digest() != b.digest();
}

#endif // LL_HBXXH_H
