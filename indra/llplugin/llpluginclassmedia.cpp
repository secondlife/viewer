/** 
 * @file llpluginclassmedia.cpp
 * @brief LLPluginClassMedia handles a plugin which knows about the "media" message class.
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
#include "indra_constants.h"

#include "llpluginclassmedia.h"
#include "llpluginmessageclasses.h"

#include "llqtwebkit.h"

static int LOW_PRIORITY_TEXTURE_SIZE_DEFAULT = 256;

static int nextPowerOf2( int value )
{
	int next_power_of_2 = 1;
	while ( next_power_of_2 < value )
	{
		next_power_of_2 <<= 1;
	}
	
	return next_power_of_2;
}

LLPluginClassMedia::LLPluginClassMedia(LLPluginClassMediaOwner *owner)
{
	mOwner = owner;
	mPlugin = NULL;
	reset();

	//debug use
	mDeleteOK = true ;
}


LLPluginClassMedia::~LLPluginClassMedia()
{
	llassert_always(mDeleteOK) ;
	reset();
}

bool LLPluginClassMedia::init(const std::string &launcher_filename, const std::string &plugin_dir, const std::string &plugin_filename, bool debug)
{	
	LL_DEBUGS("Plugin") << "launcher: " << launcher_filename << LL_ENDL;
	LL_DEBUGS("Plugin") << "dir: " << plugin_dir << LL_ENDL;
	LL_DEBUGS("Plugin") << "plugin: " << plugin_filename << LL_ENDL;
	
	mPlugin = new LLPluginProcessParent(this);
	mPlugin->setSleepTime(mSleepTime);
	
	// Queue up the media init message -- it will be sent after all the currently queued messages.
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "init");
	message.setValue("target", mTarget);
	sendMessage(message);
	
	mPlugin->init(launcher_filename, plugin_dir, plugin_filename, debug);

	return true;
}


void LLPluginClassMedia::reset()
{
	if(mPlugin != NULL)
	{
		delete mPlugin;
		mPlugin = NULL;
	}

	mTextureParamsReceived = false;
	mRequestedTextureDepth = 0;
	mRequestedTextureInternalFormat = 0;
	mRequestedTextureFormat = 0;
	mRequestedTextureType = 0;
	mRequestedTextureSwapBytes = false;
	mRequestedTextureCoordsOpenGL = false;
	mTextureSharedMemorySize = 0;
	mTextureSharedMemoryName.clear();
	mDefaultMediaWidth = 0;
	mDefaultMediaHeight = 0;
	mNaturalMediaWidth = 0;
	mNaturalMediaHeight = 0;
	mSetMediaWidth = -1;
	mSetMediaHeight = -1;
	mRequestedMediaWidth = 0;
	mRequestedMediaHeight = 0;
	mRequestedTextureWidth = 0;
	mRequestedTextureHeight = 0;
	mFullMediaWidth = 0;
	mFullMediaHeight = 0;
	mTextureWidth = 0;
	mTextureHeight = 0;
	mMediaWidth = 0;
	mMediaHeight = 0;
	mDirtyRect = LLRect::null;	
	mAutoScaleMedia = false;
	mRequestedVolume = 1.0f;
	mPriority = PRIORITY_NORMAL;
	mLowPrioritySizeLimit = LOW_PRIORITY_TEXTURE_SIZE_DEFAULT;
	mAllowDownsample = false;
	mPadding = 0;
	mLastMouseX = 0;
	mLastMouseY = 0;
	mStatus = LLPluginClassMediaOwner::MEDIA_NONE;
	mSleepTime = 1.0f / 100.0f;
	mCanCut = false;
	mCanCopy = false;
	mCanPaste = false;
	mMediaName.clear();
	mMediaDescription.clear();
	mBackgroundColor = LLColor4(1.0f, 1.0f, 1.0f, 1.0f);
	
	// media_browser class
	mNavigateURI.clear();
	mNavigateResultCode = -1;
	mNavigateResultString.clear();
	mHistoryBackAvailable = false;
	mHistoryForwardAvailable = false;
	mStatusText.clear();
	mProgressPercent = 0;	
	mClickURL.clear();
	mClickNavType.clear();
	mClickTarget.clear();
	mClickUUID.clear();
	mStatusCode = 0;
	
	// media_time class
	mCurrentTime = 0.0f;
	mDuration = 0.0f;
	mCurrentRate = 0.0f;
	mLoadedDuration = 0.0f;
}

void LLPluginClassMedia::idle(void)
{
	if(mPlugin)
	{
		mPlugin->idle();
	}
	
	if((mMediaWidth == -1) || (!mTextureParamsReceived) || (mPlugin == NULL) || (mPlugin->isBlocked()) || (mOwner == NULL))
	{
		// Can't process a size change at this time
	}
	else if((mRequestedMediaWidth != mMediaWidth) || (mRequestedMediaHeight != mMediaHeight))
	{
		// Calculate the correct size for the media texture
		mRequestedTextureHeight = mRequestedMediaHeight;
		if(mPadding < 0)
		{
			// negative values indicate the plugin wants a power of 2
			mRequestedTextureWidth = nextPowerOf2(mRequestedMediaWidth);
		}
		else
		{
			mRequestedTextureWidth = mRequestedMediaWidth;
			
			if(mPadding > 1)
			{
				// Pad up to a multiple of the specified number of bytes per row
				int rowbytes = mRequestedTextureWidth * mRequestedTextureDepth;
				int pad = rowbytes % mPadding;
				if(pad != 0)
				{
					rowbytes += mPadding - pad;
				}
				
				if(rowbytes % mRequestedTextureDepth == 0)
				{
					mRequestedTextureWidth = rowbytes / mRequestedTextureDepth;
				}
				else
				{
					LL_WARNS("Plugin") << "Unable to pad texture width, padding size " << mPadding << "is not a multiple of pixel size " << mRequestedTextureDepth << LL_ENDL;
				}
			}
		}

		
		// Size change has been requested but not initiated yet.
		size_t newsize = mRequestedTextureWidth * mRequestedTextureHeight * mRequestedTextureDepth;

		// Add an extra line for padding, just in case.
		newsize += mRequestedTextureWidth * mRequestedTextureDepth;

		if(newsize != mTextureSharedMemorySize)
		{
			if(!mTextureSharedMemoryName.empty())
			{
				// Tell the plugin to remove the old memory segment
				mPlugin->removeSharedMemory(mTextureSharedMemoryName);
				mTextureSharedMemoryName.clear();
			}
			
			mTextureSharedMemorySize = newsize;
			mTextureSharedMemoryName = mPlugin->addSharedMemory(mTextureSharedMemorySize);
			if(!mTextureSharedMemoryName.empty())
			{
				void *addr = mPlugin->getSharedMemoryAddress(mTextureSharedMemoryName);
				
				// clear texture memory to avoid random screen visual fuzz from uninitialized texture data
				memset( addr, 0x00, newsize );
				
				// We could do this to force an update, but textureValid() will still be returning false until the first roundtrip to the plugin,
				// so it may not be worthwhile.
				// mDirtyRect.setOriginAndSize(0, 0, mRequestedMediaWidth, mRequestedMediaHeight);
			}
		}
		
		// This is our local indicator that a change is in progress.
		mTextureWidth = -1;
		mTextureHeight = -1;
		mMediaWidth = -1;
		mMediaHeight = -1;

		// This invalidates any existing dirty rect.
		resetDirty();
		
		// Send a size change message to the plugin
		{
			LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "size_change");
			message.setValue("name", mTextureSharedMemoryName);
			message.setValueS32("width", mRequestedMediaWidth);
			message.setValueS32("height", mRequestedMediaHeight);
			message.setValueS32("texture_width", mRequestedTextureWidth);
			message.setValueS32("texture_height", mRequestedTextureHeight);
			message.setValueReal("background_r", mBackgroundColor.mV[VX]);
			message.setValueReal("background_g", mBackgroundColor.mV[VY]);
			message.setValueReal("background_b", mBackgroundColor.mV[VZ]);
			message.setValueReal("background_a", mBackgroundColor.mV[VW]);
			mPlugin->sendMessage(message);	// DO NOT just use sendMessage() here -- we want this to jump ahead of the queue.
			
			LL_DEBUGS("Plugin") << "Sending size_change" << LL_ENDL;
		}
	}
	
	if(mPlugin && mPlugin->isRunning())
	{
		// Send queued messages
		while(!mSendQueue.empty())
		{
			LLPluginMessage message = mSendQueue.front();
			mSendQueue.pop();
			mPlugin->sendMessage(message);
		}
	}
}

int LLPluginClassMedia::getTextureWidth() const
{
	return nextPowerOf2(mTextureWidth);
}

int LLPluginClassMedia::getTextureHeight() const
{
	return nextPowerOf2(mTextureHeight);
}

unsigned char* LLPluginClassMedia::getBitsData()
{
	unsigned char *result = NULL;
	if((mPlugin != NULL) && !mTextureSharedMemoryName.empty())
	{
		result = (unsigned char*)mPlugin->getSharedMemoryAddress(mTextureSharedMemoryName);
	}
	return result;
}

void LLPluginClassMedia::setSize(int width, int height)
{
	if((width > 0) && (height > 0))
	{
		mSetMediaWidth = width;
		mSetMediaHeight = height;
	}
	else
	{
		mSetMediaWidth = -1;
		mSetMediaHeight = -1;
	}

	setSizeInternal();
}

void LLPluginClassMedia::setSizeInternal(void)
{
	if((mSetMediaWidth > 0) && (mSetMediaHeight > 0))
	{
		mRequestedMediaWidth = mSetMediaWidth;
		mRequestedMediaHeight = mSetMediaHeight;
	}
	else if((mNaturalMediaWidth > 0) && (mNaturalMediaHeight > 0))
	{
		mRequestedMediaWidth = mNaturalMediaWidth;
		mRequestedMediaHeight = mNaturalMediaHeight;
	}
	else
	{
		mRequestedMediaWidth = mDefaultMediaWidth;
		mRequestedMediaHeight = mDefaultMediaHeight;
	}
	
	// Save these for size/interest calculations
	mFullMediaWidth = mRequestedMediaWidth;
	mFullMediaHeight = mRequestedMediaHeight;
	
	if(mAllowDownsample)
	{
		switch(mPriority)
		{
			case PRIORITY_SLIDESHOW:
			case PRIORITY_LOW:
				// Reduce maximum texture dimension to (or below) mLowPrioritySizeLimit
				while((mRequestedMediaWidth > mLowPrioritySizeLimit) || (mRequestedMediaHeight > mLowPrioritySizeLimit))
				{
					mRequestedMediaWidth /= 2;
					mRequestedMediaHeight /= 2;
				}
			break;
			
			default:
				// Don't adjust texture size
			break;
		}
	}
	
	if(mAutoScaleMedia)
	{
		mRequestedMediaWidth = nextPowerOf2(mRequestedMediaWidth);
		mRequestedMediaHeight = nextPowerOf2(mRequestedMediaHeight);
	}
	
	if(mRequestedMediaWidth > 2048)
		mRequestedMediaWidth = 2048;

	if(mRequestedMediaHeight > 2048)
		mRequestedMediaHeight = 2048;
}

void LLPluginClassMedia::setAutoScale(bool auto_scale)
{
	if(auto_scale != mAutoScaleMedia)
	{
		mAutoScaleMedia = auto_scale;
		setSizeInternal();
	}
}

bool LLPluginClassMedia::textureValid(void)
{
	if(
		!mTextureParamsReceived ||
		mTextureWidth <= 0 ||
		mTextureHeight <= 0 ||
		mMediaWidth <= 0 ||
		mMediaHeight <= 0 ||
		mRequestedMediaWidth != mMediaWidth ||
		mRequestedMediaHeight != mMediaHeight ||
		getBitsData() == NULL
	)	
		return false;
	
	return true;
}

bool LLPluginClassMedia::getDirty(LLRect *dirty_rect)
{
	bool result = !mDirtyRect.isEmpty();

	if(dirty_rect != NULL)
	{
		*dirty_rect = mDirtyRect;
	}

	return result;
}

void LLPluginClassMedia::resetDirty(void)
{
	mDirtyRect = LLRect::null;
}

std::string LLPluginClassMedia::translateModifiers(MASK modifiers)
{
	std::string result;
	
	
	if(modifiers & MASK_CONTROL)
	{
		result += "control|";
	}

	if(modifiers & MASK_ALT)
	{
		result += "alt|";
	}

	if(modifiers & MASK_SHIFT)
	{
		result += "shift|";
	}

	// TODO: should I deal with platform differences here or in callers?
	// TODO: how do we deal with the Mac "command" key?
/*
	if(modifiers & MASK_SOMETHING)
	{
		result += "meta|";
	}
*/	
	return result;
}

