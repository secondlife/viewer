/** 
 * @file llchathistory.cpp
 * @brief LLTextEditor base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llchathistory.h"

#include "llavatarnamecache.h"
#include "llinstantmessage.h"

#include "llimview.h"
#include "llcommandhandler.h"
#include "llpanel.h"
#include "lluictrlfactory.h"
#include "llscrollcontainer.h"
#include "llavatariconctrl.h"
#include "llcallingcard.h" //for LLAvatarTracker
#include "llagentdata.h"
#include "llavataractions.h"
#include "lltrans.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llmutelist.h"
#include "llstylemap.h"
#include "llslurl.h"
#include "lllayoutstack.h"
#include "llagent.h"
#include "llnotificationsutil.h"
#include "lltoastnotifypanel.h"
#include "lltooltip.h"
#include "llviewerregion.h"
#include "llviewertexteditor.h"
#include "llworld.h"
#include "lluiconstants.h"
#include "llstring.h"

#include "llviewercontrol.h"

static LLDefaultChildRegistry::Register<LLChatHistory> r("chat_history");

const static std::string NEW_LINE(rawstr_to_utf8("\n"));

const static std::string SLURL_APP_AGENT = "secondlife:///app/agent/";
const static std::string SLURL_ABOUT = "/about";

// support for secondlife:///app/objectim/{UUID}/ SLapps
class LLObjectIMHandler : public LLCommandHandler
{
public:
	// requests will be throttled from a non-trusted browser
	LLObjectIMHandler() : LLCommandHandler("objectim", UNTRUSTED_THROTTLE) {}

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (params.size() < 1)
		{
			return false;
		}

		LLUUID object_id;
		if (!object_id.set(params[0], FALSE))
		{
			return false;
		}

		LLSD payload;
		payload["object_id"] = object_id;
		payload["owner_id"] = query_map["owner"];
		payload["name"] = query_map["name"];
		payload["slurl"] = LLWeb::escapeURL(query_map["slurl"]);
		payload["group_owned"] = query_map["groupowned"];
		LLFloaterReg::showInstance("inspect_remote_object", payload);
		return true;
	}
};
LLObjectIMHandler gObjectIMHandler;

class LLChatHistoryHeader: public LLPanel
{
public:
	LLChatHistoryHeader()
	:	LLPanel(),
		mPopupMenuHandleAvatar(),
		mPopupMenuHandleObject(),
		mAvatarID(),
		mSourceType(CHAT_SOURCE_UNKNOWN),
		mFrom(),
		mSessionID(),
		mMinUserNameWidth(0),
		mUserNameFont(NULL)
	{}

	static LLChatHistoryHeader* createInstance(const std::string& file_name)
	{
		LLChatHistoryHeader* pInstance = new LLChatHistoryHeader;
		pInstance->buildFromFile(file_name);	
		return pInstance;
	}

	~LLChatHistoryHeader()
	{
		// Detach the info button so that it doesn't get destroyed (EXT-8463).
		hideInfoCtrl();
	}

	BOOL handleMouseUp(S32 x, S32 y, MASK mask)
	{
		return LLPanel::handleMouseUp(x,y,mask);
	}

	void onObjectIconContextMenuItemClicked(const LLSD& userdata)
	{
		std::string level = userdata.asString();

		if (level == "profile")
		{
			LLFloaterReg::showInstance("inspect_remote_object", mObjectData);
		}
		else if (level == "block")
		{
			LLMuteList::getInstance()->add(LLMute(getAvatarId(), mFrom, LLMute::OBJECT));

			LLFloaterSidePanelContainer::showPanel("people", "panel_block_list_sidetray", LLSD().with("blocked_to_select", getAvatarId()));
		}
	}

	void onAvatarIconContextMenuItemClicked(const LLSD& userdata)
	{
		std::string level = userdata.asString();

		if (level == "profile")
		{
			LLAvatarActions::showProfile(getAvatarId());
		}
		else if (level == "im")
		{
			LLAvatarActions::startIM(getAvatarId());
		}
		else if (level == "add")
		{
			LLAvatarActions::requestFriendshipDialog(getAvatarId(), mFrom);
		}
		else if (level == "remove")
		{
			LLAvatarActions::removeFriendDialog(getAvatarId());
		}
	}

	BOOL postBuild()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

		registrar.add("AvatarIcon.Action", boost::bind(&LLChatHistoryHeader::onAvatarIconContextMenuItemClicked, this, _2));
		registrar.add("ObjectIcon.Action", boost::bind(&LLChatHistoryHeader::onObjectIconContextMenuItemClicked, this, _2));

		LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_avatar_icon.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		mPopupMenuHandleAvatar = menu->getHandle();

		menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_object_icon.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		mPopupMenuHandleObject = menu->getHandle();

		setDoubleClickCallback(boost::bind(&LLChatHistoryHeader::showInspector, this));

		setMouseEnterCallback(boost::bind(&LLChatHistoryHeader::showInfoCtrl, this));
		setMouseLeaveCallback(boost::bind(&LLChatHistoryHeader::hideInfoCtrl, this));

		return LLPanel::postBuild();
	}

	bool pointInChild(const std::string& name,S32 x,S32 y)
	{
		LLUICtrl* child = findChild<LLUICtrl>(name);
		if(!child)
			return false;
		
		LLView* parent = child->getParent();
		if(parent!=this)
		{
			x-=parent->getRect().mLeft;
			y-=parent->getRect().mBottom;
		}

		S32 local_x = x - child->getRect().mLeft ;
		S32 local_y = y - child->getRect().mBottom ;
		return 	child->pointInView(local_x, local_y);
	}

	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask)
	{
		if(pointInChild("avatar_icon",x,y) || pointInChild("user_name",x,y))
		{
			showContextMenu(x,y);
			return TRUE;
		}

		return LLPanel::handleRightMouseDown(x,y,mask);
	}

	void showInspector()
	{
		if (mAvatarID.isNull() && CHAT_SOURCE_SYSTEM != mSourceType) return;
		
		if (mSourceType == CHAT_SOURCE_OBJECT)
		{
			LLFloaterReg::showInstance("inspect_remote_object", mObjectData);
		}
		else if (mSourceType == CHAT_SOURCE_AGENT)
		{
			LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mAvatarID));
		}
		//if chat source is system, you may add "else" here to define behaviour.
	}

	static void onClickInfoCtrl(LLUICtrl* info_ctrl)
	{
		if (!info_ctrl) return;

		LLChatHistoryHeader* header = dynamic_cast<LLChatHistoryHeader*>(info_ctrl->getParent());	
		if (!header) return;

		header->showInspector();
	}


	const LLUUID&		getAvatarId () const { return mAvatarID;}

	void setup(const LLChat& chat, const LLStyle::Params& style_params, const LLSD& args)
	{
		mAvatarID = chat.mFromID;
		mSessionID = chat.mSessionID;
		mSourceType = chat.mSourceType;

		//*TODO overly defensive thing, source type should be maintained out there
		if((chat.mFromID.isNull() && chat.mFromName.empty()) || (chat.mFromName == SYSTEM_FROM && chat.mFromID.isNull()))
		{
			mSourceType = CHAT_SOURCE_SYSTEM;
		}  

		mUserNameFont = style_params.font();
		LLTextBox* user_name = getChild<LLTextBox>("user_name");
		user_name->setReadOnlyColor(style_params.readonly_color());
		user_name->setColor(style_params.color());

		if (chat.mFromName.empty()
			|| mSourceType == CHAT_SOURCE_SYSTEM)
		{
			mFrom = LLTrans::getString("SECOND_LIFE");
			user_name->setValue(mFrom);
			updateMinUserNameWidth();
		}
		else if (mSourceType == CHAT_SOURCE_AGENT
				 && !mAvatarID.isNull()
				 && chat.mChatStyle != CHAT_STYLE_HISTORY)
		{
			// ...from a normal user, lookup the name and fill in later.
			// *NOTE: Do not do this for chat history logs, otherwise the viewer
			// will flood the People API with lookup requests on startup

			// Start with blank so sample data from XUI XML doesn't
			// flash on the screen
			user_name->setValue( LLSD() );
			LLAvatarNameCache::get(mAvatarID,
				boost::bind(&LLChatHistoryHeader::onAvatarNameCache, this, _1, _2));
		}
		else if (chat.mChatStyle == CHAT_STYLE_HISTORY ||
				 mSourceType == CHAT_SOURCE_AGENT)
		{
			//if it's an avatar name with a username add formatting
			S32 username_start = chat.mFromName.rfind(" (");
			S32 username_end = chat.mFromName.rfind(')');
			
			if (username_start != std::string::npos &&
				username_end == (chat.mFromName.length() - 1))
			{
				mFrom = chat.mFromName.substr(0, username_start);
				user_name->setValue(mFrom);

				if (gSavedSettings.getBOOL("NameTagShowUsernames"))
				{
					std::string username = chat.mFromName.substr(username_start + 2);
					username = username.substr(0, username.length() - 1);
					LLStyle::Params style_params_name;
					LLColor4 userNameColor = LLUIColorTable::instance().getColor("EmphasisColor");
					style_params_name.color(userNameColor);
					style_params_name.font.name("SansSerifSmall");
					style_params_name.font.style("NORMAL");
					style_params_name.readonly_color(userNameColor);
					user_name->appendText("  - " + username, FALSE, style_params_name);
				}
			}
			else
			{
				mFrom = chat.mFromName;
				user_name->setValue(mFrom);
				updateMinUserNameWidth();
			}
		}
		else
		{
			// ...from an object, just use name as given
			mFrom = chat.mFromName;
			user_name->setValue(mFrom);
			updateMinUserNameWidth();
		}


		setTimeField(chat);

		// Set up the icon.
		LLAvatarIconCtrl* icon = getChild<LLAvatarIconCtrl>("avatar_icon");

		if(mSourceType != CHAT_SOURCE_AGENT ||	mAvatarID.isNull())
			icon->setDrawTooltip(false);

		switch (mSourceType)
		{
			case CHAT_SOURCE_AGENT:
				icon->setValue(chat.mFromID);
				break;
			case CHAT_SOURCE_OBJECT:
				icon->setValue(LLSD("OBJECT_Icon"));
				break;
			case CHAT_SOURCE_SYSTEM:
				icon->setValue(LLSD("SL_Logo"));
				break;
			case CHAT_SOURCE_UNKNOWN: 
				icon->setValue(LLSD("Unknown_Icon"));
		}

		// In case the message came from an object, save the object info
		// to be able properly show its profile.
		if ( chat.mSourceType == CHAT_SOURCE_OBJECT)
		{
			std::string slurl = args["slurl"].asString();
			if (slurl.empty())
			{
				LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosAgent(chat.mPosAgent);
				if(region)
				{
					LLSLURL region_slurl(region->getName(), chat.mPosAgent);
					slurl = region_slurl.getLocationString();
				}
			}

			LLSD payload;
			payload["object_id"]	= chat.mFromID;
			payload["name"]			= chat.mFromName;
			payload["owner_id"]		= chat.mOwnerID;
			payload["slurl"]		= LLWeb::escapeURL(slurl);

			mObjectData = payload;
		}
	}

	/*virtual*/ void draw()
	{
		LLTextBox* user_name = getChild<LLTextBox>("user_name");
		LLTextBox* time_box = getChild<LLTextBox>("time_box");

		LLRect user_name_rect = user_name->getRect();
		S32 user_name_width = user_name_rect.getWidth();
		S32 time_box_width = time_box->getRect().getWidth();

		if (time_box->getVisible() && user_name_width <= mMinUserNameWidth)
		{
			time_box->setVisible(FALSE);

			user_name_rect.mRight += time_box_width;
			user_name->reshape(user_name_rect.getWidth(), user_name_rect.getHeight());
			user_name->setRect(user_name_rect);
		}

		if (!time_box->getVisible() && user_name_width > mMinUserNameWidth + time_box_width)
		{
			user_name_rect.mRight -= time_box_width;
			user_name->reshape(user_name_rect.getWidth(), user_name_rect.getHeight());
			user_name->setRect(user_name_rect);

			time_box->setVisible(TRUE);
		}

		LLPanel::draw();
	}

	void updateMinUserNameWidth()
	{
		if (mUserNameFont)
		{
			LLTextBox* user_name = getChild<LLTextBox>("user_name");
			const LLWString& text = user_name->getWText();
			mMinUserNameWidth = mUserNameFont->getWidth(text.c_str()) + PADDING;
		}
	}

	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
	{
		mFrom = av_name.mDisplayName;

		LLTextBox* user_name = getChild<LLTextBox>("user_name");
		user_name->setValue( LLSD(av_name.mDisplayName ) );
		user_name->setToolTip( av_name.mUsername );

		if (gSavedSettings.getBOOL("NameTagShowUsernames") && 
			LLAvatarNameCache::useDisplayNames() &&
			!av_name.mIsDisplayNameDefault)
		{
			LLStyle::Params style_params_name;
			LLColor4 userNameColor = LLUIColorTable::instance().getColor("EmphasisColor");
			style_params_name.color(userNameColor);
			style_params_name.font.name("SansSerifSmall");
			style_params_name.font.style("NORMAL");
			style_params_name.readonly_color(userNameColor);
			user_name->appendText("  - " + av_name.mUsername, FALSE, style_params_name);
		}
		setToolTip( av_name.mUsername );
		// name might have changed, update width
		updateMinUserNameWidth();
	}

