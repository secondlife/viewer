/** 
 * @file llimpanel.cpp
 * @brief LLIMPanel class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llimpanel.h"

#include "indra_constants.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"
#include "lltextbox.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llconsole.h"
#include "llfloater.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llfloateravatarinfo.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llresmgr.h"
#include "lltabcontainer.h"
#include "llimview.h"
#include "llviewertexteditor.h"
#include "llviewermessage.h"
#include "llviewerstats.h"
#include "viewer.h"
#include "llvieweruictrlfactory.h"
#include "lllogchat.h"
#include "llfloaterhtml.h"
#include "llweb.h"

//
// Constants
//
const S32 LINE_HEIGHT = 16;
const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 130;

//
// Statics
//
//
static LLString sTitleString = "Instant Message with [NAME]";
static LLString sTypingStartString = "[NAME]: ...";

// Member Functions
//

LLFloaterIMPanel::LLFloaterIMPanel(const std::string& name, const LLRect& rect,
					 const std::string& session_label,
					 const LLUUID& session_id, 
					 const LLUUID& other_participant_id,
					 EInstantMessage dialog) :
	LLFloater(name, rect, session_label),
	mInputEditor(NULL),
	mHistoryEditor(NULL),
	mSessionLabel(session_label),
	mSessionUUID(session_id),
	mOtherParticipantUUID(other_participant_id),
	mLureID(),
	mDialog(dialog),
	mTyping(FALSE),
	mOtherTyping(FALSE),
	mTypingLineStartIndex(0),
	mSentTypingState(TRUE),
	mFirstKeystrokeTimer(),
	mLastKeystrokeTimer()
{
	init();
	setLabel(session_label);
	setTitle(session_label);
	mInputEditor->setMaxTextLength(1023);
}


void LLFloaterIMPanel::init()
{
	gUICtrlFactory->buildFloater(this, "floater_instant_message.xml", NULL, FALSE);
}


BOOL LLFloaterIMPanel::postBuild() 
{
	requires("chat_editor", WIDGET_TYPE_LINE_EDITOR);
	requires("profile_btn", WIDGET_TYPE_BUTTON);
	requires("close_btn", WIDGET_TYPE_BUTTON);
	requires("im_history", WIDGET_TYPE_TEXT_EDITOR);
	requires("live_help_dialog", WIDGET_TYPE_TEXT_BOX);
	requires("title_string", WIDGET_TYPE_TEXT_BOX);
	requires("typing_start_string", WIDGET_TYPE_TEXT_BOX);

	if (checkRequirements())
	{
		mInputEditor = LLUICtrlFactory::getLineEditorByName(this, "chat_editor");
		mInputEditor->setFocusReceivedCallback( onInputEditorFocusReceived );
		mInputEditor->setFocusLostCallback( onInputEditorFocusLost );
		mInputEditor->setKeystrokeCallback( onInputEditorKeystroke );
		mInputEditor->setCallbackUserData(this);
		mInputEditor->setCommitOnFocusLost( FALSE );
		mInputEditor->setRevertOnEsc( FALSE );

		LLButton* profile_btn = LLUICtrlFactory::getButtonByName(this, "profile_btn");
		profile_btn->setClickedCallback(&LLFloaterIMPanel::onClickProfile, this);

		LLButton* close_btn = LLUICtrlFactory::getButtonByName(this, "close_btn");
		close_btn->setClickedCallback(&LLFloaterIMPanel::onClickClose, this);

		mHistoryEditor = LLViewerUICtrlFactory::getViewerTextEditorByName(this, "im_history");
		mHistoryEditor->setParseHTML(TRUE);
		if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
		{
			LLLogChat::loadHistory(mSessionLabel, &chatFromLogFile, (void *)this); 
		}

		if (IM_SESSION_GROUP_START == mDialog
			|| IM_SESSION_911_START == mDialog)
		{
			profile_btn->setEnabled(FALSE);
			if(IM_SESSION_911_START == mDialog)
			{
				LLTextBox* live_help_text = LLUICtrlFactory::getTextBoxByName(this, "live_help_dialog");
				addHistoryLine(live_help_text->getText());
			}
		}

		LLTextBox* title = LLUICtrlFactory::getTextBoxByName(this, "title_string");
		sTitleString = title->getText();
		
		LLTextBox* typing_start = LLUICtrlFactory::getTextBoxByName(this, "typing_start_string");
				
		sTypingStartString = typing_start->getText();

		return TRUE;
	}

	return FALSE;
}

// virtual
void LLFloaterIMPanel::draw()
{
	if (mTyping)
	{
		// Time out if user hasn't typed for a while.
		if (mLastKeystrokeTimer.getElapsedTimeF32() > LLAgent::TYPING_TIMEOUT_SECS)
		{
			setTyping(FALSE);
		}

		// If we are typing, and it's been a little while, send the
		// typing indicator
		if (!mSentTypingState
			&& mFirstKeystrokeTimer.getElapsedTimeF32() > 1.f)
		{
			sendTypingState(TRUE);
			mSentTypingState = TRUE;
		}
	}

	LLFloater::draw();
}


BOOL LLFloaterIMPanel::addParticipants(const LLDynamicArray<LLUUID>& ids)
{
	if(isAddAllowed())
	{
		llinfos << "LLFloaterIMPanel::addParticipants() - adding participants" << llendl;
		const S32 MAX_AGENTS = 50;
		S32 count = ids.count();
		if(count > MAX_AGENTS) return FALSE;
		LLMessageSystem *msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_MessageBlock);
		msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
		msg->addUUIDFast(_PREHASH_ToAgentID, mOtherParticipantUUID);
		msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
		msg->addU8Fast(_PREHASH_Dialog, mDialog);
		msg->addUUIDFast(_PREHASH_ID, mSessionUUID);
		msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary
		std::string name;
		gAgent.buildFullname(name);
		msg->addStringFast(_PREHASH_FromAgentName, name);
		msg->addStringFast(_PREHASH_Message, LLString::null);
		msg->addU32Fast(_PREHASH_ParentEstateID, 0);
		msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
		msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
		if (IM_SESSION_GROUP_START == mDialog)
		{
			// *HACK: binary bucket contains session label - the server
			// will actually add agents.
			llinfos << "Group IM session name '" << mSessionLabel 
				<< "'" << llendl;
			msg->addStringFast(_PREHASH_BinaryBucket, mSessionLabel);
			gAgent.sendReliableMessage();
		}
		else if (IM_SESSION_911_START == mDialog)
		{
			// HACK -- we modify the name of the session going out to
			// the helpers to help them easily identify "Help"
			// sessions in their collection of IM panels.
			LLString name;
			gAgent.getName(name);
			LLString buffer = LLString("HELP ") +  name;
			llinfos << "LiveHelp IM session '" << buffer << "'." << llendl;
			msg->addStringFast(_PREHASH_BinaryBucket, buffer.c_str());

			// automaticaly open a wormhole when this reliable message gets through
			msg->sendReliable(
					gAgent.getRegionHost(),
					3,      // retries
					TRUE,   // ping-based
					5.0f,   // timeout
					send_lure_911,
					(void**)&mSessionUUID);
		}
		else
		{
			if (mDialog != IM_SESSION_ADD
				&& mDialog != IM_SESSION_OFFLINE_ADD)
			{
				llwarns << "LLFloaterIMPanel::addParticipants() - dialog type " << mDialog
					<< " is not an ADD" << llendl;
			}
			// *FIX: this could suffer from endian issues
			S32 bucket_size = UUID_BYTES * count;
			U8* bucket = new U8[bucket_size];
			U8* pos = bucket;
			for(S32 i = 0; i < count; ++i)
			{
				memcpy(pos, &(ids.get(i)), UUID_BYTES);		/* Flawfinder: ignore */
				pos += UUID_BYTES;
			}
			msg->addBinaryDataFast(_PREHASH_BinaryBucket, bucket, bucket_size);
			delete[] bucket;
			gAgent.sendReliableMessage();
		}

	}
	else
	{
		llinfos << "LLFloaterIMPanel::addParticipants() - no need to add agents for "
				<< mDialog << llendl;
		// successful add, because everyone that needed to get added
		// was added.
	}
	return TRUE;
}