void LLPluginClassMedia::jsEnableObject( bool enable )
{
	if( ! mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked() )
	{
		return;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "js_enable_object");
	message.setValueBoolean( "enable", enable );
	sendMessage( message );
}

void LLPluginClassMedia::jsAgentLocationEvent( double x, double y, double z )
{
	if( ! mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked() )
	{
		return;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "js_agent_location");
	message.setValueReal( "x", x );
	message.setValueReal( "y", y );
	message.setValueReal( "z", z );
	sendMessage( message );
}

void LLPluginClassMedia::jsAgentGlobalLocationEvent( double x, double y, double z )
{
	if( ! mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked() )
	{
		return;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "js_agent_global_location");
	message.setValueReal( "x", x );
	message.setValueReal( "y", y );
	message.setValueReal( "z", z );
	sendMessage( message );
}

void LLPluginClassMedia::jsAgentOrientationEvent( double angle )
{
	if( ! mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked() )
	{
		return;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "js_agent_orientation");
	message.setValueReal( "angle", angle );

	sendMessage( message );
}

void LLPluginClassMedia::jsAgentLanguageEvent( const std::string& language )
{
	if( ! mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked() )
	{
		return;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "js_agent_language");
	message.setValue( "language", language );
	sendMessage( message );
}

