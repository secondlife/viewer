/**
 * @file llappviewerlinux.cpp
 * @brief The LLAppViewerLinux class definitions
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

#include "llappviewerlinux.h"

#include "llcommandlineparser.h"

#include "lldiriterator.h"
#include "llurldispatcher.h"        // SLURL from other app instance
#include "llviewernetwork.h"
#include "llviewercontrol.h"
#include "llmd5.h"
#include "llfindlocale.h"
#include "llversioninfo.h"

#include <exception>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "SDL3/SDL.h"

#include "llsdl.h"
#include "llwindowsdl.h"

#ifdef LL_GLIB
#include <gio/gio.h>
#endif

#define VIEWERAPI_SERVICE "com.secondlife.ViewerAppAPIService"
#define VIEWERAPI_PATH "/com/secondlife/ViewerAppAPI"
#define VIEWERAPI_INTERFACE "com.secondlife.ViewerAppAPI"

static const char * DBUS_SERVER = "<node name=\"/com/secondlife/ViewerAppAPI\">\n"
                                  "  <interface name=\"com.secondlife.ViewerAppAPI\">\n"
                                  "    <annotation name=\"org.freedesktop.DBus.GLib.CSymbol\" value=\"viewer_app_api\"/>\n"
                                  "    <method name=\"GoSLURL\">\n"
                                  "      <annotation name=\"org.freedesktop.DBus.GLib.CSymbol\" value=\"dispatchSLURL\"/>\n"
                                  "      <arg type=\"s\" name=\"slurl\" direction=\"in\" />\n"
                                  "    </method>\n"
                                  "  </interface>\n"
                                  "</node>";

typedef struct
{
    GObject parent;
} ViewerAppAPI;

namespace
{
    int gArgC = 0;
    char **gArgV = NULL;
    LLAppViewerLinux* gViewerAppPtr = NULL;
    void (*gOldTerminateHandler)() = NULL;
}

void check_vm_bloat()
{
#if LL_LINUX
    // watch our own VM and RSS sizes, warn if we bloated rapidly
    static const std::string STATS_FILE = "/proc/self/stat";
    FILE *fp = fopen(STATS_FILE.c_str(), "r");
    if (fp)
    {
        static long long last_vm_size = 0;
        static long long last_rss_size = 0;
        const long long significant_vm_difference = 250 * 1024*1024;
        const long long significant_rss_difference = 50 * 1024*1024;
        long long this_vm_size = 0;
        long long this_rss_size = 0;

        ssize_t res;
        size_t dummy;
        char *ptr = nullptr;
        for (int i=0; i<22; ++i) // parse past the values we don't want
        {
            res = getdelim(&ptr, &dummy, ' ', fp);
            if (-1 == res)
            {
                LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
                goto finally;
            }
            free(ptr);
            ptr = nullptr;
        }
        // 23rd space-delimited entry is vsize
        res = getdelim(&ptr, &dummy, ' ', fp);
        llassert(ptr);
        if (-1 == res)
        {
            LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
            goto finally;
        }
        this_vm_size = atoll(ptr);
        free(ptr);
        ptr = nullptr;
        // 24th space-delimited entry is RSS
        res = getdelim(&ptr, &dummy, ' ', fp);
        llassert(ptr);
        if (-1 == res)
        {
            LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
            goto finally;
        }
        this_rss_size = getpagesize() * atoll(ptr);
        free(ptr);
        ptr = nullptr;

        LL_INFOS() << "VM SIZE IS NOW " << (this_vm_size/(1024*1024)) << " MB, RSS SIZE IS NOW " << (this_rss_size/(1024*1024)) << " MB" << LL_ENDL;

        if (llabs(last_vm_size - this_vm_size) > significant_vm_difference)
        {
            if (this_vm_size > last_vm_size)
            {
                LL_WARNS() << "VM size grew by " << (this_vm_size - last_vm_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
            else
            {
                LL_INFOS() << "VM size shrank by " << (last_vm_size - this_vm_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
        }

        if (llabs(last_rss_size - this_rss_size) > significant_rss_difference)
        {
            if (this_rss_size > last_rss_size)
            {
                LL_WARNS() << "RSS size grew by " << (this_rss_size - last_rss_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
            else
            {
                LL_INFOS() << "RSS size shrank by " << (last_rss_size - this_rss_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
        }

        last_rss_size = this_rss_size;
        last_vm_size = this_vm_size;

finally:
        if (ptr)
        {
            free(ptr);
            ptr = nullptr;
        }
        fclose(fp);
    }
#endif // LL_LINUX
}

static void exceptionTerminateHandler()
{
    // reinstall default terminate() handler in case we re-terminate.
    if (gOldTerminateHandler) std::set_terminate(gOldTerminateHandler);
    // treat this like a regular viewer crash, with nice stacktrace etc.
    long *null_ptr;
    null_ptr = 0;
    *null_ptr = 0xDEADBEEF; //Force an exception that will trigger breakpad.
    // we've probably been killed-off before now, but...
    gOldTerminateHandler(); // call old terminate() handler
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    // Call Tracy first thing to have it allocate memory
    // https://github.com/wolfpld/tracy/issues/196
    LL_PROFILER_FRAME_END;
    LL_PROFILER_SET_THREAD_NAME("App");

    gSDLMainHandled = true;

    gArgC = argc;
    gArgV = argv;

    gViewerAppPtr = new LLAppViewerLinux();

    // install unexpected exception handler
    gOldTerminateHandler = std::set_terminate(exceptionTerminateHandler);

    unsetenv( "LD_PRELOAD" ); // <FS:ND/> Get rid of any preloading, we do not want this to happen during startup of plugins.

    // This needs to be set as early as possible
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, LLVersionInfo::getInstance()->getChannel().c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, LLVersionInfo::getInstance()->getVersion().c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "com.secondlife.indra.viewer");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Linden Research Inc");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "Copyright (c) Linden Research, Inc. 2025");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, "https://www.secondlife.com");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

    bool ok = gViewerAppPtr->init();
    if(!ok)
    {
        LL_WARNS() << "Application init failed." << LL_ENDL;
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    // Run the application main loop
    if (!gViewerAppPtr->frame())
    {
#if LL_GLIB
        // Pump until we've nothing left to do or passed 1/15th of a
        // second pumping for this frame.
        static LLTimer pump_timer;
        pump_timer.reset();
        pump_timer.setTimerExpirySec(1.0f / 15.0f);
        do
        {
            g_main_context_iteration(g_main_context_default(), false);
        } while( g_main_context_pending(g_main_context_default()) && !pump_timer.hasExpired());
#endif

        // hack - doesn't belong here - but this is just for debugging
        if (getenv("LL_DEBUG_BLOAT"))
        {
            check_vm_bloat();
        }

        return SDL_APP_CONTINUE;
    }

    if(LLApp::isError())
    {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_SUCCESS;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    return LLWindowSDL::handleEvents(*event);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (!LLApp::isError())
    {
        //
        // We don't want to do cleanup here if the error handler got called -
        // the assumption is that the error handler is responsible for doing
        // app cleanup if there was a problem.
        //
        gViewerAppPtr->cleanup();
    }

    delete gViewerAppPtr;
    gViewerAppPtr = nullptr;
}

bool LLAppViewerLinux::init()
{
    bool success = LLAppViewer::init();

#if LL_SEND_CRASH_REPORTS
    if (success)
    {
        LLAppViewer* pApp = LLAppViewer::instance();
        pApp->initCrashReporting();
    }
#endif

    return success;
}

bool LLAppViewerLinux::restoreErrorTrap()
{
    // *NOTE:Mani there is a case for implementing this on the mac.
    // Linux doesn't need it to my knowledge.
    return true;
}

/////////////////////////////////////////
#if LL_GLIB

typedef struct
{
        GObjectClass parent_class;
} ViewerAppAPIClass;

static void viewerappapi_init(ViewerAppAPI *server);
static void viewerappapi_class_init(ViewerAppAPIClass *klass);

G_DEFINE_TYPE(ViewerAppAPI, viewerappapi, G_TYPE_OBJECT);

void viewerappapi_class_init(ViewerAppAPIClass *klass)
{
}

static void dispatchSLURL(gchar const *slurl)
{
    LL_INFOS() << "Was asked to go to slurl: " << slurl << LL_ENDL;

    std::string url = slurl;
    LLMediaCtrl* web = NULL;
    const bool trusted_browser = false;
    LLURLDispatcher::dispatch(url, "", web, trusted_browser);
}

static void DoMethodeCall (GDBusConnection       *connection,
                                const gchar           *sender,
                                const gchar           *object_path,
                                const gchar           *interface_name,
                                const gchar           *method_name,
                                GVariant              *parameters,
                                GDBusMethodInvocation *invocation,
                                gpointer               user_data)
{
    LL_INFOS() << "DBUS message " << method_name << "  from: " << sender << " interface: " << interface_name << LL_ENDL;
    const gchar *slurl;

    g_variant_get (parameters, "(&s)", &slurl);
    dispatchSLURL(slurl);
}

GDBusNodeInfo *gBusNodeInfo = nullptr;
static const GDBusInterfaceVTable interface_vtable =
        {
                DoMethodeCall
        };
static void busAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    auto id = g_dbus_connection_register_object(connection,
                                                VIEWERAPI_PATH,
                                                gBusNodeInfo->interfaces[0],
                                                &interface_vtable,
                                                NULL,  /* user_data */
                                                             NULL,  /* user_data_free_func */
                                                             NULL); /* GError** */
    g_assert (id > 0);
}

