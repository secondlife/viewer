 /** 
 * @file llvoiceclient.cpp
 * @brief Implementation of LLVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

// library includes
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llsdutil.h"


// project includes
#include "llvoavatar.h"
#include "llbufferstream.h"
#include "llfile.h"
#ifdef LL_STANDALONE
# include "expat.h"
#else
# include "expat/expat.h"
#endif
#include "llcallbacklist.h"
#include "llcallingcard.h"   // for LLFriendObserver
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
#include "llparcel.h"
#include "llviewerparcelmgr.h"
//#include "llfirstuse.h"
#include "llspeakers.h"
#include "lltrans.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llvoavatarself.h"
#include "llvoicechannel.h"

// for base64 decoding
#include "apr_base64.h"

// for SHA1 hash
#include "apr_sha1.h"

// for MD5 hash
#include "llmd5.h"

#define USE_SESSION_GROUPS 0

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

static void setUUIDFromStringHash(LLUUID &uuid, const std::string &str)
{
	LLMD5 md5_uuid;
	md5_uuid.update((const unsigned char*)str.data(), str.size());
	md5_uuid.finalize();
	md5_uuid.raw_digest(uuid.mData);
}

static int scale_mic_volume(float volume)
{
	// incoming volume has the range [0.0 ... 2.0], with 1.0 as the default.
	// Map it to Vivox levels as follows: 0.0 -> 30, 1.0 -> 50, 2.0 -> 70
	return 30 + (int)(volume * 20.0f);
}

static int scale_speaker_volume(float volume)
{
	// incoming volume has the range [0.0 ... 1.0], with 0.5 as the default.
	// Map it to Vivox levels as follows: 0.0 -> 30, 0.5 -> 50, 1.0 -> 70
	return 30 + (int)(volume * 40.0f);
}

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
			LL_WARNS("Voice") << "ProvisionVoiceAccountRequest returned an error, retrying.  status = " << status << ", reason = \"" << reason << "\"" << LL_ENDL;
			if ( gVoiceClient ) gVoiceClient->requestVoiceAccountProvision(
				mRetries - 1);
		}
		else
		{
			LL_WARNS("Voice") << "ProvisionVoiceAccountRequest returned an error, too many retries (giving up).  status = " << status << ", reason = \"" << reason << "\"" << LL_ENDL;
			if ( gVoiceClient ) gVoiceClient->giveUp();
		}
	}

	virtual void result(const LLSD& content)
	{
		if ( gVoiceClient )
		{
			std::string voice_sip_uri_hostname;
			std::string voice_account_server_uri;
			
			LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response:" << ll_pretty_print_sd(content) << LL_ENDL;
			
			if(content.has("voice_sip_uri_hostname"))
				voice_sip_uri_hostname = content["voice_sip_uri_hostname"].asString();
			
			// this key is actually misnamed -- it will be an entire URI, not just a hostname.
			if(content.has("voice_account_server_name"))
				voice_account_server_uri = content["voice_account_server_name"].asString();
			
			gVoiceClient->login(
				content["username"].asString(),
				content["password"].asString(),
				voice_sip_uri_hostname,
				voice_account_server_uri);
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
	bool			squelchDebugOutput;
	int				returnCode;
	int				statusCode;
	std::string		statusString;
	std::string		requestId;
	std::string		actionString;
	std::string		connectorHandle;
	std::string		versionID;
	std::string		accountHandle;
	std::string		sessionHandle;
	std::string		sessionGroupHandle;
	std::string		alias;
	std::string		applicationString;

	// Members for processing events. The values are transient and only valid within a call to processResponse().
	std::string		eventTypeString;
	int				state;
	std::string		uriString;
	bool			isChannel;
	bool			incoming;
	bool			enabled;
	std::string		nameString;
	std::string		audioMediaString;
	std::string		displayNameString;
	std::string		deviceString;
	int				participantType;
	bool			isLocallyMuted;
	bool			isModeratorMuted;
	bool			isSpeaking;
	int				volume;
	F32				energy;
	std::string		messageHeader;
	std::string		messageBody;
	std::string		notificationType;
	bool			hasText;
	bool			hasAudio;
	bool			hasVideo;
	bool			terminated;
	std::string		blockMask;
	std::string		presenceOnly;
	std::string		autoAcceptMask;
	std::string		autoAddAsBuddy;
	int				numberOfAliases;
	std::string		subscriptionHandle;
	std::string		subscriptionType;
		

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
	energy = 0.f;
	hasText = false;
	hasAudio = false;
	hasVideo = false;
	terminated = false;
	ignoreDepth = 0;
	isChannel = false;
	incoming = false;
	enabled = false;
	isEvent = false;
	isLocallyMuted = false;
	isModeratorMuted = false;
	isSpeaking = false;
	participantType = 0;
	squelchDebugOutput = false;
	returnCode = -1;
	state = 0;
	statusCode = 0;
	volume = 0;
	textBuffer.clear();
	alias.clear();
	numberOfAliases = 0;
	applicationString.clear();
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
	
	// Look for input delimiter(s) in the input buffer.  If one is found, send the message to the xml parser.
	int start = 0;
	int delim;
	while((delim = mInput.find("\n\n\n", start)) != std::string::npos)
	{	
		
		// Reset internal state of the LLVivoxProtocolParser (no effect on the expat parser)
		reset();
		
		XML_ParserReset(parser, NULL);
		XML_SetElementHandler(parser, ExpatStartTag, ExpatEndTag);
		XML_SetCharacterDataHandler(parser, ExpatCharHandler);
		XML_SetUserData(parser, this);	
		XML_Parse(parser, mInput.data() + start, delim - start, false);
		
		// If this message isn't set to be squelched, output the raw XML received.
		if(!squelchDebugOutput)
		{
			LL_DEBUGS("Voice") << "parsing: " << mInput.substr(start, delim - start) << LL_ENDL;
		}
		
		start = delim + 3;
	}
	
	if(start != 0)
		mInput = mInput.substr(start);

	LL_DEBUGS("VivoxProtocolParser") << "at end, mInput is: " << mInput << LL_ENDL;
	
	if(!gVoiceClient->mConnected)
	{
		// If voice has been disabled, we just want to close the socket.  This does so.
		LL_INFOS("Voice") << "returning STATUS_STOP" << LL_ENDL;
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
		isEvent = !stricmp("Event", tag);
		
		if (!stricmp("Response", tag) || isEvent)
		{
			// Grab the attributes
			while (*attr)
			{
				const char	*key = *attr++;
				const char	*value = *attr++;
				
				if (!stricmp("requestId", key))
				{
					requestId = value;
				}
				else if (!stricmp("action", key))
				{
					actionString = value;
				}
				else if (!stricmp("type", key))
				{
					eventTypeString = value;
				}
			}
		}
		LL_DEBUGS("VivoxProtocolParser") << tag << " (" << responseDepth << ")"  << LL_ENDL;
	}
	else
	{
		if (ignoringTags)
		{
			LL_DEBUGS("VivoxProtocolParser") << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("VivoxProtocolParser") << tag << " (" << responseDepth << ")"  << LL_ENDL;
	
			// Ignore the InputXml stuff so we don't get confused
			if (!stricmp("InputXml", tag))
			{
				ignoringTags = true;
				ignoreDepth = responseDepth;
				accumulateText = false;

				LL_DEBUGS("VivoxProtocolParser") << "starting ignore, ignoreDepth is " << ignoreDepth << LL_ENDL;
			}
			else if (!stricmp("CaptureDevices", tag))
			{
				gVoiceClient->clearCaptureDevices();
			}
			else if (!stricmp("RenderDevices", tag))
			{
				gVoiceClient->clearRenderDevices();
			}
			else if (!stricmp("CaptureDevice", tag))
			{
				deviceString.clear();
			}
			else if (!stricmp("RenderDevice", tag))
			{
				deviceString.clear();
			}
			else if (!stricmp("Buddies", tag))
			{
				gVoiceClient->deleteAllBuddies();
			}
			else if (!stricmp("BlockRules", tag))
			{
				gVoiceClient->deleteAllBlockRules();
			}
			else if (!stricmp("AutoAcceptRules", tag))
			{
				gVoiceClient->deleteAllAutoAcceptRules();
			}
			
		}
	}
	responseDepth++;
}

// --------------------------------------------------------------------------------

void LLVivoxProtocolParser::EndTag(const char *tag)
{
	const std::string& string = textBuffer;

	responseDepth--;

	if (ignoringTags)
	{
		if (ignoreDepth == responseDepth)
		{
			LL_DEBUGS("VivoxProtocolParser") << "end of ignore" << LL_ENDL;
			ignoringTags = false;
		}
		else
		{
			LL_DEBUGS("VivoxProtocolParser") << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		}
	}
	
	if (!ignoringTags)
	{
		LL_DEBUGS("VivoxProtocolParser") << "processing tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;

		// Closing a tag. Finalize the text we've accumulated and reset
		if (!stricmp("ReturnCode", tag))
			returnCode = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("SessionHandle", tag))
			sessionHandle = string;
		else if (!stricmp("SessionGroupHandle", tag))
			sessionGroupHandle = string;
		else if (!stricmp("StatusCode", tag))
			statusCode = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("StatusString", tag))
			statusString = string;
		else if (!stricmp("ParticipantURI", tag))
			uriString = string;
		else if (!stricmp("Volume", tag))
			volume = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("Energy", tag))
			energy = (F32)strtod(string.c_str(), NULL);
		else if (!stricmp("IsModeratorMuted", tag))
			isModeratorMuted = !stricmp(string.c_str(), "true");
		else if (!stricmp("IsSpeaking", tag))
			isSpeaking = !stricmp(string.c_str(), "true");
		else if (!stricmp("Alias", tag))
			alias = string;
		else if (!stricmp("NumberOfAliases", tag))
			numberOfAliases = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("Application", tag))
			applicationString = string;
		else if (!stricmp("ConnectorHandle", tag))
			connectorHandle = string;
		else if (!stricmp("VersionID", tag))
			versionID = string;
		else if (!stricmp("AccountHandle", tag))
			accountHandle = string;
		else if (!stricmp("State", tag))
			state = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("URI", tag))
			uriString = string;
		else if (!stricmp("IsChannel", tag))
			isChannel = !stricmp(string.c_str(), "true");
		else if (!stricmp("Incoming", tag))
			incoming = !stricmp(string.c_str(), "true");
		else if (!stricmp("Enabled", tag))
			enabled = !stricmp(string.c_str(), "true");
		else if (!stricmp("Name", tag))
			nameString = string;
		else if (!stricmp("AudioMedia", tag))
			audioMediaString = string;
		else if (!stricmp("ChannelName", tag))
			nameString = string;
		else if (!stricmp("DisplayName", tag))
			displayNameString = string;
		else if (!stricmp("Device", tag))
			deviceString = string;
		else if (!stricmp("AccountName", tag))
			nameString = string;
		else if (!stricmp("ParticipantType", tag))
			participantType = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("IsLocallyMuted", tag))
			isLocallyMuted = !stricmp(string.c_str(), "true");
		else if (!stricmp("MicEnergy", tag))
			energy = (F32)strtod(string.c_str(), NULL);
		else if (!stricmp("ChannelName", tag))
			nameString = string;
		else if (!stricmp("ChannelURI", tag))
			uriString = string;
		else if (!stricmp("BuddyURI", tag))
			uriString = string;
		else if (!stricmp("Presence", tag))
			statusString = string;
		else if (!stricmp("CaptureDevice", tag))
		{
			gVoiceClient->addCaptureDevice(deviceString);
		}
		else if (!stricmp("RenderDevice", tag))
		{
			gVoiceClient->addRenderDevice(deviceString);
		}
		else if (!stricmp("Buddy", tag))
		{
			gVoiceClient->processBuddyListEntry(uriString, displayNameString);
		}
		else if (!stricmp("BlockRule", tag))
		{
			gVoiceClient->addBlockRule(blockMask, presenceOnly);
		}
		else if (!stricmp("BlockMask", tag))
			blockMask = string;
		else if (!stricmp("PresenceOnly", tag))
			presenceOnly = string;
		else if (!stricmp("AutoAcceptRule", tag))
		{
			gVoiceClient->addAutoAcceptRule(autoAcceptMask, autoAddAsBuddy);
		}
		else if (!stricmp("AutoAcceptMask", tag))
			autoAcceptMask = string;
		else if (!stricmp("AutoAddAsBuddy", tag))
			autoAddAsBuddy = string;
		else if (!stricmp("MessageHeader", tag))
			messageHeader = string;
		else if (!stricmp("MessageBody", tag))
			messageBody = string;
		else if (!stricmp("NotificationType", tag))
			notificationType = string;
		else if (!stricmp("HasText", tag))
			hasText = !stricmp(string.c_str(), "true");
		else if (!stricmp("HasAudio", tag))
			hasAudio = !stricmp(string.c_str(), "true");
		else if (!stricmp("HasVideo", tag))
			hasVideo = !stricmp(string.c_str(), "true");
		else if (!stricmp("Terminated", tag))
			terminated = !stricmp(string.c_str(), "true");
		else if (!stricmp("SubscriptionHandle", tag))
			subscriptionHandle = string;
		else if (!stricmp("SubscriptionType", tag))
			subscriptionType = string;
		
		textBuffer.clear();
		accumulateText= false;
		
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
	LL_DEBUGS("VivoxProtocolParser") << tag << LL_ENDL;

	// SLIM SDK: the SDK now returns a statusCode of "200" (OK) for success.  This is a change vs. previous SDKs.
	// According to Mike S., "The actual API convention is that responses with return codes of 0 are successful, regardless of the status code returned",
	// so I believe this will give correct behavior.
	
	if(returnCode == 0)
		statusCode = 0;
		
	if (isEvent)
	{
		const char *eventTypeCstr = eventTypeString.c_str();
		if (!stricmp(eventTypeCstr, "AccountLoginStateChangeEvent"))
		{
			gVoiceClient->accountLoginStateChangeEvent(accountHandle, statusCode, statusString, state);
		}
		else if (!stricmp(eventTypeCstr, "SessionAddedEvent"))
		{
			/*
			<Event type="SessionAddedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
				<SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
				<Uri>sip:confctl-1408789@bhr.vivox.com</Uri>
				<IsChannel>true</IsChannel>
				<Incoming>false</Incoming>
				<ChannelName />
			</Event>
			*/
			gVoiceClient->sessionAddedEvent(uriString, alias, sessionHandle, sessionGroupHandle, isChannel, incoming, nameString, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "SessionRemovedEvent"))
		{
			gVoiceClient->sessionRemovedEvent(sessionHandle, sessionGroupHandle);
		}
		else if (!stricmp(eventTypeCstr, "SessionGroupAddedEvent"))
		{
			gVoiceClient->sessionGroupAddedEvent(sessionGroupHandle);
		}
		else if (!stricmp(eventTypeCstr, "MediaStreamUpdatedEvent"))
		{
			/*
			<Event type="MediaStreamUpdatedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
				<SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
				<StatusCode>200</StatusCode>
				<StatusString>OK</StatusString>
				<State>2</State>
				<Incoming>false</Incoming>
			</Event>
			*/
			gVoiceClient->mediaStreamUpdatedEvent(sessionHandle, sessionGroupHandle, statusCode, statusString, state, incoming);
		}		
		else if (!stricmp(eventTypeCstr, "TextStreamUpdatedEvent"))
		{
			/*
			<Event type="TextStreamUpdatedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg1</SessionGroupHandle>
				<SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==1</SessionHandle>
				<Enabled>true</Enabled>
				<State>1</State>
				<Incoming>true</Incoming>
			</Event>
			*/
			gVoiceClient->textStreamUpdatedEvent(sessionHandle, sessionGroupHandle, enabled, state, incoming);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantAddedEvent"))
		{
			/* 
			<Event type="ParticipantAddedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg4</SessionGroupHandle>
				<SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==4</SessionHandle>
				<ParticipantUri>sip:xI5auBZ60SJWIk606-1JGRQ==@bhr.vivox.com</ParticipantUri>
				<AccountName>xI5auBZ60SJWIk606-1JGRQ==</AccountName>
				<DisplayName />
				<ParticipantType>0</ParticipantType>
			</Event>
			*/
			gVoiceClient->participantAddedEvent(sessionHandle, sessionGroupHandle, uriString, alias, nameString, displayNameString, participantType);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantRemovedEvent"))
		{
			/*
			<Event type="ParticipantRemovedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg4</SessionGroupHandle>
				<SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==4</SessionHandle>
				<ParticipantUri>sip:xtx7YNV-3SGiG7rA1fo5Ndw==@bhr.vivox.com</ParticipantUri>
				<AccountName>xtx7YNV-3SGiG7rA1fo5Ndw==</AccountName>
			</Event>
			*/
			gVoiceClient->participantRemovedEvent(sessionHandle, sessionGroupHandle, uriString, alias, nameString);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantUpdatedEvent"))
		{
			/*
			<Event type="ParticipantUpdatedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
				<SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
				<ParticipantUri>sip:xFnPP04IpREWNkuw1cOXlhw==@bhr.vivox.com</ParticipantUri>
				<IsModeratorMuted>false</IsModeratorMuted>
				<IsSpeaking>true</IsSpeaking>
				<Volume>44</Volume>
				<Energy>0.0879437</Energy>
			</Event>
			*/
			
			// These happen so often that logging them is pretty useless.
			squelchDebugOutput = true;
			
			gVoiceClient->participantUpdatedEvent(sessionHandle, sessionGroupHandle, uriString, alias, isModeratorMuted, isSpeaking, volume, energy);
		}
		else if (!stricmp(eventTypeCstr, "AuxAudioPropertiesEvent"))
		{
			gVoiceClient->auxAudioPropertiesEvent(energy);
		}
		else if (!stricmp(eventTypeCstr, "BuddyPresenceEvent"))
		{
			gVoiceClient->buddyPresenceEvent(uriString, alias, statusString, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "BuddyAndGroupListChangedEvent"))
		{
			// The buddy list was updated during parsing.
			// Need to recheck against the friends list.
			gVoiceClient->buddyListChanged();
		}
		else if (!stricmp(eventTypeCstr, "BuddyChangedEvent"))
		{
			/*
			<Event type="BuddyChangedEvent">
				<AccountHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==</AccountHandle>
				<BuddyURI>sip:x9fFHFZjOTN6OESF1DUPrZQ==@bhr.vivox.com</BuddyURI>
				<DisplayName>Monroe Tester</DisplayName>
				<BuddyData />
				<GroupID>0</GroupID>
				<ChangeType>Set</ChangeType>
			</Event>
			*/		
			// TODO: Question: Do we need to process this at all?
		}
		else if (!stricmp(eventTypeCstr, "MessageEvent"))  
		{
			gVoiceClient->messageEvent(sessionHandle, uriString, alias, messageHeader, messageBody, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "SessionNotificationEvent"))  
		{
			gVoiceClient->sessionNotificationEvent(sessionHandle, uriString, notificationType);
		}
		else if (!stricmp(eventTypeCstr, "SubscriptionEvent"))  
		{
			gVoiceClient->subscriptionEvent(uriString, subscriptionHandle, alias, displayNameString, applicationString, subscriptionType);
		}
		else if (!stricmp(eventTypeCstr, "SessionUpdatedEvent"))  
		{
			/*
			<Event type="SessionUpdatedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
				<SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
				<Uri>sip:confctl-9@bhd.vivox.com</Uri>
				<IsMuted>0</IsMuted>
				<Volume>50</Volume>
				<TransmitEnabled>1</TransmitEnabled>
				<IsFocused>0</IsFocused>
				<SpeakerPosition><Position><X>0</X><Y>0</Y><Z>0</Z></Position></SpeakerPosition>
				<SessionFontID>0</SessionFontID>
			</Event>
			*/
			// We don't need to process this, but we also shouldn't warn on it, since that confuses people.
		}
		
		else if (!stricmp(eventTypeCstr, "SessionGroupRemovedEvent"))  
		{
			/*
			<Event type="SessionGroupRemovedEvent">
				<SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
			</Event>
			*/
			// We don't need to process this, but we also shouldn't warn on it, since that confuses people.
		}
		else
		{
			LL_WARNS("VivoxProtocolParser") << "Unknown event type " << eventTypeString << LL_ENDL;
		}
	}
	else
	{
		const char *actionCstr = actionString.c_str();
		if (!stricmp(actionCstr, "Connector.Create.1"))
		{
			gVoiceClient->connectorCreateResponse(statusCode, statusString, connectorHandle, versionID);
		}
		else if (!stricmp(actionCstr, "Account.Login.1"))
		{
			gVoiceClient->loginResponse(statusCode, statusString, accountHandle, numberOfAliases);
		}
		else if (!stricmp(actionCstr, "Session.Create.1"))
		{
			gVoiceClient->sessionCreateResponse(requestId, statusCode, statusString, sessionHandle);			
		}
		else if (!stricmp(actionCstr, "SessionGroup.AddSession.1"))
		{
			gVoiceClient->sessionGroupAddSessionResponse(requestId, statusCode, statusString, sessionHandle);			
		}
		else if (!stricmp(actionCstr, "Session.Connect.1"))
		{
			gVoiceClient->sessionConnectResponse(requestId, statusCode, statusString);			
		}
		else if (!stricmp(actionCstr, "Account.Logout.1"))
		{
			gVoiceClient->logoutResponse(statusCode, statusString);			
		}
		else if (!stricmp(actionCstr, "Connector.InitiateShutdown.1"))
		{
			gVoiceClient->connectorShutdownResponse(statusCode, statusString);			
		}
		else if (!stricmp(actionCstr, "Account.ListBlockRules.1"))
		{
			gVoiceClient->accountListBlockRulesResponse(statusCode, statusString);						
		}
		else if (!stricmp(actionCstr, "Account.ListAutoAcceptRules.1"))
		{
			gVoiceClient->accountListAutoAcceptRulesResponse(statusCode, statusString);						
		}
		else if (!stricmp(actionCstr, "Session.Set3DPosition.1"))
		{
			// We don't need to process these, but they're so spammy we don't want to log them.
			squelchDebugOutput = true;
		}
/*
		else if (!stricmp(actionCstr, "Account.ChannelGetList.1"))
		{
			gVoiceClient->channelGetListResponse(statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Connector.AccountCreate.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Connector.MuteLocalMic.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Connector.MuteLocalSpeaker.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Connector.SetLocalMicVolume.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Connector.SetLocalSpeakerVolume.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Session.ListenerSetPosition.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Session.SpeakerSetPosition.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Session.AudioSourceSetPosition.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Session.GetChannelParticipants.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelCreate.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelUpdate.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelDelete.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelCreateAndInvite.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelFolderCreate.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelFolderUpdate.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelFolderDelete.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelAddModerator.1"))
		{
			
		}
		else if (!stricmp(actionCstr, "Account.ChannelDeleteModerator.1"))
		{
			
		}
*/
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

class LLVoiceClientMuteListObserver : public LLMuteListObserver
{
	/* virtual */ void onChange()  { gVoiceClient->muteListChanged();}
};

class LLVoiceClientFriendsObserver : public LLFriendObserver
{
public:
	/* virtual */ void changed(U32 mask) { gVoiceClient->updateFriends(mask);}
};

static LLVoiceClientMuteListObserver mutelist_listener;
static bool sMuteListListener_listening = false;

static LLVoiceClientFriendsObserver *friendslist_listener = NULL;

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
	LL_WARNS("Voice") << "LLVoiceClientCapResponder::error("
		<< status << ": " << reason << ")"
		<< LL_ENDL;
}

void LLVoiceClientCapResponder::result(const LLSD& content)
{
	LLSD::map_const_iterator iter;
	
	LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest response:" << ll_pretty_print_sd(content) << LL_ENDL;

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

class LLSpeakerVolumeStorage : public LLSingleton<LLSpeakerVolumeStorage>
{
	LOG_CLASS(LLSpeakerVolumeStorage);
public:

	/**
	 * Sets internal voluem level for specified user.
	 *
	 * @param[in] speaker_id - LLUUID of user to store volume level for
	 * @param[in] volume - internal volume level to be stored for user.
	 */
	void storeSpeakerVolume(const LLUUID& speaker_id, S32 volume);

	/**
	 * Gets stored internal volume level for specified speaker.
	 *
	 * If specified user is not found default level will be returned. It is equivalent of 
	 * external level 0.5 from the 0.0..1.0 range.
	 * Default internal level is calculated as: internal = 400 * external^2
	 * Maps 0.0 to 1.0 to internal values 0-400 with default 0.5 == 100
	 *
	 * @param[in] speaker_id - LLUUID of user to get his volume level
	 */
	S32 getSpeakerVolume(const LLUUID& speaker_id);

private:
	friend class LLSingleton<LLSpeakerVolumeStorage>;
	LLSpeakerVolumeStorage();
	~LLSpeakerVolumeStorage();

	const static std::string SETTINGS_FILE_NAME;

	void load();
	void save();

	typedef std::map<LLUUID, S32> speaker_data_map_t;
	speaker_data_map_t mSpeakersData;
};

const std::string LLSpeakerVolumeStorage::SETTINGS_FILE_NAME = "volume_settings.xml";

LLSpeakerVolumeStorage::LLSpeakerVolumeStorage()
{
	load();
}

LLSpeakerVolumeStorage::~LLSpeakerVolumeStorage()
{
	save();
}

void LLSpeakerVolumeStorage::storeSpeakerVolume(const LLUUID& speaker_id, S32 volume)
{
	mSpeakersData[speaker_id] = volume;
}

S32 LLSpeakerVolumeStorage::getSpeakerVolume(const LLUUID& speaker_id)
{
	// default internal level of user voice.
	const static LLUICachedControl<S32> DEFAULT_INTERNAL_VOLUME_LEVEL("VoiceDefaultInternalLevel", 100);
	S32 ret_val = DEFAULT_INTERNAL_VOLUME_LEVEL;
	speaker_data_map_t::const_iterator it = mSpeakersData.find(speaker_id);
	
	if (it != mSpeakersData.end())
	{
		ret_val = it->second;
	}
	return ret_val;
}

void LLSpeakerVolumeStorage::load()
{
	// load per-resident voice volume information
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);

	LLSD settings_llsd;
	llifstream file;
	file.open(filename);
	if (file.is_open())
	{
		LLSDSerialize::fromXML(settings_llsd, file);
	}

	for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
		iter != settings_llsd.endMap(); ++iter)
	{
		mSpeakersData.insert(std::make_pair(LLUUID(iter->first), (S32)iter->second.asInteger()));
	}
}

void LLSpeakerVolumeStorage::save()
{
	// If we quit from the login screen we will not have an SL account
	// name.  Don't try to save, otherwise we'll dump a file in
	// C:\Program Files\SecondLife\ or similar. JC
	std::string user_dir = gDirUtilp->getLindenUserDir();
	if (!user_dir.empty())
	{
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);
		LLSD settings_llsd;

		for(speaker_data_map_t::const_iterator iter = mSpeakersData.begin(); iter != mSpeakersData.end(); ++iter)
		{
			settings_llsd[iter->first.asString()] = iter->second;
		}

		llofstream file;
		file.open(filename);
		LLSDSerialize::toPrettyXML(settings_llsd, file);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////

LLVoiceClient::LLVoiceClient() :
	mState(stateDisabled),
	mSessionTerminateRequested(false),
	mRelogRequested(false),
	mConnected(false),
	mPump(NULL),
	
	mTuningMode(false),
	mTuningEnergy(0.0f),
	mTuningMicVolume(0),
	mTuningMicVolumeDirty(true),
	mTuningSpeakerVolume(0),
	mTuningSpeakerVolumeDirty(true),
	mTuningExitState(stateDisabled),
	
	mAreaVoiceDisabled(false),
	mAudioSession(NULL),
	mAudioSessionChanged(false),
	mNextAudioSession(NULL),
	
	mCurrentParcelLocalID(0),
	mNumberOfAliases(0),
	mCommandCookie(0),
	mLoginRetryCount(0),
	
	mBuddyListMapPopulated(false),
	mBlockRulesListReceived(false),
	mAutoAcceptRulesListReceived(false),
	mCaptureDeviceDirty(false),
	mRenderDeviceDirty(false),
	mSpatialCoordsDirty(false),

	mPTTDirty(true),
	mPTT(true),
	mUsePTT(true),
	mPTTIsMiddleMouse(false),
	mPTTKey(0),
	mPTTIsToggle(false),
	mUserPTTState(false),
	mMuteMic(false),
	mFriendsListDirty(true),
	
	mEarLocation(0),
	mSpeakerVolumeDirty(true),
	mSpeakerMuteDirty(true),
	mMicVolume(0),
	mMicVolumeDirty(true),
	
	mVoiceEnabled(false),
	mWriteInProgress(false),
	
	mLipSyncEnabled(false)
{	
	gVoiceClient = this;
	
	mAPIVersion = LLTrans::getString("NotConnected");

	mSpeakerVolume = scale_speaker_volume(0);
	
#if LL_DARWIN || LL_LINUX || LL_SOLARIS
		// HACK: THIS DOES NOT BELONG HERE
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
	LLVoiceClient::getInstance()->updateSettings();
}

void LLVoiceClient::terminate()
{
	if(gVoiceClient)
	{
//		gVoiceClient->leaveAudioSession();
		gVoiceClient->logout();
		// As of SDK version 4885, this should no longer be necessary.  It will linger after the socket close if it needs to.
		// ms_sleep(2000);
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

//---------------------------------------------------

void LLVoiceClient::updateSettings()
{
	setVoiceEnabled(gSavedSettings.getBOOL("EnableVoiceChat"));
	setUsePTT(gSavedSettings.getBOOL("PTTCurrentlyEnabled"));
	std::string keyString = gSavedSettings.getString("PushToTalkButton");
	setPTTKey(keyString);
	setPTTIsToggle(gSavedSettings.getBOOL("PushToTalkToggle"));
	setEarLocation(gSavedSettings.getS32("VoiceEarLocation"));

	std::string inputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
	setCaptureDevice(inputDevice);
	std::string outputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
	setRenderDevice(outputDevice);
	F32 mic_level = gSavedSettings.getF32("AudioLevelMic");
	setMicGain(mic_level);
	setLipSyncEnabled(gSavedSettings.getBOOL("LipSyncEnabled"));
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
	
		//MARK: Turn this on to log outgoing XML
//		LL_DEBUGS("Voice") << "sending: " << str << LL_ENDL;

		// check return code - sockets will fail (broken, etc.)
		err = apr_socket_send(
				mSocket->getSocket(),
				(const char*)str.data(),
				&written);
		
		if(err == 0)
		{
			// Success.
			result = true;
		}
		// TODO: handle partial writes (written is number of bytes written)
		// Need to set socket to non-blocking before this will work.
//		else if(APR_STATUS_IS_EAGAIN(err))
//		{
//			// 
//		}
		else
		{
			// Assume any socket error means something bad.  For now, just close the socket.
			char buf[MAX_STRING];
			LL_WARNS("Voice") << "apr error " << err << " ("<< apr_strerror(err, buf, MAX_STRING) << ") sending data to vivox daemon." << LL_ENDL;
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
	std::string logpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
	std::string loglevel = "0";
	
	// Transition to stateConnectorStarted when the connector handle comes back.
	setState(stateConnectorStarting);

	std::string savedLogLevel = gSavedSettings.getString("VivoxDebugLevel");
		
	if(savedLogLevel != "-1")
	{
		LL_DEBUGS("Voice") << "creating connector with logging enabled" << LL_ENDL;
		loglevel = "10";
	}
	
	stream 
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.Create.1\">"
		<< "<ClientName>V2 SDK</ClientName>"
		<< "<AccountManagementServer>" << mVoiceAccountServerURI << "</AccountManagementServer>"
		<< "<Mode>Normal</Mode>"
		<< "<Logging>"
			<< "<Folder>" << logpath << "</Folder>"
			<< "<FileNamePrefix>Connector</FileNamePrefix>"
			<< "<FileNameSuffix>.log</FileNameSuffix>"
			<< "<LogLevel>" << loglevel << "</LogLevel>"
		<< "</Logging>"
		<< "<Application>SecondLifeViewer.1</Application>"
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

	LL_INFOS("Voice") << "name \"" << mAccountDisplayName << "\" , ID " << agentID << LL_ENDL;

	sConnectingToAgni = LLViewerLogin::getInstance()->isInProductionGrid();

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
	const std::string& account_name,
	const std::string& password,
	const std::string& voice_sip_uri_hostname,
	const std::string& voice_account_server_uri)
{
	mVoiceSIPURIHostName = voice_sip_uri_hostname;
	mVoiceAccountServerURI = voice_account_server_uri;

	if(!mAccountHandle.empty())
	{
		// Already logged in.
		LL_WARNS("Voice") << "Called while already logged in." << LL_ENDL;
		
		// Don't process another login.
		return;
	}
	else if ( account_name != mAccountName )
	{
		//TODO: error?
		LL_WARNS("Voice") << "Wrong account name! " << account_name
				<< " instead of " << mAccountName << LL_ENDL;
	}
	else
	{
		mAccountPassword = password;
	}

	std::string debugSIPURIHostName = gSavedSettings.getString("VivoxDebugSIPURIHostName");
	
	if( !debugSIPURIHostName.empty() )
	{
		mVoiceSIPURIHostName = debugSIPURIHostName;
	}
	
	if( mVoiceSIPURIHostName.empty() )
	{
		// we have an empty account server name
		// so we fall back to hardcoded defaults

		if(sConnectingToAgni)
		{
			// Use the release account server
			mVoiceSIPURIHostName = "bhr.vivox.com";
		}
		else
		{
			// Use the development account server
			mVoiceSIPURIHostName = "bhd.vivox.com";
		}
	}
	
	std::string debugAccountServerURI = gSavedSettings.getString("VivoxDebugVoiceAccountServerURI");

	if( !debugAccountServerURI.empty() )
	{
		mVoiceAccountServerURI = debugAccountServerURI;
	}
	
	if( mVoiceAccountServerURI.empty() )
	{
		// If the account server URI isn't specified, construct it from the SIP URI hostname
		mVoiceAccountServerURI = "https://www." + mVoiceSIPURIHostName + "/api2/";		
	}
}

void LLVoiceClient::idle(void* user_data)
{
	LLVoiceClient* self = (LLVoiceClient*)user_data;
	self->stateMachine();
}

std::string LLVoiceClient::state2string(LLVoiceClient::state inState)
{
	std::string result = "UNKNOWN";
	
		// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break

	switch(inState)
	{
		CASE(stateDisableCleanup);
		CASE(stateDisabled);
		CASE(stateStart);
		CASE(stateDaemonLaunched);
		CASE(stateConnecting);
		CASE(stateConnected);
		CASE(stateIdle);
		CASE(stateMicTuningStart);
		CASE(stateMicTuningRunning);
		CASE(stateMicTuningStop);
		CASE(stateConnectorStart);
		CASE(stateConnectorStarting);
		CASE(stateConnectorStarted);
		CASE(stateLoginRetry);
		CASE(stateLoginRetryWait);
		CASE(stateNeedsLogin);
		CASE(stateLoggingIn);
		CASE(stateLoggedIn);
		CASE(stateCreatingSessionGroup);
		CASE(stateNoChannel);
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
	}

#undef CASE
	
	return result;
}

std::string LLVoiceClientStatusObserver::status2string(LLVoiceClientStatusObserver::EStatusType inStatus)
{
	std::string result = "UNKNOWN";
	
		// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break

	switch(inStatus)
	{
		CASE(STATUS_LOGIN_RETRY);
		CASE(STATUS_LOGGED_IN);
		CASE(STATUS_JOINING);
		CASE(STATUS_JOINED);
		CASE(STATUS_LEFT_CHANNEL);
		CASE(STATUS_VOICE_DISABLED);
		CASE(STATUS_VOICE_ENABLED);
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
	LL_DEBUGS("Voice") << "entering state " << state2string(inState) << LL_ENDL;
	
	mState = inState;
}

void LLVoiceClient::stateMachine()
{
	if(gDisconnected)
	{
		// The viewer has been disconnected from the sim.  Disable voice.
		setVoiceEnabled(false);
	}
	
	if(mVoiceEnabled)
	{
		updatePosition();
	}
	else if(mTuningMode)
	{
		// Tuning mode is special -- it needs to launch SLVoice even if voice is disabled.
	}
	else
	{
		if((getState() != stateDisabled) && (getState() != stateDisableCleanup))
		{
			// User turned off voice support.  Send the cleanup messages, close the socket, and reset.
			if(!mConnected)
			{
				// if voice was turned off after the daemon was launched but before we could connect to it, we may need to issue a kill.
				LL_INFOS("Voice") << "Disabling voice before connection to daemon, terminating." << LL_ENDL;
				killGateway();
			}
			
			logout();
			connectorShutdown();
			
			setState(stateDisableCleanup);
		}
	}
	
	// Check for parcel boundary crossing
	{
		LLViewerRegion *region = gAgent.getRegion();
		LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		
		if(region && parcel)
		{
			S32 parcelLocalID = parcel->getLocalID();
			std::string regionName = region->getName();
			std::string capURI = region->getCapability("ParcelVoiceInfoRequest");
		
//			LL_DEBUGS("Voice") << "Region name = \"" << regionName << "\", parcel local ID = " << parcelLocalID << ", cap URI = \"" << capURI << "\"" << LL_ENDL;

			// The region name starts out empty and gets filled in later.  
			// Also, the cap gets filled in a short time after the region cross, but a little too late for our purposes.
			// If either is empty, wait for the next time around.
			if(!regionName.empty())
			{
				if(!capURI.empty())
				{
					if((parcelLocalID != mCurrentParcelLocalID) || (regionName != mCurrentRegionName))
					{
						// We have changed parcels.  Initiate a parcel channel lookup.
						mCurrentParcelLocalID = parcelLocalID;
						mCurrentRegionName = regionName;
						
						parcelChanged();
					}
				}
				else
				{
					LL_WARNS_ONCE("Voice") << "region doesn't have ParcelVoiceInfoRequest capability.  This is normal for a short time after teleporting, but bad if it persists for very long." << LL_ENDL;
				}
			}
		}
	}

	switch(getState())
	{
		//MARK: stateDisableCleanup
		case stateDisableCleanup:
			// Clean up and reset everything. 
			closeSocket();
			deleteAllSessions();
			deleteAllBuddies();		
			
			mConnectorHandle.clear();
			mAccountHandle.clear();
			mAccountPassword.clear();
			mVoiceAccountServerURI.clear();
			
			setState(stateDisabled);	
		break;
		
		//MARK: stateDisabled
		case stateDisabled:
			if(mTuningMode || (mVoiceEnabled && !mAccountName.empty()))
			{
				setState(stateStart);
			}
		break;
		
		//MARK: stateStart
		case stateStart:
			if(gSavedSettings.getBOOL("CmdLineDisableVoice"))
			{
				// Voice is locked out, we must not launch the vivox daemon.
				setState(stateJail);
			}
			else if(!isGatewayRunning())
			{
				if(true)
				{
					// Launch the voice daemon
					
					// *FIX:Mani - Using the executable dir instead 
					// of mAppRODataDir, the working directory from which the app
					// is launched.
					//std::string exe_path = gDirUtilp->getAppRODataDir();
					std::string exe_path = gDirUtilp->getExecutableDir();
					exe_path += gDirUtilp->getDirDelimiter();
#if LL_WINDOWS
					exe_path += "SLVoice.exe";
#elif LL_DARWIN
					exe_path += "../Resources/SLVoice";
#else
					exe_path += "SLVoice";
#endif
					// See if the vivox executable exists
					llstat s;
					if(!LLFile::stat(exe_path, &s))
					{
						// vivox executable exists.  Build the command line and launch the daemon.
						// SLIM SDK: these arguments are no longer necessary.
//						std::string args = " -p tcp -h -c";
						std::string args;
						std::string loglevel = gSavedSettings.getString("VivoxDebugLevel");
						
						if(loglevel.empty())
						{
							loglevel = "-1";	// turn logging off completely
						}
						
						args += " -ll ";
						args += loglevel;
						
						LL_DEBUGS("Voice") << "Args for SLVoice: " << args << LL_ENDL;

#if LL_WINDOWS
						PROCESS_INFORMATION pinfo;
						STARTUPINFOW sinfo;
						memset(&sinfo, 0, sizeof(sinfo));

						std::string exe_dir = gDirUtilp->getExecutableDir();

						llutf16string exe_path16 = utf8str_to_utf16str(exe_path);
						llutf16string exe_dir16 = utf8str_to_utf16str(exe_dir);
						llutf16string args16 = utf8str_to_utf16str(args);
						// Create a writeable copy to keep Windows happy.
						U16 *argscpy_16 = new U16[args16.size() + 1];
						wcscpy_s(argscpy_16,args16.size()+1,args16.c_str());
						if(!CreateProcessW(exe_path16.c_str(), argscpy_16, NULL, NULL, FALSE, 0, NULL, exe_dir16.c_str(), &sinfo, &pinfo))
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
						
						delete[] argscpy_16;
#else	// LL_WINDOWS
						// This should be the same for mac and linux
						{
							std::vector<std::string> arglist;
							arglist.push_back(exe_path);
							
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
							
							fflush(NULL); // flush all buffers before the child inherits them
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
						LL_INFOS("Voice") << exe_path << " not found." << LL_ENDL;
					}	
				}
				else
				{		
					// SLIM SDK: port changed from 44124 to 44125.
					// We can connect to a client gateway running on another host.  This is useful for testing.
					// To do this, launch the gateway on a nearby host like this:
					//  vivox-gw.exe -p tcp -i 0.0.0.0:44125
					// and put that host's IP address here.
					mDaemonHost = LLHost(gSavedSettings.getString("VoiceHost"), gSavedSettings.getU32("VoicePort"));
				}

				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(CONNECT_THROTTLE_SECONDS);

				setState(stateDaemonLaunched);
				
				// Dirty the states we'll need to sync with the daemon when it comes up.
				mPTTDirty = true;
				mMicVolumeDirty = true;
				mSpeakerVolumeDirty = true;
				mSpeakerMuteDirty = true;
				// These only need to be set if they're not default (i.e. empty string).
				mCaptureDeviceDirty = !mCaptureDevice.empty();
				mRenderDeviceDirty = !mRenderDevice.empty();
				
				mMainSessionGroupHandle.clear();
			}
		break;

		//MARK: stateDaemonLaunched
		case stateDaemonLaunched:
			if(mUpdateTimer.hasExpired())
			{
				LL_DEBUGS("Voice") << "Connecting to vivox daemon" << LL_ENDL;

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

		//MARK: stateConnecting
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

			setState(stateConnected);
		}

		break;
		
		//MARK: stateConnected
		case stateConnected:
			// Initial devices query
			getCaptureDevicesSendMessage();
			getRenderDevicesSendMessage();

			mLoginRetryCount = 0;

			setState(stateIdle);
		break;

		//MARK: stateIdle
		case stateIdle:
			// This is the idle state where we're connected to the daemon but haven't set up a connector yet.
			if(mTuningMode)
			{
				mTuningExitState = stateIdle;
				setState(stateMicTuningStart);
			}
			else if(!mVoiceEnabled)
			{
				// We never started up the connector.  This will shut down the daemon.
				setState(stateConnectorStopped);
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
						setState(stateConnectorStart);
					}
					else
					{
						LL_WARNS_ONCE("Voice") << "region doesn't have ProvisionVoiceAccountRequest capability!" << LL_ENDL;
					}
				}
			}
		break;

		//MARK: stateMicTuningStart
		case stateMicTuningStart:
			if(mUpdateTimer.hasExpired())
			{
				if(mCaptureDeviceDirty || mRenderDeviceDirty)
				{
					// These can't be changed while in tuning mode.  Set them before starting.
					std::ostringstream stream;
					
					buildSetCaptureDevice(stream);
					buildSetRenderDevice(stream);

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
		
		//MARK: stateMicTuningRunning
		case stateMicTuningRunning:
			if(!mTuningMode || mCaptureDeviceDirty || mRenderDeviceDirty)
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
						LL_INFOS("Voice") << "setting tuning mic level to " << mTuningMicVolume << LL_ENDL;
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
		
		//MARK: stateMicTuningStop
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
												
		//MARK: stateConnectorStart
		case stateConnectorStart:
			if(!mVoiceEnabled)
			{
				// We were never logged in.  This will shut down the connector.
				setState(stateLoggedOut);
			}
			else if(!mVoiceAccountServerURI.empty())
			{
				connectorCreate();
			}
		break;
		
		//MARK: stateConnectorStarting
		case stateConnectorStarting:	// waiting for connector handle
			// connectorCreateResponse() will transition from here to stateConnectorStarted.
		break;
		
		//MARK: stateConnectorStarted
		case stateConnectorStarted:		// connector handle received
			if(!mVoiceEnabled)
			{
				// We were never logged in.  This will shut down the connector.
				setState(stateLoggedOut);
			}
			else
			{
				// The connector is started.  Send a login message.
				setState(stateNeedsLogin);
			}
		break;
				
		//MARK: stateLoginRetry
		case stateLoginRetry:
			if(mLoginRetryCount == 0)
			{
				// First retry -- display a message to the user
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGIN_RETRY);
			}
			
			mLoginRetryCount++;
			
			if(mLoginRetryCount > MAX_LOGIN_RETRIES)
			{
				LL_WARNS("Voice") << "too many login retries, giving up." << LL_ENDL;
				setState(stateLoginFailed);
			}
			else
			{
				LL_INFOS("Voice") << "will retry login in " << LOGIN_RETRY_SECONDS << " seconds." << LL_ENDL;
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(LOGIN_RETRY_SECONDS);
				setState(stateLoginRetryWait);
			}
		break;
		
		//MARK: stateLoginRetryWait
		case stateLoginRetryWait:
			if(mUpdateTimer.hasExpired())
			{
				setState(stateNeedsLogin);
			}
		break;
		
		//MARK: stateNeedsLogin
		case stateNeedsLogin:
			if(!mAccountPassword.empty())
			{
				setState(stateLoggingIn);
				loginSendMessage();
			}		
		break;
		
		//MARK: stateLoggingIn
		case stateLoggingIn:			// waiting for account handle
			// loginResponse() will transition from here to stateLoggedIn.
		break;
		
		//MARK: stateLoggedIn
		case stateLoggedIn:				// account handle received

			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);

			// request the current set of block rules (we'll need them when updating the friends list)
			accountListBlockRulesSendMessage();
			
			// request the current set of auto-accept rules
			accountListAutoAcceptRulesSendMessage();
			
			// Set up the mute list observer if it hasn't been set up already.
			if((!sMuteListListener_listening))
			{
				LLMuteList::getInstance()->addObserver(&mutelist_listener);
				sMuteListListener_listening = true;
			}

			// Set up the friends list observer if it hasn't been set up already.
			if(friendslist_listener == NULL)
			{
				friendslist_listener = new LLVoiceClientFriendsObserver;
				LLAvatarTracker::instance().addObserver(friendslist_listener);
			}
			
			// Set the initial state of mic mute, local speaker volume, etc.
			{
				std::ostringstream stream;
				
				buildLocalAudioUpdates(stream);
				
				if(!stream.str().empty())
				{
					writeString(stream.str());
				}
			}
			
#if USE_SESSION_GROUPS			
			// create the main session group
			sessionGroupCreateSendMessage();
			
			setState(stateCreatingSessionGroup);
#else
			// Not using session groups -- skip the stateCreatingSessionGroup state.
			setState(stateNoChannel);

			// Initial kick-off of channel lookup logic
			parcelChanged();		
#endif
		break;
		
		//MARK: stateCreatingSessionGroup
		case stateCreatingSessionGroup:
			if(mSessionTerminateRequested || !mVoiceEnabled)
			{
				// TODO: Question: is this the right way out of this state
				setState(stateSessionTerminated);
			}
			else if(!mMainSessionGroupHandle.empty())
			{
				setState(stateNoChannel);
				
				// Start looped recording (needed for "panic button" anti-griefing tool)
				recordingLoopStart();

				// Initial kick-off of channel lookup logic
				parcelChanged();		
			}
		break;
					
		//MARK: stateNoChannel
		case stateNoChannel:
			// Do this here as well as inside sendPositionalUpdate().  
			// Otherwise, if you log in but don't join a proximal channel (such as when your login location has voice disabled), your friends list won't sync.
			sendFriendsListUpdates();
			
			if(mSessionTerminateRequested || !mVoiceEnabled)
			{
				// TODO: Question: Is this the right way out of this state?
				setState(stateSessionTerminated);
			}
			else if(mTuningMode)
			{
				mTuningExitState = stateNoChannel;
				setState(stateMicTuningStart);
			}
			else if(sessionNeedsRelog(mNextAudioSession))
			{
				requestRelog();
				setState(stateSessionTerminated);
			}
			else if(mNextAudioSession)
			{				
				sessionState *oldSession = mAudioSession;

				mAudioSession = mNextAudioSession;
				if(!mAudioSession->mReconnect)	
				{
					mNextAudioSession = NULL;
				}
				
				// The old session may now need to be deleted.
				reapSession(oldSession);
				
				if(!mAudioSession->mHandle.empty())
				{
					// Connect to a session by session handle

					sessionMediaConnectSendMessage(mAudioSession);
				}
				else
				{
					// Connect to a session by URI
					sessionCreateSendMessage(mAudioSession, true, false);
				}

				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINING);
				setState(stateJoiningSession);
			}
			else if(!mSpatialSessionURI.empty())
			{
				// If we're not headed elsewhere and have a spatial URI, return to spatial.
				switchChannel(mSpatialSessionURI, true, false, false, mSpatialSessionCredentials);
			}
		break;

		//MARK: stateJoiningSession
		case stateJoiningSession:		// waiting for session handle
			// joinedAudioSession() will transition from here to stateSessionJoined.
			if(!mVoiceEnabled)
			{
				// User bailed out during connect -- jump straight to teardown.
				setState(stateSessionTerminated);
			}
			else if(mSessionTerminateRequested)
			{
				if(mAudioSession && !mAudioSession->mHandle.empty())
				{
					// Only allow direct exits from this state in p2p calls (for cancelling an invite).
					// Terminating a half-connected session on other types of calls seems to break something in the vivox gateway.
					if(mAudioSession->mIsP2P)
					{
						sessionMediaDisconnectSendMessage(mAudioSession);
						setState(stateSessionTerminated);
					}
				}
			}
		break;
		
		//MARK: stateSessionJoined
		case stateSessionJoined:		// session handle received
			// It appears that I need to wait for BOTH the SessionGroup.AddSession response and the SessionStateChangeEvent with state 4
			// before continuing from this state.  They can happen in either order, and if I don't wait for both, things can get stuck.
			// For now, the SessionGroup.AddSession response handler sets mSessionHandle and the SessionStateChangeEvent handler transitions to stateSessionJoined.
			// This is a cheap way to make sure both have happened before proceeding.
			if(mAudioSession && mAudioSession->mVoiceEnabled)
			{
				// Dirty state that may need to be sync'ed with the daemon.
				mPTTDirty = true;
				mSpeakerVolumeDirty = true;
				mSpatialCoordsDirty = true;
				
				setState(stateRunning);
				
				// Start the throttle timer
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);

				// Events that need to happen when a session is joined could go here.
				// Maybe send initial spatial data?
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);

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
				if(mAudioSession && mAudioSession->mIsP2P)
				{
					sessionMediaDisconnectSendMessage(mAudioSession);
					setState(stateSessionTerminated);
				}
			}
		break;
		
		//MARK: stateRunning
		case stateRunning:				// steady state
			// Disabling voice or disconnect requested.
			if(!mVoiceEnabled || mSessionTerminateRequested)
			{
				leaveAudioSession();
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
				
				if(!inSpatialChannel())
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
				if((mAudioSession && mAudioSession->mVolumeDirty) || mPTTDirty || mSpeakerVolumeDirty || mUpdateTimer.hasExpired())
				{
					mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
					sendPositionalUpdate();
				}
			}
		break;
		
		//MARK: stateLeavingSession
		case stateLeavingSession:		// waiting for terminate session response
			// The handler for the Session.Terminate response will transition from here to stateSessionTerminated.
		break;

		//MARK: stateSessionTerminated
		case stateSessionTerminated:
			
			// Must do this first, since it uses mAudioSession.
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
			
			if(mAudioSession)
			{
				sessionState *oldSession = mAudioSession;

				mAudioSession = NULL;
				// We just notified status observers about this change.  Don't do it again.
				mAudioSessionChanged = false;

				// The old session may now need to be deleted.
				reapSession(oldSession);
			}
			else
			{
				LL_WARNS("Voice") << "stateSessionTerminated with NULL mAudioSession" << LL_ENDL;
			}
	
			// Always reset the terminate request flag when we get here.
			mSessionTerminateRequested = false;

			if(mVoiceEnabled && !mRelogRequested)
			{				
				// Just leaving a channel, go back to stateNoChannel (the "logged in but have no channel" state).
				setState(stateNoChannel);
			}
			else
			{
				// Shutting down voice, continue with disconnecting.
				logout();
				
				// The state machine will take it from here
				mRelogRequested = false;
			}
			
		break;
		
		//MARK: stateLoggingOut
		case stateLoggingOut:			// waiting for logout response
			// The handler for the AccountLoginStateChangeEvent will transition from here to stateLoggedOut.
		break;
		
		//MARK: stateLoggedOut
		case stateLoggedOut:			// logout response received
			
			// Once we're logged out, all these things are invalid.
			mAccountHandle.clear();
			deleteAllSessions();
			deleteAllBuddies();

			if(mVoiceEnabled && !mRelogRequested)
			{
				// User was logged out, but wants to be logged in.  Send a new login request.
				setState(stateNeedsLogin);
			}
			else
			{
				// shut down the connector
				connectorShutdown();
			}
		break;
		
		//MARK: stateConnectorStopping
		case stateConnectorStopping:	// waiting for connector stop
			// The handler for the Connector.InitiateShutdown response will transition from here to stateConnectorStopped.
		break;

		//MARK: stateConnectorStopped
		case stateConnectorStopped:		// connector stop received
			setState(stateDisableCleanup);
		break;

		//MARK: stateConnectorFailed
		case stateConnectorFailed:
			setState(stateConnectorFailedWaiting);
		break;
		//MARK: stateConnectorFailedWaiting
		case stateConnectorFailedWaiting:
			if(!mVoiceEnabled)
			{
				setState(stateDisableCleanup);
			}
		break;

		//MARK: stateLoginFailed
		case stateLoginFailed:
			setState(stateLoginFailedWaiting);
		break;
		//MARK: stateLoginFailedWaiting
		case stateLoginFailedWaiting:
			if(!mVoiceEnabled)
			{
				setState(stateDisableCleanup);
			}
		break;

		//MARK: stateJoinSessionFailed
		case stateJoinSessionFailed:
			// Transition to error state.  Send out any notifications here.
			if(mAudioSession)
			{
				LL_WARNS("Voice") << "stateJoinSessionFailed: (" << mAudioSession->mErrorStatusCode << "): " << mAudioSession->mErrorStatusString << LL_ENDL;
			}
			else
			{
				LL_WARNS("Voice") << "stateJoinSessionFailed with no current session" << LL_ENDL;
			}
			
			notifyStatusObservers(LLVoiceClientStatusObserver::ERROR_UNKNOWN);
			setState(stateJoinSessionFailedWaiting);
		break;
		
		//MARK: stateJoinSessionFailedWaiting
		case stateJoinSessionFailedWaiting:
			// Joining a channel failed, either due to a failed channel name -> sip url lookup or an error from the join message.
			// Region crossings may leave this state and try the join again.
			if(mSessionTerminateRequested)
			{
				setState(stateSessionTerminated);
			}
		break;
		
		//MARK: stateJail
		case stateJail:
			// We have given up.  Do nothing.
		break;

	}
	
	if(mAudioSession && mAudioSession->mParticipantsChanged)
	{
		mAudioSession->mParticipantsChanged = false;
		mAudioSessionChanged = true;
	}
	
	if(mAudioSessionChanged)
	{
		mAudioSessionChanged = false;
		notifyParticipantObservers();
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

	bool autoPostCrashDumps = gSavedSettings.getBOOL("VivoxAutoPostCrashDumps");

	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Login.1\">"
		<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
		<< "<AccountName>" << mAccountName << "</AccountName>"
		<< "<AccountPassword>" << mAccountPassword << "</AccountPassword>"
		<< "<AudioSessionAnswerMode>VerifyAnswer</AudioSessionAnswerMode>"
		<< "<EnableBuddiesAndPresence>true</EnableBuddiesAndPresence>"
		<< "<BuddyManagementMode>Application</BuddyManagementMode>"
		<< "<ParticipantPropertyFrequency>5</ParticipantPropertyFrequency>"
		<< (autoPostCrashDumps?"<AutopostCrashDumps>true</AutopostCrashDumps>":"")
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::logout()
{
	// Ensure that we'll re-request provisioning before logging in again
	mAccountPassword.clear();
	mVoiceAccountServerURI.clear();
	
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

void LLVoiceClient::accountListBlockRulesSendMessage()
{
	if(!mAccountHandle.empty())
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "requesting block rules" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.ListBlockRules.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVoiceClient::accountListAutoAcceptRulesSendMessage()
{
	if(!mAccountHandle.empty())
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "requesting auto-accept rules" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.ListAutoAcceptRules.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVoiceClient::sessionGroupCreateSendMessage()
{
	if(!mAccountHandle.empty())
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "creating session group" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Create.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
			<< "<Type>Normal</Type>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVoiceClient::sessionCreateSendMessage(sessionState *session, bool startAudio, bool startText)
{
	LL_DEBUGS("Voice") << "requesting create: " << session->mSIPURI << LL_ENDL;
	
	session->mCreateInProgress = true;
	if(startAudio)
	{
		session->mMediaConnectInProgress = true;
	}

	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mSIPURI << "\" action=\"Session.Create.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "<URI>" << session->mSIPURI << "</URI>";

	static const std::string allowed_chars =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
				"0123456789"
				"-._~";

	if(!session->mHash.empty())
	{
		stream
			<< "<Password>" << LLURI::escape(session->mHash, allowed_chars) << "</Password>"
			<< "<PasswordHashAlgorithm>SHA1UserName</PasswordHashAlgorithm>";
	}
	
	stream
		<< "<ConnectAudio>" << (startAudio?"true":"false") << "</ConnectAudio>"
		<< "<ConnectText>" << (startText?"true":"false") << "</ConnectText>"
		<< "<Name>" << mChannelName << "</Name>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}

void LLVoiceClient::sessionGroupAddSessionSendMessage(sessionState *session, bool startAudio, bool startText)
{
	LL_DEBUGS("Voice") << "requesting create: " << session->mSIPURI << LL_ENDL;
	
	session->mCreateInProgress = true;
	if(startAudio)
	{
		session->mMediaConnectInProgress = true;
	}
	
	std::string password;
	if(!session->mHash.empty())
	{
		static const std::string allowed_chars =
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
					"0123456789"
					"-._~"
					;
		password = LLURI::escape(session->mHash, allowed_chars);
	}

	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mSIPURI << "\" action=\"SessionGroup.AddSession.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<URI>" << session->mSIPURI << "</URI>"
		<< "<Name>" << mChannelName << "</Name>"
		<< "<ConnectAudio>" << (startAudio?"true":"false") << "</ConnectAudio>"
		<< "<ConnectText>" << (startText?"true":"false") << "</ConnectText>"
		<< "<Password>" << password << "</Password>"
		<< "<PasswordHashAlgorithm>SHA1UserName</PasswordHashAlgorithm>"
	<< "</Request>\n\n\n"
	;
	
	writeString(stream.str());
}

void LLVoiceClient::sessionMediaConnectSendMessage(sessionState *session)
{
	LL_DEBUGS("Voice") << "connecting audio to session handle: " << session->mHandle << LL_ENDL;

	session->mMediaConnectInProgress = true;
	
	std::ostringstream stream;

	stream
	<< "<Request requestId=\"" << session->mHandle << "\" action=\"Session.MediaConnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
		<< "<Media>Audio</Media>"
	<< "</Request>\n\n\n";

	writeString(stream.str());
}

void LLVoiceClient::sessionTextConnectSendMessage(sessionState *session)
{
	LL_DEBUGS("Voice") << "connecting text to session handle: " << session->mHandle << LL_ENDL;
	
	std::ostringstream stream;

	stream
	<< "<Request requestId=\"" << session->mHandle << "\" action=\"Session.TextConnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";

	writeString(stream.str());
}

void LLVoiceClient::sessionTerminate()
{
	mSessionTerminateRequested = true;
}

void LLVoiceClient::requestRelog()
{
	mSessionTerminateRequested = true;
	mRelogRequested = true;
}


void LLVoiceClient::leaveAudioSession()
{
	if(mAudioSession)
	{
		LL_DEBUGS("Voice") << "leaving session: " << mAudioSession->mSIPURI << LL_ENDL;

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
				if(!mAudioSession->mHandle.empty())
				{

#if RECORD_EVERYTHING
					// HACK: for testing only
					// Save looped recording
					std::string savepath("/tmp/vivoxrecording");
					{
						time_t now = time(NULL);
						const size_t BUF_SIZE = 64;
						char time_str[BUF_SIZE];	/* Flawfinder: ignore */
						
						strftime(time_str, BUF_SIZE, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
						savepath += time_str;
					}
					recordingLoopSave(savepath);
#endif

					sessionMediaDisconnectSendMessage(mAudioSession);
					setState(stateLeavingSession);
				}
				else
				{
					LL_WARNS("Voice") << "called with no session handle" << LL_ENDL;	
					setState(stateSessionTerminated);
				}
			break;
			case stateJoinSessionFailed:
			case stateJoinSessionFailedWaiting:
				setState(stateSessionTerminated);
			break;
			
			default:
				LL_WARNS("Voice") << "called from unknown state" << LL_ENDL;
			break;
		}
	}
	else
	{
		LL_WARNS("Voice") << "called with no active session" << LL_ENDL;
		setState(stateSessionTerminated);
	}
}

void LLVoiceClient::sessionTerminateSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending Session.Terminate with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Terminate.1\">"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::sessionGroupTerminateSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending SessionGroup.Terminate with handle " << session->mGroupHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Terminate.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::sessionMediaDisconnectSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending Session.MediaDisconnect with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.MediaDisconnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
		<< "<Media>Audio</Media>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
	
}

void LLVoiceClient::sessionTextDisconnectSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending Session.TextDisconnect with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.TextDisconnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	
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
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mCaptureDevices.clear();
}

void LLVoiceClient::addCaptureDevice(const std::string& name)
{
	LL_DEBUGS("Voice") << name << LL_ENDL;

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
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mRenderDevices.clear();
}

void LLVoiceClient::addRenderDevice(const std::string& name)
{
	LL_DEBUGS("Voice") << name << LL_ENDL;
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
	LL_DEBUGS("Voice") << "sending CaptureAudioStart" << LL_ENDL;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStart.1\">"
    << "<Duration>" << duration << "</Duration>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVoiceClient::tuningCaptureStopSendMessage()
{
	LL_DEBUGS("Voice") << "sending CaptureAudioStop" << LL_ENDL;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());

	mTuningEnergy = 0.0f;
}

void LLVoiceClient::tuningSetMicVolume(float volume)
{
	int scaled_volume = scale_mic_volume(volume);

	if(scaled_volume != mTuningMicVolume)
	{
		mTuningMicVolume = scaled_volume;
		mTuningMicVolumeDirty = true;
	}
}

void LLVoiceClient::tuningSetSpeakerVolume(float volume)
{
	int scaled_volume = scale_speaker_volume(volume);	

	if(scaled_volume != mTuningSpeakerVolume)
	{
		mTuningSpeakerVolume = scaled_volume;
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
	LL_WARNS("Voice") << "Connection to vivox daemon lost.  Resetting state."<< LL_ENDL;

	// Try to relaunch the daemon
	setState(stateDisableCleanup);
}

void LLVoiceClient::giveUp()
{
	// All has failed.  Clean up and stop trying.
	closeSocket();
	deleteAllSessions();
	deleteAllBuddies();
	
	setState(stateJail);
}

static void oldSDKTransform (LLVector3 &left, LLVector3 &up, LLVector3 &at, LLVector3d &pos, LLVector3 &vel)
{
	F32 nat[3], nup[3], nl[3], nvel[3]; // the new at, up, left vectors and the  new position and velocity
	F64 npos[3];
	
	// The original XML command was sent like this:
	/*
			<< "<Position>"
				<< "<X>" << pos[VX] << "</X>"
				<< "<Y>" << pos[VZ] << "</Y>"
				<< "<Z>" << pos[VY] << "</Z>"
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
	*/

#if 1
	// This was the original transform done when building the XML command
	nat[0] = left.mV[VX];
	nat[1] = up.mV[VX];
	nat[2] = at.mV[VX];

	nup[0] = left.mV[VZ];
	nup[1] = up.mV[VY];
	nup[2] = at.mV[VZ];

	nl[0] = left.mV[VY];
	nl[1] = up.mV[VZ];
	nl[2] = at.mV[VY];

	npos[0] = pos.mdV[VX];
	npos[1] = pos.mdV[VZ];
	npos[2] = pos.mdV[VY];

	nvel[0] = vel.mV[VX];
	nvel[1] = vel.mV[VZ];
	nvel[2] = vel.mV[VY];

	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
	
	// This was the original transform done in the SDK
	nat[0] = at.mV[2];
	nat[1] = 0; // y component of at vector is always 0, this was up[2]
	nat[2] = -1 * left.mV[2];

	// We override whatever the application gives us
	nup[0] = 0; // x component of up vector is always 0
	nup[1] = 1; // y component of up vector is always 1
	nup[2] = 0; // z component of up vector is always 0

	nl[0] = at.mV[0];
	nl[1] = 0;  // y component of left vector is always zero, this was up[0]
	nl[2] = -1 * left.mV[0];

	npos[2] = pos.mdV[2] * -1.0;
	npos[1] = pos.mdV[1];
	npos[0] = pos.mdV[0];

	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
#else
	// This is the compose of the two transforms (at least, that's what I'm trying for)
	nat[0] = at.mV[VX];
	nat[1] = 0; // y component of at vector is always 0, this was up[2]
	nat[2] = -1 * up.mV[VZ];

	// We override whatever the application gives us
	nup[0] = 0; // x component of up vector is always 0
	nup[1] = 1; // y component of up vector is always 1
	nup[2] = 0; // z component of up vector is always 0

	nl[0] = left.mV[VX];
	nl[1] = 0;  // y component of left vector is always zero, this was up[0]
	nl[2] = -1 * left.mV[VY];

	npos[0] = pos.mdV[VX];
	npos[1] = pos.mdV[VZ];
	npos[2] = pos.mdV[VY] * -1.0;

	nvel[0] = vel.mV[VX];
	nvel[1] = vel.mV[VZ];
	nvel[2] = vel.mV[VY];

	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
	
#endif
}

void LLVoiceClient::sendPositionalUpdate(void)
{	
	std::ostringstream stream;
	
	if(mSpatialCoordsDirty)
	{
		LLVector3 l, u, a, vel;
		LLVector3d pos;

		mSpatialCoordsDirty = false;
		
		// Always send both speaker and listener positions together.
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Set3DPosition.1\">"		
			<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>";
		
		stream << "<SpeakerPosition>";

//		LL_DEBUGS("Voice") << "Sending speaker position " << mAvatarPosition << LL_ENDL;
		l = mAvatarRot.getLeftRow();
		u = mAvatarRot.getUpRow();
		a = mAvatarRot.getFwdRow();
		pos = mAvatarPosition;
		vel = mAvatarVelocity;

		// SLIM SDK: the old SDK was doing a transform on the passed coordinates that the new one doesn't do anymore.
		// The old transform is replicated by this function.
		oldSDKTransform(l, u, a, pos, vel);
		
		stream 
			<< "<Position>"
				<< "<X>" << pos.mdV[VX] << "</X>"
				<< "<Y>" << pos.mdV[VY] << "</Y>"
				<< "<Z>" << pos.mdV[VZ] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << vel.mV[VX] << "</X>"
				<< "<Y>" << vel.mV[VY] << "</Y>"
				<< "<Z>" << vel.mV[VZ] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << a.mV[VX] << "</X>"
				<< "<Y>" << a.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << u.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << u.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
			<< "<LeftOrientation>"
				<< "<X>" << l.mV [VX] << "</X>"
				<< "<Y>" << l.mV [VY] << "</Y>"
				<< "<Z>" << l.mV [VZ] << "</Z>"
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
		pos = earPosition;
		vel = earVelocity;

//		LL_DEBUGS("Voice") << "Sending listener position " << earPosition << LL_ENDL;
		
		oldSDKTransform(l, u, a, pos, vel);
		
		stream 
			<< "<Position>"
				<< "<X>" << pos.mdV[VX] << "</X>"
				<< "<Y>" << pos.mdV[VY] << "</Y>"
				<< "<Z>" << pos.mdV[VZ] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << vel.mV[VX] << "</X>"
				<< "<Y>" << vel.mV[VY] << "</Y>"
				<< "<Z>" << vel.mV[VZ] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << a.mV[VX] << "</X>"
				<< "<Y>" << a.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << u.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << u.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
			<< "<LeftOrientation>"
				<< "<X>" << l.mV [VX] << "</X>"
				<< "<Y>" << l.mV [VY] << "</Y>"
				<< "<Z>" << l.mV [VZ] << "</Z>"
			<< "</LeftOrientation>";


		stream << "</ListenerPosition>";

		stream << "</Request>\n\n\n";
	}	
	
	if(mAudioSession && mAudioSession->mVolumeDirty)
	{
		participantMap::iterator iter = mAudioSession->mParticipantsByURI.begin();

		mAudioSession->mVolumeDirty = false;
		
		for(; iter != mAudioSession->mParticipantsByURI.end(); iter++)
		{
			participantState *p = iter->second;
			
			if(p->mVolumeDirty)
			{
				// Can't set volume/mute for yourself
				if(!p->mIsSelf)
				{
					int volume = 56; // nominal default value
					bool mute = p->mOnMuteList;
					
					if(p->mUserVolume != -1)
					{
						// scale from user volume in the range 0-400 (with 100 as "normal") to vivox volume in the range 0-100 (with 56 as "normal")
						if(p->mUserVolume < 100)
							volume = (p->mUserVolume * 56) / 100;
						else
							volume = (((p->mUserVolume - 100) * (100 - 56)) / 300) + 56;
					}
					else if(p->mVolume != -1)
					{
						// Use the previously reported internal volume (comes in with a ParticipantUpdatedEvent)
						volume = p->mVolume;
					}
										

					if(mute)
					{
						// SetParticipantMuteForMe doesn't work in p2p sessions.
						// If we want the user to be muted, set their volume to 0 as well.
						// This isn't perfect, but it will at least reduce their volume to a minimum.
						volume = 0;
					}
					
					if(volume == 0)
						mute = true;

					LL_DEBUGS("Voice") << "Setting volume/mute for avatar " << p->mAvatarID << " to " << volume << (mute?"/true":"/false") << LL_ENDL;
					
					// SLIM SDK: Send both volume and mute commands.
					
					// Send a "volume for me" command for the user.
					stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetParticipantVolumeForMe.1\">"
						<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>"
						<< "<ParticipantURI>" << p->mURI << "</ParticipantURI>"
						<< "<Volume>" << volume << "</Volume>"
						<< "</Request>\n\n\n";

					if(!mAudioSession->mIsP2P)
					{
						// Send a "mute for me" command for the user
						// Doesn't work in P2P sessions
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetParticipantMuteForMe.1\">"
							<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>"
							<< "<ParticipantURI>" << p->mURI << "</ParticipantURI>"
							<< "<Mute>" << (mute?"1":"0") << "</Mute>"
							<< "<Scope>Audio</Scope>"
							<< "</Request>\n\n\n";
					}
				}
				
				p->mVolumeDirty = false;
			}
		}
	}
			
	buildLocalAudioUpdates(stream);
	
	if(!stream.str().empty())
	{
		writeString(stream.str());
	}
	
	// Friends list updates can be huge, especially on the first voice login of an account with lots of friends.
	// Batching them all together can choke SLVoice, so send them in separate writes.
	sendFriendsListUpdates();
}

void LLVoiceClient::buildSetCaptureDevice(std::ostringstream &stream)
{
	if(mCaptureDeviceDirty)
	{
		LL_DEBUGS("Voice") << "Setting input device = \"" << mCaptureDevice << "\"" << LL_ENDL;
	
		stream 
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetCaptureDevice.1\">"
			<< "<CaptureDeviceSpecifier>" << mCaptureDevice << "</CaptureDeviceSpecifier>"
		<< "</Request>"
		<< "\n\n\n";
		
		mCaptureDeviceDirty = false;
	}
}

void LLVoiceClient::buildSetRenderDevice(std::ostringstream &stream)
{
	if(mRenderDeviceDirty)
	{
		LL_DEBUGS("Voice") << "Setting output device = \"" << mRenderDevice << "\"" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetRenderDevice.1\">"
			<< "<RenderDeviceSpecifier>" << mRenderDevice << "</RenderDeviceSpecifier>"
		<< "</Request>"
		<< "\n\n\n";
		mRenderDeviceDirty = false;
	}
}

void LLVoiceClient::buildLocalAudioUpdates(std::ostringstream &stream)
{
	buildSetCaptureDevice(stream);

	buildSetRenderDevice(stream);

	if(mPTTDirty)
	{
		mPTTDirty = false;

		// Send a local mute command.
		// NOTE that the state of "PTT" is the inverse of "local mute".
		//   (i.e. when PTT is true, we send a mute command with "false", and vice versa)
		
		LL_DEBUGS("Voice") << "Sending MuteLocalMic command with parameter " << (mPTT?"false":"true") << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << (mPTT?"false":"true") << "</Value>"
			<< "</Request>\n\n\n";
		
	}

	if(mSpeakerMuteDirty)
	{
		const char *muteval = ((mSpeakerVolume <= scale_speaker_volume(0))?"true":"false");

		mSpeakerMuteDirty = false;

		LL_INFOS("Voice") << "Setting speaker mute to " << muteval  << LL_ENDL;
		
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalSpeaker.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << muteval << "</Value>"
			<< "</Request>\n\n\n";	
		
	}
	
	if(mSpeakerVolumeDirty)
	{
		mSpeakerVolumeDirty = false;

		LL_INFOS("Voice") << "Setting speaker volume to " << mSpeakerVolume  << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalSpeakerVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mSpeakerVolume << "</Value>"
			<< "</Request>\n\n\n";
			
	}
	
	if(mMicVolumeDirty)
	{
		mMicVolumeDirty = false;

		LL_INFOS("Voice") << "Setting mic volume to " << mMicVolume  << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalMicVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mMicVolume << "</Value>"
			<< "</Request>\n\n\n";				
	}

	
}

void LLVoiceClient::checkFriend(const LLUUID& id)
{
	std::string name;
	buddyListEntry *buddy = findBuddy(id);

	// Make sure we don't add a name before it's been looked up.
	if(gCacheName->getFullName(id, name))
	{

		const LLRelationship* relationInfo = LLAvatarTracker::instance().getBuddyInfo(id);
		bool canSeeMeOnline = false;
		if(relationInfo && relationInfo->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS))
			canSeeMeOnline = true;
		
		// When we get here, mNeedsSend is true and mInSLFriends is false.  Change them as necessary.
		
		if(buddy)
		{
			// This buddy is already in both lists.

			if(name != buddy->mDisplayName)
			{
				// The buddy is in the list with the wrong name.  Update it with the correct name.
				LL_WARNS("Voice") << "Buddy " << id << " has wrong name (\"" << buddy->mDisplayName << "\" should be \"" << name << "\"), updating."<< LL_ENDL;
				buddy->mDisplayName = name;
				buddy->mNeedsNameUpdate = true;		// This will cause the buddy to be resent.
			}
		}
		else
		{
			// This buddy was not in the vivox list, needs to be added.
			buddy = addBuddy(sipURIFromID(id), name);
			buddy->mUUID = id;
		}
		
		// In all the above cases, the buddy is in the SL friends list (which is how we got here).
		buddy->mInSLFriends = true;
		buddy->mCanSeeMeOnline = canSeeMeOnline;
		buddy->mNameResolved = true;
		
	}
	else
	{
		// This name hasn't been looked up yet.  Don't do anything with this buddy list entry until it has.
		if(buddy)
		{
			buddy->mNameResolved = false;
		}
		
		// Initiate a lookup.
		// The "lookup completed" callback will ensure that the friends list is rechecked after it completes.
		lookupName(id);
	}
}

void LLVoiceClient::clearAllLists()
{
	// FOR TESTING ONLY
	
	// This will send the necessary commands to delete ALL buddies, autoaccept rules, and block rules SLVoice tells us about.
	buddyListMap::iterator buddy_it;
	for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end();)
	{
		buddyListEntry *buddy = buddy_it->second;
		buddy_it++;
		
		std::ostringstream stream;

		if(buddy->mInVivoxBuddies)
		{
			// delete this entry from the vivox buddy list
			buddy->mInVivoxBuddies = false;
			LL_DEBUGS("Voice") << "delete " << buddy->mURI << " (" << buddy->mDisplayName << ")" << LL_ENDL;
			stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.BuddyDelete.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
				<< "</Request>\n\n\n";		
		}

		if(buddy->mHasBlockListEntry)
		{
			// Delete the associated block list entry (so the block list doesn't fill up with junk)
			buddy->mHasBlockListEntry = false;
			stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteBlockRule.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
				<< "</Request>\n\n\n";								
		}
		if(buddy->mHasAutoAcceptListEntry)
		{
			// Delete the associated auto-accept list entry (so the auto-accept list doesn't fill up with junk)
			buddy->mHasAutoAcceptListEntry = false;
			stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteAutoAcceptRule.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
				<< "</Request>\n\n\n";
		}

		writeString(stream.str());

	}
}

void LLVoiceClient::sendFriendsListUpdates()
{
	if(mBuddyListMapPopulated && mBlockRulesListReceived && mAutoAcceptRulesListReceived && mFriendsListDirty)
	{
		mFriendsListDirty = false;
		
		if(0)
		{
			// FOR TESTING ONLY -- clear all buddy list, block list, and auto-accept list entries.
			clearAllLists();
			return;
		}
		
		LL_INFOS("Voice") << "Checking vivox buddy list against friends list..." << LL_ENDL;
		
		buddyListMap::iterator buddy_it;
		for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end(); buddy_it++)
		{
			// reset the temp flags in the local buddy list
			buddy_it->second->mInSLFriends = false;
		}
		
		// correlate with the friends list
		{
			LLCollectAllBuddies collect;
			LLAvatarTracker::instance().applyFunctor(collect);
			LLCollectAllBuddies::buddy_map_t::const_iterator it = collect.mOnline.begin();
			LLCollectAllBuddies::buddy_map_t::const_iterator end = collect.mOnline.end();
			
			for ( ; it != end; ++it)
			{
				checkFriend(it->second);
			}
			it = collect.mOffline.begin();
			end = collect.mOffline.end();
			for ( ; it != end; ++it)
			{
				checkFriend(it->second);
			}
		}
				
		LL_INFOS("Voice") << "Sending friend list updates..." << LL_ENDL;

		for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end();)
		{
			buddyListEntry *buddy = buddy_it->second;
			buddy_it++;
			
			// Ignore entries that aren't resolved yet.
			if(buddy->mNameResolved)
			{
				std::ostringstream stream;

				if(buddy->mInSLFriends && (!buddy->mInVivoxBuddies || buddy->mNeedsNameUpdate))
				{					
					if(mNumberOfAliases > 0)
					{
						// Add (or update) this entry in the vivox buddy list
						buddy->mInVivoxBuddies = true;
						buddy->mNeedsNameUpdate = false;
						LL_DEBUGS("Voice") << "add/update " << buddy->mURI << " (" << buddy->mDisplayName << ")" << LL_ENDL;
						stream 
							<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.BuddySet.1\">"
								<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
								<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
								<< "<DisplayName>" << buddy->mDisplayName << "</DisplayName>"
								<< "<BuddyData></BuddyData>"	// Without this, SLVoice doesn't seem to parse the command.
								<< "<GroupID>0</GroupID>"
							<< "</Request>\n\n\n";	
					}
				}
				else if(!buddy->mInSLFriends)
				{
					// This entry no longer exists in your SL friends list.  Remove all traces of it from the Vivox buddy list.
 					if(buddy->mInVivoxBuddies)
					{
						// delete this entry from the vivox buddy list
						buddy->mInVivoxBuddies = false;
						LL_DEBUGS("Voice") << "delete " << buddy->mURI << " (" << buddy->mDisplayName << ")" << LL_ENDL;
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.BuddyDelete.1\">"
							<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
							<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
							<< "</Request>\n\n\n";		
					}

					if(buddy->mHasBlockListEntry)
					{
						// Delete the associated block list entry, if any
						buddy->mHasBlockListEntry = false;
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteBlockRule.1\">"
							<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
							<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
							<< "</Request>\n\n\n";								
					}
					if(buddy->mHasAutoAcceptListEntry)
					{
						// Delete the associated auto-accept list entry, if any
						buddy->mHasAutoAcceptListEntry = false;
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteAutoAcceptRule.1\">"
							<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
							<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
							<< "</Request>\n\n\n";
					}
				}
				
				if(buddy->mInSLFriends)
				{

					if(buddy->mCanSeeMeOnline)
					{
						// Buddy should not be blocked.

						// If this buddy doesn't already have either a block or autoaccept list entry, we'll update their status when we receive a SubscriptionEvent.
						
						// If the buddy has a block list entry, delete it.
						if(buddy->mHasBlockListEntry)
						{
							buddy->mHasBlockListEntry = false;
							stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteBlockRule.1\">"
								<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
								<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
								<< "</Request>\n\n\n";		
							
							
							// If we just deleted a block list entry, add an auto-accept entry.
							if(!buddy->mHasAutoAcceptListEntry)
							{
								buddy->mHasAutoAcceptListEntry = true;								
								stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.CreateAutoAcceptRule.1\">"
									<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
									<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
									<< "<AutoAddAsBuddy>0</AutoAddAsBuddy>"
									<< "</Request>\n\n\n";
							}
						}
					}
					else
					{
						// Buddy should be blocked.
						
						// If this buddy doesn't already have either a block or autoaccept list entry, we'll update their status when we receive a SubscriptionEvent.

						// If this buddy has an autoaccept entry, delete it
						if(buddy->mHasAutoAcceptListEntry)
						{
							buddy->mHasAutoAcceptListEntry = false;
							stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteAutoAcceptRule.1\">"
								<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
								<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
								<< "</Request>\n\n\n";
						
							// If we just deleted an auto-accept entry, add a block list entry.
							if(!buddy->mHasBlockListEntry)
							{
								buddy->mHasBlockListEntry = true;
								stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.CreateBlockRule.1\">"
									<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
									<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
									<< "<PresenceOnly>1</PresenceOnly>"
									<< "</Request>\n\n\n";								
							}
						}
					}

					if(!buddy->mInSLFriends && !buddy->mInVivoxBuddies)
					{
						// Delete this entry from the local buddy list.  This should NOT invalidate the iterator,
						// since it has already been incremented to the next entry.
						deleteBuddy(buddy->mURI);
					}

				}
				writeString(stream.str());
			}
		}
	}
}

/////////////////////////////
// Response/Event handlers

void LLVoiceClient::connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle, std::string &versionID)
{	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.Create response failure: " << statusString << LL_ENDL;
		setState(stateConnectorFailed);
	}
	else
	{
		// Connector created, move forward.
		LL_INFOS("Voice") << "Connector.Create succeeded, Vivox SDK version is " << versionID << LL_ENDL;
		mAPIVersion = versionID;
		mConnectorHandle = connectorHandle;
		if(getState() == stateConnectorStarting)
		{
			setState(stateConnectorStarted);
		}
	}
}

void LLVoiceClient::loginResponse(int statusCode, std::string &statusString, std::string &accountHandle, int numberOfAliases)
{ 
	LL_DEBUGS("Voice") << "Account.Login response (" << statusCode << "): " << statusString << LL_ENDL;
	
	// Status code of 20200 means "bad password".  We may want to special-case that at some point.
	
	if ( statusCode == 401 )
	{
		// Login failure which is probably caused by the delay after a user's password being updated.
		LL_INFOS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
		setState(stateLoginRetry);
	}
	else if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
		setState(stateLoginFailed);
	}
	else
	{
		// Login succeeded, move forward.
		mAccountHandle = accountHandle;
		mNumberOfAliases = numberOfAliases;
		// This needs to wait until the AccountLoginStateChangeEvent is received.
//		if(getState() == stateLoggingIn)
//		{
//			setState(stateLoggedIn);
//		}
	}
}

void LLVoiceClient::sessionCreateResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{	
	sessionState *session = findSessionBeingCreatedByURI(requestId);
	
	if(session)
	{
		session->mCreateInProgress = false;
	}
	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Session.Create response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mErrorStatusCode = statusCode;		
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
			{
				setState(stateJoinSessionFailed);
			}
			else
			{
				reapSession(session);
			}
		}
	}
	else
	{
		LL_INFOS("Voice") << "Session.Create response received (success), session handle is " << sessionHandle << LL_ENDL;
		if(session)
		{
			setSessionHandle(session, sessionHandle);
		}
	}
}

