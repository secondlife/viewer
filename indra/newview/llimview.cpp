/** 
 * @file llimview.cpp
 * @brief Container for Instant Messaging
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llimview.h"

#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llbutton.h"
#include "llstring.h"
#include "linked_lists.h"
#include "llvieweruictrlfactory.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llviewerwindow.h"
#include "llresmgr.h"
#include "llfloaternewim.h"
#include "llimpanel.h"
#include "llresizebar.h"
#include "lltabcontainer.h"
#include "viewer.h"
#include "llfloater.h"
#include "llresizehandle.h"
#include "llkeyboard.h"
#include "llui.h"
#include "llviewermenu.h"
#include "llcallingcard.h"
#include "lltoolbar.h"

const EInstantMessage EVERYONE_DIALOG = IM_NOTHING_SPECIAL;
const EInstantMessage GROUP_DIALOG = IM_SESSION_GROUP_START;
const EInstantMessage DEFAULT_DIALOG = IM_NOTHING_SPECIAL;

//
// Globals
//
LLIMView* gIMView = NULL;

//
// Statics
//
static LLString sOnlyUserMessage;
static LLString sOfflineMessage;

//
// Helper Functions
//

// returns true if a should appear before b
static BOOL group_dictionary_sort( LLGroupData* a, LLGroupData* b )
{
	return (LLString::compareDict( a->mName, b->mName ) < 0);
}


// the other_participant_id is either an agent_id, a group_id, or an inventory
// folder item_id (collection of calling cards)
static LLUUID compute_session_id(EInstantMessage dialog, const LLUUID& other_participant_id)
{
	LLUUID session_id;
	if (IM_SESSION_GROUP_START == dialog)
	{
		// slam group session_id to the group_id (other_participant_id)
		session_id = other_participant_id;
	}
	else
	{
		LLUUID agent_id = gAgent.getID();
		if (other_participant_id == agent_id)
		{
			// if we try to send an IM to ourselves then the XOR would be null
			// so we just make the session_id the same as the agent_id
			session_id = agent_id;
		}
		else
		{
			// peer-to-peer or peer-to-asset session_id is the XOR
			session_id = other_participant_id ^ agent_id;
		}
	}
	return session_id;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLFloaterIM
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LLFloaterIM::LLFloaterIM() 
{
	gUICtrlFactory->buildFloater(this, "floater_im.xml");
}

BOOL LLFloaterIM::postBuild()
{
	requires("only_user_message", WIDGET_TYPE_TEXT_BOX);
	requires("offline_message", WIDGET_TYPE_TEXT_BOX);

	if (checkRequirements())
	{
		sOnlyUserMessage = childGetText("only_user_message");
		sOfflineMessage = childGetText("offline_message");
		return TRUE;
	}
	return FALSE;
}

//// virtual
//BOOL LLFloaterIM::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
//{
//	BOOL handled = FALSE;
//	if (getEnabled()
//		&& mask == (MASK_CONTROL|MASK_SHIFT))
//	{
//		if (key == 'W')
//		{
//			LLFloater* floater = getActiveFloater();
//			if (floater)
//			{
//				if (mTabContainer->getTabCount() == 1)
//				{
//					// trying to close last tab, close
//					// entire window.
//					close();
//					handled = TRUE;
//				}
//			}
//		}
//	}
//	return handled || LLMultiFloater::handleKeyHere(key, mask, called_from_parent);
//}

void LLFloaterIM::onClose(bool app_quitting)
{
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowIM", FALSE);
	}
	setVisible(FALSE);
}

//virtual
void LLFloaterIM::addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point)
{
	// this code is needed to fix the bug where new IMs received will resize the IM floater.
	// SL-29075, SL-24556, and others
	LLRect parent_rect = getRect();
	S32 dheight = LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT;
	LLRect rect(0, parent_rect.getHeight()-dheight, parent_rect.getWidth(), 0);
	floaterp->reshape(rect.getWidth(), rect.getHeight(), TRUE);
	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIMViewFriendObserver
//
// Bridge to suport knowing when the inventory has changed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLIMViewFriendObserver : public LLFriendObserver
{
public:
	LLIMViewFriendObserver(LLIMView* tv) : mTV(tv) {}
	virtual ~LLIMViewFriendObserver() {}
	virtual void changed(U32 mask)
	{
		if(mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
		{
			mTV->refresh();
		}
	}
protected:
	LLIMView* mTV;
};


//
// Public Static Member Functions
//

// This is a helper function to determine what kind of im session
// should be used for the given agent.
// static
EInstantMessage LLIMView::defaultIMTypeForAgent(const LLUUID& agent_id)
{
	EInstantMessage type = IM_SESSION_CARDLESS_START;
	if(is_agent_friend(agent_id))
	{
		if(LLAvatarTracker::instance().isBuddyOnline(agent_id))
		{
			type = IM_SESSION_ADD;
		}
		else
		{
			type = IM_SESSION_OFFLINE_ADD;
		}
	}
	return type;
}

// static
//void LLIMView::onPinButton(void*)
//{
//	BOOL state = gSavedSettings.getBOOL( "PinTalkViewOpen" );
//	gSavedSettings.setBOOL( "PinTalkViewOpen", !state );
//}

// static 
void LLIMView::toggle(void*)
{
	static BOOL return_to_mouselook = FALSE;

	// Hide the button and show the floater or vice versa.
	llassert( gIMView );
	BOOL old_state = gIMView->getFloaterOpen();
	
	// If we're in mouselook and we triggered the Talk View, we want to talk.
	if( gAgent.cameraMouselook() && old_state )
	{
		return_to_mouselook = TRUE;
		gAgent.changeCameraToDefault();
		return;
	}

	BOOL new_state = !old_state;

	if (new_state)
	{
		// ...making visible
		if ( gAgent.cameraMouselook() )
		{
			return_to_mouselook = TRUE;
			gAgent.changeCameraToDefault();
		}
	}
	else
	{
		// ...hiding
		if ( gAgent.cameraThirdPerson() && return_to_mouselook )
		{
			gAgent.changeCameraToMouselook();
		}
		return_to_mouselook = FALSE;
	}

	gIMView->setFloaterOpen( new_state );
}

//
// Member Functions
//

LLIMView::LLIMView(const std::string& name, const LLRect& rect) :
	LLView(name, rect, FALSE),
	mFriendObserver(NULL),
	mIMReceived(FALSE)
{
	gIMView = this;
	mFriendObserver = new LLIMViewFriendObserver(this);
	LLAvatarTracker::instance().addObserver(mFriendObserver);

	mTalkFloater = new LLFloaterIM();

	// New IM Panel
	mNewIMFloater = new LLFloaterNewIM();
	mTalkFloater->addFloater(mNewIMFloater, TRUE);

	// Tabs sometimes overlap resize handle
	mTalkFloater->moveResizeHandleToFront();
}

LLIMView::~LLIMView()
{
	LLAvatarTracker::instance().removeObserver(mFriendObserver);
	delete mFriendObserver;
	// Children all cleaned up by default view destructor.
}

EWidgetType LLIMView::getWidgetType() const
{
	return WIDGET_TYPE_TALK_VIEW;
}

LLString LLIMView::getWidgetTag() const
{
	return LL_TALK_VIEW_TAG;
}

// Add a message to a session. 
void LLIMView::addMessage(
	const LLUUID& session_id,
	const LLUUID& other_participant_id,
	const char* from,
	const char* msg,
	const char* session_name,
	EInstantMessage dialog,
	U32 parent_estate_id,
	const LLUUID& region_id,
	const LLVector3& position)
{
	LLFloaterIMPanel* floater;
	LLUUID new_session_id = session_id;
	if (new_session_id.isNull())
	{
		new_session_id = compute_session_id(dialog, other_participant_id);
	}
	floater = findFloaterBySession(new_session_id);
	if (!floater)
	{
		floater = findFloaterBySession(other_participant_id);
		if (floater)
		{
			llinfos << "found the IM session " << session_id 
				<< " by participant " << other_participant_id << llendl;
		}
	}
	if(floater)
	{
		floater->addHistoryLine(msg);
	}
	else
	{
		const char* name = from;
		if(session_name && (strlen(session_name)>1))
		{
			name = session_name;
		}
		floater = createFloater(new_session_id, other_participant_id, name, dialog, FALSE);

		// When we get a new IM, and if you are a god, display a bit
		// of information about the source. This is to help liaisons
		// when answering questions.
		if(gAgent.isGodlike())
		{
			// XUI:translate
			std::ostringstream bonus_info;
			bonus_info << "*** parent estate: "
				<< parent_estate_id
				<< ((parent_estate_id == 1) ? ", mainland" : "")
				<< ((parent_estate_id == 5) ? ", teen" : "");

			// once we have web-services (or something) which returns
			// information about a region id, we can print this out
			// and even have it link to map-teleport or something.
			//<< "*** region_id: " << region_id << std::endl
			//<< "*** position: " << position << std::endl;

			floater->addHistoryLine(bonus_info.str());
		}

		floater->addHistoryLine(msg);
		make_ui_sound("UISndNewIncomingIMSession");
	}

	if( !mTalkFloater->getVisible() && !floater->getVisible())
	{
		//if the IM window is not open and the floater is not visible (i.e. not torn off)
		LLFloater* previouslyActiveFloater = mTalkFloater->getActiveFloater();

		// select the newly added floater (or the floater with the new line added to it).
		// it should be there.
		mTalkFloater->selectFloater(floater);

		//there was a previously unseen IM, make that old tab flashing
		//it is assumed that the most recently unseen IM tab is the one current selected/active
		if ( previouslyActiveFloater && getIMReceived() )
		{
			mTalkFloater->setFloaterFlashing(previouslyActiveFloater, TRUE);
		}

		//notify of a new IM
		notifyNewIM();
	}
}

void LLIMView::notifyNewIM()
{
	if(!gIMView->getFloaterOpen())
	{
		mIMReceived = TRUE;
	}
}

BOOL LLIMView::getIMReceived() const
{
	return mIMReceived;
}

// This method returns TRUE if the local viewer has a session
// currently open keyed to the uuid. 
BOOL LLIMView::isIMSessionOpen(const LLUUID& uuid)
{
	LLFloaterIMPanel* floater = findFloaterBySession(uuid);
	if(floater) return TRUE;
	return FALSE;
}

// This adds a session to the talk view. The name is the local name of
// the session, dialog specifies the type of session. If the session
// exists, it is brought forward.  Specifying id = NULL results in an
// im session to everyone. Returns the uuid of the session.
LLUUID LLIMView::addSession(const std::string& name,
							  EInstantMessage dialog,
							  const LLUUID& other_participant_id)
{
	LLUUID session_id = compute_session_id(dialog, other_participant_id);
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if(!floater)
	{
		floater = createFloater(session_id, other_participant_id, name, dialog, TRUE);
		LLDynamicArray<LLUUID> ids;
		ids.put(other_participant_id);
		noteOfflineUsers(floater, ids);
		floater->addParticipants(ids);
		mTalkFloater->showFloater(floater);
	}
	else
	{
		floater->open();
	}
	//mTabContainer->selectTabPanel(panel);
	floater->setInputFocus(TRUE);
	return floater->getSessionID();
}

// Adds a session using the given session_id.  If the session already exists 
// the dialog type is assumed correct. Returns the uuid of the session.
LLUUID LLIMView::addSession(const std::string& name,
							  EInstantMessage dialog,
							  const LLUUID& session_id,
							  const LLDynamicArray<LLUUID>& ids)
{
	if (0 == ids.getLength())
	{
		return LLUUID::null;
	}

	LLUUID other_participant_id = ids[0];
	LLUUID new_session_id = session_id;
	if (new_session_id.isNull())
	{
		new_session_id = compute_session_id(dialog, other_participant_id);
	}

	LLFloaterIMPanel* floater = findFloaterBySession(new_session_id);
	if(!floater)
	{
		// On creation, use the first element of ids as the "other_participant_id"
		floater = createFloater(new_session_id, other_participant_id, name, dialog, TRUE);
		noteOfflineUsers(floater, ids);
	}
	floater->addParticipants(ids);
	mTalkFloater->showFloater(floater);
	//mTabContainer->selectTabPanel(panel);
	floater->setInputFocus(TRUE);
	return floater->getSessionID();
}

// This removes the panel referenced by the uuid, and then restores
// internal consistency. The internal pointer is not deleted.
void LLIMView::removeSession(const LLUUID& session_id)
{
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if(floater)
	{
		mFloaters.erase(floater->getHandle());
		mTalkFloater->removeFloater(floater);
		//mTabContainer->removeTabPanel(floater);
	}
}

void LLIMView::refresh()
{
	S32 old_scroll_pos = mNewIMFloater->getScrollPos();
	mNewIMFloater->clearAllTargets();

	// build a list of groups.
	LLLinkedList<LLGroupData> group_list( group_dictionary_sort );

	LLGroupData* group;
	S32 count = gAgent.mGroups.count();
	S32 i;
	// read/sort groups on the first pass.
	for(i = 0; i < count; ++i)
	{
		group = &(gAgent.mGroups.get(i));
		group_list.addDataSorted( group );
	}

	// add groups to the floater on the second pass.
	for(group = group_list.getFirstData();
		group;
		group = group_list.getNextData())
	{
		mNewIMFloater->addTarget(group->mID, group->mName,
							   (void*)(&GROUP_DIALOG), TRUE, FALSE);
	}

	// build a set of buddies in the current buddy list.
	LLCollectAllBuddies collector;
	LLAvatarTracker::instance().applyFunctor(collector);
	LLCollectAllBuddies::buddy_map_t::iterator it;
	LLCollectAllBuddies::buddy_map_t::iterator end;
	it = collector.mOnline.begin();
	end = collector.mOnline.end();
	for( ; it != end; ++it)
	{
		mNewIMFloater->addAgent((*it).second, (void*)(&DEFAULT_DIALOG), TRUE);
	}
	it = collector.mOffline.begin();
	end = collector.mOffline.end();
	for( ; it != end; ++it)
	{
		mNewIMFloater->addAgent((*it).second, (void*)(&DEFAULT_DIALOG), FALSE);
	}
	
	if(gAgent.isGodlike())
	{
		// XUI:translate
		mNewIMFloater->addTarget(LLUUID::null, "All Residents, All Grids",
							   (void*)(&EVERYONE_DIALOG), TRUE, FALSE);
	}

	mNewIMFloater->setScrollPos( old_scroll_pos );
}

// JC - This used to set console visibility.  It doesn't any more.
void LLIMView::setFloaterOpen(BOOL set_open)
{
	gSavedSettings.setBOOL("ShowIM", set_open);

	//RN "visible" and "open" are considered synonomous for now
	if (set_open)
	{
		mTalkFloater->open();		/*Flawfinder: ignore*/
	}
	else
	{
		mTalkFloater->close();
	}

	if( set_open )
	{
		// notifyNewIM();

		// We're showing the IM, so mark view as non-pending
		mIMReceived = FALSE;
	}
}


