/**
 * @file   llprocess_test.cpp
 * @author Nat Goodspeed
 * @date   2011-12-19
 * @brief  Test for llprocess.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llprocess.h"
// STL headers
#include <vector>
#include <list>
// std headers
#include <fstream>
// external library headers
#include "llapr.h"
#include "apr_thread_proc.h"
#include <boost/function.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
// other Linden headers
#include "../test/lldoctest.h"
#include "../test/namedtempfile.h"
#include "../test/catch_and_store_what_in.h"
#include "stringize.h"
#include "llsdutil.h"
#include "llevents.h"
#include "llstring.h"
#include "wrapllerrs.h"             // CaptureLog

#if defined(LL_WINDOWS)
#define sleep(secs) _sleep((secs) * 1000)
#define EOL "\r\n"
#else
#define EOL "\n"
#include <sys/wait.h>
#endif

std::string apr_strerror_helper(apr_status_t rv)
{
    char errbuf[256];
    apr_strerror(rv, errbuf, sizeof(errbuf));
    return errbuf;
}

/*****************************************************************************
*   Helpers
*****************************************************************************/

#define ensure_equals_(left, right) \
        ensure_equals(STRINGIZE(#left << " != " << #right), (left), (right))

#define aprchk(expr) aprchk_(#expr, (expr))
static void aprchk_(const char* call, apr_status_t rv, apr_status_t expected=APR_SUCCESS)
{
    tut::ensure_equals(STRINGIZE(call << " => " << rv << ": " << apr_strerror_helper
                                 (rv)),
                       rv, expected);
}

/**
 * Read specified file using std::getline(). It is assumed to be an error if
 * the file is empty: don't use this function if that's an acceptable case.
 * Last line will not end with '\n'; this is to facilitate the usual case of
 * string compares with a single line of output.
 * @param pathname The file to read.
 * @param desc Optional description of the file for error message;
 * defaults to "in <pathname>"
 */
static std::string readfile(const std::string& pathname, const std::string& desc="")
{
    std::string use_desc(desc);
    if (use_desc.empty())
    {
        use_desc = STRINGIZE("in " << pathname);
    }
    std::ifstream inf(pathname.c_str());
    std::string output;
    tut::ensure(STRINGIZE("No output " << use_desc), bool(std::getline(inf, output)));
    std::string more;
    while (std::getline(inf, more))
    {
        output += '\n' + more;
    }
    return output;
}

/// Looping on LLProcess::isRunning() must now be accompanied by pumping
/// "mainloop" -- otherwise the status won't update and you get an infinite
/// loop.
void yield(int seconds=1)
{
    // This function simulates waiting for another viewer frame
    sleep(seconds);
    LLEventPumps::instance().obtain("mainloop").post(LLSD());
}

void waitfor(LLProcess& proc, int timeout=60)
{
    int i = 0;
    for ( ; i < timeout && proc.isRunning(); ++i)
    {
        yield();
    }
    tut::ensure(STRINGIZE("process took longer than " << timeout << " seconds to terminate"),
                i < timeout);
}

void waitfor(LLProcess::handle h, const std::string& desc, int timeout=60)
{
    int i = 0;
    for ( ; i < timeout && LLProcess::isRunning(h, desc); ++i)
    {
        yield();
    }
    tut::ensure(STRINGIZE("process took longer than " << timeout << " seconds to terminate"),
                i < timeout);
}

/**
 * Construct an LLProcess to run a Python script.
 */
struct PythonProcessLauncher
{
    /**
     * @param desc Arbitrary description for error messages
     * @param script Python script, any form acceptable to NamedTempFile,
     * typically either a std::string or an expression of the form
     * (lambda::_1 << "script content with " << variable_data)
     */
    template <typename CONTENT>
    PythonProcessLauncher(const std::string& desc, const CONTENT& script):
        mDesc(desc),
        mScript("py", script)
    {
        auto PYTHON(LLStringUtil::getenv("PYTHON"));
        tut::ensure("Set $PYTHON to the Python interpreter", !PYTHON.empty());

        mParams.desc = desc + " script";
        mParams.executable = PYTHON;
        mParams.args.add(mScript.getName());
    }

    /// Launch Python script; verify that it launched
    void launch()
    {
        try
        {
            mPy = LLProcess::create(mParams);
            tut::ensure(STRINGIZE("Couldn't launch " << mDesc << " script"), bool(mPy));
        }
        catch (const tut::failure&)
        {
            // On Windows, if APR_LOG is set, our version of APR's
            // apr_create_proc() logs to the specified file. If this test
            // failed, try to report that log.
            const char* APR_LOG = getenv("APR_LOG");
            if (APR_LOG && *APR_LOG)
            {
                std::ifstream inf(APR_LOG);
                if (! inf.is_open())
                {
                    LL_WARNS() << "Couldn't open '" << APR_LOG << "'" << LL_ENDL;
                }
                else
                {
                    LL_WARNS() << "==============================" << LL_ENDL;
                    LL_WARNS() << "From '" << APR_LOG << "':" << LL_ENDL;
                    std::string line;
                    while (std::getline(inf, line))
                    {
                        LL_WARNS() << line << LL_ENDL;
                    }
                    LL_WARNS() << "==============================" << LL_ENDL;
                }
            }
            throw;
        }
    }

    /// Run Python script and wait for it to complete.
    void run()
    {
        launch();
        // One of the irritating things about LLProcess is that
        // there's no API to wait for the child to terminate -- but given
        // its use in our graphics-intensive interactive viewer, it's
        // understandable.
        waitfor(*mPy);
    }

    /**
     * Run a Python script using LLProcess, expecting that it will
     * write to the file passed as its sys.argv[1]. Retrieve that output.
     *
     * Until January 2012, LLProcess provided distressingly few
     * mechanisms for a child process to communicate back to its caller --
     * not even its return code. We've introduced a convention by which we
     * create an empty temp file, pass the name of that file to our child
     * as sys.argv[1] and expect the script to write its output to that
     * file. This function implements the C++ (parent process) side of
     * that convention.
     */
    std::string run_read()
    {
        NamedTempFile out("out", ""); // placeholder
        // pass name of this temporary file to the script
        mParams.args.add(out.getName());
        run();
        // assuming the script wrote to that file, read it
        return readfile(out.getName(), STRINGIZE("from " << mDesc << " script"));
    }

    LLProcess::Params mParams;
    LLProcessPtr mPy;
    std::string mDesc;
    NamedExtTempFile mScript;
};

/// convenience function for PythonProcessLauncher::run()
template <typename CONTENT>
static void python(const std::string& desc, const CONTENT& script)
{
    PythonProcessLauncher py(desc, script);
    py.run();
}

/// convenience function for PythonProcessLauncher::run_read()
template <typename CONTENT>
static std::string python_out(const std::string& desc, const CONTENT& script)
{
    PythonProcessLauncher py(desc, script);
    return py.run_read();
}

/// Create a temporary directory and clean it up later.
class NamedTempDir: public boost::noncopyable
{
public:
    NamedTempDir():
        mPath(NamedTempFile::temp_path()),
        mCreated(boost::filesystem::create_directories(mPath))
    {
        mPath = boost::filesystem::canonical(mPath);
    }

    ~NamedTempDir()
    {
        if (mCreated)
        {
            boost::filesystem::remove_all(mPath);
        }
    }

    std::string getName() const { return mPath.string(); }

private:
    boost::filesystem::path mPath;
    bool mCreated;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct llprocess_data
{

        LLAPRPool pool;
    
};

TEST_CASE_FIXTURE(llprocess_data, "test_1")
{

        set_test_name("raw APR nonblocking I/O");

        // Create a script file in a temporary place.
        NamedExtTempFile script("py",
            "from __future__ import print_function" EOL
            "import sys" EOL
            "import time" EOL
            EOL
            "time.sleep(2)" EOL
            "print('stdout after wait',file=sys.stdout)" EOL
            "sys.stdout.flush()" EOL
            "time.sleep(2)" EOL
            "print('stderr after wait',file=sys.stderr)" EOL
            "sys.stderr.flush()" EOL
        );

        // Arrange to track the history of our interaction with child: what we
        // fetched, which pipe it came from, how many tries it took before we
        // got it.
        std::vector<Item> history;
        history.push_back(Item());

        // Run the child process.
        apr_procattr_t *procattr = NULL;
        aprchk(apr_procattr_create(&procattr, pool.getAPRPool()));
        aprchk(apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK));
        aprchk(apr_procattr_cmdtype_set(procattr, APR_PROGRAM_PATH));

        std::vector<const char*> argv;
        apr_proc_t child;
#if defined(LL_WINDOWS)
        argv.push_back("python");
#else
        argv.push_back("python3");
#endif
        // Have to have a named copy of this std::string so its c_str() value
        // will persist.
        std::string scriptname(script.getName());
        argv.push_back(scriptname.c_str());
        argv.push_back(NULL);

        aprchk(apr_proc_create(&child, argv[0],
                               &argv[0],
                               NULL, // if we wanted to pass explicit environment
                               procattr,
                               pool.getAPRPool()));

        // We do not want this child process to outlive our APR pool. On
        // destruction of the pool, forcibly kill the process. Tell APR to try
        // SIGTERM and wait 3 seconds. If that didn't work, use SIGKILL.
        apr_pool_note_subprocess(pool.getAPRPool(), &child, APR_KILL_AFTER_TIMEOUT);

        // arrange to call child_status_callback()
        WaitInfo wi(&child);
        apr_proc_other_child_register(&child, child_status_callback, &wi, child.in, pool.getAPRPool());

        // TODO:
        // Stuff child.in until it (would) block to verify EWOULDBLOCK/EAGAIN.
        // Have child script clear it later, then write one more line to prove
        // that it gets through.

        // Monitor two different output pipes. Because one will be closed
        // before the other, keep them in a list so we can drop whichever of
        // them is closed first.
        typedef std::pair<std::string, apr_file_t*> DescFile;
        typedef std::list<DescFile> DescFileList;
        DescFileList outfiles;
        outfiles.push_back(DescFile("out", child.out));
        outfiles.push_back(DescFile("err", child.err));

        while (! outfiles.empty())
        {
            // This peculiar for loop is designed to let us erase(dfli). With
            // a list, that invalidates only dfli itself -- but even so, we
            // lose the ability to increment it for the next item. So at the
            // top of every loop, while dfli is still valid, increment
            // dflnext. Then before the next iteration, set dfli to dflnext.
            for (DescFileList::iterator
                     dfli(outfiles.begin()), dflnext(outfiles.begin()), dflend(outfiles.end());
                 dfli != dflend; dfli = dflnext)
            {
                // Only valid to increment dflnext once we're sure it's not
                // already at dflend.
                ++dflnext;

                char buf[4096];

                apr_status_t rv = apr_file_gets(buf, sizeof(buf), dfli->second);
                if (APR_STATUS_IS_EOF(rv))
                {
//                  std::cout << "(EOF on " << dfli->first << ")\n";
//                  history.back().which = dfli->first;
//                  history.back().what  = "*eof*";
//                  history.push_back(Item());
                    outfiles.erase(dfli);
                    continue;
                
}

TEST_CASE_FIXTURE(llprocess_data, "test_2")
{

        set_test_name("setWorkingDirectory()");
        // We want to test setWorkingDirectory(). But what directory is
        // guaranteed to exist on every machine, under every OS? Have to
        // create one. Naturally, ensure we clean it up when done.
        NamedTempDir tempdir;
        PythonProcessLauncher py(get_test_name(),
                                 "from __future__ import with_statement\n"
                                 "import os, sys\n"
                                 "with open(sys.argv[1], 'w') as f:\n"
                                 "    f.write(os.path.normcase(os.path.normpath(os.getcwd())))\n");
        // Before running, call setWorkingDirectory()
        py.mParams.cwd = tempdir.getName();
        std::string expected{ tempdir.getName() 
}

TEST_CASE_FIXTURE(llprocess_data, "test_3")
{

        set_test_name("arguments");
        PythonProcessLauncher py(get_test_name(),
                                 "from __future__ import with_statement, print_function\n"
                                 "import sys\n"
                                 // note nonstandard output-file arg!
                                 "with open(sys.argv[3], 'w') as f:\n"
                                 "    for arg in sys.argv[1:]:\n"
                                 "        print(arg,file=f)\n");
        // We expect that PythonProcessLauncher has already appended
        // its own NamedTempFile to mParams.args (sys.argv[0]).
        py.mParams.args.add("first arg");          // sys.argv[1]
        py.mParams.args.add("second arg");         // sys.argv[2]
        // run_read() appends() one more argument, hence [3]
        std::string output(py.run_read());
        boost::split_iterator<std::string::const_iterator>
            li(output, boost::first_finder("\n")), lend;
        CHECK_MESSAGE(li != lend, "didn't get first arg");
        std::string arg(li->begin(), li->end());
        ensure_equals(arg, "first arg");
        ++li;
        CHECK_MESSAGE(li != lend, "didn't get second arg");
        arg.assign(li->begin(), li->end());
        ensure_equals(arg, "second arg");
        ++li;
        CHECK_MESSAGE(li != lend, "didn't get output filename?!");
        arg.assign(li->begin(), li->end());
        CHECK_MESSAGE(! arg.empty(, "output filename empty?!"));
        ++li;
        CHECK_MESSAGE(li == lend, "too many args");
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_4")
{

        set_test_name("exit(0)");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(0)\n");
        py.run();
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  0);
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_5")
{

        set_test_name("exit(2)");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(2)\n");
        py.run();
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  2);
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_6")
{

        set_test_name("syntax_error");
        PythonProcessLauncher py(get_test_name(),
                                 "syntax_error:\n");
        py.mParams.files.add(LLProcess::FileParam()); // inherit stdin
        py.mParams.files.add(LLProcess::FileParam()); // inherit stdout
        py.mParams.files.add(LLProcess::FileParam().type("pipe")); // pipe for stderr
        py.run();
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  1);
        std::istream& rpipe(py.mPy->getReadPipe(LLProcess::STDERR).get_istream());
        std::vector<char> buffer(4096);
        rpipe.read(&buffer[0], buffer.size());
        std::streamsize got(rpipe.gcount());
        CHECK_MESSAGE(got, "Nothing read from stderr pipe");
        std::string data(&buffer[0], got);
        CHECK_MESSAGE(data.find("\nSyntaxError:", "Didn't find 'SyntaxError:'") != std::string::npos);
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_7")
{

        set_test_name("explicit kill()");
        PythonProcessLauncher py(get_test_name(),
                                 "from __future__ import with_statement\n"
                                 "import sys, time\n"
                                 "with open(sys.argv[1], 'w') as f:\n"
                                 "    f.write('ok')\n"
                                 "# now sleep; expect caller to kill\n"
                                 "time.sleep(120)\n"
                                 "# if caller hasn't managed to kill by now, bad\n"
                                 "with open(sys.argv[1], 'w') as f:\n"
                                 "    f.write('bad')\n");
        NamedTempFile out("out", "not started");
        py.mParams.args.add(out.getName());
        py.launch();
        // Wait for the script to wake up and do its first write
        int i = 0, timeout = 60;
        for ( ; i < timeout; ++i)
        {
            yield();
            if (readfile(out.getName(), "from kill() script") == "ok")
                break;
        
}

TEST_CASE_FIXTURE(llprocess_data, "test_8")
{

        set_test_name("implicit kill()");
        NamedTempFile out("out", "not started");
        LLProcess::handle phandle(0);
        {
            PythonProcessLauncher py(get_test_name(),
                                     "from __future__ import with_statement\n"
                                     "import sys, time\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ok')\n"
                                     "# now sleep; expect caller to kill\n"
                                     "time.sleep(120)\n"
                                     "# if caller hasn't managed to kill by now, bad\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('bad')\n");
            py.mParams.args.add(out.getName());
            py.launch();
            // Capture handle for later
            phandle = py.mPy->getProcessHandle();
            // Wait for the script to wake up and do its first write
            int i = 0, timeout = 60;
            for ( ; i < timeout; ++i)
            {
                yield();
                if (readfile(out.getName(), "from kill() script") == "ok")
                    break;
            
}

TEST_CASE_FIXTURE(llprocess_data, "test_9")
{

        set_test_name("autokill=false");
        NamedTempFile from("from", "not started");
        NamedTempFile to("to", "");
        LLProcess::handle phandle(0);
        {
            PythonProcessLauncher py(get_test_name(),
                                     "from __future__ import with_statement\n"
                                     "import sys, time\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ok')\n"
                                     "# wait for 'go' from test program\n"
                                     "for i in range(60):\n"
                                     "    time.sleep(1)\n"
                                     "    with open(sys.argv[2]) as f:\n"
                                     "        go = f.read()\n"
                                     "    if go == 'go':\n"
                                     "        break\n"
                                     "else:\n"
                                     "    with open(sys.argv[1], 'w') as f:\n"
                                     "        f.write('never saw go')\n"
                                     "    sys.exit(1)\n"
                                     "# okay, saw 'go', write 'ack'\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ack')\n");
            py.mParams.args.add(from.getName());
            py.mParams.args.add(to.getName());
            py.mParams.autokill = false;
            py.launch();
            // Capture handle for later
            phandle = py.mPy->getProcessHandle();
            // Wait for the script to wake up and do its first write
            int i = 0, timeout = 60;
            for ( ; i < timeout; ++i)
            {
                yield();
                if (readfile(from.getName(), "from autokill script") == "ok")
                    break;
            
}

TEST_CASE_FIXTURE(llprocess_data, "test_10")
{

        set_test_name("attached=false");
        // almost just like autokill=false, except set autokill=true with
        // attached=false.
        NamedTempFile from("from", "not started");
        NamedTempFile to("to", "");
        LLProcess::handle phandle(0);
        {
            PythonProcessLauncher py(get_test_name(),
                                     "from __future__ import with_statement\n"
                                     "import sys, time\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ok')\n"
                                     "# wait for 'go' from test program\n"
                                     "for i in range(60):\n"
                                     "    time.sleep(1)\n"
                                     "    with open(sys.argv[2]) as f:\n"
                                     "        go = f.read()\n"
                                     "    if go == 'go':\n"
                                     "        break\n"
                                     "else:\n"
                                     "    with open(sys.argv[1], 'w') as f:\n"
                                     "        f.write('never saw go')\n"
                                     "    sys.exit(1)\n"
                                     "# okay, saw 'go', write 'ack'\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ack')\n");
            py.mParams.args.add(from.getName());
            py.mParams.args.add(to.getName());
            py.mParams.autokill = true;
            py.mParams.attached = false;
            py.launch();
            // Capture handle for later
            phandle = py.mPy->getProcessHandle();
            // Wait for the script to wake up and do its first write
            int i = 0, timeout = 60;
            for ( ; i < timeout; ++i)
            {
                yield();
                if (readfile(from.getName(), "from autokill script") == "ok")
                    break;
            
}

TEST_CASE_FIXTURE(llprocess_data, "test_11")
{

        set_test_name("'bogus' test");
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
            "from __future__ import print_function\n"
            "print('Hello world')\n");
        py.mParams.files.add(LLProcess::FileParam("bogus"));
        py.mPy = LLProcess::create(py.mParams);
        CHECK_MESSAGE(! py.mPy, "should have rejected 'bogus'");
        std::string message(recorder.messageWith("bogus"));
        ensure_contains("did not name 'stdin'", message, "stdin");
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_12")
{

        set_test_name("'file' test");
        // Replace this test with one or more real 'file' tests when we
        // implement 'file' support
        PythonProcessLauncher py(get_test_name(),
            "from __future__ import print_function\n"
            "print('Hello world')\n");
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam("file"));
        py.mPy = LLProcess::create(py.mParams);
        CHECK_MESSAGE(! py.mPy, "should have rejected 'file'");
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_13")
{

        set_test_name("'tpipe' test");
        // Replace this test with one or more real 'tpipe' tests when we
        // implement 'tpipe' support
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
            "from __future__ import print_function\n"
            "print('Hello world')\n");
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam("tpipe"));
        py.mPy = LLProcess::create(py.mParams);
        CHECK_MESSAGE(! py.mPy, "should have rejected 'tpipe'");
        std::string message(recorder.messageWith("tpipe"));
        ensure_contains("did not name 'stdout'", message, "stdout");
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_14")
{

        set_test_name("'npipe' test");
        // Replace this test with one or more real 'npipe' tests when we
        // implement 'npipe' support
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
            "from __future__ import print_function\n"
            "print('Hello world')\n");
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam("npipe"));
        py.mPy = LLProcess::create(py.mParams);
        CHECK_MESSAGE(! py.mPy, "should have rejected 'npipe'");
        std::string message(recorder.messageWith("npipe"));
        ensure_contains("did not name 'stderr'", message, "stderr");
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_15")
{

        set_test_name("internal pipe name warning");
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(7)\n");
        py.mParams.files.add(LLProcess::FileParam("pipe", "somename"));
        py.run();                   // verify that it did launch anyway
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  7);
        std::string message(recorder.messageWith("not yet supported"));
        ensure_contains("log message did not mention internal pipe name",
                        message, "somename");
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_16")
{

        set_test_name("get*Pipe() validation");
        PythonProcessLauncher py(get_test_name(),
            "from __future__ import print_function\n"
            "print('this output is expected')\n");
        py.mParams.files.add(LLProcess::FileParam("pipe")); // pipe for  stdin
        py.mParams.files.add(LLProcess::FileParam());       // inherit stdout
        py.mParams.files.add(LLProcess::FileParam("pipe")); // pipe for stderr
        py.run();
        TEST_getPipe(*py.mPy, getWritePipe, getOptWritePipe,
            LLProcess::STDIN,   // VALID
            LLProcess::STDOUT,  // NOPIPE
            LLProcess::STDERR); // BADPIPE
        TEST_getPipe(*py.mPy, getReadPipe,  getOptReadPipe,
            LLProcess::STDERR,  // VALID
            LLProcess::STDOUT,  // NOPIPE
            LLProcess::STDIN);  // BADPIPE
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_17")
{

        set_test_name("talk to stdin/stdout");
        PythonProcessLauncher py(get_test_name(),
                                 "from __future__ import print_function\n"
                                 "import sys, time\n"
                                 "print('ok')\n"
                                 "sys.stdout.flush()\n"
                                 "# wait for 'go' from test program\n"
                                 "go = sys.stdin.readline()\n"
                                 "if go != 'go\\n':\n"
                                 "    sys.exit('expected \"go\", saw %r' % go)\n"
                                 "print('ack')\n");
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        int i, timeout = 60;
        for (i = 0; i < timeout && py.mPy->isRunning() && childout.size() < 3; ++i)
        {
            yield();
        
}

TEST_CASE_FIXTURE(llprocess_data, "test_18")
{

        set_test_name("listen for ReadPipe events");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.stdout.write('abc')\n"
                                 "sys.stdout.flush()\n"
                                 "sys.stdin.readline()\n"
                                 "sys.stdout.write('def')\n"
                                 "sys.stdout.flush()\n"
                                 "sys.stdin.readline()\n"
                                 "sys.stdout.write('ghi\\n')\n"
                                 "sys.stdout.flush()\n"
                                 "sys.stdin.readline()\n"
                                 "sys.stdout.write('second line\\n')\n");
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        std::ostream& childin(py.mPy->getWritePipe(LLProcess::STDIN).get_ostream());
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        // lift the default limit; allow event to carry (some of) the actual data
        childout.setLimit(20);
        // listen for incoming data on childout
        EventListener listener(childout.getPump());
        // also listen with a function that prompts the child to continue
        // every time we see output
        LLTempBoundListener connection(
            childout.getPump().listen("ack", boost::bind(ack, boost::ref(childin), _1)));
        int i, timeout = 60;
        // wait through stuttering first line
        for (i = 0; i < timeout && py.mPy->isRunning() && ! childout.contains("\n"); ++i)
        {
            yield();
        
}

TEST_CASE_FIXTURE(llprocess_data, "test_19")
{

        set_test_name("ReadPipe \"eof\" event");
        PythonProcessLauncher py(get_test_name(),
            "from __future__ import print_function\n"
            "print('Hello from Python!')\n");
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        EventListener listener(childout.getPump());
        waitfor(*py.mPy);
        // We can't be positive there will only be a single event, if the OS
        // (or any other intervening layer) does crazy buffering. What we want
        // to ensure is that there was exactly ONE event with "eof" true, and
        // that it was the LAST event.
        listener.checkHistory(
            [](const EventListener::Listory& history)
            {
                auto rli(history.rbegin()), rlend(history.rend());
                CHECK_MESSAGE(rli != rlend, "no events");
                ensure("last event not \"eof\"", (*rli)["eof"].asBoolean());
                while (++rli != rlend)
                {
                    ensure("\"eof\" event not last", ! (*rli)["eof"].asBoolean());
                
}

TEST_CASE_FIXTURE(llprocess_data, "test_20")
{

        set_test_name("setLimit()");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.stdout.write(sys.argv[1])\n");
        std::string abc("abcdefghijklmnopqrstuvwxyz");
        py.mParams.args.add(abc);
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        // listen for incoming data on childout
        EventListener listener(childout.getPump());
        // but set limit
        childout.setLimit(10);
        ensure_equals("getLimit() after setlimit(10)", childout.getLimit(), 10);
        // okay, pump I/O to pick up output from child
        waitfor(*py.mPy);
        listener.checkHistory(
            [abc](const EventListener::Listory& history)
            {
                CHECK_MESSAGE(! history.empty(, "no events"));
                // For all we know, that data could have arrived in several different
                // bursts... probably not, but anyway, only check the last one.
                ensure_equals("event[\"len\"]",
                              history.back()["len"].asInteger(), abc.length());
                ensure_equals("length of setLimit(10) data",
                              history.back()["data"].asString().length(), 10);
            
}

TEST_CASE_FIXTURE(llprocess_data, "test_21")
{

        set_test_name("peek() ReadPipe data");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.stdout.write(sys.argv[1])\n");
        std::string abc("abcdefghijklmnopqrstuvwxyz");
        py.mParams.args.add(abc);
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        // okay, pump I/O to pick up output from child
        waitfor(*py.mPy);
        // peek() with substr args
        ensure_equals("peek()", childout.peek(), abc);
        ensure_equals("peek(23)", childout.peek(23), abc.substr(23));
        CHECK_MESSAGE(childout.peek(5 == 3, "peek(5, 3)"), abc.substr(5, 3));
        CHECK_MESSAGE(childout.peek(27 == 2, "peek(27, 2)"), "");
        CHECK_MESSAGE(childout.peek(23 == 5, "peek(23, 5)"), "xyz");
        // contains() -- we don't exercise as thoroughly as find() because the
        // contains() implementation is trivially (and visibly) based on find()
        ensure("contains(\":\")", ! childout.contains(":"));
        CHECK_MESSAGE(! childout.contains(':', "contains(':')"));
        ensure("contains(\"d\")", childout.contains("d"));
        CHECK_MESSAGE(childout.contains('d', "contains('d')"));
        ensure("contains(\"klm\")", childout.contains("klm"));
        ensure("contains(\"klx\")", ! childout.contains("klx"));
        // find()
        ensure("find(\":\")", childout.find(":") == LLProcess::ReadPipe::npos);
        CHECK_MESSAGE(childout.find(':', "find(':')") == LLProcess::ReadPipe::npos);
        ensure_equals("find(\"d\")", childout.find("d"), 3);
        ensure_equals("find('d')",   childout.find('d'), 3);
        ensure_equals("find(\"d\", 3)", childout.find("d", 3), 3);
        CHECK_MESSAGE(childout.find('d' == 3, "find('d', 3)"), 3);
        ensure("find(\"d\", 4)", childout.find("d", 4) == LLProcess::ReadPipe::npos);
        CHECK_MESSAGE(childout.find('d', 4, "find('d', 4)") == LLProcess::ReadPipe::npos);
        // The case of offset == end and offset > end are different. In the
        // first case, we can form a valid (albeit empty) iterator range and
        // search that. In the second, guard logic in the implementation must
        // realize we can't form a valid iterator range.
        ensure("find(\"d\", 26)", childout.find("d", 26) == LLProcess::ReadPipe::npos);
        CHECK_MESSAGE(childout.find('d', 26, "find('d', 26)") == LLProcess::ReadPipe::npos);
        ensure("find(\"d\", 27)", childout.find("d", 27) == LLProcess::ReadPipe::npos);
        CHECK_MESSAGE(childout.find('d', 27, "find('d', 27)") == LLProcess::ReadPipe::npos);
        ensure_equals("find(\"ghi\")", childout.find("ghi"), 6);
        ensure_equals("find(\"ghi\", 6)", childout.find("ghi"), 6);
        ensure("find(\"ghi\", 7)", childout.find("ghi", 7) == LLProcess::ReadPipe::npos);
        ensure("find(\"ghi\", 26)", childout.find("ghi", 26) == LLProcess::ReadPipe::npos);
        ensure("find(\"ghi\", 27)", childout.find("ghi", 27) == LLProcess::ReadPipe::npos);
    
}

TEST_CASE_FIXTURE(llprocess_data, "test_22")
{

        set_test_name("bad postend");
        std::string pumpname("postend");
        EventListener listener(LLEventPumps::instance().obtain(pumpname));
        LLProcess::Params params;
        params.desc = get_test_name();
        params.postend = pumpname;
        LLProcessPtr child = LLProcess::create(params);
        CHECK_MESSAGE(! child, "shouldn't have launched");
        listener.checkHistory(
            [&params](const EventListener::Listory& history)
            {
                ensure_equals("number of postend events", history.size(), 1);
                LLSD postend(history.front());
                CHECK_MESSAGE(! postend.has("id", "has id"));
                ensure_equals("desc", postend["desc"].asString(), std::string(params.desc));
                ensure_equals("state", postend["state"].asInteger(), LLProcess::UNSTARTED);
                CHECK_MESSAGE(! postend.has("data", "has data"));
                std::string error(postend["string"]);
                // All we get from canned parameter validation is a bool, so the
                // "validation failed" message we ourselves generate can't mention
                // "executable" by name. Just check that it's nonempty.
                //ensure_contains("error", error, "executable");
                CHECK_MESSAGE(! error.empty(, "string"));
            
}

TEST_CASE_FIXTURE(llprocess_data, "test_23")
{

        set_test_name("good postend");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(35)\n");
        std::string pumpname("postend");
        EventListener listener(LLEventPumps::instance().obtain(pumpname));
        py.mParams.postend = pumpname;
        py.launch();
        LLProcess::id childid(py.mPy->getProcessID());
        // Don't use waitfor(), which calls isRunning(); instead wait for an
        // event on pumpname.
        int i, timeout = 60;
        for (i = 0; i < timeout && listener.mHistory.empty(); ++i)
        {
            yield();
        
}

TEST_CASE_FIXTURE(llprocess_data, "test_24")
{

        set_test_name("all data visible at postend");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 // note, no '\n' in written data
                                 "sys.stdout.write('partial line')\n");
        std::string pumpname("postend");
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.mParams.postend = pumpname;
        py.launch();
        PostendListener listener(py.mPy->getReadPipe(LLProcess::STDOUT),
                                 pumpname,
                                 "partial line");
        waitfor(*py.mPy);
        CHECK_MESSAGE(listener.mTriggered, "postend never triggered");
    
}

} // TEST_SUITE
