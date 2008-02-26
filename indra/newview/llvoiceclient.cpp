/** 
 * @file llvoiceclient.cpp
 * @brief Implementation of LLVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llvoiceclient.h"

#include <boost/tokenizer.hpp>

#include "llsdutil.h"

#include "llvoavatar.h"
#include "llbufferstream.h"
#include "llfile.h"
#include "expat/expat.h"
#include "llcallbacklist.h"
#include "llviewerregion.h"
#include "llviewernetwork.h"		// for gGridChoice
#include "llbase64.h"
#include "llviewercontrol.h"
#include "llkeyboard.h"
#include "llappviewer.h"	// for gDisconnected, gDisableVoice
#include "llmutelist.h"  // to check for muted avatars
#include "llagent.h"
#include "llcachename.h"
#include "llimview.h" // for LLIMMgr
#include "llimpanel.h" // for LLVoiceChannel
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "llfirstuse.h"
#include "llviewerwindow.h"

// for base64 decoding
#include "apr-1/apr_base64.h"

// for SHA1 hash
#include "apr-1/apr_sha1.h"

// If we are connecting to agni AND the user's last name is "Linden", join this channel instead of looking up the sim name.
// If we are connecting to agni and the user's last name is NOT "Linden", disable voice.
#define AGNI_LINDENS_ONLY_CHANNEL "SL"
static bool sConnectingToAgni = false;
F32 LLVoiceClient::OVERDRIVEN_POWER_LEVEL = 0.7f;

const F32 SPEAKING_TIMEOUT = 1.f;

const int VOICE_MAJOR_VERSION = 1;
const int VOICE_MINOR_VERSION = 0;

LLVoiceClient *gVoiceClient = NULL;

// Don't retry connecting to the daemon more frequently than this:
const F32 CONNECT_THROTTLE_SECONDS = 1.0f;

// Don't send positional updates more frequently than this:
const F32 UPDATE_THROTTLE_SECONDS = 0.1f;

const F32 LOGIN_RETRY_SECONDS = 10.0f;
const int MAX_LOGIN_RETRIES = 12;

class LLViewerVoiceAccountProvisionResponder :
	public LLHTTPClient::Responder
{
public:
	LLViewerVoiceAccountProvisionResponder(int retries)
	{
		mRetries = retries;
	}

	virtual void error(U32 status, const std::string& reason)
	{
		if ( mRetries > 0 )
		{
			if ( gVoiceClient ) gVoiceClient->requestVoiceAccountProvision(
				mRetries - 1);
		}
		else
		{
			//TODO: throw an error message?
			if ( gVoiceClient ) gVoiceClient->giveUp();
		}
	}

	virtual void result(const LLSD& content)
	{
		if ( gVoiceClient )
		{
			gVoiceClient->login(
				content["username"].asString(),
				content["password"].asString());
		}
	}

private:
	int mRetries;
};

/** 
 * @class LLVivoxProtocolParser
 * @brief This class helps construct new LLIOPipe specializations
 * @see LLIOPipe
 *
 * THOROUGH_DESCRIPTION
 */
class LLVivoxProtocolParser : public LLIOPipe
{
	LOG_CLASS(LLVivoxProtocolParser);
public:
	LLVivoxProtocolParser();
	virtual ~LLVivoxProtocolParser();

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}
	
	std::string 	mInput;
	
	// Expat control members
	XML_Parser		parser;
	int				responseDepth;
	bool			ignoringTags;
	bool			isEvent;
	int				ignoreDepth;

	// Members for processing responses. The values are transient and only valid within a call to processResponse().
	int				returnCode;
	int				statusCode;
	std::string		statusString;
	std::string		uuidString;
	std::string		actionString;
	std::string		connectorHandle;
	std::string		accountHandle;
	std::string		sessionHandle;
	std::string		eventSessionHandle;

	// Members for processing events. The values are transient and only valid within a call to processResponse().
	std::string		eventTypeString;
	int				state;
	std::string		uriString;
	bool			isChannel;
	std::string		nameString;
	std::string		audioMediaString;
	std::string		displayNameString;
	int				participantType;
	bool			isLocallyMuted;
	bool			isModeratorMuted;
	bool			isSpeaking;
	int				volume;
	F32				energy;

	// Members for processing text between tags
	std::string		textBuffer;
	bool			accumulateText;
	
	void			reset();

	void			processResponse(std::string tag);

static void XMLCALL ExpatStartTag(void *data, const char *el, const char **attr);
static void XMLCALL ExpatEndTag(void *data, const char *el);
static void XMLCALL ExpatCharHandler(void *data, const XML_Char *s, int len);

	void			StartTag(const char *tag, const char **attr);
	void			EndTag(const char *tag);
	void			CharData(const char *buffer, int length);
	
};

LLVivoxProtocolParser::LLVivoxProtocolParser()
{
	parser = NULL;
	parser = XML_ParserCreate(NULL);
	
	reset();
}

void LLVivoxProtocolParser::reset()
{
	responseDepth = 0;
	ignoringTags = false;
	accumulateText = false;
	textBuffer.clear();
}

//virtual 
LLVivoxProtocolParser::~LLVivoxProtocolParser()
{
	if (parser)
		XML_ParserFree(parser);
}

// virtual
LLIOPipe::EStatus LLVivoxProtocolParser::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLBufferStream istr(channels, buffer.get());
	std::ostringstream ostr;
	while (istr.good())
	{
		char buf[1024];
		istr.read(buf, sizeof(buf));
		mInput.append(buf, istr.gcount());
	}
	
	// MBW -- XXX -- This should no longer be necessary.  Or even possible.
	// We've read all the data out of the buffer.  Make sure it doesn't accumulate.
//	buffer->clear();
	
	// Look for input delimiter(s) in the input buffer.  If one is found, send the message to the xml parser.
	int start = 0;
	int delim;
	while((delim = mInput.find("\n\n\n", start)) != std::string::npos)
	{	
		// Turn this on to log incoming XML
		if(0)
		{
			int foo = mInput.find("Set3DPosition", start);
			int bar = mInput.find("ParticipantPropertiesEvent", start);
			if(foo != std::string::npos && (foo < delim))
			{
				// This is a Set3DPosition response.  Don't print it, since these are way too spammy.
			}
			else if(bar != std::string::npos && (bar < delim))
			{
				// This is a ParticipantPropertiesEvent response.  Don't print it, since these are way too spammy.
			}
			else
			{
				llinfos << "parsing: " << mInput.substr(start, delim - start) << llendl;
			}
		}
		
		// Reset internal state of the LLVivoxProtocolParser (no effect on the expat parser)
		reset();
		
		XML_ParserReset(parser, NULL);
		XML_SetElementHandler(parser, ExpatStartTag, ExpatEndTag);
		XML_SetCharacterDataHandler(parser, ExpatCharHandler);
		XML_SetUserData(parser, this);	
		XML_Parse(parser, mInput.data() + start, delim - start, false);
		
		start = delim + 3;
	}
	
	if(start != 0)
		mInput = mInput.substr(start);

//	llinfos << "at end, mInput is: " << mInput << llendl;
	
	if(!gVoiceClient->mConnected)
	{
		// If voice has been disabled, we just want to close the socket.  This does so.
		llinfos << "returning STATUS_STOP" << llendl;
		return STATUS_STOP;
	}
	
	return STATUS_OK;
}

void XMLCALL LLVivoxProtocolParser::ExpatStartTag(void *data, const char *el, const char **attr)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->StartTag(el, attr);
	}
}

// --------------------------------------------------------------------------------

void XMLCALL LLVivoxProtocolParser::ExpatEndTag(void *data, const char *el)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->EndTag(el);
	}
}

// --------------------------------------------------------------------------------

void XMLCALL LLVivoxProtocolParser::ExpatCharHandler(void *data, const XML_Char *s, int len)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->CharData(s, len);
	}
}

// --------------------------------------------------------------------------------


void LLVivoxProtocolParser::StartTag(const char *tag, const char **attr)
{
	// Reset the text accumulator. We shouldn't have strings that are inturrupted by new tags
	textBuffer.clear();
	// only accumulate text if we're not ignoring tags.
	accumulateText = !ignoringTags;
	
	if (responseDepth == 0)
	{	
		isEvent = strcmp("Event", tag) == 0;
		
		if (strcmp("Response", tag) == 0 || isEvent)
		{
			// Grab the attributes
			while (*attr)
			{
				const char	*key = *attr++;
				const char	*value = *attr++;
				
				if (strcmp("requestId", key) == 0)
				{
					uuidString = value;
				}
				else if (strcmp("action", key) == 0)
				{
					actionString = value;
				}
				else if (strcmp("type", key) == 0)
				{
					eventTypeString = value;
				}
			}
		}
		//llinfos << tag << " (" << responseDepth << ")"  << llendl;
	}
	else
	{
		if (ignoringTags)
		{
			//llinfos << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << llendl;
		}
		else
		{
			//llinfos << tag << " (" << responseDepth << ")"  << llendl;
	
			// Ignore the InputXml stuff so we don't get confused
			if (strcmp("InputXml", tag) == 0)
			{
				ignoringTags = true;
				ignoreDepth = responseDepth;
				accumulateText = false;

				//llinfos << "starting ignore, ignoreDepth is " << ignoreDepth << llendl;
			}
			else if (strcmp("CaptureDevices", tag) == 0)
			{
				gVoiceClient->clearCaptureDevices();
			}
			else if (strcmp("RenderDevices", tag) == 0)
			{
				gVoiceClient->clearRenderDevices();
			}
		}
	}
	responseDepth++;
}

// --------------------------------------------------------------------------------

void LLVivoxProtocolParser::EndTag(const char *tag)
{
	const char	*string = textBuffer.c_str();
	bool clearbuffer = true;

	responseDepth--;

	if (ignoringTags)
	{
		if (ignoreDepth == responseDepth)
		{
			//llinfos << "end of ignore" << llendl;
			ignoringTags = false;
		}
		else
		{
			//llinfos << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << llendl;
		}
	}
	
	if (!ignoringTags)
	{
		//llinfos << "processing tag " << tag << " (depth = " << responseDepth << ")" << llendl;

		// Closing a tag. Finalize the text we've accumulated and reset
		if (strcmp("ReturnCode", tag) == 0)
			returnCode = strtol(string, NULL, 10);
		else if (strcmp("StatusCode", tag) == 0)
			statusCode = strtol(string, NULL, 10);
		else if (strcmp("ConnectorHandle", tag) == 0)
			connectorHandle = string;
		else if (strcmp("AccountHandle", tag) == 0)
			accountHandle = string;
		else if (strcmp("SessionHandle", tag) == 0)
		{
			if (isEvent)
				eventSessionHandle = string;
			else
				sessionHandle = string;
		}
		else if (strcmp("StatusString", tag) == 0)
			statusString = string;
		else if (strcmp("State", tag) == 0)
			state = strtol(string, NULL, 10);
		else if (strcmp("URI", tag) == 0)
			uriString = string;
		else if (strcmp("IsChannel", tag) == 0)
			isChannel = strcmp(string, "true") == 0;
		else if (strcmp("Name", tag) == 0)
			nameString = string;
		else if (strcmp("AudioMedia", tag) == 0)
			audioMediaString = string;
		else if (strcmp("ChannelName", tag) == 0)
			nameString = string;
		else if (strcmp("ParticipantURI", tag) == 0)
			uriString = string;
		else if (strcmp("DisplayName", tag) == 0)
			displayNameString = string;
		else if (strcmp("AccountName", tag) == 0)
			nameString = string;
		else if (strcmp("ParticipantTyppe", tag) == 0)
			participantType = strtol(string, NULL, 10);
		else if (strcmp("IsLocallyMuted", tag) == 0)
			isLocallyMuted = strcmp(string, "true") == 0;
		else if (strcmp("IsModeratorMuted", tag) == 0)
			isModeratorMuted = strcmp(string, "true") == 0;
		else if (strcmp("IsSpeaking", tag) == 0)
			isSpeaking = strcmp(string, "true") == 0;
		else if (strcmp("Volume", tag) == 0)
			volume = strtol(string, NULL, 10);
		else if (strcmp("Energy", tag) == 0)
			energy = (F32)strtod(string, NULL);
		else if (strcmp("MicEnergy", tag) == 0)
			energy = (F32)strtod(string, NULL);
		else if (strcmp("ChannelName", tag) == 0)
			nameString = string;
		else if (strcmp("ChannelURI", tag) == 0)
			uriString = string;
		else if (strcmp("ChannelListResult", tag) == 0)
		{
			gVoiceClient->addChannelMapEntry(nameString, uriString);
		}
		else if (strcmp("Device", tag) == 0)
		{
			// This closing tag shouldn't clear the accumulated text.
			clearbuffer = false;
		}
		else if (strcmp("CaptureDevice", tag) == 0)
		{
			gVoiceClient->addCaptureDevice(textBuffer);
		}
		else if (strcmp("RenderDevice", tag) == 0)
		{
			gVoiceClient->addRenderDevice(textBuffer);
		}

		if(clearbuffer)
		{
			textBuffer.clear();
			accumulateText= false;
		}
		
		if (responseDepth == 0)
		{
			// We finished all of the XML, process the data
			processResponse(tag);
		}
	}
}

// --------------------------------------------------------------------------------

void LLVivoxProtocolParser::CharData(const char *buffer, int length)
{
	/*
		This method is called for anything that isn't a tag, which can be text you
		want that lies between tags, and a lot of stuff you don't want like file formatting
		(tabs, spaces, CR/LF, etc).
		
		Only copy text if we are in accumulate mode...
	*/
	if (accumulateText)
		textBuffer.append(buffer, length);
}