void LLPluginClassMedia::jsAgentRegionEvent( const std::string& region )
{
	if( ! mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked() )
	{
		return;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "js_agent_region");
	message.setValue( "region", region );
	sendMessage( message );
}

void LLPluginClassMedia::jsAgentMaturityEvent( const std::string& maturity )
{
	if( ! mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked() )
	{
		return;
	}

	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "js_agent_maturity");
	message.setValue( "maturity", maturity );
	sendMessage( message );
}

void LLPluginClassMedia::mouseEvent(EMouseEventType type, int button, int x, int y, MASK modifiers)
{
	if(type == MOUSE_EVENT_MOVE)
	{
		if(!mPlugin || !mPlugin->isRunning() || mPlugin->isBlocked())
		{
			// Don't queue up mouse move events that can't be delivered.
			return;
		}

		if((x == mLastMouseX) && (y == mLastMouseY))
		{
			// Don't spam unnecessary mouse move events.
			return;
		}
		
		mLastMouseX = x;
		mLastMouseY = y;
	}
	
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "mouse_event");
	std::string temp;
	switch(type)
	{
		case MOUSE_EVENT_DOWN:			temp = "down";			break;
		case MOUSE_EVENT_UP:			temp = "up";			break;
		case MOUSE_EVENT_MOVE:			temp = "move";			break;
		case MOUSE_EVENT_DOUBLE_CLICK:	temp = "double_click";	break;
	}
	message.setValue("event", temp);

	message.setValueS32("button", button);

	message.setValueS32("x", x);
	
	// Incoming coordinates are OpenGL-style ((0,0) = lower left), so flip them here if the plugin has requested it.
	if(!mRequestedTextureCoordsOpenGL)
	{
		// TODO: Should I use mMediaHeight or mRequestedMediaHeight here?
		y = mMediaHeight - y;
	}
	message.setValueS32("y", y);

	message.setValue("modifiers", translateModifiers(modifiers));
	
	sendMessage(message);
}