void LLFloaterIMPanel::addHistoryLine(const std::string &utf8msg, const LLColor4& color, bool log_to_file)
{
	LLMultiFloater* hostp = getHost();
	if( !getVisible() && hostp && log_to_file)
	{
		// Only flash for logged ("real") messages
		LLTabContainer* parent = (LLTabContainer*) getParent();
		parent->setTabPanelFlashing( this, TRUE );
	}

	// Now we're adding the actual line of text, so erase the 
	// "Foo is typing..." text segment, and the optional timestamp
	// if it was present. JC
	removeTypingIndicator();

	// Actually add the line
	LLString timestring;
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("IMShowTimestamps"))
	{
		timestring = mHistoryEditor->appendTime(prepend_newline);
		prepend_newline = false;
	}
	mHistoryEditor->appendColoredText(utf8msg, false, prepend_newline, color);
	
	if (log_to_file
		&& gSavedPerAccountSettings.getBOOL("LogInstantMessages") ) 
	{
		LLString histstr =  timestring + utf8msg;

		LLLogChat::saveHistory(mSessionLabel,histstr);
	}
}


void LLFloaterIMPanel::setVisible(BOOL b)
{
	LLPanel::setVisible(b);

	LLMultiFloater* hostp = getHost();
	if( b && hostp )
	{
		LLTabContainer* parent = (LLTabContainer*) getParent();

		// When this tab is displayed, you can stop flashing.
		parent->setTabPanelFlashing( this, FALSE );

		/* Don't change containing floater title - leave it "Instant Message" JC
		LLUIString title = sTitleString;
		title.setArg("[NAME]", mSessionLabel);
		hostp->setTitle( title );
		*/
	}
}