// --------------------------------------------------------------------------------

void LLVivoxProtocolParser::processResponse(std::string tag)
{
	//llinfos << tag << llendl;

	if (isEvent)
	{
		if (eventTypeString == "LoginStateChangeEvent")
		{
			gVoiceClient->loginStateChangeEvent(accountHandle, statusCode, statusString, state);
		}
		else if (eventTypeString == "SessionNewEvent")
		{
			gVoiceClient->sessionNewEvent(accountHandle, eventSessionHandle, state, nameString, uriString);
		}
		else if (eventTypeString == "SessionStateChangeEvent")
		{
			gVoiceClient->sessionStateChangeEvent(uriString, statusCode, statusString, eventSessionHandle, state, isChannel, nameString);
		}
		else if (eventTypeString == "ParticipantStateChangeEvent")
		{
			gVoiceClient->participantStateChangeEvent(uriString, statusCode, statusString, state,  nameString, displayNameString, participantType);
			
		}
		else if (eventTypeString == "ParticipantPropertiesEvent")
		{
			gVoiceClient->participantPropertiesEvent(uriString, statusCode, statusString, isLocallyMuted, isModeratorMuted, isSpeaking, volume, energy);
		}
		else if (eventTypeString == "AuxAudioPropertiesEvent")
		{
			gVoiceClient->auxAudioPropertiesEvent(energy);
		}
	}
	else
	{
		if (actionString == "Connector.Create.1")
		{
			gVoiceClient->connectorCreateResponse(statusCode, statusString, connectorHandle);
		}
		else if (actionString == "Account.Login.1")
		{
			gVoiceClient->loginResponse(statusCode, statusString, accountHandle);
		}
		else if (actionString == "Session.Create.1")
		{
			gVoiceClient->sessionCreateResponse(statusCode, statusString, sessionHandle);			
		}
		else if (actionString == "Session.Connect.1")
		{
			gVoiceClient->sessionConnectResponse(statusCode, statusString);			
		}
		else if (actionString == "Session.Terminate.1")
		{
			gVoiceClient->sessionTerminateResponse(statusCode, statusString);			
		}
		else if (actionString == "Account.Logout.1")
		{
			gVoiceClient->logoutResponse(statusCode, statusString);			
		}
		else if (actionString == "Connector.InitiateShutdown.1")
		{
			gVoiceClient->connectorShutdownResponse(statusCode, statusString);			
		}
		else if (actionString == "Account.ChannelGetList.1")
		{
			gVoiceClient->channelGetListResponse(statusCode, statusString);
		}
/*
		else if (actionString == "Connector.AccountCreate.1")
		{
			
		}
		else if (actionString == "Connector.MuteLocalMic.1")
		{
			
		}
		else if (actionString == "Connector.MuteLocalSpeaker.1")
		{
			
		}
		else if (actionString == "Connector.SetLocalMicVolume.1")
		{
			
		}
		else if (actionString == "Connector.SetLocalSpeakerVolume.1")
		{
			
		}
		else if (actionString == "Session.ListenerSetPosition.1")
		{
			
		}
		else if (actionString == "Session.SpeakerSetPosition.1")
		{
			
		}
		else if (actionString == "Session.Set3DPosition.1")
		{
			
		}
		else if (actionString == "Session.AudioSourceSetPosition.1")
		{
			
		}
		else if (actionString == "Session.GetChannelParticipants.1")
		{
			
		}
		else if (actionString == "Account.ChannelCreate.1")
		{
			
		}
		else if (actionString == "Account.ChannelUpdate.1")
		{
			
		}
		else if (actionString == "Account.ChannelDelete.1")
		{
			
		}
		else if (actionString == "Account.ChannelCreateAndInvite.1")
		{
			
		}
		else if (actionString == "Account.ChannelFolderCreate.1")
		{
			
		}
		else if (actionString == "Account.ChannelFolderUpdate.1")
		{
			
		}
		else if (actionString == "Account.ChannelFolderDelete.1")
		{
			
		}
		else if (actionString == "Account.ChannelAddModerator.1")
		{
			
		}
		else if (actionString == "Account.ChannelDeleteModerator.1")
		{
			
		}
*/
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

class LLVoiceClientPrefsListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Note: Ignore the specific event value, look up the ones we want

		gVoiceClient->setVoiceEnabled(gSavedSettings.getBOOL("EnableVoiceChat"));
		gVoiceClient->setUsePTT(gSavedSettings.getBOOL("PTTCurrentlyEnabled"));
		std::string keyString = gSavedSettings.getString("PushToTalkButton");
		gVoiceClient->setPTTKey(keyString);
		gVoiceClient->setPTTIsToggle(gSavedSettings.getBOOL("PushToTalkToggle"));
		gVoiceClient->setEarLocation(gSavedSettings.getS32("VoiceEarLocation"));
		std::string serverName = gSavedSettings.getString("VivoxDebugServerName");
		gVoiceClient->setVivoxDebugServerName(serverName);

		std::string inputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
		gVoiceClient->setCaptureDevice(inputDevice);
		std::string outputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
		gVoiceClient->setRenderDevice(outputDevice);

		return true;
	}
};
static LLVoiceClientPrefsListener voice_prefs_listener;

class LLVoiceClientMuteListObserver : public LLMuteListObserver
{
	/* virtual */ void onChange()  { gVoiceClient->muteListChanged();}
};
static LLVoiceClientMuteListObserver mutelist_listener;
static bool sMuteListListener_listening = false;

///////////////////////////////////////////////////////////////////////////////////////////////

class LLVoiceClientCapResponder : public LLHTTPClient::Responder
{
public:
	LLVoiceClientCapResponder(void){};

	virtual void error(U32 status, const std::string& reason);	// called with bad status codes
	virtual void result(const LLSD& content);

private:
};

void LLVoiceClientCapResponder::error(U32 status, const std::string& reason)
{
	llwarns << "LLVoiceClientCapResponder::error("
		<< status << ": " << reason << ")"
		<< llendl;
}

void LLVoiceClientCapResponder::result(const LLSD& content)
{
	LLSD::map_const_iterator iter;
	for(iter = content.beginMap(); iter != content.endMap(); ++iter)
	{
		llinfos << "LLVoiceClientCapResponder::result got " 
			<< iter->first << llendl;
	}

	if ( content.has("voice_credentials") )
	{
		LLSD voice_credentials = content["voice_credentials"];
		std::string uri;
		std::string credentials;

		if ( voice_credentials.has("channel_uri") )
		{
			uri = voice_credentials["channel_uri"].asString();
		}
		if ( voice_credentials.has("channel_credentials") )
		{
			credentials =
				voice_credentials["channel_credentials"].asString();
		}

		gVoiceClient->setSpatialChannel(uri, credentials);
	}
}



#if LL_WINDOWS
static HANDLE sGatewayHandle = 0;

static bool isGatewayRunning()
{
	bool result = false;
	if(sGatewayHandle != 0)		
	{
		DWORD waitresult = WaitForSingleObject(sGatewayHandle, 0);
		if(waitresult != WAIT_OBJECT_0)
		{
			result = true;
		}			
	}
	return result;
}
static void killGateway()
{
	if(sGatewayHandle != 0)
	{
		TerminateProcess(sGatewayHandle,0);
	}
}

#else // Mac and linux

static pid_t sGatewayPID = 0;
static bool isGatewayRunning()
{
	bool result = false;
	if(sGatewayPID != 0)
	{
		// A kill with signal number 0 has no effect, just does error checking.  It should return an error if the process no longer exists.
		if(kill(sGatewayPID, 0) == 0)
		{
			result = true;
		}
	}
	return result;
}

