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

    ~NamedTempFile()
    {
        ll_apr_assert_status(apr_file_remove(mPath.c_str(), mPool));
    }

    std::string getName() const { return mPath; }

    void peep()
    {
        std::cout << "File '" << mPath << "' contains:\n";
        std::ifstream reader(mPath.c_str());
        std::string line;
        while (std::getline(reader, line))
            std::cout << line << '\n';
        std::cout << "---\n";
    }

private:
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

#endif /* ! defined(LL_NAMEDTEMPFILE_H) */
