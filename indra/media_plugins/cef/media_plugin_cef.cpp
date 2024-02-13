/**
* @file media_plugin_cef.cpp
* @brief CEF (Chromium Embedding Framework) plugin for LLMedia API plugin system
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
#include "indra_constants.h" // for indra keyboard codes

#include "llglheaders.h" // for GL_* constants
#include "llsdutil.h"
#include "llplugininstance.h"
#include "llpluginmessage.h"
#include "llpluginmessageclasses.h"
#include "llstring.h"
#include "volume_catcher.h"
#include "media_plugin_base.h"

#include "dullahan.h"

////////////////////////////////////////////////////////////////////////////////
//
class MediaPluginCEF :
	public MediaPluginBase
{
public:
	MediaPluginCEF(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data);
	~MediaPluginCEF();

	/*virtual*/
	void receiveMessage(const char* message_string);

private:
	bool init();

	void onPageChangedCallback(const unsigned char* pixels, int x, int y, const int width, const int height);
	void onCustomSchemeURLCallback(std::string url, bool user_gesture, bool is_redirect);
	void onConsoleMessageCallback(std::string message, std::string source, int line);
	void onStatusMessageCallback(std::string value);
	void onTitleChangeCallback(std::string title);
	void onTooltipCallback(std::string text);
	void onLoadStartCallback();
	void onRequestExitCallback();
	void onLoadEndCallback(int httpStatusCode, std::string url);
	void onLoadError(int status, const std::string error_text);
	void onAddressChangeCallback(std::string url);
	void onOpenPopupCallback(std::string url, std::string target);
	bool onHTTPAuthCallback(const std::string host, const std::string realm, std::string& username, std::string& password);
	void onCursorChangedCallback(dullahan::ECursorType type);
	const std::vector<std::string> onFileDialog(dullahan::EFileDialogType dialog_type, const std::string dialog_title, const std::string default_file, const std::string dialog_accept_filter, bool& use_default);
	bool onJSDialogCallback(const std::string origin_url, const std::string message_text, const std::string default_prompt_text);
	bool onJSBeforeUnloadCallback();

	void postDebugMessage(const std::string& msg);
	void authResponse(LLPluginMessage &message);

	void keyEvent(dullahan::EKeyEvent key_event, LLSD native_key_data);
	void unicodeInput(std::string event, LLSD native_key_data);

	void checkEditState();
    void setVolume();

	bool mEnableMediaPluginDebugging;
	std::string mHostLanguage;
	bool mCookiesEnabled;
	bool mPluginsEnabled;
	bool mJavascriptEnabled;
    bool mProxyEnabled;
    std::string mProxyHost;
    int mProxyPort;
	bool mDisableGPU;
	bool mDisableNetworkService;
	bool mUseMockKeyChain;
	bool mDisableWebSecurity;
	bool mFileAccessFromFileUrls;
	std::string mUserAgentSubtring;
	std::string mAuthUsername;
	std::string mAuthPassword;
	bool mAuthOK;
	bool mCanCut;
	bool mCanCopy;
	bool mCanPaste;
    std::string mRootCachePath;
	std::string mCachePath;
	std::string mContextCachePath;
	std::string mCefLogFile;
	bool mCefLogVerbose;
	std::vector<std::string> mPickedFiles;
	VolumeCatcher mVolumeCatcher;
	F32 mCurVolume;
	dullahan* mCEFLib;
};

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginCEF::MediaPluginCEF(LLPluginInstance::sendMessageFunction host_send_func, void *host_user_data) :
MediaPluginBase(host_send_func, host_user_data)
{
	mWidth = 0;
	mHeight = 0;
	mDepth = 4;
	mPixels = 0;
	mEnableMediaPluginDebugging = true;
	mHostLanguage = "en";
	mCookiesEnabled = true;
	mPluginsEnabled = false;
	mJavascriptEnabled = true;
    mProxyEnabled = false;
    mProxyHost = "";
    mProxyPort = 0;
	mDisableGPU = false;
	mDisableNetworkService = true;
	mUseMockKeyChain = true;
	mDisableWebSecurity = false;
	mFileAccessFromFileUrls = false;
	mUserAgentSubtring = "";
	mAuthUsername = "";
	mAuthPassword = "";
	mAuthOK = false;
	mCanCut = false;
	mCanCopy = false;
	mCanPaste = false;
	mCachePath = "";
	mCefLogFile = "";
	mCefLogVerbose = false;
	mPickedFiles.clear();
	mCurVolume = 0.0;

	mCEFLib = new dullahan();

	setVolume();
}