void LLVoiceClient::sessionGroupAddSessionResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{	
	sessionState *session = findSessionBeingCreatedByURI(requestId);
	
	if(session)
	{
		session->mCreateInProgress = false;
	}
	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "SessionGroup.AddSession response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mErrorStatusCode = statusCode;		
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
			{
				setState(stateJoinSessionFailed);
			}
			else
			{
				reapSession(session);
			}
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "SessionGroup.AddSession response received (success), session handle is " << sessionHandle << LL_ENDL;
		if(session)
		{
			setSessionHandle(session, sessionHandle);
		}
	}
}

void LLVoiceClient::sessionConnectResponse(std::string &requestId, int statusCode, std::string &statusString)
{
	sessionState *session = findSession(requestId);
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Session.Connect response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mMediaConnectInProgress = false;
			session->mErrorStatusCode = statusCode;		
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
				setState(stateJoinSessionFailed);
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Session.Connect response received (success)" << LL_ENDL;
	}
}

void LLVoiceClient::logoutResponse(int statusCode, std::string &statusString)
{	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Logout response failure: " << statusString << LL_ENDL;
		// Should this ever fail?  do we care if it does?
	}
}

void LLVoiceClient::connectorShutdownResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.InitiateShutdown response failure: " << statusString << LL_ENDL;
		// Should this ever fail?  do we care if it does?
	}
	
	mConnected = false;
	
	if(getState() == stateConnectorStopping)
	{
		setState(stateConnectorStopped);
	}
}

