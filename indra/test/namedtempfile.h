/**
 * @file   namedtempfile.h
 * @author Nat Goodspeed
 * @date   2012-01-13
 * @brief  NamedTempFile class for tests that need disk files as fixtures.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_NAMEDTEMPFILE_H)
#define LL_NAMEDTEMPFILE_H

#include "llerror.h"
#include "llapr.h"
#include "apr_file_io.h"
#include <string>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/noncopyable.hpp>
#include <iostream>
#include <sstream>

/**
 * Create a text file with specified content "somewhere in the
 * filesystem," cleaning up when it goes out of scope.
 */
class NamedTempFile: public boost::noncopyable
{
    LOG_CLASS(NamedTempFile);
public:
    NamedTempFile(const std::string& pfx, const std::string& content, apr_pool_t* pool=gAPRPoolp):
        mPool(pool)
    {
        createFile(pfx, boost::lambda::_1 << content);
    }

    // Disambiguate when passing string literal
    NamedTempFile(const std::string& pfx, const char* content, apr_pool_t* pool=gAPRPoolp):
        mPool(pool)
    {
        createFile(pfx, boost::lambda::_1 << content);
    }

    // Function that accepts an ostream ref and (presumably) writes stuff to
    // it, e.g.:
    // (boost::lambda::_1 << "the value is " << 17 << '\n')
    typedef boost::function<void(std::ostream&)> Streamer;

    NamedTempFile(const std::string& pfx, const Streamer& func, apr_pool_t* pool=gAPRPoolp):
        mPool(pool)
    {
        createFile(pfx, func);
    }

    virtual ~NamedTempFile()
    {
        ll_apr_assert_status(apr_file_remove(mPath.c_str(), mPool));
    }

    virtual std::string getName() const { return mPath; }

    void peep()
    {
        std::cout << "File '" << mPath << "' contains:\n";
        std::ifstream reader(mPath.c_str());
        std::string line;
        while (std::getline(reader, line))
            std::cout << line << '\n';
        std::cout << "---\n";
    }

protected:
    void createFile(const std::string& pfx, const Streamer& func)
    {
        // Create file in a temporary place.
        const char* tempdir = NULL;
        ll_apr_assert_status(apr_temp_dir_get(&tempdir, mPool));

        // Construct a temp filename template in that directory.
        char *tempname = NULL;
        ll_apr_assert_status(apr_filepath_merge(&tempname,
                                                tempdir,
                                                (pfx + "XXXXXX").c_str(),
                                                0,
                                                mPool));

        // Create a temp file from that template.
        apr_file_t* fp = NULL;
        ll_apr_assert_status(apr_file_mktemp(&fp,
                                             tempname,
                                             APR_CREATE | APR_WRITE | APR_EXCL,
                                             mPool));
        // apr_file_mktemp() alters tempname with the actual name. Not until
        // now is it valid to capture as our mPath.
        mPath = tempname;

        // Write desired content.
        std::ostringstream out;
        // Stream stuff to it.
        func(out);

        std::string data(out.str());
        apr_size_t writelen(data.length());
        ll_apr_assert_status(apr_file_write(fp, data.c_str(), &writelen));
        ll_apr_assert_status(apr_file_close(fp));
        llassert_always(writelen == data.length());
    }

    std::string mPath;
    apr_pool_t* mPool;
};

/**
 * Create a NamedTempFile with a specified filename extension. This is useful
 * when, for instance, you must be able to use the file in a Python import
 * statement.
 *
 * A NamedExtTempFile actually has two different names. We retain the original
 * no-extension name as a placeholder in the temp directory to ensure
 * uniqueness; to that we link the name plus the desired extension. Naturally,
 * both must be removed on destruction.
 */
class NamedExtTempFile: public NamedTempFile
{
    LOG_CLASS(NamedExtTempFile);
public:
    NamedExtTempFile(const std::string& ext, const std::string& content, apr_pool_t* pool=gAPRPoolp):
        NamedTempFile(remove_dot(ext), content, pool),
        mLink(mPath + ensure_dot(ext))
    {
        linkto(mLink);
    }

    // Disambiguate when passing string literal
    NamedExtTempFile(const std::string& ext, const char* content, apr_pool_t* pool=gAPRPoolp):
        NamedTempFile(remove_dot(ext), content, pool),
        mLink(mPath + ensure_dot(ext))
    {
        linkto(mLink);
    }

    NamedExtTempFile(const std::string& ext, const Streamer& func, apr_pool_t* pool=gAPRPoolp):
        NamedTempFile(remove_dot(ext), func, pool),
        mLink(mPath + ensure_dot(ext))
    {
        linkto(mLink);
    }

    virtual ~NamedExtTempFile()
    {
        ll_apr_assert_status(apr_file_remove(mLink.c_str(), mPool));
    }

    // Since the caller has gone to the trouble to create the name with the
    // extension, that should be the name we return. In this class, mPath is
    // just a placeholder to ensure that future createFile() calls won't
    // collide.
    virtual std::string getName() const { return mLink; }

    static std::string ensure_dot(const std::string& ext)
    {
        if (ext.empty())
        {
            // What SHOULD we do when the caller makes a point of using
            // NamedExtTempFile to generate a file with a particular
            // extension, then passes an empty extension? Use just "."? That
            // sounds like a Bad Idea, especially on Windows. Treat that as a
            // coding error.
            LL_ERRS("NamedExtTempFile") << "passed empty extension" << LL_ENDL;
        }
        if (ext[0] == '.')
        {
            return ext;
        }
        return std::string(".") + ext;
    }

    static std::string remove_dot(const std::string& ext)
    {
        std::string::size_type found = ext.find_first_not_of(".");
        if (found == std::string::npos)
        {
            return ext;
        }
        return ext.substr(found);
    }

private:
    void linkto(const std::string& path)
    {
        // This method assumes that since mPath (without extension) is
        // guaranteed by apr_file_mktemp() to be unique, then (mPath + any
        // extension) is also unique. This is likely, though not guaranteed:
        // files could be created in the same temp directory other than by
        // this class.
        ll_apr_assert_status(apr_file_link(mPath.c_str(), path.c_str()));
    }

    std::string mLink;
};

#endif /* ! defined(LL_NAMEDTEMPFILE_H) */