BOOL LLIMView::getFloaterOpen()
{
	return mTalkFloater->getVisible();
}
 
void LLIMView::pruneSessions()
{
	if(mNewIMFloater)
	{
		BOOL removed = TRUE;
		LLFloaterIMPanel* floater = NULL;
		while(removed)
		{
			removed = FALSE;
			std::set<LLViewHandle>::iterator handle_it;
			for(handle_it = mFloaters.begin();
				handle_it != mFloaters.end();
				++handle_it)
			{
				floater = (LLFloaterIMPanel*)LLFloater::getFloaterByHandle(*handle_it);
				if(floater && !mNewIMFloater->isUUIDAvailable(floater->getOtherParticipantID()))
				{
					// remove this floater
					removed = TRUE;
					mFloaters.erase(handle_it++);
					floater->close();
					break;
				}
			}
		}
	}
}


void LLIMView::disconnectAllSessions()
{
	if(mNewIMFloater)
	{
		mNewIMFloater->setEnabled(FALSE);
	}
	LLFloaterIMPanel* floater = NULL;
	std::set<LLViewHandle>::iterator handle_it;
	for(handle_it = mFloaters.begin();
		handle_it != mFloaters.end();
		++handle_it)
	{
		floater = (LLFloaterIMPanel*)LLFloater::getFloaterByHandle(*handle_it);
		if (floater)
		{
			floater->setEnabled(FALSE);
		}
	}
}