protected:
	static const S32 PADDING = 20;

	void showContextMenu(S32 x,S32 y)
	{
		if(mSourceType == CHAT_SOURCE_SYSTEM)
			showSystemContextMenu(x,y);
		if(mAvatarID.notNull() && mSourceType == CHAT_SOURCE_AGENT)
			showAvatarContextMenu(x,y);
		if(mAvatarID.notNull() && mSourceType == CHAT_SOURCE_OBJECT && SYSTEM_FROM != mFrom)
			showObjectContextMenu(x,y);
	}

	void showSystemContextMenu(S32 x,S32 y)
	{
	}
	
	void showObjectContextMenu(S32 x,S32 y)
	{
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandleObject.get();
		if(menu)
			LLMenuGL::showPopup(this, menu, x, y);
	}
	
	void showAvatarContextMenu(S32 x,S32 y)
	{
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandleAvatar.get();

		if(menu)
		{
			bool is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;
			
			menu->setItemEnabled("Add Friend", !is_friend);
			menu->setItemEnabled("Remove Friend", is_friend);

			if(gAgentID == mAvatarID)
			{
				menu->setItemEnabled("Add Friend", false);
				menu->setItemEnabled("Send IM", false);
				menu->setItemEnabled("Remove Friend", false);
			}

			if (mSessionID == LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, mAvatarID))
			{
				menu->setItemVisible("Send IM", false);
			}

			menu->buildDrawLabels();
			menu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(this, menu, x, y);
		}
	}

	void showInfoCtrl()
	{
		if (mAvatarID.isNull() || mFrom.empty() || CHAT_SOURCE_SYSTEM == mSourceType) return;
				
		if (!sInfoCtrl)
		{
			// *TODO: Delete the button at exit.
			sInfoCtrl = LLUICtrlFactory::createFromFile<LLUICtrl>("inspector_info_ctrl.xml", NULL, LLPanel::child_registry_t::instance());
			if (sInfoCtrl)
			{
				sInfoCtrl->setCommitCallback(boost::bind(&LLChatHistoryHeader::onClickInfoCtrl, sInfoCtrl));
			}
		}

		if (!sInfoCtrl)
		{
			llassert(sInfoCtrl != NULL);
			return;
		}

		LLTextBox* name = getChild<LLTextBox>("user_name");
		LLRect sticky_rect = name->getRect();
		S32 icon_x = llmin(sticky_rect.mLeft + name->getTextBoundingRect().getWidth() + 7, sticky_rect.mRight - 3);
		sInfoCtrl->setOrigin(icon_x, sticky_rect.getCenterY() - sInfoCtrl->getRect().getHeight() / 2 ) ;
		addChild(sInfoCtrl);
	}

	void hideInfoCtrl()
	{
		if (!sInfoCtrl) return;

		if (sInfoCtrl->getParent() == this)
		{
			removeChild(sInfoCtrl);
		}
	}