////////////////////////////////////////////////////////////////////////////////
//
MediaPluginCEF::~MediaPluginCEF()
{
	mCEFLib->shutdown();
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::postDebugMessage(const std::string& msg)
{
	if (mEnableMediaPluginDebugging)
	{
		std::stringstream str;
		str << "@Media Msg> " << msg;

		LLPluginMessage debug_message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "debug_message");
		debug_message.setValue("message_text", str.str());
		debug_message.setValue("message_level", "info");
		sendMessage(debug_message);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onPageChangedCallback(const unsigned char* pixels, int x, int y, const int width, const int height)
{
	if( mPixels && pixels )
	{
		if (mWidth == width && mHeight == height)
		{
			memcpy(mPixels, pixels, mWidth * mHeight * mDepth);
		}
		else
		{
			mCEFLib->setSize(mWidth, mHeight);
		}
		setDirty(0, 0, mWidth, mHeight);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onConsoleMessageCallback(std::string message, std::string source, int line)
{
	std::stringstream str;
	str << "Console message: " << message << " in file(" << source << ") at line " << line;
	postDebugMessage(str.str());
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onStatusMessageCallback(std::string value)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "status_text");
	message.setValue("status", value);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onTitleChangeCallback(std::string title)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", title);
	message.setValueBoolean("history_back_available", mCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available", mCEFLib->canGoForward());
	sendMessage(message);
}

void MediaPluginCEF::onTooltipCallback(std::string text)
{
    LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "tooltip_text");
    message.setValue("tooltip", text);
    sendMessage(message);
}
////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onLoadStartCallback()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_begin");
	//message.setValue("uri", event.getEventUri());  // not easily available here in CEF - needed?
	message.setValueBoolean("history_back_available", mCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available", mCEFLib->canGoForward());
	sendMessage(message);
}

/////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onLoadError(int status, const std::string error_text)
{
	std::stringstream msg;

	msg << "<b>Loading error!</b>";
	msg << "<p>";
	msg << "Message: " << error_text;
	msg << "<br>";
	msg << "Code: " << status;

	mCEFLib->showBrowserMessage(msg.str());
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onRequestExitCallback()
{
	LLPluginMessage message("base", "goodbye");
	sendMessage(message);

	// Will trigger delete on next staticReceiveMessage()
	mDeleteMe = true;
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onLoadEndCallback(int httpStatusCode, std::string url)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "navigate_complete");
	//message.setValue("uri", event.getEventUri());  // not easily available here in CEF - needed?
	message.setValueS32("result_code", httpStatusCode);
	message.setValueBoolean("history_back_available", mCEFLib->canGoBack());
	message.setValueBoolean("history_forward_available", mCEFLib->canGoForward());
	message.setValue("uri", url);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onAddressChangeCallback(std::string url)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "location_changed");
	message.setValue("uri", url);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onOpenPopupCallback(std::string url, std::string target)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "click_href");
	message.setValue("uri", url);
	message.setValue("target", target);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onCustomSchemeURLCallback(std::string url, bool user_gesture, bool is_redirect)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "click_nofollow");
    message.setValue("uri", url);

    // indicate if this interaction was from a user click (okay on a SLAPP) or 
    // via a navigation (e.g. a data URL - see SL-18151) (not okay on a SLAPP)
    const std::string nav_type = user_gesture ? "clicked" : "navigated";

	message.setValue("nav_type", nav_type);
    message.setValueBoolean("is_redirect", is_redirect);

	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginCEF::onHTTPAuthCallback(const std::string host, const std::string realm, std::string& username, std::string& password)
{
	mAuthOK = false;

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "auth_request");
	message.setValue("url", host);
	message.setValue("realm", realm);
	message.setValueBoolean("blocking_request", true);

	// The "blocking_request" key in the message means this sendMessage call will block until a response is received.
	sendMessage(message);

	if (mAuthOK)
	{
		username = mAuthUsername;
		password = mAuthPassword;
	}

	return mAuthOK;
}