void LLVoiceClient::sessionAddedEvent(
		std::string &uriString, 
		std::string &alias, 
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		bool isChannel, 
		bool incoming,
		std::string &nameString,
		std::string &applicationString)
{
	sessionState *session = NULL;

	LL_INFOS("Voice") << "session " << uriString << ", alias " << alias << ", name " << nameString << " handle " << sessionHandle << LL_ENDL;
	
	session = addSession(uriString, sessionHandle);
	if(session)
	{
		session->mGroupHandle = sessionGroupHandle;
		session->mIsChannel = isChannel;
		session->mIncoming = incoming;
		session->mAlias = alias;
			
		// Generate a caller UUID -- don't need to do this for channels
		if(!session->mIsChannel)
		{
			if(IDFromName(session->mSIPURI, session->mCallerID))
			{
				// Normal URI(base64-encoded UUID) 
			}
			else if(!session->mAlias.empty() && IDFromName(session->mAlias, session->mCallerID))
			{
				// Wrong URI, but an alias is available.  Stash the incoming URI as an alternate
				session->mAlternateSIPURI = session->mSIPURI;
				
				// and generate a proper URI from the ID.
				setSessionURI(session, sipURIFromID(session->mCallerID));
			}
			else
			{
				LL_INFOS("Voice") << "Could not generate caller id from uri, using hash of uri " << session->mSIPURI << LL_ENDL;
				setUUIDFromStringHash(session->mCallerID, session->mSIPURI);
				session->mSynthesizedCallerID = true;
				
				// Can't look up the name in this case -- we have to extract it from the URI.
				std::string namePortion = nameFromsipURI(session->mSIPURI);
				if(namePortion.empty())
				{
					// Didn't seem to be a SIP URI, just use the whole provided name.
					namePortion = nameString;
				}
				
				// Some incoming names may be separated with an underscore instead of a space.  Fix this.
				LLStringUtil::replaceChar(namePortion, '_', ' ');
				
				// Act like we just finished resolving the name (this stores it in all the right places)
				avatarNameResolved(session->mCallerID, namePortion);
			}
		
			LL_INFOS("Voice") << "caller ID: " << session->mCallerID << LL_ENDL;

			if(!session->mSynthesizedCallerID)
			{
				// If we got here, we don't have a proper name.  Initiate a lookup.
				lookupName(session->mCallerID);
			}
		}
	}
}