static void killGateway()
{
	if(sGatewayPID != 0)
	{
		kill(sGatewayPID, SIGTERM);
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////

LLVoiceClient::LLVoiceClient()
{	
	gVoiceClient = this;
	mWriteInProgress = false;
	mAreaVoiceDisabled = false;
	mPTT = true;
	mUserPTTState = false;
	mMuteMic = false;
	mSessionTerminateRequested = false;
	mCommandCookie = 0;
	mNonSpatialChannel = false;
	mNextSessionSpatial = true;
	mNextSessionNoReconnect = false;
	mSessionP2P = false;
	mCurrentParcelLocalID = 0;
	mLoginRetryCount = 0;
	mVivoxErrorStatusCode = 0;

	mNextSessionResetOnClose = false;
	mSessionResetOnClose = false;
	mSpeakerVolume = 0;
	mMicVolume = 0;

	// Initial dirty state
	mSpatialCoordsDirty = false;
	mPTTDirty = true;
	mVolumeDirty = true;
	mSpeakerVolumeDirty = true;
	mMicVolumeDirty = true;
	mCaptureDeviceDirty = false;
	mRenderDeviceDirty = false;

	// Load initial state from prefs.
	mVoiceEnabled = gSavedSettings.getBOOL("EnableVoiceChat");
	mUsePTT = gSavedSettings.getBOOL("EnablePushToTalk");
	std::string keyString = gSavedSettings.getString("PushToTalkButton");
	setPTTKey(keyString);
	mPTTIsToggle = gSavedSettings.getBOOL("PushToTalkToggle");
	mEarLocation = gSavedSettings.getS32("VoiceEarLocation");
	setVoiceVolume(gSavedSettings.getBOOL("MuteVoice") ? 0.f : gSavedSettings.getF32("AudioLevelVoice"));
	std::string captureDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	setCaptureDevice(captureDevice);
	std::string renderDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	setRenderDevice(renderDevice);
	
	// Set up our listener to get updates on all prefs values we care about.
	gSavedSettings.getControl("EnableVoiceChat")->addListener(&voice_prefs_listener);
	gSavedSettings.getControl("PTTCurrentlyEnabled")->addListener(&voice_prefs_listener);
	gSavedSettings.getControl("PushToTalkButton")->addListener(&voice_prefs_listener);
	gSavedSettings.getControl("PushToTalkToggle")->addListener(&voice_prefs_listener);
	gSavedSettings.getControl("VoiceEarLocation")->addListener(&voice_prefs_listener);
	gSavedSettings.getControl("VivoxDebugServerName")->addListener(&voice_prefs_listener);
	gSavedSettings.getControl("VoiceInputAudioDevice")->addListener(&voice_prefs_listener);
	gSavedSettings.getControl("VoiceOutputAudioDevice")->addListener(&voice_prefs_listener);

	mTuningMode = false;
	mTuningEnergy = 0.0f;
	mTuningMicVolume = 0;
	mTuningMicVolumeDirty = true;
	mTuningSpeakerVolume = 0;
	mTuningSpeakerVolumeDirty = true;
					
	//  gMuteListp isn't set up at this point, so we defer this until later.
//	gMuteListp->addObserver(&mutelist_listener);
	
	mParticipantMapChanged = false;

	// stash the pump for later use
	// This now happens when init() is called instead.
	mPump = NULL;
	
#if LL_DARWIN || LL_LINUX
		// MBW -- XXX -- THIS DOES NOT BELONG HERE
		// When the vivox daemon dies, the next write attempt on our socket generates a SIGPIPE, which kills us.
		// This should cause us to ignore SIGPIPE and handle the error through proper channels.
		// This should really be set up elsewhere.  Where should it go?
		signal(SIGPIPE, SIG_IGN);
		
		// Since we're now launching the gateway with fork/exec instead of system(), we need to deal with zombie processes.
		// Ignoring SIGCHLD should prevent zombies from being created.  Alternately, we could use wait(), but I'd rather not do that.
		signal(SIGCHLD, SIG_IGN);
#endif

	// set up state machine
	setState(stateDisabled);
	
	gIdleCallbacks.addFunction(idle, this);
}

//---------------------------------------------------

LLVoiceClient::~LLVoiceClient()
{
}

//----------------------------------------------



void LLVoiceClient::init(LLPumpIO *pump)
{
	// constructor will set up gVoiceClient
	LLVoiceClient::getInstance()->mPump = pump;
}

void LLVoiceClient::terminate()
{
	if(gVoiceClient)
	{
		gVoiceClient->sessionTerminateSendMessage();
		gVoiceClient->logout();
		gVoiceClient->connectorShutdown();
		gVoiceClient->closeSocket();		// Need to do this now -- bad things happen if the destructor does it later.
		
		// This will do unpleasant things on windows.
//		killGateway();
		
		// Don't do this anymore -- LLSingleton will take care of deleting the object.		
//		delete gVoiceClient;
		
		// Hint to other code not to access the voice client anymore.
		gVoiceClient = NULL;
	}
}


/////////////////////////////
// utility functions

bool LLVoiceClient::writeString(const std::string &str)
{
	bool result = false;
	if(mConnected)
	{
		apr_status_t err;
		apr_size_t size = (apr_size_t)str.size();
		apr_size_t written = size;
	
//		llinfos << "sending: " << str << llendl;

		// MBW -- XXX -- check return code - sockets will fail (broken, etc.)
		err = apr_socket_send(
				mSocket->getSocket(),
				(const char*)str.data(),
				&written);
		
		if(err == 0)
		{
			// Success.
			result = true;
		}
		// MBW -- XXX -- handle partial writes (written is number of bytes written)
		// Need to set socket to non-blocking before this will work.
//		else if(APR_STATUS_IS_EAGAIN(err))
//		{
//			// 
//		}
		else
		{
			// Assume any socket error means something bad.  For now, just close the socket.
			char buf[MAX_STRING];
			llwarns << "apr error " << err << " ("<< apr_strerror(err, buf, MAX_STRING) << ") sending data to vivox daemon." << llendl;
			daemonDied();
		}
	}
		
	return result;
}


/////////////////////////////
// session control messages
void LLVoiceClient::connectorCreate()
{
	std::ostringstream stream;
	std::string logpath;
	std::string loglevel = "0";
	
	// Transition to stateConnectorStarted when the connector handle comes back.
	setState(stateConnectorStarting);

	std::string savedLogLevel = gSavedSettings.getString("VivoxDebugLevel");
	
	if(savedLogLevel != "-1")
	{
		llinfos << "creating connector with logging enabled" << llendl;
		loglevel = "10";
		logpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
	}
	
	stream 
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.Create.1\">"
		<< "<ClientName>V2 SDK</ClientName>"
		<< "<AccountManagementServer>" << mAccountServerURI << "</AccountManagementServer>"
		<< "<Logging>"
			<< "<Enabled>false</Enabled>"
			<< "<Folder>" << logpath << "</Folder>"
			<< "<FileNamePrefix>Connector</FileNamePrefix>"
			<< "<FileNameSuffix>.log</FileNameSuffix>"
			<< "<LogLevel>" << loglevel << "</LogLevel>"
		<< "</Logging>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::connectorShutdown()
{
	setState(stateConnectorStopping);
	
	if(!mConnectorHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.InitiateShutdown.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
		<< "</Request>"
		<< "\n\n\n";
		
		mConnectorHandle.clear();
		
		writeString(stream.str());
	}
}

void LLVoiceClient::userAuthorized(const std::string& firstName, const std::string& lastName, const LLUUID &agentID)
{
	mAccountFirstName = firstName;
	mAccountLastName = lastName;

	mAccountDisplayName = firstName;
	mAccountDisplayName += " ";
	mAccountDisplayName += lastName;

	llinfos << "name \"" << mAccountDisplayName << "\" , ID " << agentID << llendl;

	std::string gridname = gGridName;
	LLString::toLower(gridname);
	if((gGridChoice == GRID_INFO_AGNI) || 
		((gGridChoice == GRID_INFO_OTHER) && (gridname.find("agni") != std::string::npos)))
	{
		sConnectingToAgni = true;
	}

	// MBW -- XXX -- Enable this when the bhd.vivox.com server gets a real ssl cert.	
	if(sConnectingToAgni)
	{
		// Use the release account server
		mAccountServerName = "bhr.vivox.com";
		mAccountServerURI = "https://www." + mAccountServerName + "/api2/";
	}
	else
	{
		// Use the development account server
		mAccountServerName = gSavedSettings.getString("VivoxDebugServerName");
		mAccountServerURI = "https://www." + mAccountServerName + "/api2/";
	}

	mAccountName = nameFromID(agentID);
}

void LLVoiceClient::requestVoiceAccountProvision(S32 retries)
{
	if ( gAgent.getRegion() && mVoiceEnabled )
	{
		std::string url = 
			gAgent.getRegion()->getCapability(
				"ProvisionVoiceAccountRequest");

		if ( url == "" ) return;

		LLHTTPClient::post(
			url,
			LLSD(),
			new LLViewerVoiceAccountProvisionResponder(retries));
	}
}

void LLVoiceClient::login(
	const std::string& accountName,
	const std::string &password)
{
	if((getState() >= stateLoggingIn) && (getState() < stateLoggedOut))
	{
		// Already logged in.  This is an internal error.
		llerrs << "called from wrong state." << llendl;
	}
	else if ( accountName != mAccountName )
	{
		//TODO: error?
		llinfos << "Wrong account name! " << accountName
				<< " instead of " << mAccountName << llendl;
	}
	else
	{
		mAccountPassword = password;
	}
}

void LLVoiceClient::idle(void* user_data)
{
	LLVoiceClient* self = (LLVoiceClient*)user_data;
	self->stateMachine();
}

const char *LLVoiceClient::state2string(LLVoiceClient::state inState)
{
	const char *result = "UNKNOWN";
	
		// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break

	switch(inState)
	{
		CASE(stateDisabled);
		CASE(stateStart);
		CASE(stateDaemonLaunched);
		CASE(stateConnecting);
		CASE(stateIdle);
		CASE(stateConnectorStart);
		CASE(stateConnectorStarting);
		CASE(stateConnectorStarted);
		CASE(stateLoginRetry);
		CASE(stateLoginRetryWait);
		CASE(stateNeedsLogin);
		CASE(stateLoggingIn);
		CASE(stateLoggedIn);
		CASE(stateNoChannel);
		CASE(stateMicTuningStart);
		CASE(stateMicTuningRunning);
		CASE(stateMicTuningStop);
		CASE(stateSessionCreate);
		CASE(stateSessionConnect);
		CASE(stateJoiningSession);
		CASE(stateSessionJoined);
		CASE(stateRunning);
		CASE(stateLeavingSession);
		CASE(stateSessionTerminated);
		CASE(stateLoggingOut);
		CASE(stateLoggedOut);
		CASE(stateConnectorStopping);
		CASE(stateConnectorStopped);
		CASE(stateConnectorFailed);
		CASE(stateConnectorFailedWaiting);
		CASE(stateLoginFailed);
		CASE(stateLoginFailedWaiting);
		CASE(stateJoinSessionFailed);
		CASE(stateJoinSessionFailedWaiting);
		CASE(stateJail);
		CASE(stateMicTuningNoLogin);
	}

#undef CASE
	
	return result;
}

const char *LLVoiceClientStatusObserver::status2string(LLVoiceClientStatusObserver::EStatusType inStatus)
{
	const char *result = "UNKNOWN";
	
		// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break

	switch(inStatus)
	{
		CASE(STATUS_LOGIN_RETRY);
		CASE(STATUS_LOGGED_IN);
		CASE(STATUS_JOINING);
		CASE(STATUS_JOINED);
		CASE(STATUS_LEFT_CHANNEL);
		CASE(BEGIN_ERROR_STATUS);
		CASE(ERROR_CHANNEL_FULL);
		CASE(ERROR_CHANNEL_LOCKED);
		CASE(ERROR_NOT_AVAILABLE);
		CASE(ERROR_UNKNOWN);
	default:
		break;
	}

#undef CASE
	
	return result;
}

void LLVoiceClient::setState(state inState)
{
	llinfos << "entering state " << state2string(inState) << llendl;
	
	mState = inState;
}

void LLVoiceClient::stateMachine()
{
	if(gDisconnected)
	{
		// The viewer has been disconnected from the sim.  Disable voice.
		setVoiceEnabled(false);
	}
	
	if(!mVoiceEnabled)
	{
		if(getState() != stateDisabled)
		{
			// User turned off voice support.  Send the cleanup messages, close the socket, and reset.
			if(!mConnected)
			{
				// if voice was turned off after the daemon was launched but before we could connect to it, we may need to issue a kill.
				llinfos << "Disabling voice before connection to daemon, terminating." << llendl;
				killGateway();
			}
			
			sessionTerminateSendMessage();
			logout();
			connectorShutdown();
			closeSocket();
			removeAllParticipants();

			setState(stateDisabled);
		}
	}
	
	// Check for parcel boundary crossing
	{
		LLViewerRegion *region = gAgent.getRegion();
		LLParcel *parcel = NULL;

		if(gParcelMgr)
		{
			parcel = gParcelMgr->getAgentParcel();
		}			
		
		if(region && parcel)
		{
			S32 parcelLocalID = parcel->getLocalID();
			std::string regionName = region->getName();
			std::string capURI = region->getCapability("ParcelVoiceInfoRequest");
		
//			llinfos << "Region name = \"" << regionName <<"\", " << "parcel local ID = " << parcelLocalID << llendl;

			// The region name starts out empty and gets filled in later.  
			// Also, the cap gets filled in a short time after the region cross, but a little too late for our purposes.
			// If either is empty, wait for the next time around.
			if(!regionName.empty() && !capURI.empty())
			{
				if((parcelLocalID != mCurrentParcelLocalID) || (regionName != mCurrentRegionName))
				{
					// We have changed parcels.  Initiate a parcel channel lookup.
					mCurrentParcelLocalID = parcelLocalID;
					mCurrentRegionName = regionName;
					
					parcelChanged();
				}
			}
		}
	}

	switch(getState())
	{
		case stateDisabled:
			if(mVoiceEnabled && (!mAccountName.empty() || mTuningMode))
			{
				setState(stateStart);
			}
		break;
		
		case stateStart:
			if(gDisableVoice)
			{
				// Voice is locked out, we must not launch the vivox daemon.
				setState(stateJail);
			}
			else if(!isGatewayRunning())
			{
				if(true)
				{
					// Launch the voice daemon
					std::string exe_path = gDirUtilp->getAppRODataDir();
					exe_path += gDirUtilp->getDirDelimiter();
#if LL_WINDOWS
					exe_path += "SLVoice.exe";
#else
					// This will be the same for mac and linux
					exe_path += "SLVoice";
#endif
					// See if the vivox executable exists
					llstat s;
					if(!LLFile::stat(exe_path.c_str(), &s))
					{
						// vivox executable exists.  Build the command line and launch the daemon.
						std::string args = " -p tcp -h -c";
						std::string cmd;
						std::string loglevel = gSavedSettings.getString("VivoxDebugLevel");
						
						if(loglevel.empty())
						{
							loglevel = "-1";	// turn logging off completely
						}
						
						args += " -ll ";
						args += loglevel;
						
//						llinfos << "Args for SLVoice: " << args << llendl;

#if LL_WINDOWS
						PROCESS_INFORMATION pinfo;
						STARTUPINFOA sinfo;
						memset(&sinfo, 0, sizeof(sinfo));
						std::string exe_dir = gDirUtilp->getAppRODataDir();
						cmd = "SLVoice.exe";
						cmd += args;
						
						// So retarded.  Windows requires that the second parameter to CreateProcessA be a writable (non-const) string...
						char *args2 = new char[args.size() + 1];
						strcpy(args2, args.c_str());

						if(!CreateProcessA(exe_path.c_str(), args2, NULL, NULL, FALSE, 0, NULL, exe_dir.c_str(), &sinfo, &pinfo))
						{
//							DWORD dwErr = GetLastError();
						}
						else
						{
							// foo = pinfo.dwProcessId; // get your pid here if you want to use it later on
							// CloseHandle(pinfo.hProcess); // stops leaks - nothing else
							sGatewayHandle = pinfo.hProcess;
							CloseHandle(pinfo.hThread); // stops leaks - nothing else
						}		
						
						delete[] args2;
#else	// LL_WINDOWS
						// This should be the same for mac and linux
						{
							std::vector<std::string> arglist;
							arglist.push_back(exe_path.c_str());
							
							// Split the argument string into separate strings for each argument
							typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
							boost::char_separator<char> sep(" ");
							tokenizer tokens(args, sep);
							tokenizer::iterator token_iter;

							for(token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
							{
								arglist.push_back(*token_iter);
							}
							
							// create an argv vector for the child process
							char **fakeargv = new char*[arglist.size() + 1];
							int i;
							for(i=0; i < arglist.size(); i++)
								fakeargv[i] = const_cast<char*>(arglist[i].c_str());

							fakeargv[i] = NULL;
							
							pid_t id = vfork();
							if(id == 0)
							{
								// child
								execv(exe_path.c_str(), fakeargv);
								
								// If we reach this point, the exec failed.
								// Use _exit() instead of exit() per the vfork man page.
								_exit(0);
							}

							// parent
							delete[] fakeargv;
							sGatewayPID = id;
						}
#endif	// LL_WINDOWS
						mDaemonHost = LLHost(gSavedSettings.getString("VoiceHost").c_str(), gSavedSettings.getU32("VoicePort"));
					}	
					else
					{
						llinfos << exe_path << "not found." << llendl
					}	
				}
				else
				{		
					// We can connect to a client gateway running on another host.  This is useful for testing.
					// To do this, launch the gateway on a nearby host like this:
					//  vivox-gw.exe -p tcp -i 0.0.0.0:44124
					// and put that host's IP address here.
					mDaemonHost = LLHost(gSavedSettings.getString("VoiceHost").c_str(), gSavedSettings.getU32("VoicePort"));
				}

				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(CONNECT_THROTTLE_SECONDS);

				setState(stateDaemonLaunched);
				
				// Dirty the states we'll need to sync with the daemon when it comes up.
				mPTTDirty = true;
				mSpeakerVolumeDirty = true;
				// These only need to be set if they're not default (i.e. empty string).
				mCaptureDeviceDirty = !mCaptureDevice.empty();
				mRenderDeviceDirty = !mRenderDevice.empty();
			}
		break;
		
		case stateDaemonLaunched:
//			llinfos << "Connecting to vivox daemon" << llendl;
			if(mUpdateTimer.hasExpired())
			{
				mUpdateTimer.setTimerExpirySec(CONNECT_THROTTLE_SECONDS);

				if(!mSocket)
				{
					mSocket = LLSocket::create(gAPRPoolp, LLSocket::STREAM_TCP);	
				}
				
				mConnected = mSocket->blockingConnect(mDaemonHost);
				if(mConnected)
				{
					setState(stateConnecting);
				}
				else
				{
					// If the connect failed, the socket may have been put into a bad state.  Delete it.
					closeSocket();
				}
			}
		break;

		case stateConnecting:
		// Can't do this until we have the pump available.
		if(mPump)
		{
			// MBW -- Note to self: pumps and pipes examples in
			//  indra/test/io.cpp
			//  indra/test/llpipeutil.{cpp|h}

			// Attach the pumps and pipes
				
			LLPumpIO::chain_t readChain;

			readChain.push_back(LLIOPipe::ptr_t(new LLIOSocketReader(mSocket)));
			readChain.push_back(LLIOPipe::ptr_t(new LLVivoxProtocolParser()));

			mPump->addChain(readChain, NEVER_CHAIN_EXPIRY_SECS);

			setState(stateIdle);
		}

		break;
		
		case stateIdle:
			// Initial devices query
			getCaptureDevicesSendMessage();
			getRenderDevicesSendMessage();

			mLoginRetryCount = 0;
			
			setState(stateConnectorStart);
				
		break;
		
		case stateConnectorStart:
			if(!mVoiceEnabled)
			{
				// We were never logged in.  This will shut down the connector.
				setState(stateLoggedOut);
			}
			else if(!mAccountServerURI.empty())
			{
				connectorCreate();
			}
			else if(mTuningMode)
			{
				mTuningExitState = stateConnectorStart;
				setState(stateMicTuningStart);
			}
		break;
		
		case stateConnectorStarting:	// waiting for connector handle
			// connectorCreateResponse() will transition from here to stateConnectorStarted.
		break;
		
		case stateConnectorStarted:		// connector handle received
			if(!mVoiceEnabled)
			{
				// We were never logged in.  This will shut down the connector.
				setState(stateLoggedOut);
			}
			else if(!mAccountName.empty())
			{
				LLViewerRegion *region = gAgent.getRegion();
				
				if(region)
				{
					if ( region->getCapability("ProvisionVoiceAccountRequest") != "" )
					{
						if ( mAccountPassword.empty() )
						{
							requestVoiceAccountProvision();
						}
						setState(stateNeedsLogin);
					}
				}
			}
		break;
				
		case stateMicTuningStart:
			if(mUpdateTimer.hasExpired())
			{
				if(mCaptureDeviceDirty || mRenderDeviceDirty)
				{
					// These can't be changed while in tuning mode.  Set them before starting.
					std::ostringstream stream;

					if(mCaptureDeviceDirty)
					{
						buildSetCaptureDevice(stream);
					}

					if(mRenderDeviceDirty)
					{
						buildSetRenderDevice(stream);
					}

					mCaptureDeviceDirty = false;
					mRenderDeviceDirty = false;

					if(!stream.str().empty())
					{
						writeString(stream.str());
					}				

					// This will come around again in the same state and start the capture, after the timer expires.
					mUpdateTimer.start();
					mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
				}
				else
				{
					// duration parameter is currently unused, per Mike S.
					tuningCaptureStartSendMessage(10000);

					setState(stateMicTuningRunning);
				}
			}
			
		break;
		
		case stateMicTuningRunning:
			if(!mTuningMode || !mVoiceEnabled || mSessionTerminateRequested || mCaptureDeviceDirty || mRenderDeviceDirty)
			{
				// All of these conditions make us leave tuning mode.
				setState(stateMicTuningStop);
			}
			else
			{
				// process mic/speaker volume changes
				if(mTuningMicVolumeDirty || mTuningSpeakerVolumeDirty)
				{
					std::ostringstream stream;
					
					if(mTuningMicVolumeDirty)
					{
						llinfos << "setting tuning mic level to " << mTuningMicVolume << llendl;
						stream
						<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetMicLevel.1\">"
						<< "<Level>" << mTuningMicVolume << "</Level>"
						<< "</Request>\n\n\n";
					}
					
					if(mTuningSpeakerVolumeDirty)
					{
						stream
						<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetSpeakerLevel.1\">"
						<< "<Level>" << mTuningSpeakerVolume << "</Level>"
						<< "</Request>\n\n\n";
					}
					
					mTuningMicVolumeDirty = false;
					mTuningSpeakerVolumeDirty = false;

					if(!stream.str().empty())
					{
						writeString(stream.str());
					}
				}
			}
		break;
		
		case stateMicTuningStop:
		{
			// transition out of mic tuning
			tuningCaptureStopSendMessage();
			
			setState(mTuningExitState);
			
			// if we exited just to change devices, this will keep us from re-entering too fast.
			mUpdateTimer.start();
			mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
			
		}
		break;
								
		case stateLoginRetry:
			if(mLoginRetryCount == 0)
			{
				// First retry -- display a message to the user
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGIN_RETRY);
			}
			
			mLoginRetryCount++;
			
			if(mLoginRetryCount > MAX_LOGIN_RETRIES)
			{
				llinfos << "too many login retries, giving up." << llendl;
				setState(stateLoginFailed);
			}
			else
			{
				llinfos << "will retry login in " << LOGIN_RETRY_SECONDS << " seconds." << llendl;
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(LOGIN_RETRY_SECONDS);
				setState(stateLoginRetryWait);
			}
		break;
		
		case stateLoginRetryWait:
			if(mUpdateTimer.hasExpired())
			{
				setState(stateNeedsLogin);
			}
		break;
		
		case stateNeedsLogin:
			if(!mAccountPassword.empty())
			{
				setState(stateLoggingIn);
				loginSendMessage();
			}		
		break;
		
		case stateLoggingIn:			// waiting for account handle
			// loginResponse() will transition from here to stateLoggedIn.
		break;
		
		case stateLoggedIn:				// account handle received
			// Initial kick-off of channel lookup logic
			parcelChanged();

			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);

			// Set up the mute list observer if it hasn't been set up already.
			if((!sMuteListListener_listening) && (gMuteListp))
			{
				gMuteListp->addObserver(&mutelist_listener);
				sMuteListListener_listening = true;
			}

			setState(stateNoChannel);
		break;
					
		case stateNoChannel:
			if(mSessionTerminateRequested || !mVoiceEnabled)
			{
				// MBW -- XXX -- Is this the right way out of this state?
				setState(stateSessionTerminated);
			}
			else if(mTuningMode)
			{
				mTuningExitState = stateNoChannel;
				setState(stateMicTuningStart);
			}
			else if(!mNextSessionHandle.empty())
			{
				setState(stateSessionConnect);
			}
			else if(!mNextSessionURI.empty())
			{
				setState(stateSessionCreate);
			}
		break;

		case stateSessionCreate:
			sessionCreateSendMessage();
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINING);
			setState(stateJoiningSession);
		break;
		
		case stateSessionConnect:
			sessionConnectSendMessage();
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINING);
			setState(stateJoiningSession);
		break;
		
		case stateJoiningSession:		// waiting for session handle
			// sessionCreateResponse() will transition from here to stateSessionJoined.
			if(!mVoiceEnabled)
			{
				// User bailed out during connect -- jump straight to teardown.
				setState(stateSessionTerminated);
			}
			else if(mSessionTerminateRequested)
			{
				if(!mSessionHandle.empty())
				{
					// Only allow direct exits from this state in p2p calls (for cancelling an invite).
					// Terminating a half-connected session on other types of calls seems to break something in the vivox gateway.
					if(mSessionP2P)
					{
						sessionTerminateSendMessage();
						setState(stateSessionTerminated);
					}
				}
			}
		break;
		
		case stateSessionJoined:		// session handle received
			// MBW -- XXX -- It appears that I need to wait for BOTH the Session.Create response and the SessionStateChangeEvent with state 4
			//  before continuing from this state.  They can happen in either order, and if I don't wait for both, things can get stuck.
			// For now, the Session.Create response handler sets mSessionHandle and the SessionStateChangeEvent handler transitions to stateSessionJoined.
			// This is a cheap way to make sure both have happened before proceeding.
			if(!mSessionHandle.empty())
			{
				// Events that need to happen when a session is joined could go here.
				// Maybe send initial spatial data?
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);

				// Dirty state that may need to be sync'ed with the daemon.
				mPTTDirty = true;
				mSpeakerVolumeDirty = true;
				mSpatialCoordsDirty = true;
				
				setState(stateRunning);
				
				// Start the throttle timer
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
			}
			else if(!mVoiceEnabled)
			{
				// User bailed out during connect -- jump straight to teardown.
				setState(stateSessionTerminated);
			}
			else if(mSessionTerminateRequested)
			{
				// Only allow direct exits from this state in p2p calls (for cancelling an invite).
				// Terminating a half-connected session on other types of calls seems to break something in the vivox gateway.
				if(mSessionP2P)
				{
					sessionTerminateSendMessage();
					setState(stateSessionTerminated);
				}
			}
		break;
		
		case stateRunning:				// steady state
			// sessionTerminateSendMessage() will transition from here to stateLeavingSession
			
			// Disabling voice or disconnect requested.
			if(!mVoiceEnabled || mSessionTerminateRequested)
			{
				sessionTerminateSendMessage();
			}
			else
			{
				
				// Figure out whether the PTT state needs to change
				{
					bool newPTT;
					if(mUsePTT)
					{
						// If configured to use PTT, track the user state.
						newPTT = mUserPTTState;
					}
					else
					{
						// If not configured to use PTT, it should always be true (otherwise the user will be unable to speak).
						newPTT = true;
					}
					
					if(mMuteMic)
					{
						// This always overrides any other PTT setting.
						newPTT = false;
					}
					
					// Dirty if state changed.
					if(newPTT != mPTT)
					{
						mPTT = newPTT;
						mPTTDirty = true;
					}
				}
				
				if(mNonSpatialChannel)
				{
					// When in a non-spatial channel, never send positional updates.
					mSpatialCoordsDirty = false;
				}
				else
				{
					// Do the calculation that enforces the listener<->speaker tether (and also updates the real camera position)
					enforceTether();
				}
				
				// Send an update if the ptt state has changed (which shouldn't be able to happen that often -- the user can only click so fast)
				// or every 10hz, whichever is sooner.
				if(mVolumeDirty || mPTTDirty || mSpeakerVolumeDirty || mUpdateTimer.hasExpired())
				{
					mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
					sendPositionalUpdate();
				}
			}
		break;
		
		case stateLeavingSession:		// waiting for terminate session response
			// The handler for the Session.Terminate response will transition from here to stateSessionTerminated.
		break;

		case stateSessionTerminated:
			// Always reset the terminate request flag when we get here.
			mSessionTerminateRequested = false;
			
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);

			if(mVoiceEnabled)
			{
				// SPECIAL CASE: if going back to spatial but in a parcel with an empty URI, transfer the non-spatial flag now.
				// This fixes the case where you come out of a group chat in a parcel with voice disabled, and get stuck unable to rejoin spatial chat thereafter.
				if(mNextSessionSpatial && mNextSessionURI.empty())
				{
					mNonSpatialChannel = !mNextSessionSpatial;
				}
				
				// Just leaving a channel, go back to stateNoChannel (the "logged in but have no channel" state).
				setState(stateNoChannel);
			}
			else
			{
				// Shutting down voice, continue with disconnecting.
				logout();
			}
			
		break;
		
		case stateLoggingOut:			// waiting for logout response
			// The handler for the Account.Logout response will transition from here to stateLoggedOut.
		break;
		case stateLoggedOut:			// logout response received
			// shut down the connector
			connectorShutdown();
		break;
		
		case stateConnectorStopping:	// waiting for connector stop
			// The handler for the Connector.InitiateShutdown response will transition from here to stateConnectorStopped.
		break;

		case stateConnectorStopped:		// connector stop received
			// Clean up and reset everything. 
			closeSocket();
			removeAllParticipants();
			setState(stateDisabled);
		break;

		case stateConnectorFailed:
			setState(stateConnectorFailedWaiting);
		break;
		case stateConnectorFailedWaiting:
		break;

		case stateLoginFailed:
			setState(stateLoginFailedWaiting);
		break;
		case stateLoginFailedWaiting:
			// No way to recover from these.  Yet.
		break;

		case stateJoinSessionFailed:
			// Transition to error state.  Send out any notifications here.
			llwarns << "stateJoinSessionFailed: (" << mVivoxErrorStatusCode << "): " << mVivoxErrorStatusString << llendl;
			notifyStatusObservers(LLVoiceClientStatusObserver::ERROR_UNKNOWN);
			setState(stateJoinSessionFailedWaiting);
		break;
		
		case stateJoinSessionFailedWaiting:
			// Joining a channel failed, either due to a failed channel name -> sip url lookup or an error from the join message.
			// Region crossings may leave this state and try the join again.
			if(mSessionTerminateRequested)
			{
				setState(stateSessionTerminated);
			}
		break;
		
		case stateJail:
			// We have given up.  Do nothing.
		break;

	        case stateMicTuningNoLogin:
			// *TODO: Implement me.
			llwarns << "stateMicTuningNoLogin not handled"
				<< llendl;
		break;
	}

	if(mParticipantMapChanged)
	{
		mParticipantMapChanged = false;
		notifyObservers();
	}

}