private:
	void setTimeField(const LLChat& chat)
	{
		LLTextBox* time_box = getChild<LLTextBox>("time_box");

		LLRect rect_before = time_box->getRect();

		time_box->setValue(chat.mTimeStr);

		// set necessary textbox width to fit all text
		time_box->reshapeToFitText();
		LLRect rect_after = time_box->getRect();

		// move rect to the left to correct position...
		S32 delta_pos_x = rect_before.getWidth() - rect_after.getWidth();
		S32 delta_pos_y = rect_before.getHeight() - rect_after.getHeight();
		time_box->translate(delta_pos_x, delta_pos_y);

		//... & change width of the name control
		LLView* user_name = getChild<LLView>("user_name");
		const LLRect& user_rect = user_name->getRect();
		user_name->reshape(user_rect.getWidth() + delta_pos_x, user_rect.getHeight());
	}

protected:
	LLHandle<LLView>	mPopupMenuHandleAvatar;
	LLHandle<LLView>	mPopupMenuHandleObject;

	static LLUICtrl*	sInfoCtrl;

	LLUUID			    mAvatarID;
	LLSD				mObjectData;
	EChatSourceType		mSourceType;
	std::string			mFrom;
	LLUUID				mSessionID;

	S32					mMinUserNameWidth;
	const LLFontGL*		mUserNameFont;
};