void LLVoiceClient::sessionGroupAddedEvent(std::string &sessionGroupHandle)
{
	LL_DEBUGS("Voice") << "handle " << sessionGroupHandle << LL_ENDL;
	
#if USE_SESSION_GROUPS
	if(mMainSessionGroupHandle.empty())
	{
		// This is the first (i.e. "main") session group.  Save its handle.
		mMainSessionGroupHandle = sessionGroupHandle;
	}
	else
	{
		LL_DEBUGS("Voice") << "Already had a session group handle " << mMainSessionGroupHandle << LL_ENDL;
	}
#endif
}

void LLVoiceClient::joinedAudioSession(sessionState *session)
{
	if(mAudioSession != session)
	{
		sessionState *oldSession = mAudioSession;

		mAudioSession = session;
		mAudioSessionChanged = true;

		// The old session may now need to be deleted.
		reapSession(oldSession);
	}
	
	// This is the session we're joining.
	if(getState() == stateJoiningSession)
	{
		setState(stateSessionJoined);
		
		// SLIM SDK: we don't always receive a participant state change for ourselves when joining a channel now.
		// Add the current user as a participant here.
		participantState *participant = session->addParticipant(sipURIFromName(mAccountName));
		if(participant)
		{
			participant->mIsSelf = true;
			lookupName(participant->mAvatarID);

			LL_INFOS("Voice") << "added self as participant \"" << participant->mAccountName 
					<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;
		}
		
		if(!session->mIsChannel)
		{
			// this is a p2p session.  Make sure the other end is added as a participant.
			participantState *participant = session->addParticipant(session->mSIPURI);
			if(participant)
			{
				if(participant->mAvatarIDValid)
				{
					lookupName(participant->mAvatarID);
				}
				else if(!session->mName.empty())
				{
					participant->mDisplayName = session->mName;
					avatarNameResolved(participant->mAvatarID, session->mName);
				}
				
				// TODO: Question: Do we need to set up mAvatarID/mAvatarIDValid here?
				LL_INFOS("Voice") << "added caller as participant \"" << participant->mAccountName 
						<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;
			}
		}
	}
}