void LLVoiceClient::closeSocket(void)
{
	mSocket.reset();
	mConnected = false;	
}

void LLVoiceClient::loginSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Login.1\">"
		<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
		<< "<AccountName>" << mAccountName << "</AccountName>"
		<< "<AccountPassword>" << mAccountPassword << "</AccountPassword>"
		<< "<AudioSessionAnswerMode>VerifyAnswer</AudioSessionAnswerMode>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::logout()
{
	mAccountPassword = "";
	setState(stateLoggingOut);
	logoutSendMessage();
}

void LLVoiceClient::logoutSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Logout.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		mAccountHandle.clear();

		writeString(stream.str());
	}
}

void LLVoiceClient::channelGetListSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.ChannelGetList.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
	<< "</Request>\n\n\n";

	writeString(stream.str());
}

void LLVoiceClient::sessionCreateSendMessage()
{
	llinfos << "requesting join: " << mNextSessionURI << llendl;

	mSessionURI = mNextSessionURI;
	mNonSpatialChannel = !mNextSessionSpatial;
	mSessionResetOnClose = mNextSessionResetOnClose;
	mNextSessionResetOnClose = false;
	if(mNextSessionNoReconnect)
	{
		// Clear the stashed URI so it can't reconnect
		mNextSessionURI.clear();
	}
	// Only p2p sessions are created with "no reconnect".
	mSessionP2P = mNextSessionNoReconnect;

	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Create.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "<URI>" << mSessionURI << "</URI>";

	static const std::string allowed_chars =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
				"0123456789"
				"-._~";

	if(!mNextSessionHash.empty())
	{
		stream
			<< "<Password>" << LLURI::escape(mNextSessionHash, allowed_chars) << "</Password>"
			<< "<PasswordHashAlgorithm>SHA1UserName</PasswordHashAlgorithm>";
	}
	
	stream
		<< "<Name>" << mChannelName << "</Name>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}

void LLVoiceClient::sessionConnectSendMessage()
{
	llinfos << "connecting to session handle: " << mNextSessionHandle << llendl;
	
	mSessionHandle = mNextSessionHandle;
	mSessionURI = mNextP2PSessionURI;
	mNextSessionHandle.clear();		// never want to re-use these.
	mNextP2PSessionURI.clear();
	mNonSpatialChannel = !mNextSessionSpatial;
	mSessionResetOnClose = mNextSessionResetOnClose;
	mNextSessionResetOnClose = false;
	// Joining by session ID is only used to answer p2p invitations, so we know this is a p2p session.
	mSessionP2P = true;
	
	std::ostringstream stream;
	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Connect.1\">"
		<< "<SessionHandle>" << mSessionHandle << "</SessionHandle>"
		<< "<AudioMedia>default</AudioMedia>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}

void LLVoiceClient::sessionTerminate()
{
	mSessionTerminateRequested = true;
}

void LLVoiceClient::sessionTerminateSendMessage()
{
	llinfos << "leaving session: " << mSessionURI << llendl;

	switch(getState())
	{
		case stateNoChannel:
			// In this case, we want to pretend the join failed so our state machine doesn't get stuck.
			// Skip the join failed transition state so we don't send out error notifications.
			setState(stateJoinSessionFailedWaiting);
		break;
		case stateJoiningSession:
		case stateSessionJoined:
		case stateRunning:
			if(!mSessionHandle.empty())
			{
				sessionTerminateByHandle(mSessionHandle);
				setState(stateLeavingSession);
			}
			else
			{
				llwarns << "called with no session handle" << llendl;	
				setState(stateSessionTerminated);
			}
		break;
		case stateJoinSessionFailed:
		case stateJoinSessionFailedWaiting:
			setState(stateSessionTerminated);
		break;
		
		default:
			llwarns << "called from unknown state" << llendl;
		break;
	}
}

void LLVoiceClient::sessionTerminateByHandle(std::string &sessionHandle)
{
	llinfos << "Sending Session.Terminate with handle " << sessionHandle << llendl;	

	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Terminate.1\">"
		<< "<SessionHandle>" << sessionHandle << "</SessionHandle>"
	<< "</Request>"
	<< "\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::getCaptureDevicesSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.GetCaptureDevices.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::getRenderDevicesSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.GetRenderDevices.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::clearCaptureDevices()
{
	// MBW -- XXX -- do something here
	llinfos << "called" << llendl;
	mCaptureDevices.clear();
}

void LLVoiceClient::addCaptureDevice(const std::string& name)
{
	// MBW -- XXX -- do something here
	llinfos << name << llendl;

	mCaptureDevices.push_back(name);
}

LLVoiceClient::deviceList *LLVoiceClient::getCaptureDevices()
{
	return &mCaptureDevices;
}

void LLVoiceClient::setCaptureDevice(const std::string& name)
{
	if(name == "Default")
	{
		if(!mCaptureDevice.empty())
		{
			mCaptureDevice.clear();
			mCaptureDeviceDirty = true;	
		}
	}
	else
	{
		if(mCaptureDevice != name)
		{
			mCaptureDevice = name;
			mCaptureDeviceDirty = true;	
		}
	}
}

void LLVoiceClient::clearRenderDevices()
{
	// MBW -- XXX -- do something here
	llinfos << "called" << llendl;
	mRenderDevices.clear();
}

void LLVoiceClient::addRenderDevice(const std::string& name)
{
	// MBW -- XXX -- do something here
	llinfos << name << llendl;
	mRenderDevices.push_back(name);
}

LLVoiceClient::deviceList *LLVoiceClient::getRenderDevices()
{
	return &mRenderDevices;
}

void LLVoiceClient::setRenderDevice(const std::string& name)
{
	if(name == "Default")
	{
		if(!mRenderDevice.empty())
		{
			mRenderDevice.clear();
			mRenderDeviceDirty = true;	
		}
	}
	else
	{
		if(mRenderDevice != name)
		{
			mRenderDevice = name;
			mRenderDeviceDirty = true;	
		}
	}
	
}

void LLVoiceClient::tuningStart()
{
	mTuningMode = true;
	if(getState() >= stateNoChannel)
	{
		sessionTerminate();
	}
}

void LLVoiceClient::tuningStop()
{
	mTuningMode = false;
}

bool LLVoiceClient::inTuningMode()
{
	bool result = false;
	switch(getState())
	{
	case stateMicTuningRunning:
		result = true;
		break;
	default:
		break;
	}
	return result;
}

void LLVoiceClient::tuningRenderStartSendMessage(const std::string& name, bool loop)
{		
	mTuningAudioFile = name;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStart.1\">"
    << "<SoundFilePath>" << mTuningAudioFile << "</SoundFilePath>"
    << "<Loop>" << (loop?"1":"0") << "</Loop>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::tuningRenderStopSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
    << "<SoundFilePath>" << mTuningAudioFile << "</SoundFilePath>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::tuningCaptureStartSendMessage(int duration)
{
	llinfos << "sending CaptureAudioStart" << llendl;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStart.1\">"
    << "<Duration>" << duration << "</Duration>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::tuningCaptureStopSendMessage()
{
	llinfos << "sending CaptureAudioStop" << llendl;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());

	mTuningEnergy = 0.0f;
}

void LLVoiceClient::tuningSetMicVolume(float volume)
{
	int scaledVolume = ((int)(volume * 100.0f)) - 100;
	if(scaledVolume != mTuningMicVolume)
	{
		mTuningMicVolume = scaledVolume;
		mTuningMicVolumeDirty = true;
	}
}

void LLVoiceClient::tuningSetSpeakerVolume(float volume)
{
	// incoming volume has the range [0.0 ... 1.0], with 0.5 as the default.
	// Map it as follows: 0.0 -> -100, 0.5 -> 24, 1.0 -> 50
	
	volume -= 0.5f;		// offset volume to the range [-0.5 ... 0.5], with 0 at the default.
	int scaledVolume = 24;	// offset scaledVolume by its default level
	if(volume < 0.0f)
		scaledVolume += ((int)(volume * 248.0f));	// (24 - (-100)) * 2
	else
		scaledVolume += ((int)(volume * 52.0f));	// (50 - 24) * 2

	if(scaledVolume != mTuningSpeakerVolume)
	{
		mTuningSpeakerVolume = scaledVolume;
		mTuningSpeakerVolumeDirty = true;
	}
}
				
float LLVoiceClient::tuningGetEnergy(void)
{
	return mTuningEnergy;
}

bool LLVoiceClient::deviceSettingsAvailable()
{
	bool result = true;
	
	if(!mConnected)
		result = false;
	
	if(mRenderDevices.empty())
		result = false;
	
	return result;
}

void LLVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
	if(clearCurrentList)
	{
		clearCaptureDevices();
		clearRenderDevices();
	}
	getCaptureDevicesSendMessage();
	getRenderDevicesSendMessage();
}

void LLVoiceClient::daemonDied()
{
	// The daemon died, so the connection is gone.  Reset everything and start over.
	llwarns << "Connection to vivox daemon lost.  Resetting state."<< llendl;

	closeSocket();
	removeAllParticipants();
	
	// Try to relaunch the daemon
	setState(stateDisabled);
}

void LLVoiceClient::giveUp()
{
	// All has failed.  Clean up and stop trying.
	closeSocket();
	removeAllParticipants();
	
	setState(stateJail);
}

void LLVoiceClient::sendPositionalUpdate(void)
{	
	std::ostringstream stream;
	
	if(mSpatialCoordsDirty)
	{
		LLVector3 l, u, a;
		
		// Always send both speaker and listener positions together.
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Set3DPosition.1\">"		
			<< "<SessionHandle>" << mSessionHandle << "</SessionHandle>";
		
		stream << "<SpeakerPosition>";

		l = mAvatarRot.getLeftRow();
		u = mAvatarRot.getUpRow();
		a = mAvatarRot.getFwdRow();
			
//		llinfos << "Sending speaker position " << mSpeakerPosition << llendl;

		stream 
			<< "<Position>"
				<< "<X>" << mAvatarPosition[VX] << "</X>"
				<< "<Y>" << mAvatarPosition[VZ] << "</Y>"
				<< "<Z>" << mAvatarPosition[VY] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << mAvatarVelocity[VX] << "</X>"
				<< "<Y>" << mAvatarVelocity[VZ] << "</Y>"
				<< "<Z>" << mAvatarVelocity[VY] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << l.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VX] << "</Y>"
				<< "<Z>" << a.mV[VX] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << l.mV[VZ] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
			<< "<LeftOrientation>"
				<< "<X>" << l.mV [VY] << "</X>"
				<< "<Y>" << u.mV [VZ] << "</Y>"
				<< "<Z>" << a.mV [VY] << "</Z>"
			<< "</LeftOrientation>";

		stream << "</SpeakerPosition>";

		stream << "<ListenerPosition>";

		LLVector3d	earPosition;
		LLVector3	earVelocity;
		LLMatrix3	earRot;
		
		switch(mEarLocation)
		{
			case earLocCamera:
			default:
				earPosition = mCameraPosition;
				earVelocity = mCameraVelocity;
				earRot = mCameraRot;
			break;
			
			case earLocAvatar:
				earPosition = mAvatarPosition;
				earVelocity = mAvatarVelocity;
				earRot = mAvatarRot;
			break;
			
			case earLocMixed:
				earPosition = mAvatarPosition;
				earVelocity = mAvatarVelocity;
				earRot = mCameraRot;
			break;
		}

		l = earRot.getLeftRow();
		u = earRot.getUpRow();
		a = earRot.getFwdRow();

//		llinfos << "Sending listener position " << mListenerPosition << llendl;

		stream 
			<< "<Position>"
				<< "<X>" << earPosition[VX] << "</X>"
				<< "<Y>" << earPosition[VZ] << "</Y>"
				<< "<Z>" << earPosition[VY] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << earVelocity[VX] << "</X>"
				<< "<Y>" << earVelocity[VZ] << "</Y>"
				<< "<Z>" << earVelocity[VY] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << l.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VX] << "</Y>"
				<< "<Z>" << a.mV[VX] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << l.mV[VZ] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
			<< "<LeftOrientation>"
				<< "<X>" << l.mV [VY] << "</X>"
				<< "<Y>" << u.mV [VZ] << "</Y>"
				<< "<Z>" << a.mV [VY] << "</Z>"
			<< "</LeftOrientation>";

		stream << "</ListenerPosition>";

		stream << "</Request>\n\n\n";
	}	

	if(mPTTDirty)
	{
		// Send a local mute command.
		// NOTE that the state of "PTT" is the inverse of "local mute".
		//   (i.e. when PTT is true, we send a mute command with "false", and vice versa)
		
//		llinfos << "Sending MuteLocalMic command with parameter " << (mPTT?"false":"true") << llendl;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << (mPTT?"false":"true") << "</Value>"
			<< "</Request>\n\n\n";

	}
	
	if(mVolumeDirty)
	{
		participantMap::iterator iter = mParticipantMap.begin();
		
		for(; iter != mParticipantMap.end(); iter++)
		{
			participantState *p = iter->second;
			
			if(p->mVolumeDirty)
			{
				int volume = p->mOnMuteList?0:p->mUserVolume;
				
				llinfos << "Setting volume for avatar " << p->mAvatarID << " to " << volume << llendl;
				
				// Send a mute/unumte command for the user (actually "volume for me").
				stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetParticipantVolumeForMe.1\">"
					<< "<SessionHandle>" << mSessionHandle << "</SessionHandle>"
					<< "<ParticipantURI>" << p->mURI << "</ParticipantURI>"
					<< "<Volume>" << volume << "</Volume>"
					<< "</Request>\n\n\n";

				p->mVolumeDirty = false;
			}
		}
	}
	
	if(mSpeakerMuteDirty)
	{
		const char *muteval = ((mSpeakerVolume == -100)?"true":"false");
		llinfos << "Setting speaker mute to " << muteval  << llendl;
		
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalSpeaker.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << muteval << "</Value>"
			<< "</Request>\n\n\n";		
	}
	
	if(mSpeakerVolumeDirty)
	{
		llinfos << "Setting speaker volume to " << mSpeakerVolume  << llendl;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalSpeakerVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mSpeakerVolume << "</Value>"
			<< "</Request>\n\n\n";		
	}
	
	if(mMicVolumeDirty)
	{
		llinfos << "Setting mic volume to " << mMicVolume  << llendl;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalMicVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mMicVolume << "</Value>"
			<< "</Request>\n\n\n";		
	}

	
	// MBW -- XXX -- Maybe check to make sure the capture/render devices are in the current list here?
	if(mCaptureDeviceDirty)
	{
		buildSetCaptureDevice(stream);
	}

	if(mRenderDeviceDirty)
	{
		buildSetRenderDevice(stream);
	}
	
	mSpatialCoordsDirty = false;
	mPTTDirty = false;
	mVolumeDirty = false;
	mSpeakerVolumeDirty = false;
	mMicVolumeDirty = false;
	mSpeakerMuteDirty = false;
	mCaptureDeviceDirty = false;
	mRenderDeviceDirty = false;
	
	if(!stream.str().empty())
	{
		writeString(stream.str());
	}
}

void LLVoiceClient::buildSetCaptureDevice(std::ostringstream &stream)
{
	llinfos << "Setting input device = \"" << mCaptureDevice << "\"" << llendl;
	
	stream 
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetCaptureDevice.1\">"
		<< "<CaptureDeviceSpecifier>" << mCaptureDevice << "</CaptureDeviceSpecifier>"
	<< "</Request>"
	<< "\n\n\n";
}

void LLVoiceClient::buildSetRenderDevice(std::ostringstream &stream)
{
	llinfos << "Setting output device = \"" << mRenderDevice << "\"" << llendl;

	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetRenderDevice.1\">"
		<< "<RenderDeviceSpecifier>" << mRenderDevice << "</RenderDeviceSpecifier>"
	<< "</Request>"
	<< "\n\n\n";
}

/////////////////////////////
// Response/Event handlers

void LLVoiceClient::connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle)
{	
	if(statusCode != 0)
	{
		llwarns << "Connector.Create response failure: " << statusString << llendl;
		setState(stateConnectorFailed);
	}
	else
	{
		// Connector created, move forward.
		mConnectorHandle = connectorHandle;
		if(getState() == stateConnectorStarting)
		{
			setState(stateConnectorStarted);
		}
	}
}

