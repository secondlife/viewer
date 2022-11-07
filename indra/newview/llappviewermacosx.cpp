/**
 * @file llappviewermacosx.cpp
 * @brief The LLAppViewerMacOSX class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#if !defined LL_DARWIN
    #error "Use only with Mac OS X"
#endif

#define LL_CARBON_CRASH_HANDLER 1

#include "llwindowmacosx.h"
#include "llappviewermacosx-objc.h"

#include "llappviewermacosx.h"
#include "llappviewermacosx-for-objc.h"
#include "llwindowmacosx-objc.h"
#include "llcommandlineparser.h"
#include "llsdserialize.h"

#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llmd5.h"
#include "llfloaterworldmap.h"
#include "llurldispatcher.h"
#include "llerrorcontrol.h"
#include "llvoavatarself.h"         // for gAgentAvatarp->getFullname()
#include <ApplicationServices/ApplicationServices.h>
#ifdef LL_CARBON_CRASH_HANDLER
#include <Carbon/Carbon.h>
#endif
#include <vector>
#include <exception>
#include <fstream>

#include "lldir.h"
#include "lldiriterator.h"
#include <signal.h>
#include <CoreAudio/CoreAudio.h>    // for systemwide mute
class LLMediaCtrl;      // for LLURLDispatcher

namespace 
{
    // The command line args stored.
    // They are not used immediately by the app.
    int gArgC;
    char** gArgV;
    LLAppViewerMacOSX* gViewerAppPtr = NULL;
    std::string gHandleSLURL;
}

void constructViewer()
{
    // Set the working dir to <bundle>/Contents/Resources
    if (chdir(gDirUtilp->getAppRODataDir().c_str()) == -1)
    {
        LL_WARNS("InitOSX") << "Could not change directory to "
                << gDirUtilp->getAppRODataDir() << ": " << strerror(errno)
                << LL_ENDL;
    }

    gViewerAppPtr = new LLAppViewerMacOSX();

    gViewerAppPtr->setErrorHandler(LLAppViewer::handleViewerCrash);
}

bool initViewer()
{
    bool ok = gViewerAppPtr->init();
    if(!ok)
    {
        LL_WARNS("InitOSX") << "Application init failed." << LL_ENDL;
    }
    else if (!gHandleSLURL.empty())
    {
        dispatchUrl(gHandleSLURL);
        gHandleSLURL = "";
    }
    return ok;
}

void handleQuit()
{
    LLAppViewer::instance()->userQuit();
}

// This function is called pumpMainLoop() rather than runMainLoop() because
// it passes control to the viewer's main-loop logic for a single frame. Like
// LLAppViewer::frame(), it returns 'true' when it's done. Until then, it
// expects to be called again by the timer in LLAppDelegate
// (llappdelegate-objc.mm).
bool pumpMainLoop()
{
    bool ret = LLApp::isQuitting();
    if (!ret && gViewerAppPtr != NULL)
    {
        ret = gViewerAppPtr->frame();
    } else {
        ret = true;
    }
    
    return ret;
}

void cleanupViewer()
{
    if(!LLApp::isError())
    {
        if (gViewerAppPtr)
            gViewerAppPtr->cleanup();
    }
    
    delete gViewerAppPtr;
    gViewerAppPtr = NULL;
}

void clearDumpLogsDir()
{
    if (!LLAppViewer::instance()->isSecondInstance())
    {
        gDirUtilp->deleteDirAndContents(gDirUtilp->getDumpLogsDirPath());
    }
}

// The BugsplatMac API is structured as a number of different method
// overrides, each returning a different piece of metadata. But since we
// obtain such metadata by opening and parsing a file, it seems ridiculous to
// reopen and reparse it for every individual string desired. What we want is
// to open and parse the file once, retaining the data for subsequent
// requests. That's why this is an LLSingleton.
// Another approach would be to provide a function that simply returns
// CrashMetadata, storing the struct in LLAppDelegate, but nat doesn't know
// enough Objective-C++ to code that. We'd still have to detect which of the
// method overrides is called first so that the results are order-insensitive.
class CrashMetadataSingleton: public CrashMetadata, public LLSingleton<CrashMetadataSingleton>
{
    LLSINGLETON(CrashMetadataSingleton);

    // convenience method to log each metadata field retrieved by constructor
    std::string get_metadata(const LLSD& info, const LLSD::String& key) const
    {
        std::string data(info[key].asString());
        LL_INFOS("Bugsplat") << "  " << key << "='" << data << "'" << LL_ENDL;
        return data;
    }
};

// Populate the fields of our public base-class struct.
CrashMetadataSingleton::CrashMetadataSingleton()
{
    // Note: we depend on being able to read the static_debug_info.log file
    // from the *previous* run before we overwrite it with the new one for
    // *this* run. LLAppViewer initialization must happen in the Right Order.
    staticDebugPathname = *gViewerAppPtr->getStaticDebugFile();
    std::ifstream static_file(staticDebugPathname);
    LLSD info;
    if (! static_file.is_open())
    {
        LL_WARNS("Bugsplat") << "Can't open '" << staticDebugPathname
                   << "'; no metadata about previous run" << LL_ENDL;
    }
    else if (! LLSDSerialize::deserialize(info, static_file, LLSDSerialize::SIZE_UNLIMITED))
    {
        LL_WARNS("Bugsplat") << "Can't parse '" << staticDebugPathname
                   << "'; no metadata about previous run" << LL_ENDL;
    }
    else
    {
        LL_INFOS("Bugsplat") << "Previous run metadata from '" << staticDebugPathname << "':" << LL_ENDL;
        logFilePathname      = get_metadata(info, "SLLog");
        userSettingsPathname = get_metadata(info, "SettingsFilename");
        accountSettingsPathname = get_metadata(info, "PerAccountSettingsFilename");
        OSInfo               = get_metadata(info, "OSInfo");
        agentFullname        = get_metadata(info, "LoginName");
        // Translate underscores back to spaces
        LLStringUtil::replaceChar(agentFullname, '_', ' ');
        regionName           = get_metadata(info, "CurrentRegion");
        fatalMessage         = get_metadata(info, "FatalMessage");
        
        if (gDirUtilp->fileExists(gDirUtilp->getDumpLogsDirPath()))
        {
            LLDirIterator file_iter(gDirUtilp->getDumpLogsDirPath(), "*.log");
            std::string file_name;
            bool found = true;
            while(found)
            {
                if((found = file_iter.next(file_name)))
                {
                    std::string log_filename = gDirUtilp->getDumpLogsDirPath(file_name);
                    if(LLError::logFileName() != log_filename)
                    {
                        secondLogFilePathname = log_filename;
                    }
                }
            }
        }
    }
}

// Avoid having to compile all of our LLSingleton machinery in Objective-C++.
CrashMetadata& CrashMetadata_instance()
{
    return CrashMetadataSingleton::instance();
}

void infos(const std::string& message)
{
    LL_INFOS("InitOSX", "Bugsplat") << message << LL_ENDL;
}

int main( int argc, char **argv ) 
{
    // Store off the command line args for use later.
    gArgC = argc;
    gArgV = argv;
    return createNSApp(argc, (const char**)argv);
}

LLAppViewerMacOSX::LLAppViewerMacOSX()
{
}

LLAppViewerMacOSX::~LLAppViewerMacOSX()
{
}

bool LLAppViewerMacOSX::init()
{
    return LLAppViewer::init();
}

// MacOSX may add and addition command line arguement for the process serial number.
// The option takes a form like '-psn_0_12345'. The following method should be able to recognize
// and either ignore or return a pair of values for the option.
// look for this method to be added to the parser in parseAndStoreResults.
std::pair<std::string, std::string> parse_psn(const std::string& s)
{
    if (s.find("-psn_") == 0) 
    {
        // *FIX:Mani Not sure that the value makes sense.
        // fix it once the actual -psn_XXX syntax is known.
        return std::make_pair("psn", s.substr(5));
    }
    else 
    {
        return std::make_pair(std::string(), std::string());
    }
}

bool LLAppViewerMacOSX::initParseCommandLine(LLCommandLineParser& clp)
{
    // The next two lines add the support for parsing the mac -psn_XXX arg.
    clp.addOptionDesc("psn", NULL, 1, "MacOSX process serial number");
    clp.setCustomParser(parse_psn);

    // parse the user's command line
    if(clp.parseCommandLine(gArgC, gArgV) == false)
    {
        return false;
    }

    // Get the user's preferred language string based on the Mac OS localization mechanism.
    // To add a new localization:
        // go to the "Resources" section of the project
        // get info on "language.txt"
        // in the "General" tab, click the "Add Localization" button
        // create a new localization for the language you're adding
        // set the contents of the new localization of the file to the string corresponding to our localization
        //   (i.e. "en", "ja", etc.  Use the existing ones as a guide.)
    CFURLRef url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("language"), CFSTR("txt"), NULL);
    char path[MAX_PATH];
    if(CFURLGetFileSystemRepresentation(url, false, (UInt8 *)path, sizeof(path)))
    {
        std::string lang;
        if(_read_file_into_string(lang, path))      /* Flawfinder: ignore*/
        {
            LLControlVariable* c = gSavedSettings.getControl("SystemLanguage");
            if(c)
            {
                c->setValue(lang, false);
            }
        }
    }
    CFRelease(url);
    
    return true;
}

