/**
 * @file media_plugin_base.h
 * @brief Media plugin base class for LLMedia API plugin system
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */

#include "linden_common.h"

#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"


class MediaPluginBase
{
public:
    MediaPluginBase(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
   /** Media plugin destructor. */
    virtual ~MediaPluginBase() {}

   /** Handle received message from plugin loader shell. */
    virtual void receiveMessage(const char *message_string) = 0;

    static void staticReceiveMessage(const char *message_string, void **user_data);

protected:

   /** Plugin status. */
    typedef enum
    {
        STATUS_NONE,
        STATUS_LOADING,
        STATUS_LOADED,
        STATUS_ERROR,
        STATUS_PLAYING,
        STATUS_PAUSED,
        STATUS_DONE
    } EStatus;

   /** Plugin shared memory. */
    class SharedSegmentInfo
    {
    public:
      /** Shared memory address. */
        void *mAddress;
      /** Shared memory size. */
        size_t mSize;
    };

    void sendMessage(const LLPluginMessage &message);
    void sendStatus();
    std::string statusString();
    void setStatus(EStatus status);

    /// Note: The quicktime plugin overrides this to add current time and duration to the message.
    virtual void setDirty(int left, int top, int right, int bottom);

   /** Map of shared memory names to shared memory. */
    typedef std::map<std::string, SharedSegmentInfo> SharedSegmentMap;


   /** Function to send message from plugin to plugin loader shell. */
    LLPluginInstance::sendMessageFunction mHostSendFunction;
   /** Message data being sent to plugin loader shell by mHostSendFunction. */
    void *mHostUserData;
   /** Flag to delete plugin instance (self). */
    bool mDeleteMe;
   /** Pixel array to display. TODO:DOC are pixels always 24-bit RGB format, aligned on 32-bit boundary? Also: calling this a pixel array may be misleading since 1 pixel > 1 char. */
    unsigned char* mPixels;
   /** TODO:DOC what's this for -- does a texture have its own piece of shared memory? updated on size_change_request, cleared on shm_remove */
    std::string mTextureSegmentName;
   /** Width of plugin display in pixels. */
    int mWidth;
   /** Height of plugin display in pixels. */
    int mHeight;
   /** Width of plugin texture. */
    int mTextureWidth;
   /** Height of plugin texture. */
    int mTextureHeight;
   /** Pixel depth (pixel size in bytes). */
    int mDepth;
   /** Current status of plugin. */
    EStatus mStatus;
   /** Map of shared memory segments. */
    SharedSegmentMap mSharedSegments;

};

/** The plugin <b>must</b> define this function to create its instance.
 * It should look something like this:
 * @code
 * {
 *    MediaPluginFoo *self = new MediaPluginFoo(host_send_func, host_user_data);
 *    *plugin_send_func = MediaPluginFoo::staticReceiveMessage;
 *    *plugin_user_data = (void*)self;
 *
 *    return 0;
 * }
 * @endcode
 */
int init_media_plugin(
    LLPluginInstance::sendMessageFunction host_send_func,
    void *host_user_data,
    LLPluginInstance::sendMessageFunction *plugin_send_func,
    void **plugin_user_data);