void LLVoiceClient::sessionRemovedEvent(
	std::string &sessionHandle, 
	std::string &sessionGroupHandle)
{
	LL_INFOS("Voice") << "handle " << sessionHandle << LL_ENDL;
	
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		leftAudioSession(session);

		// This message invalidates the session's handle.  Set it to empty.
		setSessionHandle(session);
		
		// This also means that the session's session group is now empty.
		// Terminate the session group so it doesn't leak.
		sessionGroupTerminateSendMessage(session);
		
		// Reset the media state (we now have no info)
		session->mMediaStreamState = streamStateUnknown;
		session->mTextStreamState = streamStateUnknown;
		
		// Conditionally delete the session
		reapSession(session);
	}
	else
	{
		LL_WARNS("Voice") << "unknown session " << sessionHandle << " removed" << LL_ENDL;
	}
}

void LLVoiceClient::reapSession(sessionState *session)
{
	if(session)
	{
		if(!session->mHandle.empty())
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (non-null session handle)" << LL_ENDL;
		}
		else if(session->mCreateInProgress)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (create in progress)" << LL_ENDL;
		}
		else if(session->mMediaConnectInProgress)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (connect in progress)" << LL_ENDL;
		}
		else if(session == mAudioSession)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (it's the current session)" << LL_ENDL;
		}
		else if(session == mNextAudioSession)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (it's the next session)" << LL_ENDL;
		}
		else
		{
			// TODO: Question: Should we check for queued text messages here?
			// We don't have a reason to keep tracking this session, so just delete it.
			LL_DEBUGS("Voice") << "deleting session " << session->mSIPURI << LL_ENDL;
			deleteSession(session);
			session = NULL;
		}	
	}
	else
	{
//		LL_DEBUGS("Voice") << "session is NULL" << LL_ENDL;
	}
}

// Returns true if the session seems to indicate we've moved to a region on a different voice server
bool LLVoiceClient::sessionNeedsRelog(sessionState *session)
{
	bool result = false;
	
	if(session != NULL)
	{
		// Only make this check for spatial channels (so it won't happen for group or p2p calls)
		if(session->mIsSpatial)
		{
			std::string::size_type atsign;
			
			atsign = session->mSIPURI.find("@");
			
			if(atsign != std::string::npos)
			{
				std::string urihost = session->mSIPURI.substr(atsign + 1);
				if(stricmp(urihost.c_str(), mVoiceSIPURIHostName.c_str()))
				{
					// The hostname in this URI is different from what we expect.  This probably means we need to relog.
					
					// We could make a ProvisionVoiceAccountRequest and compare the result with the current values of
					// mVoiceSIPURIHostName and mVoiceAccountServerURI to be really sure, but this is a pretty good indicator.
					
					result = true;
				}
			}
		}
	}
	
	return result;
}

void LLVoiceClient::leftAudioSession(
	sessionState *session)
{
	if(mAudioSession == session)
	{
		switch(getState())
		{
			case stateJoiningSession:
			case stateSessionJoined:
			case stateRunning:
			case stateLeavingSession:
			case stateJoinSessionFailed:
			case stateJoinSessionFailedWaiting:
				// normal transition
				LL_DEBUGS("Voice") << "left session " << session->mHandle << " in state " << state2string(getState()) << LL_ENDL;
				setState(stateSessionTerminated);
			break;
			
			case stateSessionTerminated:
				// this will happen sometimes -- there are cases where we send the terminate and then go straight to this state.
				LL_WARNS("Voice") << "left session " << session->mHandle << " in state " << state2string(getState()) << LL_ENDL;
			break;
			
			default:
				LL_WARNS("Voice") << "unexpected SessionStateChangeEvent (left session) in state " << state2string(getState()) << LL_ENDL;
				setState(stateSessionTerminated);
			break;
		}
	}
}

void LLVoiceClient::accountLoginStateChangeEvent(
		std::string &accountHandle, 
		int statusCode, 
		std::string &statusString, 
		int state)
{
	LL_DEBUGS("Voice") << "state is " << state << LL_ENDL;
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

		case 3:
			// The user is in the process of logging out.
			setState(stateLoggingOut);
		break;

		case 0:
			// The user has been logged out.  
			setState(stateLoggedOut);
		break;
		
		default:
			//Used to be a commented out warning
			LL_DEBUGS("Voice") << "unknown state: " << state << LL_ENDL;
		break;
	}
}

void LLVoiceClient::mediaStreamUpdatedEvent(
	std::string &sessionHandle, 
	std::string &sessionGroupHandle, 
	int statusCode, 
	std::string &statusString, 
	int state, 
	bool incoming)
{
	sessionState *session = findSession(sessionHandle);
	
	LL_DEBUGS("Voice") << "session " << sessionHandle << ", status code " << statusCode << ", string \"" << statusString << "\"" << LL_ENDL;
	
	if(session)
	{
		// We know about this session
		
		// Save the state for later use
		session->mMediaStreamState = state;
		
		switch(statusCode)
		{
			case 0:
			case 200:
				// generic success
				// Don't change the saved error code (it may have been set elsewhere)
			break;
			default:
				// save the status code for later
				session->mErrorStatusCode = statusCode;
			break;
		}
		
		switch(state)
		{
			case streamStateIdle:
				// Standard "left audio session"
				session->mVoiceEnabled = false;
				session->mMediaConnectInProgress = false;
				leftAudioSession(session);
			break;

			case streamStateConnected:
				session->mVoiceEnabled = true;
				session->mMediaConnectInProgress = false;
				joinedAudioSession(session);
			break;
			
			case streamStateRinging:
				if(incoming)
				{
					// Send the voice chat invite to the GUI layer
					// *TODO: Question: Should we correlate with the mute list here?
					session->mIMSessionID = LLIMMgr::computeSessionID(IM_SESSION_P2P_INVITE, session->mCallerID);
					session->mVoiceInvitePending = true;
					if(session->mName.empty())
					{
						lookupName(session->mCallerID);
					}
					else
					{
						// Act like we just finished resolving the name
						avatarNameResolved(session->mCallerID, session->mName);
					}
				}
			break;
			
			default:
				LL_WARNS("Voice") << "unknown state " << state << LL_ENDL;
			break;
			
		}
		
	}
	else
	{
		LL_WARNS("Voice") << "session " << sessionHandle << "not found"<< LL_ENDL;
	}
}

void LLVoiceClient::textStreamUpdatedEvent(
	std::string &sessionHandle, 
	std::string &sessionGroupHandle, 
	bool enabled,
	int state, 
	bool incoming)
{
	sessionState *session = findSession(sessionHandle);
	
	if(session)
	{
		// Save the state for later use
		session->mTextStreamState = state;
		
		// We know about this session
		switch(state)
		{
			case 0:	// We see this when the text stream closes
				LL_DEBUGS("Voice") << "stream closed" << LL_ENDL;
			break;
			
			case 1:	// We see this on an incoming call from the Connector
				// Try to send any text messages queued for this session.
				sendQueuedTextMessages(session);

				// Send the text chat invite to the GUI layer
				// TODO: Question: Should we correlate with the mute list here?
				session->mTextInvitePending = true;
				if(session->mName.empty())
				{
					lookupName(session->mCallerID);
				}
				else
				{
					// Act like we just finished resolving the name
					avatarNameResolved(session->mCallerID, session->mName);
				}
			break;

			default:
				LL_WARNS("Voice") << "unknown state " << state << LL_ENDL;
			break;
			
		}
	}
}

void LLVoiceClient::participantAddedEvent(
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		std::string &uriString, 
		std::string &alias, 
		std::string &nameString, 
		std::string &displayNameString, 
		int participantType)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->addParticipant(uriString);
		if(participant)
		{
			participant->mAccountName = nameString;

			LL_DEBUGS("Voice") << "added participant \"" << participant->mAccountName 
					<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;

			if(participant->mAvatarIDValid)
			{
				// Initiate a lookup
				lookupName(participant->mAvatarID);
			}
			else
			{
				// If we don't have a valid avatar UUID, we need to fill in the display name to make the active speakers floater work.
				std::string namePortion = nameFromsipURI(uriString);
				if(namePortion.empty())
				{
					// Problem with the SIP URI, fall back to the display name
					namePortion = displayNameString;
				}
				if(namePortion.empty())
				{
					// Problems with both of the above, fall back to the account name
					namePortion = nameString;
				}
				
				// Set the display name (which is a hint to the active speakers window not to do its own lookup)
				participant->mDisplayName = namePortion;
				avatarNameResolved(participant->mAvatarID, namePortion);
			}
		}
	}
}

void LLVoiceClient::participantRemovedEvent(
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		std::string &uriString, 
		std::string &alias, 
		std::string &nameString)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
		if(participant)
		{
			session->removeParticipant(participant);
		}
		else
		{
			LL_DEBUGS("Voice") << "unknown participant " << uriString << LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "unknown session " << sessionHandle << LL_ENDL;
	}
}