void LLFloaterIMPanel::setInputFocus( BOOL b )
{
	mInputEditor->setFocus( b );
}


void LLFloaterIMPanel::selectAll()
{
	mInputEditor->selectAll();
}


void LLFloaterIMPanel::selectNone()
{
	mInputEditor->deselect();
}


BOOL LLFloaterIMPanel::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;
	if( getVisible() && mEnabled && !called_from_parent && gFocusMgr.childHasKeyboardFocus(this))
	{
		if( KEY_RETURN == key && mask == MASK_NONE)
		{
			sendMsg();
			handled = TRUE;

			// Close talk panels on hitting return
			// but not shift-return or control-return
			if ( !gSavedSettings.getBOOL("PinTalkViewOpen") && !(mask & MASK_CONTROL) && !(mask & MASK_SHIFT) )
			{
				gIMView->toggle(NULL);
			}
		}
		else if ( KEY_ESCAPE == key )
		{
			handled = TRUE;
			gFocusMgr.setKeyboardFocus(NULL, NULL);

			// Close talk panel with escape
			if( !gSavedSettings.getBOOL("PinTalkViewOpen") )
			{
				gIMView->toggle(NULL);
			}
		}
	}

	// May need to call base class LLPanel::handleKeyHere if not handled
	// in order to tab between buttons.  JNC 1.2.2002
	return handled;
}

BOOL LLFloaterIMPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								  EDragAndDropType cargo_type,
								  void* cargo_data,
								  EAcceptance* accept,
								  LLString& tooltip_msg)
{
	BOOL accepted = FALSE;
	switch(cargo_type)
	{
	case DAD_CALLINGCARD:
		accepted = dropCallingCard((LLInventoryItem*)cargo_data, drop);
		break;
	case DAD_CATEGORY:
		accepted = dropCategory((LLInventoryCategory*)cargo_data, drop);
		break;
	default:
		break;
	}
	if (accepted)
	{
		*accept = ACCEPT_YES_MULTI;
	}
	else
	{
		*accept = ACCEPT_NO;
	}
	return TRUE;
} 

BOOL LLFloaterIMPanel::dropCallingCard(LLInventoryItem* item, BOOL drop)
{
	BOOL rv = isAddAllowed();
	if(rv && item && item->getCreatorUUID().notNull())
	{
		if(drop)
		{
			LLDynamicArray<LLUUID> ids;
			ids.put(item->getCreatorUUID());
			addParticipants(ids);
		}
	}
	else
	{
		// set to false if creator uuid is null.
		rv = FALSE;
	}
	return rv;
}

BOOL LLFloaterIMPanel::dropCategory(LLInventoryCategory* category, BOOL drop)
{
	BOOL rv = isAddAllowed();
	if(rv && category)
	{
		LLInventoryModel::cat_array_t cats;
		LLInventoryModel::item_array_t items;
		LLUniqueBuddyCollector buddies;
		gInventory.collectDescendentsIf(category->getUUID(),
										cats,
										items,
										LLInventoryModel::EXCLUDE_TRASH,
										buddies);
		S32 count = items.count();
		if(count == 0)
		{
			rv = FALSE;
		}
		else if(drop)
		{
			LLDynamicArray<LLUUID> ids;
			for(S32 i = 0; i < count; ++i)
			{
				ids.put(items.get(i)->getCreatorUUID());
			}
			addParticipants(ids);
		}
	}
	return rv;
}

BOOL LLFloaterIMPanel::isAddAllowed() const
{

	return ((IM_SESSION_ADD == mDialog) 
			|| (IM_SESSION_OFFLINE_ADD == mDialog)
			|| (IM_SESSION_GROUP_START == mDialog)
			|| (IM_SESSION_911_START == mDialog)
			|| (IM_SESSION_CARDLESS_START == mDialog));
}