LLUICtrl* LLChatHistoryHeader::sInfoCtrl = NULL;

LLChatHistory::LLChatHistory(const LLChatHistory::Params& p)
:	LLUICtrl(p),
	mMessageHeaderFilename(p.message_header),
	mMessageSeparatorFilename(p.message_separator),
	mLeftTextPad(p.left_text_pad),
	mRightTextPad(p.right_text_pad),
	mLeftWidgetPad(p.left_widget_pad),
	mRightWidgetPad(p.right_widget_pad),
	mTopSeparatorPad(p.top_separator_pad),
	mBottomSeparatorPad(p.bottom_separator_pad),
	mTopHeaderPad(p.top_header_pad),
	mBottomHeaderPad(p.bottom_header_pad),
	mIsLastMessageFromLog(false)
{
	LLTextEditor::Params editor_params(p);
	editor_params.rect = getLocalRect();
	editor_params.follows.flags = FOLLOWS_ALL;
	editor_params.enabled = false; // read only
	editor_params.show_context_menu = "true";
	mEditor = LLUICtrlFactory::create<LLTextEditor>(editor_params, this);
}

LLChatHistory::~LLChatHistory()
{
	this->clear();
}

void LLChatHistory::initFromParams(const LLChatHistory::Params& p)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	LLRect stack_rect = getLocalRect();
	stack_rect.mRight -= scrollbar_size;
	LLLayoutStack::Params layout_p;
	layout_p.rect = stack_rect;
	layout_p.follows.flags = FOLLOWS_ALL;
	layout_p.orientation = LLLayoutStack::VERTICAL;
	layout_p.mouse_opaque = false;
	
	LLLayoutStack* stackp = LLUICtrlFactory::create<LLLayoutStack>(layout_p, this);
	
	const S32 NEW_TEXT_NOTICE_HEIGHT = 20;
	
	LLLayoutPanel::Params panel_p;
	panel_p.name = "spacer";
	panel_p.background_visible = false;
	panel_p.has_border = false;
	panel_p.mouse_opaque = false;
	panel_p.min_dim = 30;
	panel_p.auto_resize = true;
	panel_p.user_resize = false;

	stackp->addPanel(LLUICtrlFactory::create<LLLayoutPanel>(panel_p), LLLayoutStack::ANIMATE);

	panel_p.name = "new_text_notice_holder";
	LLRect new_text_notice_rect = getLocalRect();
	new_text_notice_rect.mTop = new_text_notice_rect.mBottom + NEW_TEXT_NOTICE_HEIGHT;
	panel_p.rect = new_text_notice_rect;
	panel_p.background_opaque = true;
	panel_p.background_visible = true;
	panel_p.visible = false;
	panel_p.min_dim = 0;
	panel_p.auto_resize = false;
	panel_p.user_resize = false;
	mMoreChatPanel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	
	LLTextBox::Params text_p(p.more_chat_text);
	text_p.rect = mMoreChatPanel->getLocalRect();
	text_p.follows.flags = FOLLOWS_ALL;
	text_p.name = "more_chat_text";
	mMoreChatText = LLUICtrlFactory::create<LLTextBox>(text_p, mMoreChatPanel);
	mMoreChatText->setClickedCallback(boost::bind(&LLChatHistory::onClickMoreText, this));

	stackp->addPanel(mMoreChatPanel, LLLayoutStack::ANIMATE);
}