// *FIX:Mani It would be nice to provide a clean interface to get the
// default_unix_signal_handler for the LLApp class.
extern void default_unix_signal_handler(int, siginfo_t *, void *);
bool LLAppViewerMacOSX::restoreErrorTrap()
{
    // This method intends to reinstate signal handlers.
    // *NOTE:Mani It was found that the first execution of a shader was overriding
    // our initial signal handlers somehow.
    // This method will be called (at least) once per mainloop execution.
    // *NOTE:Mani The signals used below are copied over from the 
    // setup_signals() func in LLApp.cpp
    // LLApp could use some way of overriding that func, but for this viewer
    // fix I opt to avoid affecting the server code.
    
    // Set up signal handlers that may result in program termination
    //
    struct sigaction act;
    struct sigaction old_act;
    act.sa_sigaction = default_unix_signal_handler;
    sigemptyset( &act.sa_mask );
    act.sa_flags = SA_SIGINFO;
    
    unsigned int reset_count = 0;
    
#define SET_SIG(SIGNAL) sigaction(SIGNAL, &act, &old_act); \
                        if(act.sa_sigaction != old_act.sa_sigaction) ++reset_count;
    // Synchronous signals
#   ifndef LL_BUGSPLAT
    SET_SIG(SIGABRT) // let bugsplat catch this
#   endif        
    SET_SIG(SIGALRM)
    SET_SIG(SIGBUS)
    SET_SIG(SIGFPE)
    SET_SIG(SIGHUP) 
    SET_SIG(SIGILL)
    SET_SIG(SIGPIPE)
    SET_SIG(SIGSEGV)
    SET_SIG(SIGSYS)
    
    SET_SIG(LL_HEARTBEAT_SIGNAL)
    SET_SIG(LL_SMACKDOWN_SIGNAL)
    
    // Asynchronous signals that are normally ignored
    SET_SIG(SIGCHLD)
    SET_SIG(SIGUSR2)
    
    // Asynchronous signals that result in attempted graceful exit
    SET_SIG(SIGHUP)
    SET_SIG(SIGTERM)
    SET_SIG(SIGINT)
    
    // Asynchronous signals that result in core
    SET_SIG(SIGQUIT)    
#undef SET_SIG
    
    return reset_count == 0;
}