// static
void LLFloaterIMPanel::onTabClick(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->setInputFocus(TRUE);
}


// static
void LLFloaterIMPanel::onClickProfile( void* userdata )
{
	//  Bring up the Profile window
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	
	if (self->mOtherParticipantUUID.notNull())
	{
		LLFloaterAvatarInfo::showFromDirectory(self->getOtherParticipantID());
	}
}

// static
void LLFloaterIMPanel::onClickClose( void* userdata )
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	if(self)
	{
		self->close();
	}
}

void LLFloaterIMPanel::addTeleportButton(const LLUUID& lure_id)
{
	LLButton* btn = LLViewerUICtrlFactory::getButtonByName(this, "Teleport Btn");
	if (!btn)
	{
		S32 BTN_VPAD = 2;
		S32 BTN_HPAD = 2;

		const char* teleport_label = "Teleport";
	
		const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF );
		S32 p_btn_width = 75;
		S32 c_btn_width = 60;
		S32 t_btn_width = 75;
	
		// adjust the size of the editor to make room for the new button
		LLRect rect = mInputEditor->getRect();
		S32 editor_right = rect.mRight - t_btn_width;
		rect.mRight = editor_right;
		mInputEditor->reshape(rect.getWidth(), rect.getHeight(), FALSE);
		mInputEditor->setRect(rect);
	
		const S32 IMPANEL_PAD = 1 + LLPANEL_BORDER_WIDTH;
		const S32 IMPANEL_INPUT_HEIGHT = 20;

		rect.setLeftTopAndSize(
			mRect.getWidth() - IMPANEL_PAD - p_btn_width - c_btn_width - t_btn_width - BTN_HPAD - RESIZE_HANDLE_WIDTH,
			IMPANEL_INPUT_HEIGHT + IMPANEL_PAD - BTN_VPAD,
			t_btn_width,
			BTN_HEIGHT);

		btn = new LLButton( 
			"Teleport Btn", rect, 
			"","",	"",
			&LLFloaterIMPanel::onTeleport, this,
			font, teleport_label, teleport_label );
		
		btn->setFollowsBottom();
		btn->setFollowsRight();
		addChild( btn );
	}
	btn->setEnabled(TRUE);
	mLureID = lure_id;
}

void LLFloaterIMPanel::removeTeleportButton()
{
	// TODO -- purge the button
	LLButton* btn = LLViewerUICtrlFactory::getButtonByName(this, "Teleport Btn");
	if (btn)
	{
		btn->setEnabled(FALSE);
	}
}

// static 
void LLFloaterIMPanel::onTeleport(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	if(self)
	{
		send_simple_im(self->mLureID,
							"",
							IM_LURE_911,
							LLUUID::null);
		self->removeTeleportButton();
	}
}

// static
void LLFloaterIMPanel::onInputEditorFocusReceived( LLUICtrl* caller, void* userdata )
{
	LLFloaterIMPanel* self= (LLFloaterIMPanel*) userdata;
	self->mHistoryEditor->setCursorAndScrollToEnd();
}

// static
void LLFloaterIMPanel::onInputEditorFocusLost(LLLineEditor* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->setTyping(FALSE);
}

// static
void LLFloaterIMPanel::onInputEditorKeystroke(LLLineEditor* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	LLString text = self->mInputEditor->getText();
	if (!text.empty())
	{
		self->setTyping(TRUE);
	}
	else
	{
		// Deleting all text counts as stopping typing.
		self->setTyping(FALSE);
	}
}

void LLFloaterIMPanel::close(bool app_quitting)
{
	setTyping(FALSE);

	if(mSessionUUID.notNull())
	{
		std::string name;
		gAgent.buildFullname(name);
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			mOtherParticipantUUID,
			name.c_str(), 
			"",
			IM_ONLINE,
			IM_SESSION_DROP,
			mSessionUUID);
		gAgent.sendReliableMessage();
	}
	gIMView->removeSession(mSessionUUID);

	destroy();
}