/*void LLChatHistory::updateTextRect()
{
	static LLUICachedControl<S32> texteditor_border ("UITextEditorBorder", 0);

	LLRect old_text_rect = mVisibleTextRect;
	mVisibleTextRect = mScroller->getContentWindowRect();
	mVisibleTextRect.stretch(-texteditor_border);
	mVisibleTextRect.mLeft += mLeftTextPad;
	mVisibleTextRect.mRight -= mRightTextPad;
	if (mVisibleTextRect != old_text_rect)
	{
		needsReflow();
	}
}*/

LLView* LLChatHistory::getSeparator()
{
	LLPanel* separator = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>(mMessageSeparatorFilename, NULL, LLPanel::child_registry_t::instance());
	return separator;
}

LLView* LLChatHistory::getHeader(const LLChat& chat,const LLStyle::Params& style_params, const LLSD& args)
{
	LLChatHistoryHeader* header = LLChatHistoryHeader::createInstance(mMessageHeaderFilename);
	header->setup(chat, style_params, args);
	return header;
}

void LLChatHistory::onClickMoreText()
{
	mEditor->endOfDoc();
}

void LLChatHistory::clear()
{
	mLastFromName.clear();
	mEditor->clear();
	mLastFromID = LLUUID::null;
}

static LLFastTimer::DeclareTimer FTM_APPEND_MESSAGE("Append Chat Message");