void LLVoiceClient::loginResponse(int statusCode, std::string &statusString, std::string &accountHandle)
{ 
	llinfos << "Account.Login response (" << statusCode << "): " << statusString << llendl;
	
	// Status code of 20200 means "bad password".  We may want to special-case that at some point.
	
	if ( statusCode == 401 )
	{
		// Login failure which is probably caused by the delay after a user's password being updated.
		llinfos << "Account.Login response failure (" << statusCode << "): " << statusString << llendl;
		setState(stateLoginRetry);
	}
	else if(statusCode != 0)
	{
		llwarns << "Account.Login response failure (" << statusCode << "): " << statusString << llendl;
		setState(stateLoginFailed);
	}
	else
	{
		// Login succeeded, move forward.
		mAccountHandle = accountHandle;
		// MBW -- XXX -- This needs to wait until the LoginStateChangeEvent is received.
//		if(getState() == stateLoggingIn)
//		{
//			setState(stateLoggedIn);
//		}
	}
}

void LLVoiceClient::channelGetListResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		llwarns << "Account.ChannelGetList response failure: " << statusString << llendl;
		switchChannel();
	}
	else
	{
		// Got the channel list, try to do a lookup.
		std::string uri = findChannelURI(mChannelName);
		if(uri.empty())
		{	
			// Lookup failed, can't join a channel for this area.
			llinfos << "failed to map channel name: " << mChannelName << llendl;
		}
		else
		{
			// We have a sip URL for this area.
			llinfos << "mapped channel " << mChannelName << " to URI "<< uri << llendl;
		}
		
		// switchChannel with an empty uri string will do the right thing (leave channel and not rejoin)
		switchChannel(uri);
	}
}