// This method returns the im panel corresponding to the uuid
// provided. The uuid can either be a session id or an agent
// id. Returns NULL if there is no matching panel.
LLFloaterIMPanel* LLIMView::findFloaterBySession(const LLUUID& session_id)
{
	LLFloaterIMPanel* rv = NULL;
	std::set<LLViewHandle>::iterator handle_it;
	for(handle_it = mFloaters.begin();
		handle_it != mFloaters.end();
		++handle_it)
	{
		rv = (LLFloaterIMPanel*)LLFloater::getFloaterByHandle(*handle_it);
		if(rv && session_id == rv->getSessionID())
		{
			break;
		}
		rv = NULL;
	}
	return rv;
}


BOOL LLIMView::hasSession(const LLUUID& session_id)
{
	return (findFloaterBySession(session_id) != NULL);
}


// create a floater and update internal representation for
// consistency. Returns the pointer, caller (the class instance since
// it is a private method) is not responsible for deleting the
// pointer.  Add the floater to this but do not select it.
LLFloaterIMPanel* LLIMView::createFloater(const LLUUID& session_id,
								   const LLUUID& other_participant_id,
								   const std::string& session_label,
								   EInstantMessage dialog,
								   BOOL user_initiated)
{
	if (session_id.isNull())
	{
		llwarns << "Creating LLFloaterIMPanel with null session ID" << llendl;
	}
	llinfos << "LLIMView::createFloater: from " << other_participant_id 
			<< " in session " << session_id << llendl;
	LLFloaterIMPanel* floater = new LLFloaterIMPanel(session_label,
													 LLRect(),
													 session_label,
													 session_id,
													 other_participant_id,
													 dialog);
	LLTabContainerCommon::eInsertionPoint i_pt = user_initiated ? LLTabContainerCommon::RIGHT_OF_CURRENT : LLTabContainerCommon::END;
	mTalkFloater->addFloater(floater, FALSE, i_pt);
	mFloaters.insert(floater->getHandle());
	return floater;
}

