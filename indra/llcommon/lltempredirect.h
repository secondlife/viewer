/**
 * @file   lltempredirect.h
 * @author Nat Goodspeed
 * @date   2019-10-31
 * @brief  RAII low-level file-descriptor redirection
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLTEMPREDIRECT_H)
#define LL_LLTEMPREDIRECT_H

// Functions in this namespace are intended to insulate the caller from the
// aggravating distinction between ::close() and Microsoft _close().
namespace llfd
{

int close(int fd);
int dup(int target);
int dup2(int target, int reference);
FILE* open(int fd, const char* mode);
int fileno(FILE* stream);

} // namespace llfd

/**
 * LLTempRedirect is an RAII class that performs file redirection on low-level
 * file descriptors, expressed as ints. (Use llfd::fileno() to obtain the file
 * descriptor from a classic-C FILE*. There is no portable way to obtain the
 * file descriptor from a std::fstream.)
 *
 * Instantiate LLTempRedirect with a target file descriptor (e.g. for some
 * open file) and a reference file descriptor (e.g. for stderr). From that
 * point until the LLTempRedirect instance is destroyed, all OS-level writes
 * to the reference file descriptor will be redirected to the target file.
 *
 * Because dup2() is used for redirection, the original passed target file
 * descriptor remains open. If you want LLTempRedirect's destructor to close
 * the target file, close() the target file descriptor after passing it to
 * LLTempRedirect's constructor.
 *
 * LLTempRedirect's constructor saves the original target of the reference
 * file descriptor. Its destructor restores the reference file descriptor to
 * point once again to its original target.
 */
class LLTempRedirect
{
public:
    LLTempRedirect();
    /**
     * For the lifespan of this LLTempRedirect instance, all writes to
     * 'reference' will be redirected to 'target'. When this LLTempRedirect is
     * destroyed, the original target for 'reference' will be restored.
     *
     * Pass 'target' as NULL if you simply want to save and restore
     * 'reference' against possible redirection in the meantime.
     */
    LLTempRedirect(FILE* target, FILE* reference);
    /**
     * For the lifespan of this LLTempRedirect instance, all writes to
     * 'reference' will be redirected to 'target'. When this LLTempRedirect is
     * destroyed, the original target for 'reference' will be restored.
     *
     * Pass 'target' as -1 if you simply want to save and restore
     * 'reference' against possible redirection in the meantime.
     */
    LLTempRedirect(int target,   int reference);
    LLTempRedirect(const LLTempRedirect&) = delete;
    LLTempRedirect(LLTempRedirect&& other);

    ~LLTempRedirect();

    LLTempRedirect& operator=(const LLTempRedirect&) = delete;
    LLTempRedirect& operator=(LLTempRedirect&& other);

    /// returns (duplicate file descriptor for) the original target of the
    /// 'reference' file descriptor passed to our constructor
    int getOriginalTarget() const { return mOrigTarget; }
    /// returns the original 'reference' file descriptor passed to our
    /// constructor
    int getReference()      const { return mReference; }

private:
    void reset();

    int mOrigTarget, mReference;
};

#endif /* ! defined(LL_LLTEMPREDIRECT_H) */