bool LLPluginClassMedia::keyEvent(EKeyEventType type, int key_code, MASK modifiers, LLSD native_key_data)
{
	bool result = true;
	
	// FIXME:
	// HACK: we don't have an easy way to tell if the plugin is going to handle a particular keycode.
	// For now, return false for the ones the webkit plugin won't handle properly.
	
	switch(key_code)
	{
		case KEY_BACKSPACE:		
		case KEY_TAB:			
		case KEY_RETURN:		
		case KEY_PAD_RETURN:	
		case KEY_SHIFT:			
		case KEY_CONTROL:		
		case KEY_ALT:			
		case KEY_CAPSLOCK:		
		case KEY_ESCAPE:		
		case KEY_PAGE_UP:		
		case KEY_PAGE_DOWN:		
		case KEY_END:			
		case KEY_HOME:			
		case KEY_LEFT:			
		case KEY_UP:			
		case KEY_RIGHT:			
		case KEY_DOWN:			
		case KEY_INSERT:		
		case KEY_DELETE:
			// These will be handled		
		break;
		
		default:
			// regular ASCII characters will also be handled
			if(key_code >= KEY_SPECIAL)
			{
				// Other "special" codes will not work properly.
				result = false;
			}
		break;
	}

#if LL_DARWIN	
	if(modifiers & MASK_ALT)
	{
		// Option-key modified characters should be handled by the unicode input path instead of this one.
		result = false;
	}
#endif

	if(result)
	{
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "key_event");
		std::string temp;
		switch(type)
		{
			case KEY_EVENT_DOWN:			temp = "down";			break;
			case KEY_EVENT_UP:				temp = "up";			break;
			case KEY_EVENT_REPEAT:			temp = "repeat";		break;
		}
		message.setValue("event", temp);
		
		message.setValueS32("key", key_code);

		message.setValue("modifiers", translateModifiers(modifiers));
		message.setValueLLSD("native_key_data", native_key_data);
		
		sendMessage(message);
	}
		
	return result;
}

void LLPluginClassMedia::scrollEvent(int x, int y, MASK modifiers)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "scroll_event");

	message.setValueS32("x", x);
	message.setValueS32("y", y);
	message.setValue("modifiers", translateModifiers(modifiers));
	
	sendMessage(message);
}
	
bool LLPluginClassMedia::textInput(const std::string &text, MASK modifiers, LLSD native_key_data)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "text_event");

	message.setValue("text", text);
	message.setValue("modifiers", translateModifiers(modifiers));
	message.setValueLLSD("native_key_data", native_key_data);
	
	sendMessage(message);
	
	return true;
}

void LLPluginClassMedia::loadURI(const std::string &uri)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "load_uri");

	message.setValue("uri", uri);
	
	sendMessage(message);
}

const char* LLPluginClassMedia::priorityToString(EPriority priority)
{
	const char* result = "UNKNOWN";
	switch(priority)
	{
		case PRIORITY_UNLOADED:		result = "unloaded";	break;
		case PRIORITY_STOPPED:		result = "stopped";		break;
		case PRIORITY_HIDDEN:		result = "hidden";		break;
		case PRIORITY_SLIDESHOW:	result = "slideshow";	break;
		case PRIORITY_LOW:			result = "low";			break;
		case PRIORITY_NORMAL:		result = "normal";		break;
		case PRIORITY_HIGH:			result = "high";		break;
	}
	
	return result;
}