void LLVoiceClient::participantUpdatedEvent(
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		std::string &uriString, 
		std::string &alias, 
		bool isModeratorMuted, 
		bool isSpeaking, 
		int volume, 
		F32 energy)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
		
		if(participant)
		{
			participant->mIsSpeaking = isSpeaking;
			participant->mIsModeratorMuted = isModeratorMuted;

			// SLIM SDK: convert range: ensure that energy is set to zero if is_speaking is false
			if (isSpeaking)
			{
				participant->mSpeakingTimeout.reset();
				participant->mPower = energy;
			}
			else
			{
				participant->mPower = 0.0f;
			}
			participant->mVolume = volume;

			
			// *HACK: mantipov: added while working on EXT-3544
			/*
			Sometimes LLVoiceClient::participantUpdatedEvent callback is called BEFORE 
			LLViewerChatterBoxSessionAgentListUpdates::post() sometimes AFTER.
			
			participantUpdatedEvent updates voice participant state in particular participantState::mIsModeratorMuted
			Originally we wanted to update session Speaker Manager to fire LLSpeakerVoiceModerationEvent to fix the EXT-3544 bug.
			Calling of the LLSpeakerMgr::update() method was added into LLIMMgr::processAgentListUpdates.
			
			But in case participantUpdatedEvent() is called after LLViewerChatterBoxSessionAgentListUpdates::post()
			voice participant mIsModeratorMuted is changed after speakers are updated in Speaker Manager
			and event is not fired.

			So, we have to call LLSpeakerMgr::update() here. In any case it is better than call it
			in LLCallFloater::draw()
			*/
			LLVoiceChannel* voice_cnl = LLVoiceChannel::getCurrentVoiceChannel();

			// ignore session ID of local chat
			if (voice_cnl && voice_cnl->getSessionID().notNull())
			{
				LLSpeakerMgr* speaker_manager = LLIMModel::getInstance()->getSpeakerManager(voice_cnl->getSessionID());
				if (speaker_manager)
				{
					speaker_manager->update(true);
				}
			}
		}
		else
		{
			LL_WARNS("Voice") << "unknown participant: " << uriString << LL_ENDL;
		}
	}
	else
	{
		LL_INFOS("Voice") << "unknown session " << sessionHandle << LL_ENDL;
	}
}

void LLVoiceClient::buddyPresenceEvent(
		std::string &uriString, 
		std::string &alias, 
		std::string &statusString,
		std::string &applicationString)
{
	buddyListEntry *buddy = findBuddy(uriString);
	
	if(buddy)
	{
		LL_DEBUGS("Voice") << "Presence event for " << buddy->mDisplayName << " status \"" << statusString << "\", application \"" << applicationString << "\""<< LL_ENDL;
		LL_DEBUGS("Voice") << "before: mOnlineSL = " << (buddy->mOnlineSL?"true":"false") << ", mOnlineSLim = " << (buddy->mOnlineSLim?"true":"false") << LL_ENDL;

		if(applicationString.empty())
		{
			// This presence event is from a client that doesn't set up the Application string.  Do things the old-skool way.
			// NOTE: this will be needed to support people who aren't on the 3010-class SDK yet.

			if ( stricmp("Unknown", statusString.c_str())== 0) 
			{
				// User went offline with a non-SLim-enabled viewer.
				buddy->mOnlineSL = false;
			}
			else if ( stricmp("Online", statusString.c_str())== 0) 
			{
				// User came online with a non-SLim-enabled viewer.
				buddy->mOnlineSL = true;
			}
			else
			{
				// If the user is online through SLim, their status will be "Online-slc", "Away", or something else.
				// NOTE: we should never see this unless someone is running an OLD version of SLim -- the versions that should be in use now all set the application string.
				buddy->mOnlineSLim = true;
			} 
		}
		else if(applicationString.find("SecondLifeViewer") != std::string::npos)
		{
			// This presence event is from a viewer that sets the application string
			if ( stricmp("Unknown", statusString.c_str())== 0) 
			{
				// Viewer says they're offline
				buddy->mOnlineSL = false;
			}
			else
			{
				// Viewer says they're online
				buddy->mOnlineSL = true;
			}
		}
		else
		{
			// This presence event is from something which is NOT the SL viewer (assume it's SLim).
			if ( stricmp("Unknown", statusString.c_str())== 0) 
			{
				// SLim says they're offline
				buddy->mOnlineSLim = false;
			}
			else
			{
				// SLim says they're online
				buddy->mOnlineSLim = true;
			}
		} 

		LL_DEBUGS("Voice") << "after: mOnlineSL = " << (buddy->mOnlineSL?"true":"false") << ", mOnlineSLim = " << (buddy->mOnlineSLim?"true":"false") << LL_ENDL;
		
		// HACK -- increment the internal change serial number in the LLRelationship (without changing the actual status), so the UI notices the change.
		LLAvatarTracker::instance().setBuddyOnline(buddy->mUUID,LLAvatarTracker::instance().isBuddyOnline(buddy->mUUID));

		notifyFriendObservers();
	}
	else
	{
		LL_DEBUGS("Voice") << "Presence for unknown buddy " << uriString << LL_ENDL;
	}	
}

void LLVoiceClient::messageEvent(
		std::string &sessionHandle, 
		std::string &uriString, 
		std::string &alias, 
		std::string &messageHeader, 
		std::string &messageBody,
		std::string &applicationString)
{
	LL_DEBUGS("Voice") << "Message event, session " << sessionHandle << " from " << uriString << LL_ENDL;
//	LL_DEBUGS("Voice") << "    header " << messageHeader << ", body: \n" << messageBody << LL_ENDL;
	
	if(messageHeader.find("text/html") != std::string::npos)
	{
		std::string message;

		{
			const std::string startMarker = "<body";
			const std::string startMarker2 = ">";
			const std::string endMarker = "</body>";
			const std::string startSpan = "<span";
			const std::string endSpan = "</span>";
			std::string::size_type start;
			std::string::size_type end;
			
			// Default to displaying the raw string, so the message gets through.
			message = messageBody;

			// Find the actual message text within the XML fragment
			start = messageBody.find(startMarker);
			start = messageBody.find(startMarker2, start);
			end = messageBody.find(endMarker);

			if(start != std::string::npos)
			{
				start += startMarker2.size();
				
				if(end != std::string::npos)
					end -= start;
					
				message.assign(messageBody, start, end);
			}
			else 
			{
				// Didn't find a <body>, try looking for a <span> instead.
				start = messageBody.find(startSpan);
				start = messageBody.find(startMarker2, start);
				end = messageBody.find(endSpan);
				
				if(start != std::string::npos)
				{
					start += startMarker2.size();
					
					if(end != std::string::npos)
						end -= start;
					
					message.assign(messageBody, start, end);
				}			
			}
		}	
		
//		LL_DEBUGS("Voice") << "    raw message = \n" << message << LL_ENDL;

		// strip formatting tags
		{
			std::string::size_type start;
			std::string::size_type end;
			
			while((start = message.find('<')) != std::string::npos)
			{
				if((end = message.find('>', start + 1)) != std::string::npos)
				{
					// Strip out the tag
					message.erase(start, (end + 1) - start);
				}
				else
				{
					// Avoid an infinite loop
					break;
				}
			}
		}
		
		// Decode ampersand-escaped chars
		{
			std::string::size_type mark = 0;

			// The text may contain text encoded with &lt;, &gt;, and &amp;
			mark = 0;
			while((mark = message.find("&lt;", mark)) != std::string::npos)
			{
				message.replace(mark, 4, "<");
				mark += 1;
			}
			
			mark = 0;
			while((mark = message.find("&gt;", mark)) != std::string::npos)
			{
				message.replace(mark, 4, ">");
				mark += 1;
			}
			
			mark = 0;
			while((mark = message.find("&amp;", mark)) != std::string::npos)
			{
				message.replace(mark, 5, "&");
				mark += 1;
			}
		}
		
		// strip leading/trailing whitespace (since we always seem to get a couple newlines)
		LLStringUtil::trim(message);
		
//		LL_DEBUGS("Voice") << "    stripped message = \n" << message << LL_ENDL;
		
		sessionState *session = findSession(sessionHandle);
		if(session)
		{
			bool is_busy = gAgent.getBusy();
			bool is_muted = LLMuteList::getInstance()->isMuted(session->mCallerID, session->mName, LLMute::flagTextChat);
			bool is_linden = LLMuteList::getInstance()->isLinden(session->mName);
			bool quiet_chat = false;
			LLChat chat;

			chat.mMuted = is_muted && !is_linden;
			
			if(!chat.mMuted)
			{
				chat.mFromID = session->mCallerID;
				chat.mFromName = session->mName;
				chat.mSourceType = CHAT_SOURCE_AGENT;

				if(is_busy && !is_linden)
				{
					quiet_chat = true;
					// TODO: Question: Return busy mode response here?  Or maybe when session is started instead?
				}
								
				LL_DEBUGS("Voice") << "adding message, name " << session->mName << " session " << session->mIMSessionID << ", target " << session->mCallerID << LL_ENDL;
				gIMMgr->addMessage(session->mIMSessionID,
						session->mCallerID,
						session->mName.c_str(),
						message.c_str(),
						LLStringUtil::null,		// default arg
						IM_NOTHING_SPECIAL,		// default arg
						0,						// default arg
						LLUUID::null,			// default arg
						LLVector3::zero,		// default arg
						true);					// prepend name and make it a link to the user's profile
			}
		}		
	}
}

void LLVoiceClient::sessionNotificationEvent(std::string &sessionHandle, std::string &uriString, std::string &notificationType)
{
	sessionState *session = findSession(sessionHandle);
	
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
		if(participant)
		{
			if (!stricmp(notificationType.c_str(), "Typing"))
			{
				// Other end started typing
				// TODO: The proper way to add a typing notification seems to be LLIMMgr::processIMTypingStart().
				// It requires an LLIMInfo for the message, which we don't have here.
			}
			else if (!stricmp(notificationType.c_str(), "NotTyping"))
			{
				// Other end stopped typing
				// TODO: The proper way to remove a typing notification seems to be LLIMMgr::processIMTypingStop().
				// It requires an LLIMInfo for the message, which we don't have here.
			}
			else
			{
				LL_DEBUGS("Voice") << "Unknown notification type " << notificationType << "for participant " << uriString << " in session " << session->mSIPURI << LL_ENDL;
			}
		}
		else
		{
			LL_DEBUGS("Voice") << "Unknown participant " << uriString << " in session " << session->mSIPURI << LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Unknown session handle " << sessionHandle << LL_ENDL;
	}
}

void LLVoiceClient::subscriptionEvent(std::string &buddyURI, std::string &subscriptionHandle, std::string &alias, std::string &displayName, std::string &applicationString, std::string &subscriptionType)
{
	buddyListEntry *buddy = findBuddy(buddyURI);
	
	if(!buddy)
	{
		// Couldn't find buddy by URI, try converting the alias...
		if(!alias.empty())
		{
			LLUUID id;
			if(IDFromName(alias, id))
			{
				buddy = findBuddy(id);
			}
		}
	}
	
	if(buddy)
	{
		std::ostringstream stream;
		
		if(buddy->mCanSeeMeOnline)
		{
			// Sending the response will create an auto-accept rule
			buddy->mHasAutoAcceptListEntry = true;
		}
		else
		{
			// Sending the response will create a block rule
			buddy->mHasBlockListEntry = true;
		}
		
		if(buddy->mInSLFriends)
		{
			buddy->mInVivoxBuddies = true;
		}
		
		stream
			<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.SendSubscriptionReply.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
				<< "<RuleType>" << (buddy->mCanSeeMeOnline?"Allow":"Hide") << "</RuleType>"
				<< "<AutoAccept>"<< (buddy->mInSLFriends?"1":"0")<< "</AutoAccept>"
				<< "<SubscriptionHandle>" << subscriptionHandle << "</SubscriptionHandle>"
			<< "</Request>"
			<< "\n\n\n";
			
		writeString(stream.str());
	}
}

void LLVoiceClient::auxAudioPropertiesEvent(F32 energy)
{
	LL_DEBUGS("Voice") << "got energy " << energy << LL_ENDL;
	mTuningEnergy = energy;
}

void LLVoiceClient::buddyListChanged()
{
	// This is called after we receive a BuddyAndGroupListChangedEvent.
	mBuddyListMapPopulated = true;
	mFriendsListDirty = true;
}

void LLVoiceClient::muteListChanged()
{
	// The user's mute list has been updated.  Go through the current participant list and sync it with the mute list.
	if(mAudioSession)
	{
		participantMap::iterator iter = mAudioSession->mParticipantsByURI.begin();
		
		for(; iter != mAudioSession->mParticipantsByURI.end(); iter++)
		{
			participantState *p = iter->second;
			
			// Check to see if this participant is on the mute list already
			if(p->updateMuteState())
				mAudioSession->mVolumeDirty = true;
		}
	}
}

void LLVoiceClient::updateFriends(U32 mask)
{
	if(mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::POWERS))
	{
		// Just resend the whole friend list to the daemon
		mFriendsListDirty = true;
	}
}

/////////////////////////////
// Managing list of participants
LLVoiceClient::participantState::participantState(const std::string &uri) : 
	 mURI(uri), 
	 mPTT(false), 
	 mIsSpeaking(false), 
	 mIsModeratorMuted(false), 
	 mLastSpokeTimestamp(0.f), 
	 mPower(0.f), 
	 mVolume(-1), 
	 mOnMuteList(false), 
	 mUserVolume(-1), 
	 mVolumeDirty(false), 
	 mAvatarIDValid(false),
	 mIsSelf(false)
{
}

LLVoiceClient::participantState *LLVoiceClient::sessionState::addParticipant(const std::string &uri)
{
	participantState *result = NULL;
	bool useAlternateURI = false;
	
	// Note: this is mostly the body of LLVoiceClient::sessionState::findParticipant(), but since we need to know if it
	// matched the alternate SIP URI (so we can add it properly), we need to reproduce it here.
	{
		participantMap::iterator iter = mParticipantsByURI.find(&uri);

		if(iter == mParticipantsByURI.end())
		{
			if(!mAlternateSIPURI.empty() && (uri == mAlternateSIPURI))
			{
				// This is a p2p session (probably with the SLIM client) with an alternate URI for the other participant.
				// Use mSIPURI instead, since it will be properly encoded.
				iter = mParticipantsByURI.find(&(mSIPURI));
				useAlternateURI = true;
			}
		}

		if(iter != mParticipantsByURI.end())
		{
			result = iter->second;
		}
	}
		
	if(!result)
	{
		// participant isn't already in one list or the other.
		result = new participantState(useAlternateURI?mSIPURI:uri);
		mParticipantsByURI.insert(participantMap::value_type(&(result->mURI), result));
		mParticipantsChanged = true;
		
		// Try to do a reverse transform on the URI to get the GUID back.
		{
			LLUUID id;
			if(IDFromName(result->mURI, id))
			{
				result->mAvatarIDValid = true;
				result->mAvatarID = id;

				if(result->updateMuteState())
					mVolumeDirty = true;
			}
			else
			{
				// Create a UUID by hashing the URI, but do NOT set mAvatarIDValid.
				// This tells both code in LLVoiceClient and code in llfloateractivespeakers.cpp that the ID will not be in the name cache.
				setUUIDFromStringHash(result->mAvatarID, uri);
			}
		}
		
		mParticipantsByUUID.insert(participantUUIDMap::value_type(&(result->mAvatarID), result));

		result->mUserVolume = LLSpeakerVolumeStorage::getInstance()->getSpeakerVolume(result->mAvatarID);

		LL_DEBUGS("Voice") << "participant \"" << result->mURI << "\" added." << LL_ENDL;
	}
	
	return result;
}

bool LLVoiceClient::participantState::updateMuteState()
{
	bool result = false;
	
	if(mAvatarIDValid)
	{
		bool isMuted = LLMuteList::getInstance()->isMuted(mAvatarID, LLMute::flagVoiceChat);
		if(mOnMuteList != isMuted)
		{
			mOnMuteList = isMuted;
			mVolumeDirty = true;
			result = true;
		}
	}
	return result;
}

bool LLVoiceClient::participantState::isAvatar()
{
	return mAvatarIDValid;
}

void LLVoiceClient::sessionState::removeParticipant(LLVoiceClient::participantState *participant)
{
	if(participant)
	{
		participantMap::iterator iter = mParticipantsByURI.find(&(participant->mURI));
		participantUUIDMap::iterator iter2 = mParticipantsByUUID.find(&(participant->mAvatarID));
		
		LL_DEBUGS("Voice") << "participant \"" << participant->mURI <<  "\" (" << participant->mAvatarID << ") removed." << LL_ENDL;
		
		if(iter == mParticipantsByURI.end())
		{
			LL_ERRS("Voice") << "Internal error: participant " << participant->mURI << " not in URI map" << LL_ENDL;
		}
		else if(iter2 == mParticipantsByUUID.end())
		{
			LL_ERRS("Voice") << "Internal error: participant ID " << participant->mAvatarID << " not in UUID map" << LL_ENDL;
		}
		else if(iter->second != iter2->second)
		{
			LL_ERRS("Voice") << "Internal error: participant mismatch!" << LL_ENDL;
		}
		else
		{
			mParticipantsByURI.erase(iter);
			mParticipantsByUUID.erase(iter2);
			
			delete participant;
			mParticipantsChanged = true;
		}
	}
}

void LLVoiceClient::sessionState::removeAllParticipants()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;

	while(!mParticipantsByURI.empty())
	{
		removeParticipant(mParticipantsByURI.begin()->second);
	}
	
	if(!mParticipantsByUUID.empty())
	{
		LL_ERRS("Voice") << "Internal error: empty URI map, non-empty UUID map" << LL_ENDL;
	}
}

LLVoiceClient::participantMap *LLVoiceClient::getParticipantList(void)
{
	participantMap *result = NULL;
	if(mAudioSession)
	{
		result = &(mAudioSession->mParticipantsByURI);
	}
	return result;
}

void LLVoiceClient::getParticipantsUUIDSet(std::set<LLUUID>& participant_uuids)
{
	if (NULL == mAudioSession) return;

	participantUUIDMap::const_iterator it = mAudioSession->mParticipantsByUUID.begin(),
		it_end = mAudioSession->mParticipantsByUUID.end();
	for (; it != it_end; ++it)
	{
		participant_uuids.insert((*(*it).first));
	}
}

LLVoiceClient::participantState *LLVoiceClient::sessionState::findParticipant(const std::string &uri)
{
	participantState *result = NULL;
	
	participantMap::iterator iter = mParticipantsByURI.find(&uri);

	if(iter == mParticipantsByURI.end())
	{
		if(!mAlternateSIPURI.empty() && (uri == mAlternateSIPURI))
		{
			// This is a p2p session (probably with the SLIM client) with an alternate URI for the other participant.
			// Look up the other URI
			iter = mParticipantsByURI.find(&(mSIPURI));
		}
	}

	if(iter != mParticipantsByURI.end())
	{
		result = iter->second;
	}
		
	return result;
}

LLVoiceClient::participantState* LLVoiceClient::sessionState::findParticipantByID(const LLUUID& id)
{
	participantState * result = NULL;
	participantUUIDMap::iterator iter = mParticipantsByUUID.find(&id);

	if(iter != mParticipantsByUUID.end())
	{
		result = iter->second;
	}

	return result;
}

LLVoiceClient::participantState* LLVoiceClient::findParticipantByID(const LLUUID& id)
{
	participantState * result = NULL;
	
	if(mAudioSession)
	{
		result = mAudioSession->findParticipantByID(id);
	}
	
	return result;
}


void LLVoiceClient::parcelChanged()
{
	if(getState() >= stateNoChannel)
	{
		// If the user is logged in, start a channel lookup.
		LL_DEBUGS("Voice") << "sending ParcelVoiceInfoRequest (" << mCurrentRegionName << ", " << mCurrentParcelLocalID << ")" << LL_ENDL;

		std::string url = gAgent.getRegion()->getCapability("ParcelVoiceInfoRequest");
		LLSD data;
		LLHTTPClient::post(
			url,
			data,
			new LLVoiceClientCapResponder);
	}
	else
	{
		// The transition to stateNoChannel needs to kick this off again.
		LL_INFOS("Voice") << "not logged in yet, deferring" << LL_ENDL;
	}
}