std::string LLAppViewerMacOSX::generateSerialNumber()
{
    char serial_md5[MD5HEX_STR_SIZE];       // Flawfinder: ignore
    serial_md5[0] = 0;

    // JC: Sample code from http://developer.apple.com/technotes/tn/tn1103.html
    CFStringRef serialNumber = NULL;
    io_service_t    platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault,
                                                                 IOServiceMatching("IOPlatformExpertDevice"));
    if (platformExpert)
    {
        serialNumber = (CFStringRef) IORegistryEntryCreateCFProperty(platformExpert,
                                                                     CFSTR(kIOPlatformSerialNumberKey),
                                                                     kCFAllocatorDefault, 0);       
        IOObjectRelease(platformExpert);
    }
    
    if (serialNumber)
    {
        char buffer[MAX_STRING];        // Flawfinder: ignore
        if (CFStringGetCString(serialNumber, buffer, MAX_STRING, kCFStringEncodingASCII))
        {
            LLMD5 md5( (unsigned char*)buffer );
            md5.hex_digest(serial_md5);
        }
        CFRelease(serialNumber);
    }

    return serial_md5;
}

void handleUrl(const char* url_utf8)
{
    if (url_utf8 && gViewerAppPtr)
    {
        gHandleSLURL = "";
        dispatchUrl(url_utf8);
    }
    else if (url_utf8)
    {
        gHandleSLURL = url_utf8;
    }
}

void dispatchUrl(std::string url)
{
    // Safari 3.2 silently mangles secondlife:///app/ URLs into
    // secondlife:/app/ (only one leading slash).
    // Fix them up to meet the URL specification. JC
    const std::string prefix = "secondlife:/app/";
    std::string test_prefix = url.substr(0, prefix.length());
    LLStringUtil::toLower(test_prefix);
    if (test_prefix == prefix)
    {
        url.replace(0, prefix.length(), "secondlife:///app/");
    }
    
    LLMediaCtrl* web = NULL;
    const bool trusted_browser = false;
    LLURLDispatcher::dispatch(url, "", web, trusted_browser);
}
