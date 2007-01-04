/** 
 * @file llpanelavatar.h
 * @brief LLPanelAvatar and related class definitions
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELAVATAR_H
#define LL_LLPANELAVATAR_H

#include "llpanel.h"
#include "v3dmath.h"
#include "lluuid.h"
#include "linked_lists.h"
#include "llmozlib.h"

class LLButton;
class LLCheckBoxCtrl;
class LLDropTarget;
class LLInventoryItem;
class LLLineEditor;
class LLNameEditor;
class LLPanelAvatar;
class LLScrollListCtrl;
class LLTabContainerCommon;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLUICtrl;
class LLViewerImage;
class LLViewerObject;
class LLMessageSystem;
class LLIconCtrl;
class LLWebBrowserCtrl;

enum EOnlineStatus
{
	ONLINE_STATUS_UNKNOWN = -1,
	ONLINE_STATUS_NO      = 0,
	ONLINE_STATUS_YES     = 1
};

class LLPanelAvatarFirstLife
: public LLPanel
{
public:
	LLPanelAvatarFirstLife(const std::string& name, const LLRect &rect, LLPanelAvatar* panel_avatar);
	/*virtual*/ ~LLPanelAvatarFirstLife();
	/*virtual*/ BOOL	postBuild(void);

	void enableControls(BOOL own_avatar);

protected:
	LLPanelAvatar* mPanelAvatar;
};


class LLPanelAvatarSecondLife
: public LLPanel
{
public:
	LLPanelAvatarSecondLife(const std::string& name, const LLRect &rect, LLPanelAvatar* panel_avatar );
	/*virtual*/ ~LLPanelAvatarSecondLife();

	/*virtual*/ BOOL	postBuild(void);
	/*virtual*/ void draw();

	static void onClickImage(			void *userdata);
	static void onClickFriends(			void *userdata);
	static void onDoubleClickGroup(void* userdata);
	static void onClickPublishHelp(void *userdata);

	// Clear out the controls anticipating new network data.
	void clearControls();
	void enableControls(BOOL own_avatar);
	void updateOnlineText(BOOL online, BOOL have_calling_card);
	void updatePartnerName();

	void setPartnerID(LLUUID id) { mPartnerID = id; }
	
protected:
	LLPanelAvatar* mPanelAvatar;

	LLUUID				mPartnerID;
};

// WARNING!  The order of the inheritance here matters!!  Do not change.  - KLW
class LLPanelAvatarWeb : 
	public LLPanel
#if LL_LIBXUL_ENABLED
	, public LLEmbeddedBrowserWindowObserver