void LLVoiceClient::switchChannel(
	std::string uri,
	bool spatial,
	bool no_reconnect,
	bool is_p2p,
	std::string hash)
{
	bool needsSwitch = false;
	
	LL_DEBUGS("Voice") 
		<< "called in state " << state2string(getState()) 
		<< " with uri \"" << uri << "\"" 
		<< (spatial?", spatial is true":", spatial is false")
		<< LL_ENDL;
	
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
				if(mNextAudioSession)
				{
					if(mNextAudioSession->mSIPURI != uri)
						needsSwitch = true;
				}
				else
				{
					// mNextAudioSession is null -- this probably means we're on our way back to spatial.
					if(!uri.empty())
					{
						// We do want to process a switch in this case.
						needsSwitch = true;
					}
				}
			}
			else
			{
				// Otherwise, compare against the URI we're in now.
				if(mAudioSession)
				{
					if(mAudioSession->mSIPURI != uri)
					{
						needsSwitch = true;
					}
				}
				else
				{
					if(!uri.empty())
					{
						// mAudioSession is null -- it's not clear what case would cause this.
						// For now, log it as a warning and see if it ever crops up.
						LL_WARNS("Voice") << "No current audio session." << LL_ENDL;
					}
				}
			}
		break;
	}
	
	if(needsSwitch)
	{
		if(uri.empty())
		{
			// Leave any channel we may be in
			LL_DEBUGS("Voice") << "leaving channel" << LL_ENDL;

			sessionState *oldSession = mNextAudioSession;
			mNextAudioSession = NULL;

			// The old session may now need to be deleted.
			reapSession(oldSession);

			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
		}
		else
		{
			LL_DEBUGS("Voice") << "switching to channel " << uri << LL_ENDL;

			mNextAudioSession = addSession(uri);
			mNextAudioSession->mHash = hash;
			mNextAudioSession->mIsSpatial = spatial;
			mNextAudioSession->mReconnect = !no_reconnect;
			mNextAudioSession->mIsP2P = is_p2p;
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

void LLVoiceClient::joinSession(sessionState *session)
{
	mNextAudioSession = session;
	
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
	switchChannel(uri, false, false, false, credentials);
}

void LLVoiceClient::setSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	mSpatialSessionURI = uri;
	mSpatialSessionCredentials = credentials;
	mAreaVoiceDisabled = mSpatialSessionURI.empty();

	LL_DEBUGS("Voice") << "got spatial channel uri: \"" << uri << "\"" << LL_ENDL;
	
	if((mAudioSession && !(mAudioSession->mIsSpatial)) || (mNextAudioSession && !(mNextAudioSession->mIsSpatial)))
	{
		// User is in a non-spatial chat or joining a non-spatial chat.  Don't switch channels.
		LL_INFOS("Voice") << "in non-spatial chat, not switching channels" << LL_ENDL;
	}
	else
	{
		switchChannel(mSpatialSessionURI, true, false, false, mSpatialSessionCredentials);
	}
}

void LLVoiceClient::callUser(const LLUUID &uuid)
{
	std::string userURI = sipURIFromID(uuid);

	switchChannel(userURI, false, true, true);
}

LLVoiceClient::sessionState* LLVoiceClient::startUserIMSession(const LLUUID &uuid)
{
	// Figure out if a session with the user already exists
	sessionState *session = findSession(uuid);
	if(!session)
	{
		// No session with user, need to start one.
		std::string uri = sipURIFromID(uuid);
		session = addSession(uri);
		session->mIsSpatial = false;
		session->mReconnect = false;	
		session->mIsP2P = true;
		session->mCallerID = uuid;
	}
	
	if(session)
	{
		if(session->mHandle.empty())
		{
			// Session isn't active -- start it up.
			sessionCreateSendMessage(session, false, true);
		}
		else
		{	
			// Session is already active -- start up text.
			sessionTextConnectSendMessage(session);
		}
	}
	
	return session;
}

bool LLVoiceClient::sendTextMessage(const LLUUID& participant_id, const std::string& message)
{
	bool result = false;

	// Attempt to locate the indicated session
	sessionState *session = startUserIMSession(participant_id);
	if(session)
	{
		// found the session, attempt to send the message
		session->mTextMsgQueue.push(message);
		
		// Try to send queued messages (will do nothing if the session is not open yet)
		sendQueuedTextMessages(session);

		// The message is queued, so we succeed.
		result = true;
	}	
	else
	{
		LL_DEBUGS("Voice") << "Session not found for participant ID " << participant_id << LL_ENDL;
	}
	
	return result;
}

void LLVoiceClient::sendQueuedTextMessages(sessionState *session)
{
	if(session->mTextStreamState == 1)
	{
		if(!session->mTextMsgQueue.empty())
		{
			std::ostringstream stream;
			
			while(!session->mTextMsgQueue.empty())
			{
				std::string message = session->mTextMsgQueue.front();
				session->mTextMsgQueue.pop();
				stream
				<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SendMessage.1\">"
					<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
					<< "<MessageHeader>text/HTML</MessageHeader>"
					<< "<MessageBody>" << message << "</MessageBody>"
				<< "</Request>"
				<< "\n\n\n";
			}		
			writeString(stream.str());
		}
	}
	else
	{
		// Session isn't connected yet, defer until later.
	}
}

void LLVoiceClient::endUserIMSession(const LLUUID &uuid)
{
	// Figure out if a session with the user exists
	sessionState *session = findSession(uuid);
	if(session)
	{
		// found the session
		if(!session->mHandle.empty())
		{
			sessionTextDisconnectSendMessage(session);
		}
	}	
	else
	{
		LL_DEBUGS("Voice") << "Session not found for participant ID " << uuid << LL_ENDL;
	}
}

bool LLVoiceClient::answerInvite(std::string &sessionHandle)
{
	// this is only ever used to answer incoming p2p call invites.
	
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		session->mIsSpatial = false;
		session->mReconnect = false;	
		session->mIsP2P = true;

		joinSession(session);
		return true;
	}
	
	return false;
}

bool LLVoiceClient::isOnlineSIP(const LLUUID &id)
{
	bool result = false;
	buddyListEntry *buddy = findBuddy(id);
	if(buddy)
	{
		result = buddy->mOnlineSLim;
		LL_DEBUGS("Voice") << "Buddy " << buddy->mDisplayName << " is SIP " << (result?"online":"offline") << LL_ENDL;
	}

	if(!result)
	{
		// This user isn't on the buddy list or doesn't show online status through the buddy list, but could be a participant in an existing session if they initiated a text IM.
		sessionState *session = findSession(id);
		if(session && !session->mHandle.empty())
		{
			if((session->mTextStreamState != streamStateUnknown) || (session->mMediaStreamState > streamStateIdle))
			{
				LL_DEBUGS("Voice") << "Open session with " << id << " found, returning SIP online state" << LL_ENDL;
				// we have a p2p text session open with this user, so by definition they're online.
				result = true;
			}
		}
	}
	
	return result;
}

// Returns true if the indicated participant in the current audio session is really an SL avatar.
// Currently this will be false only for PSTN callers into group chats, and PSTN p2p calls.
bool LLVoiceClient::isParticipantAvatar(const LLUUID &id)
{
	bool result = true; 
	sessionState *session = findSession(id);
	
	if(session != NULL)
	{
		// this is a p2p session with the indicated caller, or the session with the specified UUID.
		if(session->mSynthesizedCallerID)
			result = false;
	}
	else
	{
		// Didn't find a matching session -- check the current audio session for a matching participant
		if(mAudioSession != NULL)
		{
			participantState *participant = findParticipantByID(id);
			if(participant != NULL)
			{
				result = participant->isAvatar();
			}
		}
	}
	
	return result;
}

// Returns true if calling back the session URI after the session has closed is possible.
// Currently this will be false only for PSTN P2P calls.		
bool LLVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
	bool result = true; 
	sessionState *session = findSession(session_id);
	
	if(session != NULL)
	{
		result = session->isCallBackPossible();
	}
	
	return result;
}

// Returns true if the session can accepte text IM's.
// Currently this will be false only for PSTN P2P calls.
bool LLVoiceClient::isSessionTextIMPossible(const LLUUID &session_id)
{
	bool result = true; 
	sessionState *session = findSession(session_id);
	
	if(session != NULL)
	{
		result = session->isTextIMPossible();
	}
	
	return result;
}
		

void LLVoiceClient::declineInvite(std::string &sessionHandle)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		sessionMediaDisconnectSendMessage(session);
	}
}

void LLVoiceClient::leaveNonSpatialChannel()
{
	LL_DEBUGS("Voice") 
		<< "called in state " << state2string(getState()) 
		<< LL_ENDL;
	
	// Make sure we don't rejoin the current session.	
	sessionState *oldNextSession = mNextAudioSession;
	mNextAudioSession = NULL;
	
	// Most likely this will still be the current session at this point, but check it anyway.
	reapSession(oldNextSession);
	
	verifySessionState();
	
	sessionTerminate();
}

std::string LLVoiceClient::getCurrentChannel()
{
	std::string result;
	
	if((getState() == stateRunning) && !mSessionTerminateRequested)
	{
		result = getAudioSessionURI();
	}
	
	return result;
}

bool LLVoiceClient::inProximalChannel()
{
	bool result = false;
	
	if((getState() == stateRunning) && !mSessionTerminateRequested)
	{
		result = inSpatialChannel();
	}
	
	return result;
}

std::string LLVoiceClient::sipURIFromID(const LLUUID &id)
{
	std::string result;
	result = "sip:";
	result += nameFromID(id);
	result += "@";
	result += mVoiceSIPURIHostName;
	
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
		result += mVoiceSIPURIHostName;
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
	
	if (uuid.isNull()) {
		//VIVOX, the uuid emtpy look for the mURIString and return that instead.
		//result.assign(uuid.mURIStringName);
		LLStringUtil::replaceChar(result, '_', ' ');
		return result;
	}
	// Prepending this apparently prevents conflicts with reserved names inside the vivox and diamondware code.
	result = "x";
	
	// Base64 encode and replace the pieces of base64 that are less compatible 
	// with e-mail local-parts.
	// See RFC-4648 "Base 64 Encoding with URL and Filename Safe Alphabet"
	result += LLBase64::encode(uuid.mData, UUID_BYTES);
	LLStringUtil::replaceChar(result, '+', '-');
	LLStringUtil::replaceChar(result, '/', '_');
	
	// If you need to transform a GUID to this form on the Mac OS X command line, this will do so:
	// echo -n x && (echo e669132a-6c43-4ee1-a78d-6c82fff59f32 |xxd -r -p |openssl base64|tr '/+' '_-')
	
	// The reverse transform can be done with:
	// echo 'x5mkTKmxDTuGnjWyC__WfMg==' |cut -b 2- -|tr '_-' '/+' |openssl base64 -d|xxd -p
	
	return result;
}

bool LLVoiceClient::IDFromName(const std::string inName, LLUUID &uuid)
{
	bool result = false;
	
	// SLIM SDK: The "name" may actually be a SIP URI such as: "sip:xFnPP04IpREWNkuw1cOXlhw==@bhr.vivox.com"
	// If it is, convert to a bare name before doing the transform.
	std::string name = nameFromsipURI(inName);
	
	// Doesn't look like a SIP URI, assume it's an actual name.
	if(name.empty())
		name = inName;

	// This will only work if the name is of the proper form.
	// As an example, the account name for Monroe Linden (UUID 1673cfd3-8229-4445-8d92-ec3570e5e587) is:
	// "xFnPP04IpREWNkuw1cOXlhw=="
	
	if((name.size() == 25) && (name[0] == 'x') && (name[23] == '=') && (name[24] == '='))
	{
		// The name appears to have the right form.

		// Reverse the transforms done by nameFromID
		std::string temp = name;
		LLStringUtil::replaceChar(temp, '-', '+');
		LLStringUtil::replaceChar(temp, '_', '/');

		U8 rawuuid[UUID_BYTES + 1]; 
		int len = apr_base64_decode_binary(rawuuid, temp.c_str() + 1);
		if(len == UUID_BYTES)
		{
			// The decode succeeded.  Stuff the bits into the result's UUID
			memcpy(uuid.mData, rawuuid, UUID_BYTES);
			result = true;
		}
	} 
	
	if(!result)
	{
		// VIVOX:  not a standard account name, just copy the URI name mURIString field
		// and hope for the best.  bpj
		uuid.setNull();  // VIVOX, set the uuid field to nulls
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
	result += mVoiceSIPURIHostName;

//	LLStringUtil::toLower(result);

	return result;
}

std::string LLVoiceClient::nameFromsipURI(const std::string &uri)
{
	std::string result;

	std::string::size_type sipOffset, atOffset;
	sipOffset = uri.find("sip:");
	atOffset = uri.find("@");
	if((sipOffset != std::string::npos) && (atOffset != std::string::npos))
	{
		result = uri.substr(sipOffset + 4, atOffset - (sipOffset + 4));
	}
	
	return result;
}

bool LLVoiceClient::inSpatialChannel(void)
{
	bool result = false;
	
	if(mAudioSession)
		result = mAudioSession->mIsSpatial;
		
	return result;
}

std::string LLVoiceClient::getAudioSessionURI()
{
	std::string result;
	
	if(mAudioSession)
		result = mAudioSession->mSIPURI;
		
	return result;
}

std::string LLVoiceClient::getAudioSessionHandle()
{
	std::string result;
	
	if(mAudioSession)
		result = mAudioSession->mHandle;
		
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

void LLVoiceClient::updatePosition(void)
{
	
	if(gVoiceClient)
	{
		LLVOAvatar *agent = gAgent.getAvatarObject();
		LLViewerRegion *region = gAgent.getRegion();
		if(region && agent)
		{
			LLMatrix3 rot;
			LLVector3d pos;

			// TODO: If camera and avatar velocity are actually used by the voice system, we could compute them here...
			// They're currently always set to zero.

			// Send the current camera position to the voice code
			rot.setRows(LLViewerCamera::getInstance()->getAtAxis(), LLViewerCamera::getInstance()->getLeftAxis (),  LLViewerCamera::getInstance()->getUpAxis());		
			pos = gAgent.getRegion()->getPosGlobalFromRegion(LLViewerCamera::getInstance()->getOrigin());
			
			gVoiceClient->setCameraPosition(
					pos,				// position
					LLVector3::zero, 	// velocity
					rot);				// rotation matrix
					
			// Send the current avatar position to the voice code
			rot = agent->getRootJoint()->getWorldRotation().getMatrix3();
	
			pos = agent->getPositionGlobal();
			// TODO: Can we get the head offset from outside the LLVOAvatar?
//			pos += LLVector3d(mHeadOffset);
			pos += LLVector3d(0.f, 0.f, 1.f);
		
			gVoiceClient->setAvatarPosition(
					pos,				// position
					LLVector3::zero, 	// velocity
					rot);				// rotation matrix
		}
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
		LL_DEBUGS("Voice") << "leaving channel for teleport/logout" << LL_ENDL;
		mChannelName.clear();
		sessionTerminate();
	}
}

void LLVoiceClient::setMuteMic(bool muted)
{
	mMuteMic = muted;
}

bool LLVoiceClient::getMuteMic() const
{
	return mMuteMic;
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
		LLVoiceClientStatusObserver::EStatusType status;

		if (enabled)
		{
			LLVoiceChannel::getCurrentVoiceChannel()->activate();
			status = LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED;
		}
		else
		{
			// Turning voice off looses your current channel -- this makes sure the UI isn't out of sync when you re-enable it.
			LLVoiceChannel::getCurrentVoiceChannel()->deactivate();
			status = LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED;
		}

		notifyStatusObservers(status);
	}
}

bool LLVoiceClient::voiceEnabled()
{
	return gSavedSettings.getBOOL("EnableVoiceChat") && !gSavedSettings.getBOOL("CmdLineDisableVoice");
}

//AD *TODO: investigate possible merge of voiceWorking() and voiceEnabled() into one non-static method
bool LLVoiceClient::voiceWorking()
{
	//Added stateSessionTerminated state to avoid problems with call in parcels with disabled voice (EXT-4758)
	return (stateLoggedIn <= mState) && (mState <= stateSessionTerminated);
}

void LLVoiceClient::setLipSyncEnabled(BOOL enabled)
{
	mLipSyncEnabled = enabled;
}

BOOL LLVoiceClient::lipSyncEnabled()
{
	   
	if ( mVoiceEnabled && stateDisabled != getState() )
	{
		return mLipSyncEnabled;
	}
	else
	{
		return FALSE;
	}
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

bool LLVoiceClient::getPTTIsToggle()
{
	return mPTTIsToggle;
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
		LL_DEBUGS("Voice") << "Setting mEarLocation to " << loc << LL_ENDL;
		
		mEarLocation = loc;
		mSpatialCoordsDirty = true;
	}
}

void LLVoiceClient::setVoiceVolume(F32 volume)
{
	int scaled_volume = scale_speaker_volume(volume);	

	if(scaled_volume != mSpeakerVolume)
	{
		int min_volume = scale_speaker_volume(0);
		if((scaled_volume == min_volume) || (mSpeakerVolume == min_volume))
		{
			mSpeakerMuteDirty = true;
		}

		mSpeakerVolume = scaled_volume;
		mSpeakerVolumeDirty = true;
	}
}

void LLVoiceClient::setMicGain(F32 volume)
{
	int scaled_volume = scale_mic_volume(volume);
	
	if(scaled_volume != mMicVolume)
	{
		mMicVolume = scaled_volume;
		mMicVolumeDirty = true;
	}
}

void LLVoiceClient::keyDown(KEY key, MASK mask)
{	
	if (gKeyboard->getKeyRepeated(key))
	{
		// ignore auto-repeat keys
		return;
	}

	if(!mPTTIsMiddleMouse)
	{
		bool down = (mPTTKey != KEY_NONE)
			&& gKeyboard->getKeyDown(mPTTKey);
		inputUserControlState(down);
	}
}
void LLVoiceClient::keyUp(KEY key, MASK mask)
{
	if(!mPTTIsMiddleMouse)
	{
		bool down = (mPTTKey != KEY_NONE)
			&& gKeyboard->getKeyDown(mPTTKey);
		inputUserControlState(down);
	}
}
void LLVoiceClient::inputUserControlState(bool down)
{
	if(mPTTIsToggle)
	{
		if(down) // toggle open-mic state on 'down'
		{
			toggleUserPTTState();
		}
	}
	else // set open-mic state as an absolute
	{
		setUserPTTState(down);
	}
}
void LLVoiceClient::middleMouseState(bool down)
{
	if(mPTTIsMiddleMouse)
	{
		inputUserControlState(down);
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


std::string LLVoiceClient::getDisplayName(const LLUUID& id)
{
	std::string result;
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
		S32 ires = 100; // nominal default volume
		
		if(participant->mIsSelf)
		{
			// Always make it look like the user's own volume is set at the default.
		}
		else if(participant->mUserVolume != -1)
		{
			// Use the internal volume
			ires = participant->mUserVolume;
			
			// Enable this when debugging voice slider issues.  It's way to spammy even for debug-level logging.
//			LL_DEBUGS("Voice") << "mapping from mUserVolume " << ires << LL_ENDL;
		}
		else if(participant->mVolume != -1)
		{
			// Map backwards from vivox volume 

			// Enable this when debugging voice slider issues.  It's way to spammy even for debug-level logging.
//			LL_DEBUGS("Voice") << "mapping from mVolume " << participant->mVolume << LL_ENDL;

			if(participant->mVolume < 56)
			{
				ires = (participant->mVolume * 100) / 56;
			}
			else
			{
				ires = (((participant->mVolume - 56) * 300) / (100 - 56)) + 100;
			}
		}
		result = sqrtf(((F32)ires) / 400.f);
	}

	// Enable this when debugging voice slider issues.  It's way to spammy even for debug-level logging.
//	LL_DEBUGS("Voice") << "returning " << result << LL_ENDL;

	return result;
}

void LLVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
	if(mAudioSession)
	{
		participantState *participant = findParticipantByID(id);
		if (participant)
		{
			// volume can amplify by as much as 4x!
			S32 ivol = (S32)(400.f * volume * volume);
			participant->mUserVolume = llclamp(ivol, 0, 400);
			participant->mVolumeDirty = TRUE;
			mAudioSession->mVolumeDirty = TRUE;

			// store this volume setting for future sessions
			LLSpeakerVolumeStorage::getInstance()->storeSpeakerVolume(id, participant->mUserVolume);
		}
	}
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

void LLVoiceClient::recordingLoopStart(int seconds, int deltaFramesPerControlFrame)
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlRecording (Start)" << LL_ENDL;
	
	if(!mMainSessionGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Start</RecordingControlType>" 
		<< "<DeltaFramesPerControlFrame>" << deltaFramesPerControlFrame << "</DeltaFramesPerControlFrame>"
		<< "<Filename>" << "" << "</Filename>"
		<< "<EnableAudioRecordingEvents>false</EnableAudioRecordingEvents>"
		<< "<LoopModeDurationSeconds>" << seconds << "</LoopModeDurationSeconds>"
		<< "</Request>\n\n\n";


		writeString(stream.str());
	}
}

void LLVoiceClient::recordingLoopSave(const std::string& filename)
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlRecording (Flush)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Flush</RecordingControlType>" 
		<< "<Filename>" << filename << "</Filename>"
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLVoiceClient::recordingStop()
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlRecording (Stop)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Stop</RecordingControlType>" 
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLVoiceClient::filePlaybackStart(const std::string& filename)
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlPlayback (Start)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlPlayback.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Start</RecordingControlType>" 
		<< "<Filename>" << filename << "</Filename>"
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLVoiceClient::filePlaybackStop()
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlPlayback (Stop)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlPlayback.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Stop</RecordingControlType>" 
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLVoiceClient::filePlaybackSetPaused(bool paused)
{
	// TODO: Implement once Vivox gives me a sample
}

void LLVoiceClient::filePlaybackSetMode(bool vox, float speed)
{
	// TODO: Implement once Vivox gives me a sample
}

LLVoiceClient::sessionState::sessionState() :
	mErrorStatusCode(0),
	mMediaStreamState(streamStateUnknown),
	mTextStreamState(streamStateUnknown),
	mCreateInProgress(false),
	mMediaConnectInProgress(false),
	mVoiceInvitePending(false),
	mTextInvitePending(false),
	mSynthesizedCallerID(false),
	mIsChannel(false),
	mIsSpatial(false),
	mIsP2P(false),
	mIncoming(false),
	mVoiceEnabled(false),
	mReconnect(false),
	mVolumeDirty(false),
	mParticipantsChanged(false)
{
}

LLVoiceClient::sessionState::~sessionState()
{
	removeAllParticipants();
}

bool LLVoiceClient::sessionState::isCallBackPossible()
{
	// This may change to be explicitly specified by vivox in the future...
	// Currently, only PSTN P2P calls cannot be returned.
	// Conveniently, this is also the only case where we synthesize a caller UUID.
	return !mSynthesizedCallerID;
}

bool LLVoiceClient::sessionState::isTextIMPossible()
{
	// This may change to be explicitly specified by vivox in the future...
	return !mSynthesizedCallerID;
}


LLVoiceClient::sessionIterator LLVoiceClient::sessionsBegin(void)
{
	return mSessions.begin();
}

LLVoiceClient::sessionIterator LLVoiceClient::sessionsEnd(void)
{
	return mSessions.end();
}


LLVoiceClient::sessionState *LLVoiceClient::findSession(const std::string &handle)
{
	sessionState *result = NULL;
	sessionMap::iterator iter = mSessionsByHandle.find(&handle);
	if(iter != mSessionsByHandle.end())
	{
		result = iter->second;
	}
	
	return result;
}

LLVoiceClient::sessionState *LLVoiceClient::findSessionBeingCreatedByURI(const std::string &uri)
{	
	sessionState *result = NULL;
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		if(session->mCreateInProgress && (session->mSIPURI == uri))
		{
			result = session;
			break;
		}
	}
	
	return result;
}