void LLIMView::noteOfflineUsers(LLFloaterIMPanel* floater,
								  const LLDynamicArray<LLUUID>& ids)
{
	S32 count = ids.count();
	if(count == 0)
	{
		floater->addHistoryLine(sOnlyUserMessage);
	}
	else
	{
		const LLRelationship* info = NULL;
		LLAvatarTracker& at = LLAvatarTracker::instance();
		for(S32 i = 0; i < count; ++i)
		{
			info = at.getBuddyInfo(ids.get(i));
			char first[DB_FIRST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			char last[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			if(info && !info->isOnline()
			   && gCacheName->getName(ids.get(i), first, last))
			{
				LLUIString offline = sOfflineMessage;
				offline.setArg("[FIRST]", first);
				offline.setArg("[LAST]", last);
				floater->addHistoryLine(offline);
			}
		}
	}
}

void LLIMView::processIMTypingStart(const LLIMInfo* im_info)
{
	processIMTypingCore(im_info, TRUE);
}

void LLIMView::processIMTypingStop(const LLIMInfo* im_info)
{
	processIMTypingCore(im_info, FALSE);
}

void LLIMView::processIMTypingCore(const LLIMInfo* im_info, BOOL typing)
{
	LLUUID session_id = compute_session_id(im_info->mIMType, im_info->mFromID);
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if (floater)
	{
		floater->processIMTyping(im_info, typing);
	}
}