static void nameAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

static void nameLost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{

}
void viewerappapi_init(ViewerAppAPI *server)
{
    gBusNodeInfo = g_dbus_node_info_new_for_xml (DBUS_SERVER, NULL);
    g_assert (gBusNodeInfo != NULL);

    g_bus_own_name(G_BUS_TYPE_SESSION,
                   VIEWERAPI_SERVICE,
                   G_BUS_NAME_OWNER_FLAGS_NONE,
                   busAcquired,
                   nameAcquired,
                   nameLost,
                   NULL,
                   NULL);

}

///

//virtual
bool LLAppViewerLinux::initSLURLHandler()
{
    //ViewerAppAPI *api_server = (ViewerAppAPI*)
    g_object_new(viewerappapi_get_type(), NULL);

    return true;
}

//virtual
bool LLAppViewerLinux::sendURLToOtherInstance(const std::string& url)
{
    auto *pBus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, nullptr);

    if( !pBus )
    {
        LL_WARNS() << "Getting dbus failed." << LL_ENDL;
        return false;
    }

    auto pProxy = g_dbus_proxy_new_sync(pBus, G_DBUS_PROXY_FLAGS_NONE, nullptr,
                                        VIEWERAPI_SERVICE, VIEWERAPI_PATH,
                                        VIEWERAPI_INTERFACE, nullptr, nullptr);

    if( !pProxy )
    {
        LL_WARNS() << "Cannot create new dbus proxy." << LL_ENDL;
        g_object_unref( pBus );
        return false;
    }

    auto *pArgs = g_variant_new( "(s)", url.c_str() );
    if( !pArgs )
    {
        LL_WARNS() << "Cannot create new variant." << LL_ENDL;
        g_object_unref( pBus );
        return false;
    }

    auto pRes  = g_dbus_proxy_call_sync(pProxy,
                                        "GoSLURL",
                                        pArgs,
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1, nullptr, nullptr);



    if( pRes )
        g_variant_unref( pRes );
    g_object_unref( pProxy );
    g_object_unref( pBus );
    return true;
}