void LLVoiceClient::sessionCreateResponse(int statusCode, std::string &statusString, std::string &sessionHandle)
{	
	if(statusCode != 0)
	{
		llwarns << "Session.Create response failure (" << statusCode << "): " << statusString << llendl;
//		if(statusCode == 1015)
//		{
//			if(getState() == stateJoiningSession)
//			{
//				// this happened during a real join.  Going to sessionTerminated should cause a retry in appropriate cases.
//				llwarns << "session handle \"" << sessionHandle << "\", mSessionStateEventHandle \"" << mSessionStateEventHandle << "\""<< llendl;
//				if(!sessionHandle.empty())
//				{
//					// This session is bad.  Terminate it.
//					mSessionHandle = sessionHandle;
//					sessionTerminateByHandle(sessionHandle);
//					setState(stateLeavingSession);
//				}
//				else if(!mSessionStateEventHandle.empty())
//				{
//					mSessionHandle = mSessionStateEventHandle;
//					sessionTerminateByHandle(mSessionStateEventHandle);
//					setState(stateLeavingSession);
//				}
//				else
//				{
//					setState(stateSessionTerminated);
//				}
//			}
//			else
//			{
//				// We didn't think we were in the middle of a join.  Don't change state.
//				llwarns << "Not in stateJoiningSession, ignoring" << llendl;
//			}
//		}
//		else
		{
			mVivoxErrorStatusCode = statusCode;		
			mVivoxErrorStatusString = statusString;
			setState(stateJoinSessionFailed);
		}
	}
	else
	{
		llinfos << "Session.Create response received (success), session handle is " << sessionHandle << llendl;
		if(getState() == stateJoiningSession)
		{
			// This is also grabbed in the SessionStateChangeEvent handler, but it might be useful to have it early...
			mSessionHandle = sessionHandle;
		}
		else
		{
			// We should never get a session.create response in any state except stateJoiningSession.  Things are out of sync.  Kill this session.
			sessionTerminateByHandle(sessionHandle);
		}
	}
}

void LLVoiceClient::sessionConnectResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		llwarns << "Session.Connect response failure (" << statusCode << "): " << statusString << llendl;
//		if(statusCode == 1015)
//		{
//			llwarns << "terminating existing session" << llendl;
//			sessionTerminate();
//		}
//		else
		{
			mVivoxErrorStatusCode = statusCode;		
			mVivoxErrorStatusString = statusString;
			setState(stateJoinSessionFailed);
		}
	}
	else
	{
		llinfos << "Session.Connect response received (success)" << llendl;
	}
}

void LLVoiceClient::sessionTerminateResponse(int statusCode, std::string &statusString)
{	
	if(statusCode != 0)
	{
		llwarns << "Session.Terminate response failure: (" << statusCode << "): " << statusString << llendl;
		if(getState() == stateLeavingSession)
		{
			// This is probably "(404): Server reporting Failure. Not a member of this conference."
			// Do this so we don't get stuck.
			setState(stateSessionTerminated);
		}
	}
	
}

void LLVoiceClient::logoutResponse(int statusCode, std::string &statusString)
{	
	if(statusCode != 0)
	{
		llwarns << "Account.Logout response failure: " << statusString << llendl;
		// MBW -- XXX -- Should this ever fail?  do we care if it does?
	}
	
	if(getState() == stateLoggingOut)
	{
		setState(stateLoggedOut);
	}
}

void LLVoiceClient::connectorShutdownResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		llwarns << "Connector.InitiateShutdown response failure: " << statusString << llendl;
		// MBW -- XXX -- Should this ever fail?  do we care if it does?
	}
	
	mConnected = false;
	
	if(getState() == stateConnectorStopping)
	{
		setState(stateConnectorStopped);
	}
}

void LLVoiceClient::sessionStateChangeEvent(
		std::string &uriString, 
		int statusCode, 
		std::string &statusString, 
		std::string &sessionHandle,
		int state,  
		bool isChannel, 
		std::string &nameString)
{
	switch(state)
	{
		case 4:	// I see this when joining the session
			llinfos << "joined session " << uriString << ", name " << nameString << " handle " << mNextSessionHandle << llendl;

			// Session create succeeded, move forward.
			mSessionStateEventHandle = sessionHandle;
			mSessionStateEventURI = uriString;
			if(sessionHandle == mSessionHandle)
			{
				// This is the session we're joining.
				if(getState() == stateJoiningSession)
				{
					setState(stateSessionJoined);
					//RN: the uriString being returned by vivox here is actually your account uri, not the channel
					// you are attempting to join, so ignore it
					//llinfos << "received URI " << uriString << "(previously " << mSessionURI << ")" << llendl;
					//mSessionURI = uriString;
				}
			}
			else if(sessionHandle == mNextSessionHandle)
			{
//				llinfos << "received URI " << uriString << ", name " << nameString << " for next session (handle " << mNextSessionHandle << ")" << llendl;
			}
			else
			{
				llwarns << "joining unknown session handle " << sessionHandle << ", URI " << uriString << ", name " << nameString << llendl;
				// MBW -- XXX -- Should we send a Session.Terminate here?
			}
			
		break;
		case 5:	// I see this when leaving the session
			llinfos << "left session " << uriString << ", name " << nameString << " handle " << mNextSessionHandle << llendl;

			// Set the session handle to the empty string.  If we get back to stateJoiningSession, we'll want to wait for the new session handle.
			if(sessionHandle == mSessionHandle)
			{
				// MBW -- XXX -- I think this is no longer necessary, now that we've got mNextSessionURI/mNextSessionHandle
				// mSessionURI.clear();
				// clear the session handle here just for sanity.
				mSessionHandle.clear();
				if(mSessionResetOnClose)
				{
					mSessionResetOnClose = false;
					mNonSpatialChannel = false;
					mNextSessionSpatial = true;
					parcelChanged();
				}
			
				removeAllParticipants();
				
				switch(getState())
				{
					case stateJoiningSession:
					case stateSessionJoined:
					case stateRunning:
					case stateLeavingSession:
					case stateJoinSessionFailed:
					case stateJoinSessionFailedWaiting:
						// normal transition
						llinfos << "left session " << sessionHandle << "in state " << state2string(getState()) << llendl;
						setState(stateSessionTerminated);
					break;
					
					case stateSessionTerminated:
						// this will happen sometimes -- there are cases where we send the terminate and then go straight to this state.
						llwarns << "left session " << sessionHandle << "in state " << state2string(getState()) << llendl;
					break;
					
					default:
						llwarns << "unexpected SessionStateChangeEvent (left session) in state " << state2string(getState()) << llendl;
						setState(stateSessionTerminated);
					break;
				}

				// store status values for later notification of observers
				mVivoxErrorStatusCode = statusCode;		
				mVivoxErrorStatusString = statusString;
			}
			else
			{
				llinfos << "leaving unknown session handle " << sessionHandle << ", URI " << uriString << ", name " << nameString << llendl;
			}

			mSessionStateEventHandle.clear();
			mSessionStateEventURI.clear();
		break;
		default:
			llwarns << "unknown state: " << state << llendl;
		break;
	}
}

void LLVoiceClient::loginStateChangeEvent(
		std::string &accountHandle, 
		int statusCode, 
		std::string &statusString, 
		int state)
{
	llinfos << "state is " << state << llendl;
	/*
		According to Mike S., status codes for this event are:
		login_state_logged_out=0,
        login_state_logged_in = 1,
        login_state_logging_in = 2,
        login_state_logging_out = 3,
        login_state_resetting = 4,
        login_state_error=100	
	*/
	
	switch(state)
	{
		case 1:
		if(getState() == stateLoggingIn)
		{
			setState(stateLoggedIn);
		}
		break;
		
		default:
//			llwarns << "unknown state: " << state << llendl;
		break;
	}
}

void LLVoiceClient::sessionNewEvent(
		std::string &accountHandle, 
		std::string &eventSessionHandle, 
		int state, 
		std::string &nameString, 
		std::string &uriString)
{
//	llinfos << "state is " << state << llendl;
	
	switch(state)
	{
		case 0:
			{
				llinfos << "session handle = " << eventSessionHandle << ", name = " << nameString << ", uri = " << uriString << llendl;

				LLUUID caller_id;
				if(IDFromName(nameString, caller_id))
				{
					gIMMgr->inviteToSession(
						LLIMMgr::computeSessionID(
							IM_SESSION_P2P_INVITE,
							caller_id),
						LLString::null,
						caller_id, 
						LLString::null, 
						IM_SESSION_P2P_INVITE, 
						LLIMMgr::INVITATION_TYPE_VOICE,
						eventSessionHandle);
				}
				else
				{
					llwarns << "Could not generate caller id from uri " << uriString << llendl;
				}
			}
		break;
		
		default:
			llwarns << "unknown state: " << state << llendl;
		break;
	}
}

void LLVoiceClient::participantStateChangeEvent(
		std::string &uriString, 
		int statusCode, 
		std::string &statusString, 
		int state,  
		std::string &nameString, 
		std::string &displayNameString, 
		int participantType)
{
	participantState *participant = NULL;
	llinfos << "state is " << state << llendl;

	switch(state)
	{
		case 7:	// I see this when a participant joins
			participant = addParticipant(uriString);
			if(participant)
			{
				participant->mName = nameString;
				llinfos << "added participant \"" << participant->mName 
						<< "\" (" << participant->mAvatarID << ")"<< llendl;
			}
		break;
		case 9:	// I see this when a participant leaves
			participant = findParticipant(uriString);
			if(participant)
			{
				removeParticipant(participant);
			}
		break;
		default:
//			llwarns << "unknown state: " << state << llendl;
		break;
	}
}

void LLVoiceClient::participantPropertiesEvent(
		std::string &uriString, 
		int statusCode, 
		std::string &statusString, 
		bool isLocallyMuted, 
		bool isModeratorMuted, 
		bool isSpeaking, 
		int volume, 
		F32 energy)
{
	participantState *participant = findParticipant(uriString);
	if(participant)
	{
		participant->mPTT = !isLocallyMuted;
		participant->mIsSpeaking = isSpeaking;
		participant->mIsModeratorMuted = isModeratorMuted;
		if (isSpeaking)
		{
			participant->mSpeakingTimeout.reset();
		}
		participant->mPower = energy;
		participant->mVolume = volume;
	}
	else
	{
		llwarns << "unknown participant: " << uriString << llendl;
	}
}