void LLFloaterIMPanel::sendMsg()
{
	LLWString text = mInputEditor->getWText();
	LLWString::trim(text);
	if (!gAgent.isGodlike() 
		&& (mDialog == IM_NOTHING_SPECIAL)
		&& mOtherParticipantUUID.isNull())
	{
		llinfos << "Cannot send IM to everyone unless you're a god." << llendl;
		return;
	}
	if(text.length() > 0)
	{
		// Truncate and convert to UTF8 for transport
		std::string utf8_text = wstring_to_utf8str(text);
		utf8_text = utf8str_truncate(utf8_text, MAX_MSG_BUF_SIZE - 1);
		std::string name;
		gAgent.buildFullname(name);

		const LLRelationship* info = NULL;
		info = LLAvatarTracker::instance().getBuddyInfo(mOtherParticipantUUID);
		U8 offline = (!info || info->isOnline()) ? IM_ONLINE : IM_OFFLINE;

		// default to IM_SESSION_SEND unless it's nothing special - in
		// which case it's probably an IM to everyone.
		U8 dialog = (mDialog == IM_NOTHING_SPECIAL)
			? (U8)IM_NOTHING_SPECIAL : (U8)IM_SESSION_SEND;
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			mOtherParticipantUUID,
			name.c_str(),
			utf8_text.c_str(),
			offline,
			(EInstantMessage)dialog,
			mSessionUUID);
		gAgent.sendReliableMessage();

		// local echo
		if((mDialog == IM_NOTHING_SPECIAL) && (mOtherParticipantUUID.notNull()))
		{
			std::string history_echo;
			gAgent.buildFullname(history_echo);

			// Look for IRC-style emotes here.
			char tmpstr[5];		/* Flawfinder: ignore */
			strncpy(tmpstr,utf8_text.substr(0,4).c_str(), sizeof(tmpstr) -1);		/* Flawfinder: ignore */
			tmpstr[sizeof(tmpstr) -1] = '\0';
			if (!strncmp(tmpstr, "/me ", 4) || !strncmp(tmpstr, "/me'", 4))
			{
				utf8_text.replace(0,3,"");
			}
			else
			{
				history_echo += ": ";
			}
			history_echo += utf8_text;
			addHistoryLine(history_echo);
		}

		gViewerStats->incStat(LLViewerStats::ST_IM_COUNT);
	}
	mInputEditor->setText("");

	// Don't need to actually send the typing stop message, the other
	// client will infer it from receiving the message.
	mTyping = FALSE;
	mSentTypingState = TRUE;
}


void LLFloaterIMPanel::setTyping(BOOL typing)
{
	if (typing)
	{
		// Every time you type something, reset this timer
		mLastKeystrokeTimer.reset();

		if (!mTyping)
		{
			// You just started typing.
			mFirstKeystrokeTimer.reset();

			// Will send typing state after a short delay.
			mSentTypingState = FALSE;
		}
	}
	else
	{
		if (mTyping)
		{
			// you just stopped typing, send state immediately
			sendTypingState(FALSE);
			mSentTypingState = TRUE;
		}
	}

	mTyping = typing;
}

void LLFloaterIMPanel::sendTypingState(BOOL typing)
{
	// Don't want to send typing indicators to multiple people, potentially too
	// much network traffic.  Only send in person-to-person IMs.
	if (mDialog != IM_NOTHING_SPECIAL) return;

	std::string name;
	gAgent.buildFullname(name);

	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		mOtherParticipantUUID,
		name.c_str(),
		"typing",
		IM_ONLINE,
		(typing ? IM_TYPING_START : IM_TYPING_STOP),
		mSessionUUID);
	gAgent.sendReliableMessage();
}


void LLFloaterIMPanel::processIMTyping(const LLIMInfo* im_info, BOOL typing)
{
	if (typing)
	{
		// other user started typing
		addTypingIndicator(im_info);
	}
	else
	{
		// other user stopped typing
		removeTypingIndicator();
	}
}


void LLFloaterIMPanel::addTypingIndicator(const LLIMInfo* im_info)
{
	mTypingLineStartIndex = mHistoryEditor->getText().length();

	LLUIString typing_start = sTypingStartString;
	typing_start.setArg("[NAME]", im_info->mName);
	bool log_to_file = false;
	addHistoryLine(typing_start, LLColor4::grey, log_to_file);
	mOtherTyping = TRUE;
}


void LLFloaterIMPanel::removeTypingIndicator()
{
	if (mOtherTyping)
	{
		// Must do this first, otherwise addHistoryLine calls us again.
		mOtherTyping = FALSE;

		S32 chars_to_remove = mHistoryEditor->getText().length() - mTypingLineStartIndex;
		mHistoryEditor->removeTextFromEnd(chars_to_remove);
	}
}

//static
void LLFloaterIMPanel::chatFromLogFile(LLString line, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	
	//self->addHistoryLine(line, LLColor4::grey, FALSE);
	self->mHistoryEditor->appendColoredText(line, false, true, LLColor4::grey);

}