void LLPluginClassMedia::setPriority(EPriority priority)
{
	if(mPriority != priority)
	{
		mPriority = priority;

		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "set_priority");
		
		std::string priority_string = priorityToString(priority);
		switch(priority)
		{
			case PRIORITY_UNLOADED:	
				mSleepTime = 1.0f;
			break;
			case PRIORITY_STOPPED:	
				mSleepTime = 1.0f;
			break;
			case PRIORITY_HIDDEN:	
				mSleepTime = 1.0f;
			break;
			case PRIORITY_SLIDESHOW:
				mSleepTime = 1.0f;
			break;
			case PRIORITY_LOW:		
				mSleepTime = 1.0f / 25.0f;
			break;
			case PRIORITY_NORMAL:	
				mSleepTime = 1.0f / 50.0f;
			break;
			case PRIORITY_HIGH:		
				mSleepTime = 1.0f / 100.0f;
			break;
		}
		
		message.setValue("priority", priority_string);

		sendMessage(message);
		
		if(mPlugin)
		{
			mPlugin->setSleepTime(mSleepTime);
		}
		
		LL_DEBUGS("PluginPriority") << this << ": setting priority to " << priority_string << LL_ENDL;
		
		// This may affect the calculated size, so recalculate it here.
		setSizeInternal();
	}
}

void LLPluginClassMedia::setLowPrioritySizeLimit(int size)
{
	int power = nextPowerOf2(size);
	if(mLowPrioritySizeLimit != power)
	{
		mLowPrioritySizeLimit = power;

		// This may affect the calculated size, so recalculate it here.
		setSizeInternal();
	}
}

F64 LLPluginClassMedia::getCPUUsage()
{
	F64 result = 0.0f;
	
	if(mPlugin)
	{
		result = mPlugin->getCPUUsage();
	}
	
	return result;
}

void LLPluginClassMedia::sendPickFileResponse(const std::string &file)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "pick_file_response");
	message.setValue("file", file);
	if(mPlugin && mPlugin->isBlocked())
	{
		// If the plugin sent a blocking pick-file request, the response should unblock it.
		message.setValueBoolean("blocking_response", true);
	}
	sendMessage(message);
}

void LLPluginClassMedia::sendAuthResponse(bool ok, const std::string &username, const std::string &password)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "auth_response");
	message.setValueBoolean("ok", ok);
	message.setValue("username", username);
	message.setValue("password", password);
	if(mPlugin && mPlugin->isBlocked())
	{
		// If the plugin sent a blocking pick-file request, the response should unblock it.
		message.setValueBoolean("blocking_response", true);
	}
	sendMessage(message);
}

void LLPluginClassMedia::cut()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "edit_cut");
	sendMessage(message);
}

void LLPluginClassMedia::copy()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "edit_copy");
	sendMessage(message);
}

void LLPluginClassMedia::paste()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "edit_paste");
	sendMessage(message);
}

void LLPluginClassMedia::setUserDataPath(const std::string &user_data_path)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "set_user_data_path");
	message.setValue("path", user_data_path);
	sendMessage(message);
}

void LLPluginClassMedia::setLanguageCode(const std::string &language_code)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "set_language_code");
	message.setValue("language", language_code);
	sendMessage(message);
}

void LLPluginClassMedia::setPluginsEnabled(const bool enabled)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "plugins_enabled");
	message.setValueBoolean("enable", enabled);
	sendMessage(message);
}

void LLPluginClassMedia::setJavascriptEnabled(const bool enabled)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "javascript_enabled");
	message.setValueBoolean("enable", enabled);
	sendMessage(message);
}


void LLPluginClassMedia::enableMediaPluginDebugging( bool enable )
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA, "enable_media_plugin_debugging");
	message.setValueBoolean( "enable", enable );
	sendMessage( message );
}

void LLPluginClassMedia::setTarget(const std::string &target)
{
	mTarget = target;
}