////////////////////////////////////////////////////////////////////////////////
//
const std::vector<std::string> MediaPluginCEF::onFileDialog(dullahan::EFileDialogType dialog_type, const std::string dialog_title, const std::string default_file, std::string dialog_accept_filter, bool& use_default)
{
	// do not use the default CEF file picker
	use_default = false;

	if (dialog_type == dullahan::FD_OPEN_FILE)
	{
		mPickedFiles.clear();

		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "pick_file");
		message.setValueBoolean("blocking_request", true);
		message.setValueBoolean("multiple_files", false);

		sendMessage(message);

		return mPickedFiles;
	}
	else if (dialog_type == dullahan::FD_OPEN_MULTIPLE_FILES)
	{
		mPickedFiles.clear();

		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "pick_file");
		message.setValueBoolean("blocking_request", true);
		message.setValueBoolean("multiple_files", true);

		sendMessage(message);

		return mPickedFiles;
	}
	else if (dialog_type == dullahan::FD_SAVE_FILE)
	{
		mPickedFiles.clear();
		mAuthOK = false;

		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "file_download");
		message.setValueBoolean("blocking_request", true);
		message.setValue("filename", default_file);

		sendMessage(message);

		return mPickedFiles;
	}

	return std::vector<std::string>();
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginCEF::onJSDialogCallback(const std::string origin_url, const std::string message_text, const std::string default_prompt_text)
{
	// return true indicates we suppress the JavaScript alert UI entirely
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginCEF::onJSBeforeUnloadCallback()
{
	// return true indicates we suppress the JavaScript UI entirely
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::onCursorChangedCallback(dullahan::ECursorType type)
{
	std::string name = "";

	switch (type)
	{
        case dullahan::CT_POINTER:
            name = "UI_CURSOR_ARROW";
            break;
        case dullahan::CT_CROSS:
            name = "UI_CURSOR_CROSS";
            break;
        case dullahan::CT_HAND:
            name = "UI_CURSOR_HAND";
            break;
		case dullahan::CT_IBEAM:
			name = "UI_CURSOR_IBEAM";
			break;
        case dullahan::CT_WAIT:
            name = "UI_CURSOR_WAIT";
            break;
        //case dullahan::CT_HELP:
        case dullahan::CT_ROWRESIZE:
        case dullahan::CT_NORTHRESIZE:
        case dullahan::CT_SOUTHRESIZE:
        case dullahan::CT_NORTHSOUTHRESIZE:
            name = "UI_CURSOR_SIZENS";
            break;
        case dullahan::CT_COLUMNRESIZE:
        case dullahan::CT_EASTRESIZE:
        case dullahan::CT_WESTRESIZE:
        case dullahan::CT_EASTWESTRESIZE:
            name = "UI_CURSOR_SIZEWE";
            break;
        case dullahan::CT_NORTHEASTRESIZE:
        case dullahan::CT_SOUTHWESTRESIZE:
        case dullahan::CT_NORTHEASTSOUTHWESTRESIZE:
            name = "UI_CURSOR_SIZENESW";
            break;
        case dullahan::CT_SOUTHEASTRESIZE:
        case dullahan::CT_NORTHWESTRESIZE:
        case dullahan::CT_NORTHWESTSOUTHEASTRESIZE:
            name = "UI_CURSOR_SIZENWSE";
            break;
        case dullahan::CT_MOVE:
            name = "UI_CURSOR_SIZEALL";
            break;
        //case dullahan::CT_MIDDLEPANNING:
        //case dullahan::CT_EASTPANNING:
        //case dullahan::CT_NORTHPANNING:
        //case dullahan::CT_NORTHEASTPANNING:
        //case dullahan::CT_NORTHWESTPANNING:
        //case dullahan::CT_SOUTHPANNING:
        //case dullahan::CT_SOUTHEASTPANNING:
        //case dullahan::CT_SOUTHWESTPANNING:
        //case dullahan::CT_WESTPANNING:
        //case dullahan::CT_VERTICALTEXT:
        //case dullahan::CT_CELL:
        //case dullahan::CT_CONTEXTMENU:
        case dullahan::CT_ALIAS:
            name = "UI_CURSOR_TOOLMEDIAOPEN";
            break;
        case dullahan::CT_PROGRESS:
            name = "UI_CURSOR_WORKING";
            break;
        case dullahan::CT_COPY:
            name = "UI_CURSOR_ARROWCOPY";
            break;
        case dullahan::CT_NONE:
            name = "UI_CURSOR_NO";
            break;
        case dullahan::CT_NODROP:
        case dullahan::CT_NOTALLOWED:
            name = "UI_CURSOR_NOLOCKED";
            break;
        case dullahan::CT_ZOOMIN:
            name = "UI_CURSOR_TOOLZOOMIN";
            break;
        case dullahan::CT_ZOOMOUT:
            name = "UI_CURSOR_TOOLZOOMOUT";
            break;
        case dullahan::CT_GRAB:
            name = "UI_CURSOR_TOOLGRAB";
            break;
        //case dullahan::CT_GRABING:
        //case dullahan::CT_CUSTOM:

		default:
			LL_WARNS() << "Unknown cursor ID: " << (int)type << LL_ENDL;
			break;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "cursor_changed");
	message.setValue("name", name);
	sendMessage(message);
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::authResponse(LLPluginMessage &message)
{
	mAuthOK = message.getValueBoolean("ok");
	if (mAuthOK)
	{
		mAuthUsername = message.getValue("username");
		mAuthPassword = message.getValue("password");
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::receiveMessage(const char* message_string)
{
	//  std::cerr << "MediaPluginCEF::receiveMessage: received message: \"" << message_string << "\"" << std::endl;
	LLPluginMessage message_in;

	if (message_in.parse(message_string) >= 0)
	{
		std::string message_class = message_in.getClass();
		std::string message_name = message_in.getName();
		if (message_class == LLPLUGIN_MESSAGE_CLASS_BASE)
		{
			if (message_name == "init")
			{
				LLPluginMessage message("base", "init_response");
				LLSD versions = LLSD::emptyMap();
				versions[LLPLUGIN_MESSAGE_CLASS_BASE] = LLPLUGIN_MESSAGE_CLASS_BASE_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA] = LLPLUGIN_MESSAGE_CLASS_MEDIA_VERSION;
				versions[LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER] = LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER_VERSION;
				message.setValueLLSD("versions", versions);

				std::string plugin_version = "CEF plugin 1.1.412";
				message.setValue("plugin_version", plugin_version);
				sendMessage(message);
			}
			else if (message_name == "idle")
			{
				mCEFLib->update();

				mVolumeCatcher.pump();

				// this seems bad but unless the state changes (it won't until we figure out
				// how to get CEF to tell us if copy/cut/paste is available) then this function
				// will return immediately
				checkEditState();
			}
			else if (message_name == "cleanup")
			{
				mCEFLib->requestExit();
			}
			else if (message_name == "force_exit")
			{
				mDeleteMe = true;
			}
			else if (message_name == "shm_added")
			{
				SharedSegmentInfo info;
				info.mAddress = message_in.getValuePointer("address");
				info.mSize = (size_t)message_in.getValueS32("size");
				std::string name = message_in.getValue("name");

				mSharedSegments.insert(SharedSegmentMap::value_type(name, info));

			}
			else if (message_name == "shm_remove")
			{
				std::string name = message_in.getValue("name");

				SharedSegmentMap::iterator iter = mSharedSegments.find(name);
				if (iter != mSharedSegments.end())
				{
					if (mPixels == iter->second.mAddress)
					{
						mPixels = NULL;
						mTextureSegmentName.clear();
					}
					mSharedSegments.erase(iter);
				}
				else
				{
				}

				LLPluginMessage message("base", "shm_remove_response");
				message.setValue("name", name);
				sendMessage(message);
			}
			else
			{
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
		{
            if (message_name == "init")
            {
                // event callbacks from Dullahan
                mCEFLib->setOnPageChangedCallback(std::bind(&MediaPluginCEF::onPageChangedCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
                mCEFLib->setOnCustomSchemeURLCallback(std::bind(&MediaPluginCEF::onCustomSchemeURLCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                mCEFLib->setOnConsoleMessageCallback(std::bind(&MediaPluginCEF::onConsoleMessageCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                mCEFLib->setOnStatusMessageCallback(std::bind(&MediaPluginCEF::onStatusMessageCallback, this, std::placeholders::_1));
                mCEFLib->setOnTitleChangeCallback(std::bind(&MediaPluginCEF::onTitleChangeCallback, this, std::placeholders::_1));
                mCEFLib->setOnTooltipCallback(std::bind(&MediaPluginCEF::onTooltipCallback, this, std::placeholders::_1));
                mCEFLib->setOnLoadStartCallback(std::bind(&MediaPluginCEF::onLoadStartCallback, this));
                mCEFLib->setOnLoadEndCallback(std::bind(&MediaPluginCEF::onLoadEndCallback, this, std::placeholders::_1, std::placeholders::_2));
                mCEFLib->setOnLoadErrorCallback(std::bind(&MediaPluginCEF::onLoadError, this, std::placeholders::_1, std::placeholders::_2));
                mCEFLib->setOnAddressChangeCallback(std::bind(&MediaPluginCEF::onAddressChangeCallback, this, std::placeholders::_1));
                mCEFLib->setOnOpenPopupCallback(std::bind(&MediaPluginCEF::onOpenPopupCallback, this, std::placeholders::_1, std::placeholders::_2));
                mCEFLib->setOnHTTPAuthCallback(std::bind(&MediaPluginCEF::onHTTPAuthCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
                mCEFLib->setOnFileDialogCallback(std::bind(&MediaPluginCEF::onFileDialog, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
                mCEFLib->setOnCursorChangedCallback(std::bind(&MediaPluginCEF::onCursorChangedCallback, this, std::placeholders::_1));
                mCEFLib->setOnRequestExitCallback(std::bind(&MediaPluginCEF::onRequestExitCallback, this));
                mCEFLib->setOnJSDialogCallback(std::bind(&MediaPluginCEF::onJSDialogCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                mCEFLib->setOnJSBeforeUnloadCallback(std::bind(&MediaPluginCEF::onJSBeforeUnloadCallback, this));

                dullahan::dullahan_settings settings;
#if LL_WINDOWS
                // As of CEF version 83+, for Windows versions, we need to tell CEF 
                // where the host helper process is since this DLL is not in the same
                // dir as the executable that loaded it (SLPlugin.exe). The code in 
                // Dullahan that tried to figure out the location automatically uses 
                // the location of the exe which isn't helpful so we tell it explicitly.
                std::vector<wchar_t> buffer(MAX_PATH + 1);
                GetCurrentDirectoryW(MAX_PATH, &buffer[0]);
                settings.host_process_path = ll_convert_wide_to_string(&buffer[0]);
#endif
                settings.accept_language_list = mHostLanguage;

                // SL-15560: Product team overruled my change to set the default
                // embedded background color to match the floater background
                // and set it to white
                settings.background_color = 0xffffffff;	// white 

                settings.cache_enabled = true;
                settings.root_cache_path = mRootCachePath;
                settings.cache_path = mCachePath;
                settings.context_cache_path = mContextCachePath;
                settings.cookies_enabled = mCookiesEnabled;

                // configure proxy argument if enabled and valid
                if (mProxyEnabled && mProxyHost.length())
                {
                    std::ostringstream proxy_url;
                    proxy_url << mProxyHost << ":" << mProxyPort;
                    settings.proxy_host_port = proxy_url.str();
                }
				settings.disable_gpu = mDisableGPU;
#if LL_DARWIN
				settings.disable_network_service = mDisableNetworkService;
				settings.use_mock_keychain = mUseMockKeyChain;
#endif
                // these were added to facilitate loading images directly into a local
                // web page for the prototype 360 project in 2017 - something that is 
                // disallowed normally by the browser security model. Now the the source
                // (cubemap) images are stores as JavaScript, we can avoid opening up
                // this security hole (it was only set for the 360 floater but still 
                // a concern). Leaving them here, explicitly turn off vs removing 
                // entirely from this source file so that others are aware of them 
                // in the future.
                settings.disable_web_security = false;
                settings.file_access_from_file_urls = false;

                settings.flash_enabled = mPluginsEnabled;

				// This setting applies to all plugins, not just Flash
				// Regarding, SL-15559 PDF files do not load in CEF v91,
				// it turns out that on Windows, PDF support is treated
				// as a plugin on Windows only so turning all plugins
				// off, disabled built in PDF support.  (Works okay in
				// macOS surprisingly). To mitigrate this, we set the global
				// media enabled flag to whatever the consumer wants and 
				// explicitly disable Flash with a different setting (below)
				settings.plugins_enabled = mPluginsEnabled;

				// SL-14897 Disable Flash support in the embedded browser
				settings.flash_enabled = false;

				settings.flip_mouse_y = false;
				settings.flip_pixels_y = true;
				settings.frame_rate = 60;
				settings.force_wave_audio = true;
				settings.initial_height = 1024;
				settings.initial_width = 1024;
				settings.java_enabled = false;
				settings.javascript_enabled = mJavascriptEnabled;
				settings.media_stream_enabled = false; // MAINT-6060 - WebRTC media removed until we can add granularity/query UI
				
				settings.user_agent_substring = mCEFLib->makeCompatibleUserAgentString(mUserAgentSubtring);
				settings.webgl_enabled = true;
				settings.log_file = mCefLogFile;
				settings.log_verbose = mCefLogVerbose;
				settings.autoplay_without_gesture = true;

				std::vector<std::string> custom_schemes(1, "secondlife");
				mCEFLib->setCustomSchemes(custom_schemes);

				bool result = mCEFLib->init(settings);
                if (!result)
                {
                    // if this fails, the media system in viewer will put up a message
                }

				// now we can set page zoom factor
				F32 factor = (F32)message_in.getValueReal("factor");
#if LL_DARWIN
				//temporary fix for SL-10473: issue with displaying checkboxes on Mojave
				factor*=1.001;
#endif
				mCEFLib->setPageZoom(factor);

				// Plugin gets to decide the texture parameters to use.
				mDepth = 4;
				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "texture_params");
				message.setValueS32("default_width", 1024);
				message.setValueS32("default_height", 1024);
				message.setValueS32("depth", mDepth);
				message.setValueU32("internalformat", GL_RGB);
				message.setValueU32("format", GL_BGRA);
				message.setValueU32("type", GL_UNSIGNED_BYTE);
				message.setValueBoolean("coords_opengl", true);
				sendMessage(message);
			}
			else if (message_name == "set_user_data_path")
			{
				std::string user_data_path_cache = message_in.getValue("cache_path");
				std::string subfolder = message_in.getValue("username");

				mRootCachePath = user_data_path_cache + "cef_cache";
                if (!subfolder.empty())
                {
                    std::string delim;
#if LL_WINDOWS
                    // media plugin doesn't have access to gDirUtilp
                    delim = "\\";
#else
                    delim = "/";
#endif
                    mCachePath = mRootCachePath + delim + subfolder;
                }
                else
                {
                    mCachePath = mRootCachePath;
                }
                mContextCachePath = ""; // disabled by ""
				mCefLogFile = message_in.getValue("cef_log_file");
				mCefLogVerbose = message_in.getValueBoolean("cef_verbose_log");
			}
			else if (message_name == "size_change")
			{
				std::string name = message_in.getValue("name");
				S32 width = message_in.getValueS32("width");
				S32 height = message_in.getValueS32("height");
				S32 texture_width = message_in.getValueS32("texture_width");
				S32 texture_height = message_in.getValueS32("texture_height");

				if (!name.empty())
				{
					// Find the shared memory region with this name
					SharedSegmentMap::iterator iter = mSharedSegments.find(name);
					if (iter != mSharedSegments.end())
					{
						mPixels = (unsigned char*)iter->second.mAddress;
						mWidth = width;
						mHeight = height;

						mTextureWidth = texture_width;
						mTextureHeight = texture_height;

						mCEFLib->setSize(mWidth, mHeight);
					};
				};

				LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change_response");
				message.setValue("name", name);
				message.setValueS32("width", width);
				message.setValueS32("height", height);
				message.setValueS32("texture_width", texture_width);
				message.setValueS32("texture_height", texture_height);
				sendMessage(message);

			}
			else if (message_name == "set_language_code")
			{
				mHostLanguage = message_in.getValue("language");
			}
			else if (message_name == "load_uri")
			{
				std::string uri = message_in.getValue("uri");
				mCEFLib->navigate(uri);
			}
			else if (message_name == "execute_javascript")
			{
				std::string code = message_in.getValue("code");
				mCEFLib->executeJavaScript(code);
			}
			else if (message_name == "set_cookie")
			{
				std::string uri = message_in.getValue("uri");
				std::string name = message_in.getValue("name");
				std::string value = message_in.getValue("value");
				std::string domain = message_in.getValue("domain");
				std::string path = message_in.getValue("path");
				bool httponly = message_in.getValueBoolean("httponly");
				bool secure = message_in.getValueBoolean("secure");
				mCEFLib->setCookie(uri, name, value, domain, path, httponly, secure);
			}
			else if (message_name == "mouse_event")
			{
				std::string event = message_in.getValue("event");

				S32 x = message_in.getValueS32("x");
				S32 y = message_in.getValueS32("y");

				// only even send left mouse button events to the CEF library
				// (partially prompted by crash in macOS CEF when sending right button events)
				// we catch the right click in viewer and display our own context menu anyway
				S32 button = message_in.getValueS32("button");
				dullahan::EMouseButton btn = dullahan::MB_MOUSE_BUTTON_LEFT;

				if (event == "down" && button == 0)
				{
					mCEFLib->mouseButton(btn, dullahan::ME_MOUSE_DOWN, x, y);
					mCEFLib->setFocus();

					std::stringstream str;
					str << "Mouse down at = " << x << ", " << y;
					postDebugMessage(str.str());
				}
				else if (event == "up" && button == 0)
				{
					mCEFLib->mouseButton(btn, dullahan::ME_MOUSE_UP, x, y);

					std::stringstream str;
					str << "Mouse up at = " << x << ", " << y;
					postDebugMessage(str.str());
				}
				else if (event == "double_click")
				{
					mCEFLib->mouseButton(btn, dullahan::ME_MOUSE_DOUBLE_CLICK, x, y);
				}
				else
				{
					mCEFLib->mouseMove(x, y);
				}
			}
			else if (message_name == "scroll_event")
			{
				// Mouse coordinates for cef to be able to scroll 'containers'
				S32 x = message_in.getValueS32("x");
				S32 y = message_in.getValueS32("y");

				// Wheel's clicks
				S32 delta_x = message_in.getValueS32("clicks_x");
				S32 delta_y = message_in.getValueS32("clicks_y");
				const int scaling_factor = 40;
				delta_x *= -scaling_factor;
				delta_y *= -scaling_factor;

				mCEFLib->mouseWheel(x, y, delta_x, delta_y);
			}
			else if (message_name == "text_event")
			{
				std::string event = message_in.getValue("event");
				LLSD native_key_data = message_in.getValueLLSD("native_key_data");
				unicodeInput(event, native_key_data);
			}
			else if (message_name == "key_event")
			{
#if LL_DARWIN
				std::string event = message_in.getValue("event");
                LLSD native_key_data = message_in.getValueLLSD("native_key_data");

                dullahan::EKeyEvent key_event = dullahan::KE_KEY_UP;
                if (event == "down")
                {
                    key_event = dullahan::KE_KEY_DOWN;
                }
                else if (event == "repeat")
                {
                    key_event = dullahan::KE_KEY_REPEAT;
                }

                keyEvent(key_event, native_key_data);

#elif LL_WINDOWS
				std::string event = message_in.getValue("event");
				LLSD native_key_data = message_in.getValueLLSD("native_key_data");

				// Treat unknown events as key-up for safety.
				dullahan::EKeyEvent key_event = dullahan::KE_KEY_UP;
				if (event == "down")
				{
					key_event = dullahan::KE_KEY_DOWN;
				}
				else if (event == "repeat")
				{
					key_event = dullahan::KE_KEY_REPEAT;
				}

				keyEvent(key_event, native_key_data);
#endif
			}
			else if (message_name == "enable_media_plugin_debugging")
			{
				mEnableMediaPluginDebugging = message_in.getValueBoolean("enable");
			}
			if (message_name == "pick_file_response")
			{
				LLSD file_list_llsd = message_in.getValueLLSD("file_list");

				LLSD::array_const_iterator iter = file_list_llsd.beginArray();
				LLSD::array_const_iterator end = file_list_llsd.endArray();
				for (; iter != end; ++iter)
				{
					mPickedFiles.push_back(((*iter).asString()));
				}
			}
			if (message_name == "auth_response")
			{
				authResponse(message_in);
			}
			if (message_name == "edit_cut")
			{
				mCEFLib->editCut();
			}
			if (message_name == "edit_copy")
			{
				mCEFLib->editCopy();
			}
			if (message_name == "edit_paste")
			{
				mCEFLib->editPaste();
			}
		}
		else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER)
		{
			if (message_name == "set_page_zoom_factor")
			{
				F32 factor = (F32)message_in.getValueReal("factor");
#if LL_DARWIN
				//temporary fix for SL-10473: issue with displaying checkboxes on Mojave
				factor*=1.001;
#endif
				mCEFLib->setPageZoom(factor);
			}
			if (message_name == "browse_stop")
			{
				mCEFLib->stop();
			}
			else if (message_name == "browse_reload")
			{
				bool ignore_cache = true;
				mCEFLib->reload(ignore_cache);
			}
			else if (message_name == "browse_forward")
			{
				mCEFLib->goForward();
			}
			else if (message_name == "browse_back")
			{
				mCEFLib->goBack();
			}
			else if (message_name == "cookies_enabled")
			{
				mCookiesEnabled = message_in.getValueBoolean("enable");
			}
			else if (message_name == "clear_cookies")
			{
				mCEFLib->deleteAllCookies();
			}
			else if (message_name == "set_user_agent")
			{
				mUserAgentSubtring = message_in.getValue("user_agent");
			}
			else if (message_name == "show_web_inspector")
			{
				mCEFLib->showDevTools();
			}
			else if (message_name == "plugins_enabled")
			{
				mPluginsEnabled = message_in.getValueBoolean("enable");
			}
			else if (message_name == "javascript_enabled")
			{
				mJavascriptEnabled = message_in.getValueBoolean("enable");
			}
			else if (message_name == "gpu_disabled")
			{
				mDisableGPU = message_in.getValueBoolean("disable");
			}
            else if (message_name == "proxy_setup")
            {
                mProxyEnabled = message_in.getValueBoolean("enable");
                mProxyHost = message_in.getValue("host");
                mProxyPort = message_in.getValueS32("port");
            }
			else if (message_name == "web_security_disabled")
			{
				mDisableWebSecurity = message_in.getValueBoolean("disabled");
			}
			else if (message_name == "file_access_from_file_urls")
			{
				mFileAccessFromFileUrls = message_in.getValueBoolean("enabled");
			}
		}
        else if (message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME)
        {
            if (message_name == "set_volume")
            {
				F32 volume = (F32)message_in.getValueReal("volume");
				mCurVolume = volume;
                setVolume();
            }
        }
        else
		{
		};
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::keyEvent(dullahan::EKeyEvent key_event, LLSD native_key_data = LLSD::emptyMap())
{
#if LL_DARWIN
	U32 event_modifiers = native_key_data["event_modifiers"].asInteger();
	U32 event_keycode = native_key_data["event_keycode"].asInteger();
	U32 event_chars = native_key_data["event_chars"].asInteger();
	U32 event_umodchars = native_key_data["event_umodchars"].asInteger();
	bool event_isrepeat = native_key_data["event_isrepeat"].asBoolean();

	// adding new code below in unicodeInput means we don't send ascii chars
	// here too or we get double key presses on a mac.
	bool esc_key = (event_umodchars == 27);
	bool tab_key_up = (event_umodchars == 9) && (key_event == dullahan::EKeyEvent::KE_KEY_UP);
	if ((esc_key || ((unsigned char)event_chars < 0x10 || (unsigned char)event_chars >= 0x7f )) && !tab_key_up)
	{
		mCEFLib->nativeKeyboardEventOSX(key_event, event_modifiers, 
										event_keycode, event_chars, 
										event_umodchars, event_isrepeat);
	}
#elif LL_WINDOWS
	U32 msg = ll_U32_from_sd(native_key_data["msg"]);
	U32 wparam = ll_U32_from_sd(native_key_data["w_param"]);
	U64 lparam = ll_U32_from_sd(native_key_data["l_param"]);

	mCEFLib->nativeKeyboardEventWin(msg, wparam, lparam);
#endif
};

void MediaPluginCEF::unicodeInput(std::string event, LLSD native_key_data = LLSD::emptyMap())
{
#if LL_DARWIN
	// i didn't think this code was needed for macOS but without it, the IME
	// input in japanese (and likely others too) doesn't work correctly.
	// see maint-7654
	U32 event_modifiers = native_key_data["event_modifiers"].asInteger();
	U32 event_keycode = native_key_data["event_keycode"].asInteger();
	U32 event_chars = native_key_data["event_chars"].asInteger();
	U32 event_umodchars = native_key_data["event_umodchars"].asInteger();
	bool event_isrepeat = native_key_data["event_isrepeat"].asBoolean();

    dullahan::EKeyEvent key_event = dullahan::KE_KEY_UP;
    if (event == "down")
    {
        key_event = dullahan::KE_KEY_DOWN;
    }

	mCEFLib->nativeKeyboardEventOSX(key_event, event_modifiers, 
									event_keycode, event_chars, 
									event_umodchars, event_isrepeat);
#elif LL_WINDOWS
	event = ""; // not needed here but prevents unused var warning as error
	U32 msg = ll_U32_from_sd(native_key_data["msg"]);
	U32 wparam = ll_U32_from_sd(native_key_data["w_param"]);
	U64 lparam = ll_U32_from_sd(native_key_data["l_param"]);
	mCEFLib->nativeKeyboardEventWin(msg, wparam, lparam);
#endif
};

////////////////////////////////////////////////////////////////////////////////
//
void MediaPluginCEF::checkEditState()
{
	bool can_cut = mCEFLib->editCanCut();
	bool can_copy = mCEFLib->editCanCopy();
	bool can_paste = mCEFLib->editCanPaste();

	if ((can_cut != mCanCut) || (can_copy != mCanCopy) || (can_paste != mCanPaste))
	{
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "edit_state");

		if (can_cut != mCanCut)
		{
			mCanCut = can_cut;
			message.setValueBoolean("cut", can_cut);
		}

		if (can_copy != mCanCopy)
		{
			mCanCopy = can_copy;
			message.setValueBoolean("copy", can_copy);
		}

		if (can_paste != mCanPaste)
		{
			mCanPaste = can_paste;
			message.setValueBoolean("paste", can_paste);
		}

		sendMessage(message);
	}
}

void MediaPluginCEF::setVolume()
{
	mVolumeCatcher.setVolume(mCurVolume);
}

////////////////////////////////////////////////////////////////////////////////
//
bool MediaPluginCEF::init()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "name_text");
	message.setValue("name", "CEF Plugin");
	sendMessage(message);

	return true;
};

////////////////////////////////////////////////////////////////////////////////
//
int init_media_plugin(LLPluginInstance::sendMessageFunction host_send_func,
	void* host_user_data,
	LLPluginInstance::sendMessageFunction *plugin_send_func,
	void **plugin_user_data)
{
	MediaPluginCEF* self = new MediaPluginCEF(host_send_func, host_user_data);
	*plugin_send_func = MediaPluginCEF::staticReceiveMessage;
	*plugin_user_data = (void*)self;

	return 0;
}