void LLVoiceClient::auxAudioPropertiesEvent(F32 energy)
{
//	llinfos << "got energy " << energy << llendl;
	mTuningEnergy = energy;
}

void LLVoiceClient::muteListChanged()
{
	// The user's mute list has been updated.  Go through the current participant list and sync it with the mute list.

	participantMap::iterator iter = mParticipantMap.begin();
	
	for(; iter != mParticipantMap.end(); iter++)
	{
		participantState *p = iter->second;
		
		// Check to see if this participant is on the mute list already
		updateMuteState(p);
	}
}

/////////////////////////////
// Managing list of participants
LLVoiceClient::participantState::participantState(const std::string &uri) : 
	 mURI(uri), mPTT(false), mIsSpeaking(false), mIsModeratorMuted(false), mPower(0.0), mServiceType(serviceTypeUnknown),
	 mOnMuteList(false), mUserVolume(100), mVolumeDirty(false), mAvatarIDValid(false)
{
}

LLVoiceClient::participantState *LLVoiceClient::addParticipant(const std::string &uri)
{
	participantState *result = NULL;

	participantMap::iterator iter = mParticipantMap.find(uri);
	
	if(iter != mParticipantMap.end())
	{
		// Found a matching participant already in the map.
		result = iter->second;
	}

	if(!result)
	{
		// participant isn't already in one list or the other.
		result = new participantState(uri);
		mParticipantMap.insert(participantMap::value_type(uri, result));
		mParticipantMapChanged = true;
		
		// Try to do a reverse transform on the URI to get the GUID back.
		{
			LLUUID id;
			if(IDFromName(uri, id))
			{
				result->mAvatarIDValid = true;
				result->mAvatarID = id;

				updateMuteState(result);
			}
		}
		
		llinfos << "participant \"" << result->mURI << "\" added." << llendl;
	}
	
	return result;
}

void LLVoiceClient::updateMuteState(participantState *p)
{
	if(p->mAvatarIDValid)
	{
		bool isMuted = gMuteListp->isMuted(p->mAvatarID, LLMute::flagVoiceChat);
		if(p->mOnMuteList != isMuted)
		{
			p->mOnMuteList = isMuted;
			p->mVolumeDirty = true;
			mVolumeDirty = true;
		}
	}
}

void LLVoiceClient::removeParticipant(LLVoiceClient::participantState *participant)
{
	if(participant)
	{
		participantMap::iterator iter = mParticipantMap.find(participant->mURI);
				
		llinfos << "participant \"" << participant->mURI <<  "\" (" << participant->mAvatarID << ") removed." << llendl;

		mParticipantMap.erase(iter);
		delete participant;
		mParticipantMapChanged = true;
	}
}

void LLVoiceClient::removeAllParticipants()
{
	llinfos << "called" << llendl;

	while(!mParticipantMap.empty())
	{
		removeParticipant(mParticipantMap.begin()->second);
	}
}

LLVoiceClient::participantMap *LLVoiceClient::getParticipantList(void)
{
	return &mParticipantMap;
}


LLVoiceClient::participantState *LLVoiceClient::findParticipant(const std::string &uri)
{
	participantState *result = NULL;
	
	// Don't find any participants if we're not connected.  This is so that we don't continue to get stale data
	// after the daemon dies.
	if(mConnected)
	{
		participantMap::iterator iter = mParticipantMap.find(uri);
	
		if(iter != mParticipantMap.end())
		{
			result = iter->second;
		}
	}
			
	return result;
}


LLVoiceClient::participantState *LLVoiceClient::findParticipantByAvatar(LLVOAvatar *avatar)
{
	participantState * result = NULL;

	// You'd think this would work, but it doesn't...
//	std::string uri = sipURIFromAvatar(avatar);
	
	// Currently, the URI is just the account name.
	std::string loginName = nameFromAvatar(avatar);
	result = findParticipant(loginName);
	
	if(result != NULL)
	{
		if(!result->mAvatarIDValid)
		{
			result->mAvatarID = avatar->getID();
			result->mAvatarIDValid = true;
			
			// We just figured out the avatar ID, so the participant list has "changed" from the perspective of anyone who uses that to identify participants.
			mParticipantMapChanged = true;
			
			updateMuteState(result);
		}
		

	}

	return result;
}

LLVoiceClient::participantState* LLVoiceClient::findParticipantByID(const LLUUID& id)
{
	participantState * result = NULL;

	// Currently, the URI is just the account name.
	std::string loginName = nameFromID(id);
	result = findParticipant(loginName);

	return result;
}


void LLVoiceClient::clearChannelMap(void)
{
	mChannelMap.clear();
}

void LLVoiceClient::addChannelMapEntry(std::string &name, std::string &uri)
{
//	llinfos << "Adding channel name mapping: " << name << " -> " << uri << llendl;
	mChannelMap.insert(channelMap::value_type(name, uri));
}

std::string LLVoiceClient::findChannelURI(std::string &name)
{
	std::string result;
	
	channelMap::iterator iter = mChannelMap.find(name);

	if(iter != mChannelMap.end())
	{
		result = iter->second;
	}
	
	return result;
}

void LLVoiceClient::parcelChanged()
{
	if(getState() >= stateLoggedIn)
	{
		// If the user is logged in, start a channel lookup.
		llinfos << "sending ParcelVoiceInfoRequest (" << mCurrentRegionName << ", " << mCurrentParcelLocalID << ")" << llendl;

		std::string url = gAgent.getRegion()->getCapability("ParcelVoiceInfoRequest");
		LLSD data;
		LLHTTPClient::post(
			url,
			data,
			new LLVoiceClientCapResponder);
	}
	else
	{
		// The transition to stateLoggedIn needs to kick this off again.
		llinfos << "not logged in yet, deferring" << llendl;
	}
}

void LLVoiceClient::switchChannel(
	std::string uri,
	bool spatial,
	bool noReconnect,
	std::string hash)
{
	bool needsSwitch = false;
	
	llinfos << "called in state " << state2string(getState()) << " with uri \"" << uri << "\"" << llendl;
	
	switch(getState())
	{
		case stateJoinSessionFailed:
		case stateJoinSessionFailedWaiting:
		case stateNoChannel:
			// Always switch to the new URI from these states.
			needsSwitch = true;
		break;
		
		default:
			if(mSessionTerminateRequested)
			{
				// If a terminate has been requested, we need to compare against where the URI we're already headed to.
				if(mNextSessionURI != uri)
					needsSwitch = true;
			}
			else
			{
				// Otherwise, compare against the URI we're in now.
				if(mSessionURI != uri)
					needsSwitch = true;
			}
		break;
	}
	
	if(needsSwitch)
	{
		mNextSessionURI = uri;
		mNextSessionHash = hash;
		mNextSessionHandle.clear();
		mNextP2PSessionURI.clear();
		mNextSessionSpatial = spatial;
		mNextSessionNoReconnect = noReconnect;
		
		if(uri.empty())
		{
			// Leave any channel we may be in
			llinfos << "leaving channel" << llendl;
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
		}
		else
		{
			llinfos << "switching to channel " << uri << llendl;
		}
		
		if(getState() <= stateNoChannel)
		{
			// We're already set up to join a channel, just needed to fill in the session URI
		}
		else
		{
			// State machine will come around and rejoin if uri/handle is not empty.
			sessionTerminate();
		}
	}
}

void LLVoiceClient::joinSession(std::string handle, std::string uri)
{
	mNextSessionURI.clear();
	mNextSessionHash.clear();
	mNextP2PSessionURI = uri;
	mNextSessionHandle = handle;
	mNextSessionSpatial = false;
	mNextSessionNoReconnect = false;

	if(getState() <= stateNoChannel)
	{
		// We're already set up to join a channel, just needed to fill in the session handle
	}
	else
	{
		// State machine will come around and rejoin if uri/handle is not empty.
		sessionTerminate();
	}
}

void LLVoiceClient::setNonSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	switchChannel(uri, false, false, credentials);
}

void LLVoiceClient::setSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	mSpatialSessionURI = uri;
	mAreaVoiceDisabled = mSpatialSessionURI.empty();

	llinfos << "got spatial channel uri: \"" << uri << "\"" << llendl;
	
	if(mNonSpatialChannel || !mNextSessionSpatial)
	{
		// User is in a non-spatial chat or joining a non-spatial chat.  Don't switch channels.
		llinfos << "in non-spatial chat, not switching channels" << llendl;
	}
	else
	{
		switchChannel(mSpatialSessionURI, true, false, credentials);
	}
}

void LLVoiceClient::callUser(LLUUID &uuid)
{
	std::string userURI = sipURIFromID(uuid);

	switchChannel(userURI, false, true);
}

void LLVoiceClient::answerInvite(std::string &sessionHandle, LLUUID& other_user_id)
{
	joinSession(sessionHandle, sipURIFromID(other_user_id));
}

void LLVoiceClient::declineInvite(std::string &sessionHandle)
{
	sessionTerminateByHandle(sessionHandle);
}

void LLVoiceClient::leaveNonSpatialChannel()
{
	switchChannel(mSpatialSessionURI);
}

std::string LLVoiceClient::getCurrentChannel()
{
	if((getState() == stateRunning) && !mSessionTerminateRequested)
	{
		return mSessionURI;
	}
	
	return "";
}

bool LLVoiceClient::inProximalChannel()
{
	bool result = false;
	
	if((getState() == stateRunning) && !mSessionTerminateRequested)
	{
		result = !mNonSpatialChannel;
	}
	
	return result;
}

std::string LLVoiceClient::sipURIFromID(const LLUUID &id)
{
	std::string result;
	result = "sip:";
	result += nameFromID(id);
	result += "@";
	result += mAccountServerName;
	
	return result;
}

std::string LLVoiceClient::sipURIFromAvatar(LLVOAvatar *avatar)
{
	std::string result;
	if(avatar)
	{
		result = "sip:";
		result += nameFromID(avatar->getID());
		result += "@";
		result += mAccountServerName;
	}
	
	return result;
}

std::string LLVoiceClient::nameFromAvatar(LLVOAvatar *avatar)
{
	std::string result;
	if(avatar)
	{
		result = nameFromID(avatar->getID());
	}	
	return result;
}

std::string LLVoiceClient::nameFromID(const LLUUID &uuid)
{
	std::string result;
	U8 rawuuid[UUID_BYTES + 1]; 
	uuid.toCompressedString((char*)rawuuid);
	
	// Prepending this apparently prevents conflicts with reserved names inside the vivox and diamondware code.
	result = "x";
	
	// Base64 encode and replace the pieces of base64 that are less compatible 
	// with e-mail local-parts.
	// See RFC-4648 "Base 64 Encoding with URL and Filename Safe Alphabet"
	result += LLBase64::encode(rawuuid, UUID_BYTES);
	LLString::replaceChar(result, '+', '-');
	LLString::replaceChar(result, '/', '_');
	
	// If you need to transform a GUID to this form on the Mac OS X command line, this will do so:
	// echo -n x && (echo e669132a-6c43-4ee1-a78d-6c82fff59f32 |xxd -r -p |openssl base64|tr '/+' '_-')
	
	return result;
}

bool LLVoiceClient::IDFromName(const std::string name, LLUUID &uuid)
{
	bool result = false;
	
	// This will only work if the name is of the proper form.
	// As an example, the account name for Monroe Linden (UUID 1673cfd3-8229-4445-8d92-ec3570e5e587) is:
	// "xFnPP04IpREWNkuw1cOXlhw=="
	
	if((name.size() == 25) && (name[0] == 'x') && (name[23] == '=') && (name[24] == '='))
	{
		// The name appears to have the right form.

		// Reverse the transforms done by nameFromID
		std::string temp = name;
		LLString::replaceChar(temp, '-', '+');
		LLString::replaceChar(temp, '_', '/');

		U8 rawuuid[UUID_BYTES + 1]; 
		int len = apr_base64_decode_binary(rawuuid, temp.c_str() + 1);
		if(len == UUID_BYTES)
		{
			// The decode succeeded.  Stuff the bits into the result's UUID
			// MBW -- XXX -- there's no analogue of LLUUID::toCompressedString that allows you to set a UUID from binary data.
			// The data field is public, so we cheat thusly:
			memcpy(uuid.mData, rawuuid, UUID_BYTES);
			result = true;
		}
	}
	
	return result;
}

std::string LLVoiceClient::displayNameFromAvatar(LLVOAvatar *avatar)
{
	return avatar->getFullname();
}

std::string LLVoiceClient::sipURIFromName(std::string &name)
{
	std::string result;
	result = "sip:";
	result += name;
	result += "@";
	result += mAccountServerName;

//	LLString::toLower(result);

	return result;
}

/////////////////////////////
// Sending updates of current state

void LLVoiceClient::enforceTether(void)
{
	LLVector3d tethered	= mCameraRequestedPosition;

	// constrain 'tethered' to within 50m of mAvatarPosition.
	{
		F32 max_dist = 50.0f;
		LLVector3d camera_offset = mCameraRequestedPosition - mAvatarPosition;
		F32 camera_distance = (F32)camera_offset.magVec();
		if(camera_distance > max_dist)
		{
			tethered = mAvatarPosition + 
				(max_dist / camera_distance) * camera_offset;
		}
	}
	
	if(dist_vec(mCameraPosition, tethered) > 0.1)
	{
		mCameraPosition = tethered;
		mSpatialCoordsDirty = true;
	}
}