/* virtual */ 
void LLPluginClassMedia::receivePluginMessage(const LLPluginMessage &message)
{
	std::string message_class = message.getClass();
	
	if(message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA)
	{
		std::string message_name = message.getName();
		if(message_name == "texture_params")
		{
			mRequestedTextureDepth = message.getValueS32("depth");
			mRequestedTextureInternalFormat = message.getValueU32("internalformat");
			mRequestedTextureFormat = message.getValueU32("format");
			mRequestedTextureType = message.getValueU32("type");
			mRequestedTextureSwapBytes = message.getValueBoolean("swap_bytes");
			mRequestedTextureCoordsOpenGL = message.getValueBoolean("coords_opengl");			
			
			// These two are optional, and will default to 0 if they're not specified.
			mDefaultMediaWidth = message.getValueS32("default_width");
			mDefaultMediaHeight = message.getValueS32("default_height");
			
			mAllowDownsample = message.getValueBoolean("allow_downsample");
			mPadding = message.getValueS32("padding");

			setSizeInternal();
			
			mTextureParamsReceived = true;
		}
		else if(message_name == "updated")
		{			
			if(message.hasValue("left"))
			{
				LLRect newDirtyRect;
				newDirtyRect.mLeft = message.getValueS32("left");
				newDirtyRect.mTop = message.getValueS32("top");
				newDirtyRect.mRight = message.getValueS32("right");
				newDirtyRect.mBottom = message.getValueS32("bottom");
							
				// The plugin is likely to have top and bottom switched, due to vertical flip and OpenGL coordinate confusion.
				// If they're backwards, swap them.
				if(newDirtyRect.mTop < newDirtyRect.mBottom)
				{
					S32 temp = newDirtyRect.mTop;
					newDirtyRect.mTop = newDirtyRect.mBottom;
					newDirtyRect.mBottom = temp;
				}
				
				if(mDirtyRect.isEmpty())
				{
					mDirtyRect = newDirtyRect;
				}
				else
				{
					mDirtyRect.unionWith(newDirtyRect);
				}

				LL_DEBUGS("Plugin") << "adjusted incoming rect is: (" 
					<< newDirtyRect.mLeft << ", "
					<< newDirtyRect.mTop << ", "
					<< newDirtyRect.mRight << ", "
					<< newDirtyRect.mBottom << "), new dirty rect is: ("
					<< mDirtyRect.mLeft << ", "
					<< mDirtyRect.mTop << ", "
					<< mDirtyRect.mRight << ", "
					<< mDirtyRect.mBottom << ")"
					<< LL_ENDL;
				
				mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_CONTENT_UPDATED);
			}			
			

			bool time_duration_updated = false;
			int previous_percent = mProgressPercent;

			if(message.hasValue("current_time"))
			{
				mCurrentTime = message.getValueReal("current_time");
				time_duration_updated = true;
			}
			if(message.hasValue("duration"))
			{
				mDuration = message.getValueReal("duration");
				time_duration_updated = true;
			}

			if(message.hasValue("current_rate"))
			{
				mCurrentRate = message.getValueReal("current_rate");
			}
			
			if(message.hasValue("loaded_duration"))
			{
				mLoadedDuration = message.getValueReal("loaded_duration");
				time_duration_updated = true;
			}
			else
			{
				// If the message doesn't contain a loaded_duration param, assume it's equal to duration
				mLoadedDuration = mDuration;
			}
			
			// Calculate a percentage based on the loaded duration and total duration.
			if(mDuration != 0.0f)	// Don't divide by zero.
			{
				mProgressPercent = (int)((mLoadedDuration * 100.0f)/mDuration);
			}

			if(time_duration_updated)
			{
				mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_TIME_DURATION_UPDATED);
			}
			
			if(previous_percent != mProgressPercent)
			{
				mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_PROGRESS_UPDATED);
			}
		}
		else if(message_name == "media_status")
		{
			std::string status = message.getValue("status");
			
			LL_DEBUGS("Plugin") << "Status changed to: " << status << LL_ENDL;
			
			if(status == "loading")
			{
				mStatus = LLPluginClassMediaOwner::MEDIA_LOADING;
			}
			else if(status == "loaded")
			{
				mStatus = LLPluginClassMediaOwner::MEDIA_LOADED;
			}
			else if(status == "error")
			{
				mStatus = LLPluginClassMediaOwner::MEDIA_ERROR;
			}
			else if(status == "playing")
			{
				mStatus = LLPluginClassMediaOwner::MEDIA_PLAYING;
			}
			else if(status == "paused")
			{
				mStatus = LLPluginClassMediaOwner::MEDIA_PAUSED;
			}
			else if(status == "done")
			{
				mStatus = LLPluginClassMediaOwner::MEDIA_DONE;
			}
			else
			{
				// empty string or any unknown string
				mStatus = LLPluginClassMediaOwner::MEDIA_NONE;
			}
		}
		else if(message_name == "size_change_request")
		{
			S32 width = message.getValueS32("width");
			S32 height = message.getValueS32("height");
			std::string name = message.getValue("name");

			// TODO: check that name matches?
			mNaturalMediaWidth = width;
			mNaturalMediaHeight = height;
			
			setSizeInternal();
		}
		else if(message_name == "size_change_response")
		{
			std::string name = message.getValue("name");
			
			// TODO: check that name matches?
			
			mTextureWidth = message.getValueS32("texture_width");
			mTextureHeight = message.getValueS32("texture_height");
			mMediaWidth = message.getValueS32("width");
			mMediaHeight = message.getValueS32("height");
			
			// This invalidates any existing dirty rect.
			resetDirty();
			
			// TODO: should we verify that the plugin sent back the right values?  
			// Two size changes in a row may cause them to not match, due to queueing, etc.

			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_SIZE_CHANGED);
		}
		else if(message_name == "cursor_changed")
		{
			mCursorName = message.getValue("name");

			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_CURSOR_CHANGED);
		}
		else if(message_name == "edit_state")
		{
			if(message.hasValue("cut"))
			{
				mCanCut = message.getValueBoolean("cut");
			}
			if(message.hasValue("copy"))
			{
				mCanCopy = message.getValueBoolean("copy");
			}
			if(message.hasValue("paste"))
			{
				mCanPaste = message.getValueBoolean("paste");
			}
		}
		else if(message_name == "name_text")
		{
			mMediaName = message.getValue("name");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_NAME_CHANGED);
		}
		else if(message_name == "pick_file")
		{
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_PICK_FILE_REQUEST);
		}
		else if(message_name == "auth_request")
		{
			mAuthURL = message.getValue("url");
			mAuthRealm = message.getValue("realm");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_AUTH_REQUEST);
		}
		else if(message_name == "debug_message")
		{
			mDebugMessageText = message.getValue("message_text");
			mDebugMessageLevel = message.getValue("message_level");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_DEBUG_MESSAGE);
		}
		else
		{
			LL_WARNS("Plugin") << "Unknown " << message_name << " class message: " << message_name << LL_ENDL;
		}
	}
	else if(message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER)
	{
		std::string message_name = message.getName();
		if(message_name == "navigate_begin")
		{
			mNavigateURI = message.getValue("uri");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_NAVIGATE_BEGIN);
		}
		else if(message_name == "navigate_complete")
		{
			mNavigateURI = message.getValue("uri");
			mNavigateResultCode = message.getValueS32("result_code");
			mNavigateResultString = message.getValue("result_string");
			mHistoryBackAvailable = message.getValueBoolean("history_back_available");
			mHistoryForwardAvailable = message.getValueBoolean("history_forward_available");
			
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_NAVIGATE_COMPLETE);
		}
		else if(message_name == "progress")
		{
			mProgressPercent = message.getValueS32("percent");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_PROGRESS_UPDATED);
		}
		else if(message_name == "status_text")
		{
			mStatusText = message.getValue("status");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_STATUS_TEXT_CHANGED);
		}
		else if(message_name == "location_changed")
		{
			mLocation = message.getValue("uri");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_LOCATION_CHANGED);
		}
		else if(message_name == "click_href")
		{
			mClickURL = message.getValue("uri");
			mClickTarget = message.getValue("target");
			mClickUUID = message.getValue("uuid");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_CLICK_LINK_HREF);
		}
		else if(message_name == "click_nofollow")
		{
			mClickURL = message.getValue("uri");
			mClickNavType = message.getValue("nav_type");
			mClickTarget.clear();
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_CLICK_LINK_NOFOLLOW);
		}
		else if(message_name == "navigate_error_page")
		{
			mStatusCode = message.getValueS32("status_code");
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_NAVIGATE_ERROR_PAGE);
		}
		else if(message_name == "cookie_set")
		{
			if(mOwner)
			{
				mOwner->handleCookieSet(this, message.getValue("cookie"));
			}
		}
		else if(message_name == "close_request")
		{
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_CLOSE_REQUEST);
		}
		else if(message_name == "geometry_change")
		{
			mClickUUID = message.getValue("uuid");
			mGeometryX = message.getValueS32("x");
			mGeometryY = message.getValueS32("y");
			mGeometryWidth = message.getValueS32("width");
			mGeometryHeight = message.getValueS32("height");
				
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_GEOMETRY_CHANGE);
		}
		else if(message_name == "link_hovered")
		{
			// text is not currently used -- the tooltip hover text is taken from the "title".
			mHoverLink = message.getValue("link");
			mHoverText = message.getValue("title");
			// message.getValue("text");
				
			mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_LINK_HOVERED);
		}
		else
		{
			LL_WARNS("Plugin") << "Unknown " << message_name << " class message: " << message_name << LL_ENDL;
		}
	}
	else if(message_class == LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME)
	{
		std::string message_name = message.getName();

		// This class hasn't defined any incoming messages yet.
//		if(message_name == "message_name")
//		{
//		}
//		else 
		{
			LL_WARNS("Plugin") << "Unknown " << message_name << " class message: " << message_name << LL_ENDL;
		}
	}

}