void LLChatHistory::appendMessage(const LLChat& chat, const LLSD &args, const LLStyle::Params& input_append_params)
{
	LLFastTimer _(FTM_APPEND_MESSAGE);
	bool use_plain_text_chat_history = args["use_plain_text_chat_history"].asBoolean();

	llassert(mEditor);
	if (!mEditor)
	{
		return;
	}

	mEditor->setPlainText(use_plain_text_chat_history);

	if (!mEditor->scrolledToEnd() && chat.mFromID != gAgent.getID() && !chat.mFromName.empty())
	{
		mUnreadChatSources.insert(chat.mFromName);
		mMoreChatPanel->setVisible(TRUE);
		std::string chatters;
		for (unread_chat_source_t::iterator it = mUnreadChatSources.begin();
			it != mUnreadChatSources.end();)
		{
			chatters += *it;
			if (++it != mUnreadChatSources.end())
			{
				chatters += ", ";
			}
		}
		LLStringUtil::format_map_t args;
		args["SOURCES"] = chatters;

		if (mUnreadChatSources.size() == 1)
		{
			mMoreChatText->setValue(LLTrans::getString("unread_chat_single", args));
		}
		else
		{
			mMoreChatText->setValue(LLTrans::getString("unread_chat_multiple", args));
		}
		S32 height = mMoreChatText->getTextPixelHeight() + 5;
		mMoreChatPanel->reshape(mMoreChatPanel->getRect().getWidth(), height);
	}

	LLColor4 txt_color = LLUIColorTable::instance().getColor("White");
	LLViewerChat::getChatColor(chat,txt_color);
	LLFontGL* fontp = LLViewerChat::getChatFont();	
	std::string font_name = LLFontGL::nameFromFont(fontp);
	std::string font_size = LLFontGL::sizeFromFont(fontp);	
	LLStyle::Params style_params;
	style_params.color(txt_color);
	style_params.readonly_color(txt_color);
	style_params.font.name(font_name);
	style_params.font.size(font_size);	
	style_params.font.style(input_append_params.font.style);

	std::string prefix = chat.mText.substr(0, 4);

	//IRC styled /me messages.
	bool irc_me = prefix == "/me " || prefix == "/me'";

	// Delimiter after a name in header copy/past and in plain text mode
	std::string delimiter = ": ";
	std::string shout = LLTrans::getString("shout");
	std::string whisper = LLTrans::getString("whisper");
	if (chat.mChatType == CHAT_TYPE_SHOUT || 
		chat.mChatType == CHAT_TYPE_WHISPER ||
		chat.mText.compare(0, shout.length(), shout) == 0 ||
		chat.mText.compare(0, whisper.length(), whisper) == 0)
	{
		delimiter = " ";
	}

	// Don't add any delimiter after name in irc styled messages
	if (irc_me || chat.mChatStyle == CHAT_STYLE_IRC)
	{
		delimiter = LLStringUtil::null;
		style_params.font.style = "ITALIC";
	}

	bool message_from_log = chat.mChatStyle == CHAT_STYLE_HISTORY;
	// We graying out chat history by graying out messages that contains full date in a time string
	if (message_from_log)
	{
		style_params.color(LLColor4::grey);
		style_params.readonly_color(LLColor4::grey);
	}

	if (use_plain_text_chat_history)
	{
		LLStyle::Params timestamp_style(style_params);
		if (!message_from_log)
		{
			LLColor4 timestamp_color = LLUIColorTable::instance().getColor("ChatTimestampColor");
			timestamp_style.color(timestamp_color);
			timestamp_style.readonly_color(timestamp_color);
		}
		mEditor->appendText("[" + chat.mTimeStr + "] ", mEditor->getLength() != 0, timestamp_style);

		if (utf8str_trim(chat.mFromName).size() != 0)
		{
			// Don't hotlink any messages from the system (e.g. "Second Life:"), so just add those in plain text.
			if ( chat.mSourceType == CHAT_SOURCE_OBJECT && chat.mFromID.notNull())
			{
				// for object IMs, create a secondlife:///app/objectim SLapp
				std::string url = LLViewerChat::getSenderSLURL(chat, args);

				// set the link for the object name to be the objectim SLapp
				// (don't let object names with hyperlinks override our objectim Url)
				LLStyle::Params link_params(style_params);
				LLColor4 link_color = LLUIColorTable::instance().getColor("HTMLLinkColor");
				link_params.color = link_color;
				link_params.readonly_color = link_color;
				link_params.is_link = true;
				link_params.link_href = url;

				mEditor->appendText(chat.mFromName + delimiter,
									false, link_params);
			}
			else if ( chat.mFromName != SYSTEM_FROM && chat.mFromID.notNull() && !message_from_log)
			{
				LLStyle::Params link_params(style_params);
				link_params.overwriteFrom(LLStyleMap::instance().lookupAgent(chat.mFromID));

				// Add link to avatar's inspector and delimiter to message.
				mEditor->appendText(std::string(link_params.link_href) + delimiter, false, link_params);
			}
			else
			{
				mEditor->appendText("<nolink>" + chat.mFromName + "</nolink>" + delimiter, false, style_params);
			}
		}
	}
	else
	{
		LLView* view = NULL;
		LLInlineViewSegment::Params p;
		p.force_newline = true;
		p.left_pad = mLeftWidgetPad;
		p.right_pad = mRightWidgetPad;

		LLDate new_message_time = LLDate::now();

		if (mLastFromName == chat.mFromName 
			&& mLastFromID == chat.mFromID
			&& mLastMessageTime.notNull() 
			&& (new_message_time.secondsSinceEpoch() - mLastMessageTime.secondsSinceEpoch()) < 60.0
			&& mIsLastMessageFromLog == message_from_log)  //distinguish between current and previous chat session's histories
		{
			view = getSeparator();
			p.top_pad = mTopSeparatorPad;
			p.bottom_pad = mBottomSeparatorPad;
		}
		else
		{
			view = getHeader(chat, style_params, args);
			if (mEditor->getLength() == 0)
				p.top_pad = 0;
			else
				p.top_pad = mTopHeaderPad;
			p.bottom_pad = mBottomHeaderPad;
			
		}
		p.view = view;

		//Prepare the rect for the view
		LLRect target_rect = mEditor->getDocumentView()->getRect();
		// squeeze down the widget by subtracting padding off left and right
		target_rect.mLeft += mLeftWidgetPad + mEditor->getHPad();
		target_rect.mRight -= mRightWidgetPad;
		view->reshape(target_rect.getWidth(), view->getRect().getHeight());
		view->setOrigin(target_rect.mLeft, view->getRect().mBottom);

		std::string widget_associated_text = "\n[" + chat.mTimeStr + "] ";
		if (utf8str_trim(chat.mFromName).size() != 0 && chat.mFromName != SYSTEM_FROM)
			widget_associated_text += chat.mFromName + delimiter;

		mEditor->appendWidget(p, widget_associated_text, false);
		mLastFromName = chat.mFromName;
		mLastFromID = chat.mFromID;
		mLastMessageTime = new_message_time;
		mIsLastMessageFromLog = message_from_log;
	}

	if (chat.mNotifId.notNull())
	{
		LLNotificationPtr notification = LLNotificationsUtil::find(chat.mNotifId);
		if (notification != NULL)
		{
			LLIMToastNotifyPanel* notify_box = new LLIMToastNotifyPanel(
					notification, chat.mSessionID, LLRect::null, !use_plain_text_chat_history);
			//we can't set follows in xml since it broke toasts behavior
			notify_box->setFollowsLeft();
			notify_box->setFollowsRight();
			notify_box->setFollowsTop();

			ctrl_list_t ctrls = notify_box->getControlPanel()->getCtrlList();
			S32 offset = 0;
			// Children were added by addChild() which uses push_front to insert them into list,
			// so to get buttons in correct order reverse iterator is used (EXT-5906) 
			for (ctrl_list_t::reverse_iterator it = ctrls.rbegin(); it != ctrls.rend(); it++)
			{
				LLButton * button = dynamic_cast<LLButton*> (*it);
				if (button != NULL)
				{
					button->setOrigin( offset,
							button->getRect().mBottom);
					button->setLeftHPad(2 * HPAD);
					button->setRightHPad(2 * HPAD);
					// set zero width before perform autoResize()
					button->setRect(LLRect(button->getRect().mLeft,
							button->getRect().mTop, button->getRect().mLeft,
							button->getRect().mBottom));
					button->setAutoResize(true);
					button->autoResize();
					offset += HPAD + button->getRect().getWidth();
					button->setFollowsNone();
				}
			}

			//Prepare the rect for the view
			LLRect target_rect = mEditor->getDocumentView()->getRect();
			// squeeze down the widget by subtracting padding off left and right
			target_rect.mLeft += mLeftWidgetPad + mEditor->getHPad();
			target_rect.mRight -= mRightWidgetPad;
			notify_box->reshape(target_rect.getWidth(),	notify_box->getRect().getHeight());
			notify_box->setOrigin(target_rect.mLeft, notify_box->getRect().mBottom);

			LLInlineViewSegment::Params params;
			params.view = notify_box;
			params.left_pad = mLeftWidgetPad;
			params.right_pad = mRightWidgetPad;
			mEditor->appendWidget(params, "\n", false);
		}
	}
	else
	{
		std::string message = irc_me ? chat.mText.substr(3) : chat.mText;


		//MESSAGE TEXT PROCESSING
		//*HACK getting rid of redundant sender names in system notifications sent using sender name (see EXT-5010)
		if (use_plain_text_chat_history && gAgentID != chat.mFromID && chat.mFromID.notNull())
		{
			std::string slurl_about = SLURL_APP_AGENT + chat.mFromID.asString() + SLURL_ABOUT;
			if (message.length() > slurl_about.length() && 
				message.compare(0, slurl_about.length(), slurl_about) == 0)
			{
				message = message.substr(slurl_about.length(), message.length()-1);
			}
		}

		if (irc_me && !use_plain_text_chat_history)
		{
			message = chat.mFromName + message;
		}
		
		mEditor->appendText(message, FALSE, style_params);
	}

	mEditor->blockUndo();

	// automatically scroll to end when receiving chat from myself
	if (chat.mFromID == gAgentID)
	{
		mEditor->setCursorAndScrollToEnd();
	}
}

void LLChatHistory::draw()
{
	if (mEditor->scrolledToEnd())
	{
		mUnreadChatSources.clear();
		mMoreChatPanel->setVisible(FALSE);
	}

	LLUICtrl::draw();
}