#else // LL_GLIB
bool LLAppViewerLinux::initSLURLHandler()
{
    return false; // not implemented without dbus
}
bool LLAppViewerLinux::sendURLToOtherInstance(const std::string& url)
{
    return false; // not implemented without dbus
}
#endif // LL_GLIB

void LLAppViewerLinux::initCrashReporting(bool reportFreeze)
{
}

bool LLAppViewerLinux::beingDebugged()
{
    static enum {unknown, no, yes} debugged = unknown;

    if (debugged == unknown)
    {
        pid_t ppid = getppid();
        char *name;
        int ret;

        ret = asprintf(&name, "/proc/%d/exe", ppid);
        if (ret != -1)
        {
            char buf[1024];
            ssize_t n;

            n = readlink(name, buf, sizeof(buf) - 1);
            if (n != -1)
            {
                char *base = strrchr(buf, '/');
                buf[n + 1] = '\0';
                if (base == NULL)
                {
                    base = buf;
                } else {
                    base += 1;
                }

                if (strcmp(base, "gdb") == 0 || strcmp(base, "lldb") == 0)
                {
                    debugged = yes;
                }
            }
            free(name);
        }
    }

    return debugged == yes;
}

bool LLAppViewerLinux::initParseCommandLine(LLCommandLineParser& clp)
{
    if (!clp.parseCommandLine(gArgC, gArgV))
    {
        return false;
    }

    // Find the system language.
    FL_Locale *locale = NULL;
    FL_Success success = FL_FindLocale(&locale, FL_MESSAGES);
    if (success != 0)
    {
        if (success >= 2 && locale->lang) // confident!
        {
            LL_INFOS("AppInit") << "Language " << ll_safe_string(locale->lang) << LL_ENDL;
            LL_INFOS("AppInit") << "Location " << ll_safe_string(locale->country) << LL_ENDL;
            LL_INFOS("AppInit") << "Variant " << ll_safe_string(locale->variant) << LL_ENDL;

            LLControlVariable* c = gSavedSettings.getControl("SystemLanguage");
            if(c)
            {
                c->setValue(std::string(locale->lang), false);
            }
        }
    }
    FL_FreeLocale(&locale);

    return true;
}

std::string LLAppViewerLinux::generateSerialNumber()
{
    char serial_md5[MD5HEX_STR_SIZE];
    serial_md5[0] = 0;
    std::string best;
    std::string uuiddir("/dev/disk/by-uuid/");

    // trawl /dev/disk/by-uuid looking for a good-looking UUID to grab
    std::string this_name;

    LLDirIterator iter(uuiddir, "*");
    while (iter.next(this_name))
    {
        if (this_name.length() > best.length() ||
            (this_name.length() == best.length() &&
             this_name > best))
        {
            // longest (and secondarily alphabetically last) so far
            best = this_name;
        }
    }

    // we don't return the actual serial number, just a hash of it.
    LLMD5 md5( reinterpret_cast<const unsigned char*>(best.c_str()) );
    md5.hex_digest(serial_md5);

    return serial_md5;
}