LLVoiceClient::sessionState *LLVoiceClient::findSession(const LLUUID &participant_id)
{
	sessionState *result = NULL;
	
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		if((session->mCallerID == participant_id) || (session->mIMSessionID == participant_id))
		{
			result = session;
			break;
		}
	}
	
	return result;
}

LLVoiceClient::sessionState *LLVoiceClient::addSession(const std::string &uri, const std::string &handle)
{
	sessionState *result = NULL;
	
	if(handle.empty())
	{
		// No handle supplied.
		// Check whether there's already a session with this URI
		for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
		{
			sessionState *s = *iter;
			if((s->mSIPURI == uri) || (s->mAlternateSIPURI == uri))
			{
				// TODO: I need to think about this logic... it's possible that this case should raise an internal error.
				result = s;
				break;
			}
		}
	}
	else // (!handle.empty())
	{
		// Check for an existing session with this handle
		sessionMap::iterator iter = mSessionsByHandle.find(&handle);
		
		if(iter != mSessionsByHandle.end())
		{
			result = iter->second;
		}
	}

	if(!result)
	{
		// No existing session found.
		
		LL_DEBUGS("Voice") << "adding new session: handle " << handle << " URI " << uri << LL_ENDL;
		result = new sessionState();
		result->mSIPURI = uri;
		result->mHandle = handle;
		
		mSessions.insert(result);

		if(!result->mHandle.empty())
		{
			mSessionsByHandle.insert(sessionMap::value_type(&(result->mHandle), result));
		}
	}
	else
	{
		// Found an existing session
		
		if(uri != result->mSIPURI)
		{
			// TODO: Should this be an internal error?
			LL_DEBUGS("Voice") << "changing uri from " << result->mSIPURI << " to " << uri << LL_ENDL;
			setSessionURI(result, uri);
		}

		if(handle != result->mHandle)
		{
			if(handle.empty())
			{
				// There's at least one race condition where where addSession was clearing an existing session handle, which caused things to break.
				LL_DEBUGS("Voice") << "NOT clearing handle " << result->mHandle << LL_ENDL;
			}
			else
			{
				// TODO: Should this be an internal error?
				LL_DEBUGS("Voice") << "changing handle from " << result->mHandle << " to " << handle << LL_ENDL;
				setSessionHandle(result, handle);
			}
		}
		
		LL_DEBUGS("Voice") << "returning existing session: handle " << handle << " URI " << uri << LL_ENDL;
	}

	verifySessionState();
		
	return result;
}

void LLVoiceClient::setSessionHandle(sessionState *session, const std::string &handle)
{
	// Have to remove the session from the handle-indexed map before changing the handle, or things will break badly.
	
	if(!session->mHandle.empty())
	{
		// Remove session from the map if it should have been there.
		sessionMap::iterator iter = mSessionsByHandle.find(&(session->mHandle));
		if(iter != mSessionsByHandle.end())
		{
			if(iter->second != session)
			{
				LL_ERRS("Voice") << "Internal error: session mismatch!" << LL_ENDL;
			}

			mSessionsByHandle.erase(iter);
		}
		else
		{
			LL_ERRS("Voice") << "Internal error: session handle not found in map!" << LL_ENDL;
		}
	}
			
	session->mHandle = handle;

	if(!handle.empty())
	{
		mSessionsByHandle.insert(sessionMap::value_type(&(session->mHandle), session));
	}

	verifySessionState();
}

void LLVoiceClient::setSessionURI(sessionState *session, const std::string &uri)
{
	// There used to be a map of session URIs to sessions, which made this complex....
	session->mSIPURI = uri;

	verifySessionState();
}

void LLVoiceClient::deleteSession(sessionState *session)
{
	// Remove the session from the handle map
	if(!session->mHandle.empty())
	{
		sessionMap::iterator iter = mSessionsByHandle.find(&(session->mHandle));
		if(iter != mSessionsByHandle.end())
		{
			if(iter->second != session)
			{
				LL_ERRS("Voice") << "Internal error: session mismatch" << LL_ENDL;
			}
			mSessionsByHandle.erase(iter);
		}
	}

	// Remove the session from the URI map
	mSessions.erase(session);
	
	// At this point, the session should be unhooked from all lists and all state should be consistent.
	verifySessionState();

	// If this is the current audio session, clean up the pointer which will soon be dangling.
	if(mAudioSession == session)
	{
		mAudioSession = NULL;
		mAudioSessionChanged = true;
	}

	// ditto for the next audio session
	if(mNextAudioSession == session)
	{
		mNextAudioSession = NULL;
	}

	// delete the session
	delete session;
}

void LLVoiceClient::deleteAllSessions()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;

	while(!mSessions.empty())
	{
		deleteSession(*(sessionsBegin()));
	}
	
	if(!mSessionsByHandle.empty())
	{
		LL_ERRS("Voice") << "Internal error: empty session map, non-empty handle map" << LL_ENDL;
	}
}

void LLVoiceClient::verifySessionState(void)
{
	// This is mostly intended for debugging problems with session state management.
	LL_DEBUGS("Voice") << "Total session count: " << mSessions.size() << " , session handle map size: " << mSessionsByHandle.size() << LL_ENDL;

	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;

		LL_DEBUGS("Voice") << "session " << session << ": handle " << session->mHandle << ", URI " << session->mSIPURI << LL_ENDL;
		
		if(!session->mHandle.empty())
		{
			// every session with a non-empty handle needs to be in the handle map
			sessionMap::iterator i2 = mSessionsByHandle.find(&(session->mHandle));
			if(i2 == mSessionsByHandle.end())
			{
				LL_ERRS("Voice") << "internal error (handle " << session->mHandle << " not found in session map)" << LL_ENDL;
			}
			else
			{
				if(i2->second != session)
				{
					LL_ERRS("Voice") << "internal error (handle " << session->mHandle << " in session map points to another session)" << LL_ENDL;
				}
			}
		}
	}
		
	// check that every entry in the handle map points to a valid session in the session set
	for(sessionMap::iterator iter = mSessionsByHandle.begin(); iter != mSessionsByHandle.end(); iter++)
	{
		sessionState *session = iter->second;
		sessionIterator i2 = mSessions.find(session);
		if(i2 == mSessions.end())
		{
			LL_ERRS("Voice") << "internal error (session for handle " << session->mHandle << " not found in session map)" << LL_ENDL;
		}
		else
		{
			if(session->mHandle != (*i2)->mHandle)
			{
				LL_ERRS("Voice") << "internal error (session for handle " << session->mHandle << " points to session with different handle " << (*i2)->mHandle << ")" << LL_ENDL;
			}
		}
	}
}

LLVoiceClient::buddyListEntry::buddyListEntry(const std::string &uri) :
	mURI(uri)
{
	mOnlineSL = false;
	mOnlineSLim = false;
	mCanSeeMeOnline = true;
	mHasBlockListEntry = false;
	mHasAutoAcceptListEntry = false;
	mNameResolved = false;
	mInVivoxBuddies = false;
	mInSLFriends = false;
	mNeedsNameUpdate = false;
}

void LLVoiceClient::processBuddyListEntry(const std::string &uri, const std::string &displayName)
{
	buddyListEntry *buddy = addBuddy(uri, displayName);
	buddy->mInVivoxBuddies = true;	
}

LLVoiceClient::buddyListEntry *LLVoiceClient::addBuddy(const std::string &uri)
{
	std::string empty;
	buddyListEntry *buddy = addBuddy(uri, empty);
	if(buddy->mDisplayName.empty())
	{
		buddy->mNameResolved = false;
	}
	return buddy;
}

LLVoiceClient::buddyListEntry *LLVoiceClient::addBuddy(const std::string &uri, const std::string &displayName)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter = mBuddyListMap.find(&uri);
	
	if(iter != mBuddyListMap.end())
	{
		// Found a matching buddy already in the map.
		LL_DEBUGS("Voice") << "adding existing buddy " << uri << LL_ENDL;
		result = iter->second;
	}

	if(!result)
	{
		// participant isn't already in one list or the other.
		LL_DEBUGS("Voice") << "adding new buddy " << uri << LL_ENDL;
		result = new buddyListEntry(uri);
		result->mDisplayName = displayName;

		if(IDFromName(uri, result->mUUID)) 
		{
			// Extracted UUID from name successfully.
		}
		else
		{
			LL_DEBUGS("Voice") << "Couldn't find ID for buddy " << uri << " (\"" << displayName << "\")" << LL_ENDL;
		}

		mBuddyListMap.insert(buddyListMap::value_type(&(result->mURI), result));
	}
	
	return result;
}

LLVoiceClient::buddyListEntry *LLVoiceClient::findBuddy(const std::string &uri)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter = mBuddyListMap.find(&uri);
	if(iter != mBuddyListMap.end())
	{
		result = iter->second;
	}
	
	return result;
}

LLVoiceClient::buddyListEntry *LLVoiceClient::findBuddy(const LLUUID &id)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter;

	for(iter = mBuddyListMap.begin(); iter != mBuddyListMap.end(); iter++)
	{
		if(iter->second->mUUID == id)
		{
			result = iter->second;
			break;
		}
	}
	
	return result;
}

LLVoiceClient::buddyListEntry *LLVoiceClient::findBuddyByDisplayName(const std::string &name)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter;

	for(iter = mBuddyListMap.begin(); iter != mBuddyListMap.end(); iter++)
	{
		if(iter->second->mDisplayName == name)
		{
			result = iter->second;
			break;
		}
	}
	
	return result;
}

void LLVoiceClient::deleteBuddy(const std::string &uri)
{
	buddyListMap::iterator iter = mBuddyListMap.find(&uri);
	if(iter != mBuddyListMap.end())
	{
		LL_DEBUGS("Voice") << "deleting buddy " << uri << LL_ENDL;
		buddyListEntry *buddy = iter->second;
		mBuddyListMap.erase(iter);
		delete buddy;
	}
	else
	{
		LL_DEBUGS("Voice") << "attempt to delete nonexistent buddy " << uri << LL_ENDL;
	}
	
}

void LLVoiceClient::deleteAllBuddies(void)
{
	while(!mBuddyListMap.empty())
	{
		deleteBuddy(*(mBuddyListMap.begin()->first));
	}
	
	// Don't want to correlate with friends list when we've emptied the buddy list.
	mBuddyListMapPopulated = false;
	
	// Don't want to correlate with friends list when we've reset the block rules.
	mBlockRulesListReceived = false;
	mAutoAcceptRulesListReceived = false;
}

void LLVoiceClient::deleteAllBlockRules(void)
{
	// Clear the block list entry flags from all local buddy list entries
	buddyListMap::iterator buddy_it;
	for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end(); buddy_it++)
	{
		buddy_it->second->mHasBlockListEntry = false;
	}
}

void LLVoiceClient::deleteAllAutoAcceptRules(void)
{
	// Clear the auto-accept list entry flags from all local buddy list entries
	buddyListMap::iterator buddy_it;
	for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end(); buddy_it++)
	{
		buddy_it->second->mHasAutoAcceptListEntry = false;
	}
}

void LLVoiceClient::addBlockRule(const std::string &blockMask, const std::string &presenceOnly)
{
	buddyListEntry *buddy = NULL;

	// blockMask is the SIP URI of a friends list entry
	buddyListMap::iterator iter = mBuddyListMap.find(&blockMask);
	if(iter != mBuddyListMap.end())
	{
		LL_DEBUGS("Voice") << "block list entry for " << blockMask << LL_ENDL;
		buddy = iter->second;
	}

	if(buddy == NULL)
	{
		LL_DEBUGS("Voice") << "block list entry for unknown buddy " << blockMask << LL_ENDL;
		buddy = addBuddy(blockMask);
	}
	
	if(buddy != NULL)
	{
		buddy->mHasBlockListEntry = true;
	}
}

void LLVoiceClient::addAutoAcceptRule(const std::string &autoAcceptMask, const std::string &autoAddAsBuddy)
{
	buddyListEntry *buddy = NULL;

	// blockMask is the SIP URI of a friends list entry
	buddyListMap::iterator iter = mBuddyListMap.find(&autoAcceptMask);
	if(iter != mBuddyListMap.end())
	{
		LL_DEBUGS("Voice") << "auto-accept list entry for " << autoAcceptMask << LL_ENDL;
		buddy = iter->second;
	}

	if(buddy == NULL)
	{
		LL_DEBUGS("Voice") << "auto-accept list entry for unknown buddy " << autoAcceptMask << LL_ENDL;
		buddy = addBuddy(autoAcceptMask);
	}

	if(buddy != NULL)
	{
		buddy->mHasAutoAcceptListEntry = true;
	}
}

void LLVoiceClient::accountListBlockRulesResponse(int statusCode, const std::string &statusString)
{
	// Block list entries were updated via addBlockRule() during parsing.  Just flag that we're done.
	mBlockRulesListReceived = true;
}

void LLVoiceClient::accountListAutoAcceptRulesResponse(int statusCode, const std::string &statusString)
{
	// Block list entries were updated via addBlockRule() during parsing.  Just flag that we're done.
	mAutoAcceptRulesListReceived = true;
}

void LLVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.insert(observer);
}

void LLVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.erase(observer);
}

void LLVoiceClient::notifyParticipantObservers()
{
	for (observer_set_t::iterator it = mParticipantObservers.begin();
		it != mParticipantObservers.end();
		)
	{
		LLVoiceClientParticipantObserver* observer = *it;
		observer->onChange();
		// In case onChange() deleted an entry.
		it = mParticipantObservers.upper_bound(observer);
	}
}

void LLVoiceClient::addObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.insert(observer);
}

void LLVoiceClient::removeObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.erase(observer);
}

void LLVoiceClient::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
{
	if(mAudioSession)
	{
		if(status == LLVoiceClientStatusObserver::ERROR_UNKNOWN)
		{
			switch(mAudioSession->mErrorStatusCode)
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
			mAudioSession->mErrorStatusCode = 0;
		}
		else if(status == LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL)
		{
			switch(mAudioSession->mErrorStatusCode)
			{
				case 404:	// NOT_FOUND
				case 480:	// TEMPORARILY_UNAVAILABLE
				case 408:	// REQUEST_TIMEOUT
					// call failed because other user was not available
					// treat this as an error case
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;

					// Reset the error code to make sure it won't be reused later by accident.
					mAudioSession->mErrorStatusCode = 0;
				break;
			}
		}
	}
		
	LL_DEBUGS("Voice") 
		<< " " << LLVoiceClientStatusObserver::status2string(status)  
		<< ", session URI " << getAudioSessionURI() 
		<< (inSpatialChannel()?", proximal is true":", proximal is false")
	<< LL_ENDL;

	for (status_observer_set_t::iterator it = mStatusObservers.begin();
		it != mStatusObservers.end();
		)
	{
		LLVoiceClientStatusObserver* observer = *it;
		observer->onChange(status, getAudioSessionURI(), inSpatialChannel());
		// In case onError() deleted an entry.
		it = mStatusObservers.upper_bound(observer);
	}

}

void LLVoiceClient::addObserver(LLFriendObserver* observer)
{
	mFriendObservers.insert(observer);
}

void LLVoiceClient::removeObserver(LLFriendObserver* observer)
{
	mFriendObservers.erase(observer);
}

void LLVoiceClient::notifyFriendObservers()
{
	for (friend_observer_set_t::iterator it = mFriendObservers.begin();
		it != mFriendObservers.end();
		)
	{
		LLFriendObserver* observer = *it;
		it++;
		// The only friend-related thing we notify on is online/offline transitions.
		observer->changed(LLFriendObserver::ONLINE);
	}
}

void LLVoiceClient::lookupName(const LLUUID &id)
{
	gCacheName->get(id, false,
		boost::bind(&LLVoiceClient::avatarNameResolved, this, _1, _2));
}

void LLVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
	// If the avatar whose name just resolved is on our friends list, resync the friends list.
	if(LLAvatarTracker::instance().getBuddyInfo(id) != NULL)
	{
		mFriendsListDirty = true;
	}
	
	// Iterate over all sessions.
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;

		// Check for this user as a participant in this session
		participantState *participant = session->findParticipantByID(id);
		if(participant)
		{
			// Found -- fill in the name
			participant->mAccountName = name;
			// and post a "participants updated" message to listeners later.
			session->mParticipantsChanged = true;
		}
		
		// Check whether this is a p2p session whose caller name just resolved
		if(session->mCallerID == id)
		{
			// this session's "caller ID" just resolved.  Fill in the name.
			session->mName = name;
			if(session->mTextInvitePending)
			{
				session->mTextInvitePending = false;

				// We don't need to call gIMMgr->addP2PSession() here.  The first incoming message will create the panel.				
			}
			if(session->mVoiceInvitePending)
			{
				session->mVoiceInvitePending = false;

				gIMMgr->inviteToSession(
					session->mIMSessionID,
					session->mName,
					session->mCallerID, 
					session->mName, 
					IM_SESSION_P2P_INVITE, 
					LLIMMgr::INVITATION_TYPE_VOICE,
					session->mHandle,
					session->mSIPURI);
			}
			
		}
	}
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

		//this is a misnomer, as it can also be when you are not in
		//a parcel at all.  Should really be something like
		//LLViewerVoiceInfoChanged.....
		if ( input.has("body") )
		{
			LLSD body = input["body"];

			//body has "region_name" (str), "parcel_local_id"(int),
			//"voice_credentials" (map).

			//body["voice_credentials"] has "channel_uri" (str),
			//body["voice_credentials"] has "channel_credentials" (str)

			//if we really wanted to be extra careful,
			//we'd check the supplied
			//local parcel id to make sure it's for the same parcel
			//we believe we're in
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
					LLNotificationsUtil::add("VoiceVersionMismatch");
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