/* virtual */ 
void LLPluginClassMedia::pluginLaunchFailed()
{
	mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_PLUGIN_FAILED_LAUNCH);
}

/* virtual */ 
void LLPluginClassMedia::pluginDied()
{
	mediaEvent(LLPluginClassMediaOwner::MEDIA_EVENT_PLUGIN_FAILED);
}

void LLPluginClassMedia::mediaEvent(LLPluginClassMediaOwner::EMediaEvent event)
{
	if(mOwner)
	{
		mOwner->handleMediaEvent(this, event);
	}
}

void LLPluginClassMedia::sendMessage(const LLPluginMessage &message)
{
	if(mPlugin && mPlugin->isRunning())
	{
		mPlugin->sendMessage(message);
	}
	else
	{
		// The plugin isn't set up yet -- queue this message to be sent after initialization.
		mSendQueue.push(message);
	}
}

////////////////////////////////////////////////////////////
// MARK: media_browser class functions
bool LLPluginClassMedia::pluginSupportsMediaBrowser(void)
{
	std::string version = mPlugin->getMessageClassVersion(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER);
	return !version.empty();
}

void LLPluginClassMedia::focus(bool focused)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "focus");

	message.setValueBoolean("focused", focused);
	
	sendMessage(message);
}

void LLPluginClassMedia::set_page_zoom_factor( double factor )
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "set_page_zoom_factor");

	message.setValueReal("factor", factor);
	sendMessage(message);
}

