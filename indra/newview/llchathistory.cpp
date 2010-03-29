/** 
 * @file llchathistory.cpp
 * @brief LLTextEditor base class
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

#include "llinstantmessage.h"

#include "llimview.h"
#include "llchathistory.h"
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


#include "llsidetray.h"//for blocked objects panel

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
		payload["slurl"] = query_map["slurl"];
		payload["group_owned"] = query_map["groupowned"];
		LLFloaterReg::showInstance("inspect_remote_object", payload);
		return true;
	}
};
LLObjectIMHandler gObjectIMHandler;

class LLChatHistoryHeader: public LLPanel
{
public:
	static LLChatHistoryHeader* createInstance(const std::string& file_name)
	{
		LLChatHistoryHeader* pInstance = new LLChatHistoryHeader;
		LLUICtrlFactory::getInstance()->buildPanel(pInstance, file_name);	
		return pInstance;
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
			LLSD params;
			params["object_id"] = getAvatarId();

			LLFloaterReg::showInstance("inspect_object", params);
		}
		else if (level == "block")
		{
			LLMuteList::getInstance()->add(LLMute(getAvatarId(), mFrom, LLMute::OBJECT));

			LLSideTray::getInstance()->showPanel("panel_block_list_sidetray", LLSD().with("blocked_to_select", getAvatarId()));
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
			LLFloaterReg::showInstance("inspect_object", LLSD().with("object_id", mAvatarID));
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

	void setup(const LLChat& chat,const LLStyle::Params& style_params) 
	{
		mAvatarID = chat.mFromID;
		mSessionID = chat.mSessionID;
		mSourceType = chat.mSourceType;
		gCacheName->get(mAvatarID, false, boost::bind(&LLChatHistoryHeader::nameUpdatedCallback, this, _1, _2, _3));

		//*TODO overly defensive thing, source type should be maintained out there
		if((chat.mFromID.isNull() && chat.mFromName.empty()) || chat.mFromName == SYSTEM_FROM)
		{
			mSourceType = CHAT_SOURCE_SYSTEM;
		}

		LLTextBox* userName = getChild<LLTextBox>("user_name");

		userName->setReadOnlyColor(style_params.readonly_color());
		userName->setColor(style_params.color());
		
		userName->setValue(chat.mFromName);
		mFrom = chat.mFromName;
		if (chat.mFromName.empty() || CHAT_SOURCE_SYSTEM == mSourceType)
		{
			mFrom = LLTrans::getString("SECOND_LIFE");
			userName->setValue(mFrom);
		}


		mMinUserNameWidth = style_params.font()->getWidth(userName->getWText().c_str()) + PADDING;

		setTimeField(chat);
		
		LLAvatarIconCtrl* icon = getChild<LLAvatarIconCtrl>("avatar_icon");

		if(mSourceType != CHAT_SOURCE_AGENT)
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

	void nameUpdatedCallback(const LLUUID& id,const std::string& full_name, bool is_group)
	{
		if (id != mAvatarID)
			return;
		mFrom = full_name;
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
		if (mAvatarID.isNull() || mFrom.empty() || SYSTEM_FROM == mFrom) return;
				
		if (!sInfoCtrl)
		{
			sInfoCtrl = LLUICtrlFactory::createFromFile<LLUICtrl>("inspector_info_ctrl.xml", NULL, LLPanel::child_registry_t::instance());
			sInfoCtrl->setCommitCallback(boost::bind(&LLChatHistoryHeader::onClickInfoCtrl, sInfoCtrl));
		}

		LLTextBase* name = getChild<LLTextBase>("user_name");
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
	EChatSourceType		mSourceType;
	std::string			mFrom;
	LLUUID				mSessionID;

	S32					mMinUserNameWidth;
};

LLUICtrl* LLChatHistoryHeader::sInfoCtrl = NULL;

LLChatHistory::LLChatHistory(const LLChatHistory::Params& p)
:	LLUICtrl(p),
	mMessageHeaderFilename(p.message_header),
	mMessageSeparatorFilename(p.message_separator),
	mMessagePlaintextSeparatorFilename(p.message_plaintext_separator),
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
	layout_p.orientation = "vertical";
	layout_p.mouse_opaque = false;
	
	LLLayoutStack* stackp = LLUICtrlFactory::create<LLLayoutStack>(layout_p, this);
	
	const S32 NEW_TEXT_NOTICE_HEIGHT = 20;
	
	LLPanel::Params panel_p;
	panel_p.name = "spacer";
	panel_p.background_visible = false;
	panel_p.has_border = false;
	panel_p.mouse_opaque = false;
	stackp->addPanel(LLUICtrlFactory::create<LLPanel>(panel_p), 0, 30, S32_MAX, S32_MAX, true, false, LLLayoutStack::ANIMATE);

	panel_p.name = "new_text_notice_holder";
	LLRect new_text_notice_rect = getLocalRect();
	new_text_notice_rect.mTop = new_text_notice_rect.mBottom + NEW_TEXT_NOTICE_HEIGHT;
	panel_p.rect = new_text_notice_rect;
	panel_p.background_opaque = true;
	panel_p.background_visible = true;
	panel_p.visible = false;
	mMoreChatPanel = LLUICtrlFactory::create<LLPanel>(panel_p);
	
	LLTextBox::Params text_p(p.more_chat_text);
	text_p.rect = mMoreChatPanel->getLocalRect();
	text_p.follows.flags = FOLLOWS_ALL;
	text_p.name = "more_chat_text";
	mMoreChatText = LLUICtrlFactory::create<LLTextBox>(text_p, mMoreChatPanel);
	mMoreChatText->setClickedCallback(boost::bind(&LLChatHistory::onClickMoreText, this));

	stackp->addPanel(mMoreChatPanel, 0, 0, S32_MAX, S32_MAX, false, false, LLLayoutStack::ANIMATE);
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

LLView* LLChatHistory::getPlaintextSeparator()
{
	LLPanel* separator = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>(mMessagePlaintextSeparatorFilename, NULL, LLPanel::child_registry_t::instance());
	return separator;
}

LLView* LLChatHistory::getHeader(const LLChat& chat,const LLStyle::Params& style_params)
{
	LLChatHistoryHeader* header = LLChatHistoryHeader::createInstance(mMessageHeaderFilename);
	header->setup(chat,style_params);
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

void LLChatHistory::appendMessage(const LLChat& chat, const LLSD &args, const LLStyle::Params& input_append_params)
{
	bool use_plain_text_chat_history = args["use_plain_text_chat_history"].asBoolean();

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
		// append plaintext separator
		LLView* separator = getPlaintextSeparator();
		LLInlineViewSegment::Params p;
		p.force_newline = true;
		p.left_pad = mLeftWidgetPad;
		p.right_pad = mRightWidgetPad;
		p.view = separator;
		//mEditor->appendWidget(p, "\n", false);  // TODO: this is absolute minimal fix for EXT-3818 because it's late for 2.0
		mEditor->appendWidget(p, "", false);      // This should be properly fixed in 2.1

		mEditor->appendText("[" + chat.mTimeStr + "] ", mEditor->getText().size() != 0, style_params);

		if (utf8str_trim(chat.mFromName).size() != 0)
		{
			// Don't hotlink any messages from the system (e.g. "Second Life:"), so just add those in plain text.
			if ( chat.mSourceType == CHAT_SOURCE_OBJECT && chat.mFromID.notNull())
			{
				// for object IMs, create a secondlife:///app/objectim SLapp
				std::string url = LLSLURL::buildCommand("objectim", chat.mFromID, "");
				url += "?name=" + chat.mFromName;
				url += "&owner=" + args["owner_id"].asString();

				std::string slurl = args["slurl"].asString();
				if (slurl.empty())
				{
					LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosAgent(chat.mPosAgent);
					if (region)
					{
						S32 x, y, z;
						LLSLURL::globalPosToXYZ(LLVector3d(chat.mPosAgent), x, y, z);
						slurl = region->getName() + llformat("/%d/%d/%d", x, y, z);
					}
				}
				url += "&slurl=" + slurl;

				// set the link for the object name to be the objectim SLapp
				// (don't let object names with hyperlinks override our objectim Url)
				LLStyle::Params link_params(style_params);
				link_params.color.control = "HTMLLinkColor";
				link_params.link_href = url;
				mEditor->appendText("<nolink>" + chat.mFromName + "</nolink>"  + delimiter,
									false, link_params);
			}
			else if ( chat.mFromName != SYSTEM_FROM && chat.mFromID.notNull() && !message_from_log)
			{
				LLStyle::Params link_params(style_params);
				link_params.overwriteFrom(LLStyleMap::instance().lookupAgent(chat.mFromID));
				// Convert the name to a hotlink and add to message.
				mEditor->appendText(chat.mFromName + delimiter, false, link_params);
			}
			else
			{
				mEditor->appendText(chat.mFromName + delimiter, false, style_params);
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
			view = getHeader(chat, style_params);
			if (mEditor->getText().size() == 0)
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
					notification);
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

			LLTextEditor* text_editor = notify_box->getChild<LLTextEditor>("text_editor_box", TRUE);
			S32 text_heigth = 0;
			if(text_editor != NULL)
			{
				text_heigth = text_editor->getTextBoundingRect().getHeight();
			}

			//Prepare the rect for the view
			LLRect target_rect = mEditor->getDocumentView()->getRect();
			// squeeze down the widget by subtracting padding off left and right
			target_rect.mLeft += mLeftWidgetPad + mEditor->getHPad();
			target_rect.mRight -= mRightWidgetPad;
			notify_box->reshape(target_rect.getWidth(),
					notify_box->getRect().getHeight());
			notify_box->setOrigin(target_rect.mLeft, notify_box->getRect().mBottom);

			if (text_editor != NULL)
			{
				S32 text_heigth_delta =
						text_editor->getTextBoundingRect().getHeight()
								- text_heigth;
				notify_box->reshape(target_rect.getWidth(),
								notify_box->getRect().getHeight() + text_heigth_delta);
			}

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

void LLChatHistory::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	bool is_scrolled_to_end = mEditor->scrolledToEnd();
	LLUICtrl::reshape( width, height, called_from_parent );
	// update scroll
	if (is_scrolled_to_end)
		mEditor->setCursorAndScrollToEnd();
}