#endif
{
public:
	LLPanelAvatarWeb(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	/*virtual*/ ~LLPanelAvatarWeb();
	/*virtual*/ BOOL	postBuild(void);

	void enableControls(BOOL own_avatar);

	void setWebURL(std::string url);

	void load();
	void load(std::string url);
	static void onClickLoad(void* data);
	static void onClickOpen(void* data);
	static void onCommitURL(LLUICtrl* ctrl, void* data);
	static void onClickWebProfileHelp(void *);

#if LL_LIBXUL_ENABLED
	// browser observer impls
	virtual void onStatusTextChange( const EventType& eventIn );
	virtual void onLocationChange( const EventType& eventIn );
#endif

protected:
	LLPanelAvatar*		mPanelAvatar;
	std::string			mURL;
	LLWebBrowserCtrl*	mWebBrowser;
};

class LLPanelAvatarAdvanced : public LLPanel
{
public:
	LLPanelAvatarAdvanced(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	/*virtual*/ ~LLPanelAvatarAdvanced();
	/*virtual*/ BOOL	postBuild(void);

	void enableControls(BOOL own_avatar);
	void setWantSkills(U32 want_to_mask, const std::string& want_to_text,
					   U32 skills_mask, const std::string& skills_text,
					   const std::string& languages_text);
	void getWantSkills(U32* want_to_mask, std::string& want_to_text,
					   U32* skills_mask, std::string& skills_text,
					   std::string& languages_text);

protected:
	LLPanelAvatar*		mPanelAvatar;
	S32					mWantToCount;
	S32					mSkillsCount;
	LLCheckBoxCtrl		*mWantToCheck[8];
	LLLineEditor		*mWantToEdit;
	LLCheckBoxCtrl		*mSkillsCheck[8];
	LLLineEditor		*mSkillsEdit;
};


class LLPanelAvatarNotes : public LLPanel
{
public:
	LLPanelAvatarNotes(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	/*virtual*/ ~LLPanelAvatarNotes();
	/*virtual*/ BOOL	postBuild(void);

	void enableControls(BOOL own_avatar);

	static void onCommitNotes(LLUICtrl* field, void* userdata);

protected:
	LLPanelAvatar* mPanelAvatar;
};


class LLPanelAvatarClassified : public LLPanel
{
public:
	LLPanelAvatarClassified(const LLString& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	/*virtual*/ ~LLPanelAvatarClassified();

	/*virtual*/ BOOL	postBuild(void);
	/*virtual*/ void draw();

	void refresh();

	void apply();
	void enableControls(BOOL own_avatar);

	BOOL titleIsValid();

	// Delete all the classified sub-panels from the tab container
	void deleteClassifiedPanels();

	// Unpack the outline of classified for this avatar (count, names, but not
	// actual data).
	void processAvatarClassifiedReply(LLMessageSystem* msg, void**);

private:
	static void onClickNew(void* data);
	static void onClickDelete(void* data);

	static void callbackDelete(S32 option, void* data);
	static void callbackNew(S32 option, void* data);

private:
	LLPanelAvatar* mPanelAvatar;
};

class LLPanelAvatarPicks : public LLPanel
{
public:
	LLPanelAvatarPicks(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar);
	/*virtual*/ ~LLPanelAvatarPicks();

	/*virtual*/ BOOL	postBuild(void);
	/*virtual*/ void draw();

	void refresh();

	void enableControls(BOOL own_avatar);

	// Delete all the pick sub-panels from the tab container
	void deletePickPanels();

	// Unpack the outline of picks for this avatar (count, names, but not
	// actual data).
	void processAvatarPicksReply(LLMessageSystem* msg, void**);
	void processAvatarClassifiedReply(LLMessageSystem* msg, void**);

private:
	static void onClickNew(void* data);
	static void onClickDelete(void* data);

	static void callbackDelete(S32 option, void* data);

private:
	LLPanelAvatar* mPanelAvatar;
};


class LLPanelAvatar : public LLPanel
{
public:
	LLPanelAvatar(const std::string& name, const LLRect &rect, BOOL allow_edit);
	/*virtual*/ ~LLPanelAvatar();

	/*virtual*/ BOOL	postBuild(void);

	void setAvatar(LLViewerObject *avatarp);

	// Fill in the avatar ID and handle some field fill-in, as well as 
	// button enablement.
	// Pass one of the ONLINE_STATUS_foo constants above.
	void setAvatarID(const LLUUID &avatar_id, const LLString &name, EOnlineStatus online_status);

	const LLUUID& getAvatarID() const { return mAvatarID; }

	void disableRate();
	
	void resetGroupList();

	void sendAvatarStatisticsRequest();

	void sendAvatarPropertiesRequest();
	void sendAvatarPropertiesUpdate();

	void sendAvatarNotesRequest();
	void sendAvatarNotesUpdate();

	void sendAvatarPicksRequest();

	void selectTab(S32 tabnum);
	void selectTabByName(std::string tab_name);

	BOOL haveData() { return mHaveProperties && mHaveStatistics; }

	static void processAvatarPropertiesReply(LLMessageSystem *msg, void **);
	static void processAvatarInterestsReply(LLMessageSystem *msg, void **);
	static void processAvatarGroupsReply(LLMessageSystem* msg, void**);
	static void processAvatarStatisticsReply(LLMessageSystem *msg, void **);
	static void processAvatarNotesReply(LLMessageSystem *msg, void **);
	static void processAvatarPicksReply(LLMessageSystem *msg, void **);
	static void processAvatarClassifiedReply(LLMessageSystem *msg, void **);

	static void onClickTrack(	void *userdata);
	static void onClickIM(		void *userdata);
	static void onClickOfferTeleport(	void *userdata);
	static void onClickPay(	void *userdata);
	static void onClickRate(	void *userdata);
	static void onClickOK(		void *userdata);
	static void onClickCancel(	void *userdata);
	static void onClickKick(	void *userdata);
	static void onClickFreeze(	void *userdata);
	static void onClickUnfreeze(void *userdata);
	static void onClickCSR(		void *userdata);
	static void onClickMute(	void *userdata);
	static void onClickAddFriend(void* data);

	static void finishKick(S32 option, const LLString& text, void* userdata);
	static void finishFreeze(S32 option, const LLString& text, void* userdata);
	static void finishUnfreeze(S32 option, const LLString& text, void* userdata);

	static void showProfileCallback(S32 option, void *userdata);

	// Teen users are not allowed to see or enter data into the first life page,
	// or their own about/interests text entry fields.
	static BOOL sAllowFirstLife;

	static	void*	createPanelAvatar(void*	data);
	static	void*	createFloaterAvatarInfo(void*	data);
	static	void*	createPanelAvatarSecondLife(void*	data);
	static	void*	createPanelAvatarWeb(void*	data);
	static	void*	createPanelAvatarInterests(void*	data);
	static	void*	createPanelAvatarPicks(void*	data);
	static	void*	createPanelAvatarClassified(void*	data);
	static	void*	createPanelAvatarFirstLife(void*	data);
	static	void*	createPanelAvatarNotes(void*	data);

public:
	LLPanelAvatarSecondLife*	mPanelSecondLife;
	LLPanelAvatarAdvanced*		mPanelAdvanced;
	LLPanelAvatarClassified*	mPanelClassified;
	LLPanelAvatarPicks*			mPanelPicks;
	LLPanelAvatarNotes*			mPanelNotes;
	LLPanelAvatarFirstLife*		mPanelFirstLife;
	LLPanelAvatarWeb*			mPanelWeb;

	LLDropTarget* 				mDropTarget;
	
protected:
	void						enableOKIfReady();
	LLUUID						mAvatarID;			// for which avatar is this window?
	BOOL						mIsFriend;			// Are we friends?
	BOOL						mHaveProperties;
	BOOL						mHaveStatistics;
	LLTabContainerCommon*		mTab;
	BOOL						mAllowEdit;
	BOOL						mDisableRate;

	static LLLinkedList<LLPanelAvatar> sAllPanels;
};

// helper funcs
void add_left_label(LLPanel *panel, const LLString& name, S32 y);


#endif // LL_LLPANELAVATAR_H