void LLVoiceClient::setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot)
{
	mCameraRequestedPosition = position;
	
	if(mCameraVelocity != velocity)
	{
		mCameraVelocity = velocity;
		mSpatialCoordsDirty = true;
	}
	
	if(mCameraRot != rot)
	{
		mCameraRot = rot;
		mSpatialCoordsDirty = true;
	}
}

void LLVoiceClient::setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot)
{
	if(dist_vec(mAvatarPosition, position) > 0.1)
	{
		mAvatarPosition = position;
		mSpatialCoordsDirty = true;
	}
	
	if(mAvatarVelocity != velocity)
	{
		mAvatarVelocity = velocity;
		mSpatialCoordsDirty = true;
	}
	
	if(mAvatarRot != rot)
	{
		mAvatarRot = rot;
		mSpatialCoordsDirty = true;
	}
}

bool LLVoiceClient::channelFromRegion(LLViewerRegion *region, std::string &name)
{
	bool result = false;
	
	if(region)
	{
		name = region->getName();
	}
	
	if(!name.empty())
		result = true;
	
	return result;
}

void LLVoiceClient::leaveChannel(void)
{
	if(getState() == stateRunning)
	{
//		llinfos << "leaving channel for teleport/logout" << llendl;
		mChannelName.clear();
		sessionTerminate();
	}
}

void LLVoiceClient::setMuteMic(bool muted)
{
	mMuteMic = muted;
}

void LLVoiceClient::setUserPTTState(bool ptt)
{
	mUserPTTState = ptt;
}

bool LLVoiceClient::getUserPTTState()
{
	return mUserPTTState;
}

void LLVoiceClient::toggleUserPTTState(void)
{
	mUserPTTState = !mUserPTTState;
}

void LLVoiceClient::setVoiceEnabled(bool enabled)
{
	if (enabled != mVoiceEnabled)
	{
		mVoiceEnabled = enabled;
		if (enabled)
		{
			LLVoiceChannel::getCurrentVoiceChannel()->activate();
		}
		else
		{
			// for now, leave active channel, to auto join when turning voice back on
			//LLVoiceChannel::getCurrentVoiceChannel->deactivate();
		}
	}
}

bool LLVoiceClient::voiceEnabled()
{
	return gSavedSettings.getBOOL("EnableVoiceChat") && !gDisableVoice;
}

void LLVoiceClient::setUsePTT(bool usePTT)
{
	if(usePTT && !mUsePTT)
	{
		// When the user turns on PTT, reset the current state.
		mUserPTTState = false;
	}
	mUsePTT = usePTT;
}

void LLVoiceClient::setPTTIsToggle(bool PTTIsToggle)
{
	if(!PTTIsToggle && mPTTIsToggle)
	{
		// When the user turns off toggle, reset the current state.
		mUserPTTState = false;
	}
	
	mPTTIsToggle = PTTIsToggle;
}


void LLVoiceClient::setPTTKey(std::string &key)
{
	if(key == "MiddleMouse")
	{
		mPTTIsMiddleMouse = true;
	}
	else
	{
		mPTTIsMiddleMouse = false;
		if(!LLKeyboard::keyFromString(key, &mPTTKey))
		{
			// If the call failed, don't match any key.
			key = KEY_NONE;
		}
	}
}

void LLVoiceClient::setEarLocation(S32 loc)
{
	if(mEarLocation != loc)
	{
		llinfos << "Setting mEarLocation to " << loc << llendl;
		
		mEarLocation = loc;
		mSpatialCoordsDirty = true;
	}
}

void LLVoiceClient::setVoiceVolume(F32 volume)
{
//	llinfos << "volume is " << volume << llendl;

	// incoming volume has the range [0.0 ... 1.0], with 0.5 as the default.
	// Map it as follows: 0.0 -> -100, 0.5 -> 24, 1.0 -> 50
	
	volume -= 0.5f;		// offset volume to the range [-0.5 ... 0.5], with 0 at the default.
	int scaledVolume = 24;	// offset scaledVolume by its default level
	if(volume < 0.0f)
		scaledVolume += ((int)(volume * 248.0f));	// (24 - (-100)) * 2
	else
		scaledVolume += ((int)(volume * 52.0f));	// (50 - 24) * 2
	
	if(scaledVolume != mSpeakerVolume)
	{
		if((scaledVolume == -100) || (mSpeakerVolume == -100))
		{
			mSpeakerMuteDirty = true;
		}

		mSpeakerVolume = scaledVolume;
		mSpeakerVolumeDirty = true;
	}
}

void LLVoiceClient::setMicGain(F32 volume)
{
	int scaledVolume = ((int)(volume * 100.0f)) - 100;
	if(scaledVolume != mMicVolume)
	{
		mMicVolume = scaledVolume;
		mMicVolumeDirty = true;
	}
}

void LLVoiceClient::setVivoxDebugServerName(std::string &serverName)
{
	if(!mAccountServerName.empty())
	{
		// The name has been filled in already, which means we know whether we're connecting to agni or not.
		if(!sConnectingToAgni)
		{
			// Only use the setting if we're connecting to a development grid -- always use bhr when on agni.
			mAccountServerName = serverName;
		}
	}
}

void LLVoiceClient::keyDown(KEY key, MASK mask)
{	
//	llinfos << "key is " << LLKeyboard::stringFromKey(key) << llendl;

	if (gKeyboard->getKeyRepeated(key))
	{
		// ignore auto-repeat keys
		return;
	}

	if(!mPTTIsMiddleMouse)
	{
		if(mPTTIsToggle)
		{
			if(key == mPTTKey)
			{
				toggleUserPTTState();
			}
		}
		else if(mPTTKey != KEY_NONE)
		{
			setUserPTTState(gKeyboard->getKeyDown(mPTTKey));
		}
	}
}
void LLVoiceClient::keyUp(KEY key, MASK mask)
{
	if(!mPTTIsMiddleMouse)
	{
		if(!mPTTIsToggle && (mPTTKey != KEY_NONE))
		{
			setUserPTTState(gKeyboard->getKeyDown(mPTTKey));
		}
	}
}
void LLVoiceClient::middleMouseState(bool down)
{
	if(mPTTIsMiddleMouse)
	{
		if(mPTTIsToggle)
		{
			if(down)
			{
				toggleUserPTTState();
			}
		}
		else
		{
			setUserPTTState(down);
		}
	}
}

/////////////////////////////
// Accessors for data related to nearby speakers
BOOL LLVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	BOOL result = FALSE;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// For now, if we have any data about the user that came through the chat channel, assume they're voice-enabled.
		result = TRUE;
	}
	
	return result;
}

BOOL LLVoiceClient::getIsSpeaking(const LLUUID& id)
{
	BOOL result = FALSE;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		if (participant->mSpeakingTimeout.getElapsedTimeF32() > SPEAKING_TIMEOUT)
		{
			participant->mIsSpeaking = FALSE;
		}
		result = participant->mIsSpeaking;
	}
	
	return result;
}

BOOL LLVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	BOOL result = FALSE;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mIsModeratorMuted;
	}
	
	return result;
}

F32 LLVoiceClient::getCurrentPower(const LLUUID& id)
{		
	F32 result = 0;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mPower;
	}
	
	return result;
}


LLString LLVoiceClient::getDisplayName(const LLUUID& id)
{
	LLString result;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mDisplayName;
	}
	
	return result;
}


BOOL LLVoiceClient::getUsingPTT(const LLUUID& id)
{
	BOOL result = FALSE;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// Does "using PTT" mean they're configured with a push-to-talk button?
		// For now, we know there's no PTT mechanism in place, so nobody is using it.
	}
	
	return result;
}

BOOL LLVoiceClient::getPTTPressed(const LLUUID& id)
{
	BOOL result = FALSE;
	
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mPTT;
	}
	
	return result;
}

BOOL LLVoiceClient::getOnMuteList(const LLUUID& id)
{
	BOOL result = FALSE;
	
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mOnMuteList;
	}

	return result;
}

// External accessiors. Maps 0.0 to 1.0 to internal values 0-400 with .5 == 100
// internal = 400 * external^2
F32 LLVoiceClient::getUserVolume(const LLUUID& id)
{
	F32 result = 0.0f;
	
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		S32 ires = participant->mUserVolume; // 0-400
		result = sqrtf(((F32)ires) / 400.f);
	}

	return result;
}

void LLVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
	participantState *participant = findParticipantByID(id);
	if (participant)
	{
		// volume can amplify by as much as 4x!
		S32 ivol = (S32)(400.f * volume * volume);
		participant->mUserVolume = llclamp(ivol, 0, 400);
		participant->mVolumeDirty = TRUE;
		mVolumeDirty = TRUE;
	}
}



LLVoiceClient::serviceType LLVoiceClient::getServiceType(const LLUUID& id)
{
	serviceType result = serviceTypeUnknown;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mServiceType;
	}
	
	return result;
}

std::string LLVoiceClient::getGroupID(const LLUUID& id)
{
	std::string result;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mGroupID;
	}
	
	return result;
}

BOOL LLVoiceClient::getAreaVoiceDisabled()
{
	return mAreaVoiceDisabled;
}

void LLVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
	mObservers.insert(observer);
}

void LLVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
	mObservers.erase(observer);
}

void LLVoiceClient::notifyObservers()
{
	for (observer_set_t::iterator it = mObservers.begin();
		it != mObservers.end();
		)
	{
		LLVoiceClientParticipantObserver* observer = *it;
		observer->onChange();
		// In case onChange() deleted an entry.
		it = mObservers.upper_bound(observer);
	}
}

void LLVoiceClient::addStatusObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.insert(observer);
}

void LLVoiceClient::removeStatusObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.erase(observer);
}

void LLVoiceClient::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
{
	if(status == LLVoiceClientStatusObserver::ERROR_UNKNOWN)
	{
		switch(mVivoxErrorStatusCode)
		{
		case 20713:		status = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL; 		break;
		case 20714:		status = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED; 	break;
		case 20715:
			//invalid channel, we may be using a set of poorly cached
			//info
			status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
			break;
		case 1009:
			//invalid username and password
			status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
			break;
		}

		// Reset the error code to make sure it won't be reused later by accident.
		mVivoxErrorStatusCode = 0;
	}
	
	if (status == LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL 
		//NOT_FOUND || TEMPORARILY_UNAVAILABLE || REQUEST_TIMEOUT
		&& (mVivoxErrorStatusCode == 404 || mVivoxErrorStatusCode == 480 || mVivoxErrorStatusCode == 408)) 
	{
		// call failed because other user was not available
		// treat this as an error case
		status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;

		// Reset the error code to make sure it won't be reused later by accident.
		mVivoxErrorStatusCode = 0;
	}
	
	llinfos << " " << LLVoiceClientStatusObserver::status2string(status)  << ", session URI " << mSessionURI << llendl;

	for (status_observer_set_t::iterator it = mStatusObservers.begin();
		it != mStatusObservers.end();
		)
	{
		LLVoiceClientStatusObserver* observer = *it;
		observer->onChange(status, mSessionURI, !mNonSpatialChannel);
		// In case onError() deleted an entry.
		it = mStatusObservers.upper_bound(observer);
	}

}

//static
void LLVoiceClient::onAvatarNameLookup(const LLUUID& id, const char* first, const char* last, BOOL is_group, void* user_data)
{
	participantState* statep = gVoiceClient->findParticipantByID(id);

	if (statep)
	{
		statep->mDisplayName = llformat("%s %s", first, last);
	}
	
	gVoiceClient->notifyObservers();
}

class LLViewerParcelVoiceInfo : public LLHTTPNode
{
	virtual void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		//the parcel you are in has changed something about its
		//voice information

		if ( input.has("body") )
		{
			LLSD body = input["body"];

			//body has "region_name" (str), "parcel_local_id"(int),
			//"voice_credentials" (map).

			//body["voice_credentials"] has "channel_uri" (str),
			//body["voice_credentials"] has "channel_credentials" (str)
			if ( body.has("voice_credentials") )
			{
				LLSD voice_credentials = body["voice_credentials"];
				std::string uri;
				std::string credentials;

				if ( voice_credentials.has("channel_uri") )
				{
					uri = voice_credentials["channel_uri"].asString();
				}
				if ( voice_credentials.has("channel_credentials") )
				{
					credentials =
						voice_credentials["channel_credentials"].asString();
				}

				gVoiceClient->setSpatialChannel(uri, credentials);
			}
		}
	}
};

class LLViewerRequiredVoiceVersion : public LLHTTPNode
{
	static BOOL sAlertedUser;
	virtual void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		//You received this messsage (most likely on region cross or
		//teleport)
		if ( input.has("body") && input["body"].has("major_version") )
		{
			int major_voice_version =
				input["body"]["major_version"].asInteger();
// 			int minor_voice_version =
// 				input["body"]["minor_version"].asInteger();

			if (gVoiceClient &&
				(major_voice_version > VOICE_MAJOR_VERSION) )
			{
				if (!sAlertedUser)
				{
					//sAlertedUser = TRUE;
					gViewerWindow->alertXml("VoiceVersionMismatch");
					gSavedSettings.setBOOL("EnableVoiceChat", FALSE); // toggles listener
				}
			}
		}
	}
};
BOOL LLViewerRequiredVoiceVersion::sAlertedUser = FALSE;

LLHTTPRegistration<LLViewerParcelVoiceInfo>
    gHTTPRegistrationMessageParcelVoiceInfo(
		"/message/ParcelVoiceInfo");

LLHTTPRegistration<LLViewerRequiredVoiceVersion>
    gHTTPRegistrationMessageRequiredVoiceVersion(
		"/message/RequiredVoiceVersion");