void LLPluginClassMedia::clear_cache()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "clear_cache");
	sendMessage(message);
}

void LLPluginClassMedia::clear_cookies()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "clear_cookies");
	sendMessage(message);
}

void LLPluginClassMedia::set_cookies(const std::string &cookies)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "set_cookies");
	message.setValue("cookies", cookies);	
	sendMessage(message);
}

void LLPluginClassMedia::enable_cookies(bool enable)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "enable_cookies");
	message.setValueBoolean("enable", enable);
	sendMessage(message);
}

void LLPluginClassMedia::proxy_setup(bool enable, const std::string &host, int port)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "proxy_setup");

	message.setValueBoolean("enable", enable);
	message.setValue("host", host);
	message.setValueS32("port", port);

	sendMessage(message);
}

void LLPluginClassMedia::browse_stop()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "browse_stop");
	sendMessage(message);
}

void LLPluginClassMedia::browse_reload(bool ignore_cache)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "browse_reload");

	message.setValueBoolean("ignore_cache", ignore_cache);
	
	sendMessage(message);
}

void LLPluginClassMedia::browse_forward()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "browse_forward");
	sendMessage(message);
}

void LLPluginClassMedia::browse_back()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "browse_back");
	sendMessage(message);
}

void LLPluginClassMedia::setBrowserUserAgent(const std::string& user_agent)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "set_user_agent");

	message.setValue("user_agent", user_agent);

	sendMessage(message);
}

void LLPluginClassMedia::showWebInspector( bool show )
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "show_web_inspector");
	message.setValueBoolean("show", true);	// only open for now - closed manually by user
	sendMessage(message);
}

void LLPluginClassMedia::proxyWindowOpened(const std::string &target, const std::string &uuid)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "proxy_window_opened");

	message.setValue("target", target);
	message.setValue("uuid", uuid);

	sendMessage(message);
}

void LLPluginClassMedia::proxyWindowClosed(const std::string &uuid)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "proxy_window_closed");

	message.setValue("uuid", uuid);

	sendMessage(message);
}

void LLPluginClassMedia::ignore_ssl_cert_errors(bool ignore)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "ignore_ssl_cert_errors");
	message.setValueBoolean("ignore", ignore);
	sendMessage(message);
}

void LLPluginClassMedia::addCertificateFilePath(const std::string& path)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "add_certificate_file_path");
	message.setValue("path", path);
	sendMessage(message);
}

void LLPluginClassMedia::crashPlugin()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "crash");

	sendMessage(message);
}

void LLPluginClassMedia::hangPlugin()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_INTERNAL, "hang");

	sendMessage(message);
}


////////////////////////////////////////////////////////////
// MARK: media_time class functions
bool LLPluginClassMedia::pluginSupportsMediaTime(void)
{
	std::string version = mPlugin->getMessageClassVersion(LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME);
	return !version.empty();
}

void LLPluginClassMedia::stop()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME, "stop");
	sendMessage(message);
}

void LLPluginClassMedia::start(float rate)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME, "start");

	message.setValueReal("rate", rate);

	sendMessage(message);
}

void LLPluginClassMedia::pause()
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME, "pause");
	sendMessage(message);
}

void LLPluginClassMedia::seek(float time)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME, "seek");

	message.setValueReal("time", time);
	
	sendMessage(message);
}

void LLPluginClassMedia::setLoop(bool loop)
{
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME, "set_loop");

	message.setValueBoolean("loop", loop);

	sendMessage(message);
}

void LLPluginClassMedia::setVolume(float volume)
{
	if(volume != mRequestedVolume)
	{
		mRequestedVolume = volume;
		
		LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_TIME, "set_volume");

		message.setValueReal("volume", volume);
		
		sendMessage(message);
	}
}

float LLPluginClassMedia::getVolume()
{
	return mRequestedVolume;
}

void LLPluginClassMedia::initializeUrlHistory(const LLSD& url_history)
{
	// Send URL history to plugin
	LLPluginMessage message(LLPLUGIN_MESSAGE_CLASS_MEDIA_BROWSER, "init_history");
	message.setValueLLSD("history", url_history);
	sendMessage(message);

	LL_DEBUGS("Plugin") << "Sending history" << LL_ENDL;
}

