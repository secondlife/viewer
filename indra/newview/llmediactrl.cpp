/**
 * @file LLMediaCtrl.cpp
 * @brief Web browser UI control
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#include "lltooltip.h"

#include "llmediactrl.h"

// viewer includes
#include "llfloaterworldmap.h"
#include "lluictrlfactory.h"
#include "llurldispatcher.h"
#include "llviewborder.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewertexture.h"
#include "llviewerwindow.h"
#include "llweb.h"
#include "llrender.h"
#include "llpluginclassmedia.h"
#include "llslurl.h"
#include "lluictrlfactory.h"	// LLDefaultChildRegistry
#include "llkeyboard.h"

// linden library includes
#include "llfocusmgr.h"
#include "llsdutil.h"
#include "lllayoutstack.h"
#include "lliconctrl.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llnotifications.h"
#include "lllineeditor.h"

extern BOOL gRestoreGL;

static LLDefaultChildRegistry::Register<LLMediaCtrl> r("web_browser");

class LLWindowShade : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<LLNotificationPtr> notification;
		Optional<LLUIImage*>		 bg_image;

		Params()
		:	bg_image("bg_image")
		{
			mouse_opaque = false;
		}
	};

	void show();
	/*virtual*/ void draw();
	void hide();

private:
	friend class LLUICtrlFactory;

	LLWindowShade(const Params& p);
	void initFromParams(const Params& params);

	void onCloseNotification();
	void onClickNotificationButton(const std::string& name);
	void onEnterNotificationText(LLUICtrl* ctrl, const std::string& name);
	void onClickIgnore(LLUICtrl* ctrl);

	LLNotificationPtr	mNotification;
	LLSD				mNotificationResponse;
};

LLWindowShade::LLWindowShade(const LLWindowShade::Params& params)
:	LLView(params),
	mNotification(params.notification)
{
}

void LLWindowShade::initFromParams(const LLWindowShade::Params& params)
{
	LLView::initFromParams(params);

	LLLayoutStack::Params layout_p;
	layout_p.name = "notification_stack";
	layout_p.rect = LLRect(0,getLocalRect().mTop,getLocalRect().mRight, 30);
	layout_p.follows.flags = FOLLOWS_ALL;
	layout_p.mouse_opaque = false;
	layout_p.orientation = LLLayoutStack::VERTICAL;

	LLLayoutStack* stackp = LLUICtrlFactory::create<LLLayoutStack>(layout_p);
	addChild(stackp);

	LLLayoutPanel::Params panel_p;
	panel_p.rect = LLRect(0, 30, 800, 0);
	panel_p.min_height = 30;
	panel_p.name = "notification_area";
	panel_p.visible = false;
	panel_p.user_resize = false;
	panel_p.background_visible = true;
	panel_p.bg_alpha_image = params.bg_image;
	panel_p.auto_resize = false;
	LLLayoutPanel* notification_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(notification_panel);

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.auto_resize = true;
	panel_p.mouse_opaque = false;
	LLLayoutPanel* dummy_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(dummy_panel);

	layout_p = LLUICtrlFactory::getDefaultParams<LLLayoutStack>();
	layout_p.rect = LLRect(0, 30, 800, 0);
	layout_p.follows.flags = FOLLOWS_ALL;
	layout_p.orientation = LLLayoutStack::HORIZONTAL;
	stackp = LLUICtrlFactory::create<LLLayoutStack>(layout_p);
	notification_panel->addChild(stackp);

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.rect.height = 30;
	LLLayoutPanel* panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(panel);

	LLIconCtrl::Params icon_p;
	icon_p.name = "notification_icon";
	icon_p.rect = LLRect(5, 23, 21, 8);
	panel->addChild(LLUICtrlFactory::create<LLIconCtrl>(icon_p));

	LLTextBox::Params text_p;
	text_p.rect = LLRect(31, 20, 430, 0);
	text_p.text_color = LLColor4::black;
	text_p.font = LLFontGL::getFontSansSerif();
	text_p.font.style = "BOLD";
	text_p.name = "notification_text";
	text_p.use_ellipses = true;
	panel->addChild(LLUICtrlFactory::create<LLTextBox>(text_p));

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.auto_resize = false;
	panel_p.user_resize = false;
	panel_p.name="form_elements";
	panel_p.rect = LLRect(0, 30, 130, 0);
	LLLayoutPanel* form_elements_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(form_elements_panel);

	panel_p = LLUICtrlFactory::getDefaultParams<LLLayoutPanel>();
	panel_p.auto_resize = false;
	panel_p.user_resize = false;
	panel_p.rect = LLRect(0, 30, 25, 0);
	LLLayoutPanel* close_panel = LLUICtrlFactory::create<LLLayoutPanel>(panel_p);
	stackp->addChild(close_panel);

	LLButton::Params button_p;
	button_p.name = "close_notification";
	button_p.rect = LLRect(5, 23, 21, 7);
	button_p.image_color=LLUIColorTable::instance().getColor("DkGray_66");
    button_p.image_unselected.name="Icon_Close_Foreground";
	button_p.image_selected.name="Icon_Close_Press";
	button_p.click_callback.function = boost::bind(&LLWindowShade::onCloseNotification, this);

	close_panel->addChild(LLUICtrlFactory::create<LLButton>(button_p));

	LLSD payload = mNotification->getPayload();

	LLNotificationFormPtr formp = mNotification->getForm();
	LLLayoutPanel& notification_area = getChildRef<LLLayoutPanel>("notification_area");
	notification_area.getChild<LLUICtrl>("notification_icon")->setValue(mNotification->getIcon());
	notification_area.getChild<LLUICtrl>("notification_text")->setValue(mNotification->getMessage());
	notification_area.getChild<LLUICtrl>("notification_text")->setToolTip(mNotification->getMessage());
	LLNotificationForm::EIgnoreType ignore_type = formp->getIgnoreType(); 
	LLLayoutPanel& form_elements = notification_area.getChildRef<LLLayoutPanel>("form_elements");
	form_elements.deleteAllChildren();

	const S32 FORM_PADDING_HORIZONTAL = 10;
	const S32 FORM_PADDING_VERTICAL = 3;
	S32 cur_x = FORM_PADDING_HORIZONTAL;

	if (ignore_type != LLNotificationForm::IGNORE_NO)
	{
		LLCheckBoxCtrl::Params checkbox_p;
		checkbox_p.name = "ignore_check";
		checkbox_p.rect = LLRect(cur_x, form_elements.getRect().getHeight() - FORM_PADDING_VERTICAL, cur_x, FORM_PADDING_VERTICAL);
		checkbox_p.label = formp->getIgnoreMessage();
		checkbox_p.label_text.text_color = LLColor4::black;
		checkbox_p.commit_callback.function = boost::bind(&LLWindowShade::onClickIgnore, this, _1);
		checkbox_p.initial_value = formp->getIgnored();

		LLCheckBoxCtrl* check = LLUICtrlFactory::create<LLCheckBoxCtrl>(checkbox_p);
		check->setRect(check->getBoundingRect());
		form_elements.addChild(check);
		cur_x = check->getRect().mRight + FORM_PADDING_HORIZONTAL;
	}

	for (S32 i = 0; i < formp->getNumElements(); i++)
	{
		LLSD form_element = formp->getElement(i);
		std::string type = form_element["type"].asString();
		if (type == "button")
		{
			LLButton::Params button_p;
			button_p.name = form_element["name"];
			button_p.label = form_element["text"];
			button_p.rect = LLRect(cur_x, form_elements.getRect().getHeight() - FORM_PADDING_VERTICAL, cur_x, FORM_PADDING_VERTICAL);
			button_p.click_callback.function = boost::bind(&LLWindowShade::onClickNotificationButton, this, form_element["name"].asString());
			button_p.auto_resize = true;

			LLButton* button = LLUICtrlFactory::create<LLButton>(button_p);
			button->autoResize();
			form_elements.addChild(button);

			cur_x = button->getRect().mRight + FORM_PADDING_HORIZONTAL;
		}
		else if (type == "text" || type == "password")
		{
			LLTextBox::Params label_p;
			label_p.name = form_element["name"].asString() + "_label";
			label_p.rect = LLRect(cur_x, form_elements.getRect().getHeight() - FORM_PADDING_VERTICAL, cur_x + 120, FORM_PADDING_VERTICAL);
			label_p.initial_value = form_element["text"];
			label_p.text_color = LLColor4::black;
			LLTextBox* textbox = LLUICtrlFactory::create<LLTextBox>(label_p);
			textbox->reshapeToFitText();
			form_elements.addChild(textbox);
			cur_x = textbox->getRect().mRight + FORM_PADDING_HORIZONTAL;

			LLLineEditor::Params line_p;
			line_p.name = form_element["name"];
			line_p.commit_callback.function = boost::bind(&LLWindowShade::onEnterNotificationText, this, _1, form_element["name"].asString());
			line_p.commit_on_focus_lost = true;
			line_p.is_password = type == "password";
			line_p.rect = LLRect(cur_x, form_elements.getRect().getHeight() - FORM_PADDING_VERTICAL, cur_x + 120, FORM_PADDING_VERTICAL);

			LLLineEditor* line_editor = LLUICtrlFactory::create<LLLineEditor>(line_p);
			form_elements.addChild(line_editor);
			cur_x = line_editor->getRect().mRight + FORM_PADDING_HORIZONTAL;
		}
	}

	form_elements.reshape(cur_x, form_elements.getRect().getHeight());	
}

void LLWindowShade::show()
{
	LLLayoutPanel& panel = getChildRef<LLLayoutPanel>("notification_area");
	panel.setVisible(true);
}

void LLWindowShade::draw()
{
	LLView::draw();
	if (mNotification && !mNotification->isActive())
	{
		hide();
	}
}

void LLWindowShade::hide()
{
	LLLayoutPanel& panel = getChildRef<LLLayoutPanel>("notification_area");
	panel.setVisible(false);
}

void LLWindowShade::onCloseNotification()
{
	LLNotifications::instance().cancel(mNotification);
}

void LLWindowShade::onClickIgnore(LLUICtrl* ctrl)
{
	bool check = ctrl->getValue().asBoolean();
	if (mNotification && mNotification->getForm()->getIgnoreType() == LLNotificationForm::IGNORE_SHOW_AGAIN)
	{
		// question was "show again" so invert value to get "ignore"
		check = !check;
	}
	mNotification->setIgnored(check);
}

void LLWindowShade::onClickNotificationButton(const std::string& name)
{
	if (!mNotification) return;

	mNotificationResponse[name] = true;

	mNotification->respond(mNotificationResponse);
}

void LLWindowShade::onEnterNotificationText(LLUICtrl* ctrl, const std::string& name)
{
	mNotificationResponse[name] = ctrl->getValue().asString();
}


LLMediaCtrl::Params::Params()
:	start_url("start_url"),
	border_visible("border_visible", true),
	ignore_ui_scale("ignore_ui_scale", true),
	hide_loading("hide_loading", false),
	decouple_texture_size("decouple_texture_size", false),
	texture_width("texture_width", 1024),
	texture_height("texture_height", 1024),
	caret_color("caret_color"),
	initial_mime_type("initial_mime_type"),
	trusted_content("trusted_content", false)
{
	tab_stop(false);
}

LLMediaCtrl::LLMediaCtrl( const Params& p) :
	LLPanel( p ),
	LLInstanceTracker<LLMediaCtrl, LLUUID>(LLUUID::generateNewID()),
	mTextureDepthBytes( 4 ),
	mBorder(NULL),
	mFrequentUpdates( true ),
	mForceUpdate( false ),
	mHomePageUrl( "" ),
	mIgnoreUIScale( true ),
	mAlwaysRefresh( false ),
	mMediaSource( 0 ),
	mTakeFocusOnClick( true ),
	mCurrentNavUrl( "" ),
	mStretchToFill( true ),
	mMaintainAspectRatio ( true ),
	mHideLoading (false),
	mHidingInitialLoad (false),
	mDecoupleTextureSize ( false ),
	mTextureWidth ( 1024 ),
	mTextureHeight ( 1024 ),
	mClearCache(false),
	mHomePageMimeType(p.initial_mime_type),
	mTrusted(p.trusted_content),
	mWindowShade(NULL),
	mHoverTextChanged(false)
{
	{
		LLColor4 color = p.caret_color().get();
		setCaretColor( (unsigned int)color.mV[0], (unsigned int)color.mV[1], (unsigned int)color.mV[2] );
	}

	setIgnoreUIScale(p.ignore_ui_scale);
	
	setHomePageUrl(p.start_url, p.initial_mime_type);
	
	setBorderVisible(p.border_visible);
	
	mHideLoading = p.hide_loading;
	
	setDecoupleTextureSize(p.decouple_texture_size);
	
	setTextureSize(p.texture_width, p.texture_height);

	if(!getDecoupleTextureSize())
	{
		S32 screen_width = mIgnoreUIScale ? 
			llround((F32)getRect().getWidth() * LLUI::sGLScaleFactor.mV[VX]) : getRect().getWidth();
		S32 screen_height = mIgnoreUIScale ? 
			llround((F32)getRect().getHeight() * LLUI::sGLScaleFactor.mV[VY]) : getRect().getHeight();
			
		setTextureSize(screen_width, screen_height);
	}
	
	mMediaTextureID = getKey();
	
	// We don't need to create the media source up front anymore unless we have a non-empty home URL to navigate to.
	if(!mHomePageUrl.empty())
	{
		navigateHome();
	}
		
	// FIXME: How do we create a bevel now?
//	LLRect border_rect( 0, getRect().getHeight() + 2, getRect().getWidth() + 2, 0 );
//	mBorder = new LLViewBorder( std::string("web control border"), border_rect, LLViewBorder::BEVEL_IN );
//	addChild( mBorder );
}

LLMediaCtrl::~LLMediaCtrl()
{

	if (mMediaSource)
	{
		mMediaSource->remObserver( this );
		mMediaSource = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setBorderVisible( BOOL border_visible )
{
	if ( mBorder )
	{
		mBorder->setVisible( border_visible );
	};
};

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setTakeFocusOnClick( bool take_focus )
{
	mTakeFocusOnClick = take_focus;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleHover( S32 x, S32 y, MASK mask )
{
	if (LLPanel::handleHover(x, y, mask)) return TRUE;
	convertInputCoords(x, y);

	if (mMediaSource)
	{
		mMediaSource->mouseMove(x, y, mask);
		gViewerWindow->setCursor(mMediaSource->getLastSetCursor());
	}
	
	// TODO: Is this the right way to handle hover text changes driven by the plugin?
	if(mHoverTextChanged)
	{
		mHoverTextChanged = false;
		handleToolTip(x, y, mask);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleScrollWheel( S32 x, S32 y, S32 clicks )
{
	if (LLPanel::handleScrollWheel(x, y, clicks)) return TRUE;
	if (mMediaSource && mMediaSource->hasMedia())
		mMediaSource->getMediaPlugin()->scrollEvent(0, clicks, gKeyboard->currentMask(TRUE));

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//	virtual 
BOOL LLMediaCtrl::handleToolTip(S32 x, S32 y, MASK mask)
{
	std::string hover_text;
	
	if (mMediaSource && mMediaSource->hasMedia())
		hover_text = mMediaSource->getMediaPlugin()->getHoverText();
	
	if(hover_text.empty())
	{
		return FALSE;
	}
	else
	{
		S32 screen_x, screen_y;

		localPointToScreen(x, y, &screen_x, &screen_y);
		LLRect sticky_rect_screen;
		sticky_rect_screen.setCenterAndSize(screen_x, screen_y, 20, 20);

		LLToolTipMgr::instance().show(LLToolTip::Params()
			.message(hover_text)
			.sticky_rect(sticky_rect_screen));		
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (LLPanel::handleMouseUp(x, y, mask)) return TRUE;
	convertInputCoords(x, y);

	if (mMediaSource)
	{
		mMediaSource->mouseUp(x, y, mask);

		// *HACK: LLMediaImplLLMozLib automatically takes focus on mouseup,
		// in addition to the onFocusReceived() call below.  Undo this. JC
		if (!mTakeFocusOnClick)
		{
			mMediaSource->focus(false);
			gViewerWindow->focusClient();
		}
	}
	
	gFocusMgr.setMouseCapture( NULL );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleMouseDown( S32 x, S32 y, MASK mask )
{
	if (LLPanel::handleMouseDown(x, y, mask)) return TRUE;
	convertInputCoords(x, y);

	if (mMediaSource)
		mMediaSource->mouseDown(x, y, mask);
	
	gFocusMgr.setMouseCapture( this );

	if (mTakeFocusOnClick)
	{
		setFocus( TRUE );
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleRightMouseUp( S32 x, S32 y, MASK mask )
{
	if (LLPanel::handleRightMouseUp(x, y, mask)) return TRUE;
	convertInputCoords(x, y);

	if (mMediaSource)
	{
		mMediaSource->mouseUp(x, y, mask, 1);

		// *HACK: LLMediaImplLLMozLib automatically takes focus on mouseup,
		// in addition to the onFocusReceived() call below.  Undo this. JC
		if (!mTakeFocusOnClick)
		{
			mMediaSource->focus(false);
			gViewerWindow->focusClient();
		}
	}
	
	gFocusMgr.setMouseCapture( NULL );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	if (LLPanel::handleRightMouseDown(x, y, mask)) return TRUE;
	convertInputCoords(x, y);

	if (mMediaSource)
		mMediaSource->mouseDown(x, y, mask, 1);
	
	gFocusMgr.setMouseCapture( this );

	if (mTakeFocusOnClick)
	{
		setFocus( TRUE );
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	if (LLPanel::handleDoubleClick(x, y, mask)) return TRUE;
	convertInputCoords(x, y);

	if (mMediaSource)
		mMediaSource->mouseDoubleClick( x, y, mask);

	gFocusMgr.setMouseCapture( this );

	if (mTakeFocusOnClick)
	{
		setFocus( TRUE );
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onFocusReceived()
{
	if (mMediaSource)
	{
		mMediaSource->focus(true);
		
		// Set focus for edit menu items
		LLEditMenuHandler::gEditMenuHandler = mMediaSource;
	}
	
	LLPanel::onFocusReceived();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onFocusLost()
{
	if (mMediaSource)
	{
		mMediaSource->focus(false);

		if( LLEditMenuHandler::gEditMenuHandler == mMediaSource )
		{
			// Clear focus for edit menu items
			LLEditMenuHandler::gEditMenuHandler = NULL;
		}
	}

	gViewerWindow->focusClient();

	LLPanel::onFocusLost();
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::postBuild ()
{
	setVisibleCallback(boost::bind(&LLMediaCtrl::onVisibilityChange, this, _2));
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleKeyHere( KEY key, MASK mask )
{
	BOOL result = FALSE;
	
	if (mMediaSource)
	{
		result = mMediaSource->handleKeyHere(key, mask);
	}
	
	if ( ! result )
		result = LLPanel::handleKeyHere(key, mask);
		
	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::handleVisibilityChange ( BOOL new_visibility )
{
	llinfos << "visibility changed to " << (new_visibility?"true":"false") << llendl;
	if(mMediaSource)
	{
		mMediaSource->setVisible( new_visibility );
	}
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleUnicodeCharHere(llwchar uni_char)
{
	BOOL result = FALSE;
	
	if (mMediaSource)
	{
		result = mMediaSource->handleUnicodeCharHere(uni_char);
	}

	if ( ! result )
		result = LLPanel::handleUnicodeCharHere(uni_char);

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onVisibilityChange ( const LLSD& new_visibility )
{
	// set state of frequent updates automatically if visibility changes
	if ( new_visibility.asBoolean() )
	{
		mFrequentUpdates = true;
	}
	else
	{
		mFrequentUpdates = false;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::reshape( S32 width, S32 height, BOOL called_from_parent )
{
	if(!getDecoupleTextureSize())
	{
		S32 screen_width = mIgnoreUIScale ? llround((F32)width * LLUI::sGLScaleFactor.mV[VX]) : width;
		S32 screen_height = mIgnoreUIScale ? llround((F32)height * LLUI::sGLScaleFactor.mV[VY]) : height;

		// when floater is minimized, these sizes are negative
		if ( screen_height > 0 && screen_width > 0 )
		{
			setTextureSize(screen_width, screen_height);
		}
	}
	
	LLUICtrl::reshape( width, height, called_from_parent );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateBack()
{
	if (mMediaSource && mMediaSource->hasMedia())
	{
		mMediaSource->getMediaPlugin()->browse_back();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateForward()
{
	if (mMediaSource && mMediaSource->hasMedia())
	{
		mMediaSource->getMediaPlugin()->browse_forward();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::canNavigateBack()
{
	if (mMediaSource)
		return mMediaSource->canNavigateBack();
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::canNavigateForward()
{
	if (mMediaSource)
		return mMediaSource->canNavigateForward();
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::set404RedirectUrl( std::string redirect_url )
{
	if(mMediaSource && mMediaSource->hasMedia())
		mMediaSource->getMediaPlugin()->set_status_redirect( 404, redirect_url );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::clr404RedirectUrl()
{
	if(mMediaSource && mMediaSource->hasMedia())
		mMediaSource->getMediaPlugin()->set_status_redirect(404, "");
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::clearCache()
{
	if(mMediaSource)
	{
		mMediaSource->clearCache();
	}
	else
	{
		mClearCache = true;
	}

}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateTo( std::string url_in, std::string mime_type)
{
	// don't browse to anything that starts with secondlife:// or sl://
	const std::string protocol1 = "secondlife://";
	const std::string protocol2 = "sl://";
	if ((LLStringUtil::compareInsensitive(url_in.substr(0, protocol1.length()), protocol1) == 0) ||
	    (LLStringUtil::compareInsensitive(url_in.substr(0, protocol2.length()), protocol2) == 0))
	{
		// TODO: Print out/log this attempt?
		// llinfos << "Rejecting attempt to load restricted website :" << urlIn << llendl;
		return;
	}
	
	if (ensureMediaSourceExists())
	{
		mCurrentNavUrl = url_in;
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mMediaSource->navigateTo(url_in, mime_type, mime_type.empty());
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateToLocalPage( const std::string& subdir, const std::string& filename_in )
{
	std::string language = LLUI::getLanguage();
	std::string delim = gDirUtilp->getDirDelimiter();
	std::string filename;

	filename += subdir;
	filename += delim;
	filename += filename_in;

	std::string expanded_filename = gDirUtilp->findSkinnedFilename("html", language, filename);

	if (! gDirUtilp->fileExists(expanded_filename))
	{
		if (language != "en")
		{
			expanded_filename = gDirUtilp->findSkinnedFilename("html", "en", filename);
			if (! gDirUtilp->fileExists(expanded_filename))
			{
				llwarns << "File " << subdir << delim << filename_in << "not found" << llendl;
				return;
			}
		}
		else
		{
			llwarns << "File " << subdir << delim << filename_in << "not found" << llendl;
			return;
		}
	}
	if (ensureMediaSourceExists())
	{
		mCurrentNavUrl = expanded_filename;
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mMediaSource->navigateTo(expanded_filename, "text/html", false);
	}

}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateHome()
{
	if (ensureMediaSourceExists())
	{
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mMediaSource->navigateHome();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setHomePageUrl( const std::string& urlIn, const std::string& mime_type )
{
	mHomePageUrl = urlIn;
	if (mMediaSource)
	{
		mMediaSource->setHomeURL(mHomePageUrl, mime_type);
	}
}

void LLMediaCtrl::setTarget(const std::string& target)
{
	mTarget = target;
	if (mMediaSource)
	{
		mMediaSource->setTarget(mTarget);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::setCaretColor(unsigned int red, unsigned int green, unsigned int blue)
{
	//NOOP
	return false;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setTextureSize(S32 width, S32 height)
{
	mTextureWidth = width;
	mTextureHeight = height;
	
	if(mMediaSource)
	{
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mForceUpdate = true;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLMediaCtrl::getHomePageUrl()
{
	return 	mHomePageUrl;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::ensureMediaSourceExists()
{	
	if(mMediaSource.isNull())
	{
		// If we don't already have a media source, try to create one.
		mMediaSource = LLViewerMedia::newMediaImpl(mMediaTextureID, mTextureWidth, mTextureHeight);
		if ( mMediaSource )
		{
			mMediaSource->setUsedInUI(true);
			mMediaSource->setHomeURL(mHomePageUrl, mHomePageMimeType);
			mMediaSource->setTarget(mTarget);
			mMediaSource->setVisible( getVisible() );
			mMediaSource->addObserver( this );
			mMediaSource->setBackgroundColor( getBackgroundColor() );
			mMediaSource->setTrustedBrowser(mTrusted);
			if(mClearCache)
			{
				mMediaSource->clearCache();
				mClearCache = false;
			}
			
			if(mHideLoading)
			{
				mHidingInitialLoad = true;
			}
		}
		else
		{
			llwarns << "media source create failed " << llendl;
			// return;
		}
	}
	
	return !mMediaSource.isNull();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::unloadMediaSource()
{
	mMediaSource = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
LLPluginClassMedia* LLMediaCtrl::getMediaPlugin()
{ 
	return mMediaSource.isNull() ? NULL : mMediaSource->getMediaPlugin(); 
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::draw()
{
	if ( gRestoreGL == 1 )
	{
		LLRect r = getRect();
		reshape( r.getWidth(), r.getHeight(), FALSE );
		return;
	}

	// NOTE: optimization needed here - probably only need to do this once
	// unless tearoffs change the parent which they probably do.
	const LLUICtrl* ptr = findRootMostFocusRoot();
	if ( ptr && ptr->hasFocus() )
	{
		setFrequentUpdates( true );
	}
	else
	{
		setFrequentUpdates( false );
	};
	
	bool draw_media = false;
	
	LLPluginClassMedia* media_plugin = NULL;
	LLViewerMediaTexture* media_texture = NULL;
	
	if(mMediaSource && mMediaSource->hasMedia())
	{
		media_plugin = mMediaSource->getMediaPlugin();

		if(media_plugin && (media_plugin->textureValid()))
		{
			media_texture = LLViewerTextureManager::findMediaTexture(mMediaTextureID);
			if(media_texture)
			{
				draw_media = true;
			}
		}
	}
	
	if(mHidingInitialLoad)
	{
		// If we're hiding loading, don't draw at all.
		draw_media = false;
	}
	
	bool background_visible = isBackgroundVisible();
	bool background_opaque = isBackgroundOpaque();
	
	if(draw_media)
	{
		// alpha off for this
		LLGLSUIDefault gls_ui;
		LLGLDisable gls_alphaTest( GL_ALPHA_TEST );

		gGL.pushUIMatrix();
		{
			if (mIgnoreUIScale)
			{
				gGL.loadUIIdentity();
				// font system stores true screen origin, need to scale this by UI scale factor
				// to get render origin for this view (with unit scale)
				gGL.translateUI(floorf(LLFontGL::sCurOrigin.mX * LLUI::sGLScaleFactor.mV[VX]), 
							floorf(LLFontGL::sCurOrigin.mY * LLUI::sGLScaleFactor.mV[VY]), 
							LLFontGL::sCurOrigin.mZ);
			}

			// scale texture to fit the space using texture coords
			gGL.getTexUnit(0)->bind(media_texture);
			gGL.color4fv( LLColor4::white.mV );
			F32 max_u = ( F32 )media_plugin->getWidth() / ( F32 )media_plugin->getTextureWidth();
			F32 max_v = ( F32 )media_plugin->getHeight() / ( F32 )media_plugin->getTextureHeight();

			LLRect r = getRect();
			S32 width, height;
			S32 x_offset = 0;
			S32 y_offset = 0;
			
			if(mStretchToFill)
			{
				if(mMaintainAspectRatio)
				{
					F32 media_aspect = (F32)(media_plugin->getWidth()) / (F32)(media_plugin->getHeight());
					F32 view_aspect = (F32)(r.getWidth()) / (F32)(r.getHeight());
					if(media_aspect > view_aspect)
					{
						// max width, adjusted height
						width = r.getWidth();
						height = llmin(llmax(llround(width / media_aspect), 0), r.getHeight());
					}
					else
					{
						// max height, adjusted width
						height = r.getHeight();
						width = llmin(llmax(llround(height * media_aspect), 0), r.getWidth());
					}
				}
				else
				{
					width = r.getWidth();
					height = r.getHeight();
				}
			}
			else
			{
				width = llmin(media_plugin->getWidth(), r.getWidth());
				height = llmin(media_plugin->getHeight(), r.getHeight());
			}
			
			x_offset = (r.getWidth() - width) / 2;
			y_offset = (r.getHeight() - height) / 2;		

			if(mIgnoreUIScale)
			{
				x_offset = llround((F32)x_offset * LLUI::sGLScaleFactor.mV[VX]);
				y_offset = llround((F32)y_offset * LLUI::sGLScaleFactor.mV[VY]);
				width = llround((F32)width * LLUI::sGLScaleFactor.mV[VX]);
				height = llround((F32)height * LLUI::sGLScaleFactor.mV[VY]);
			}

			// draw the browser
			gGL.setSceneBlendType(LLRender::BT_REPLACE);
			gGL.begin( LLRender::QUADS );
			if (! media_plugin->getTextureCoordsOpenGL())
			{
				// render using web browser reported width and height, instead of trying to invert GL scale
				gGL.texCoord2f( max_u, 0.f );
				gGL.vertex2i( x_offset + width, y_offset + height );

				gGL.texCoord2f( 0.f, 0.f );
				gGL.vertex2i( x_offset, y_offset + height );

				gGL.texCoord2f( 0.f, max_v );
				gGL.vertex2i( x_offset, y_offset );

				gGL.texCoord2f( max_u, max_v );
				gGL.vertex2i( x_offset + width, y_offset );
			}
			else
			{
				// render using web browser reported width and height, instead of trying to invert GL scale
				gGL.texCoord2f( max_u, max_v );
				gGL.vertex2i( x_offset + width, y_offset + height );

				gGL.texCoord2f( 0.f, max_v );
				gGL.vertex2i( x_offset, y_offset + height );

				gGL.texCoord2f( 0.f, 0.f );
				gGL.vertex2i( x_offset, y_offset );

				gGL.texCoord2f( max_u, 0.f );
				gGL.vertex2i( x_offset + width, y_offset );
			}
			gGL.end();
			gGL.setSceneBlendType(LLRender::BT_ALPHA);
		}
		gGL.popUIMatrix();
	
	}
	else
	{
		// Setting these will make LLPanel::draw draw the opaque background color.
		setBackgroundVisible(true);
		setBackgroundOpaque(true);
	}
	
	// highlight if keyboard focus here. (TODO: this needs some work)
	if ( mBorder && mBorder->getVisible() )
		mBorder->setKeyboardFocusHighlight( gFocusMgr.childHasKeyboardFocus( this ) );

	LLPanel::draw();

	// Restore the previous values
	setBackgroundVisible(background_visible);
	setBackgroundOpaque(background_opaque);
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::convertInputCoords(S32& x, S32& y)
{
	bool coords_opengl = false;
	
	if(mMediaSource && mMediaSource->hasMedia())
	{
		coords_opengl = mMediaSource->getMediaPlugin()->getTextureCoordsOpenGL();
	}
	
	x = mIgnoreUIScale ? llround((F32)x * LLUI::sGLScaleFactor.mV[VX]) : x;
	if ( ! coords_opengl )
	{
		y = mIgnoreUIScale ? llround((F32)(y) * LLUI::sGLScaleFactor.mV[VY]) : y;
	}
	else
	{
		y = mIgnoreUIScale ? llround((F32)(getRect().getHeight() - y) * LLUI::sGLScaleFactor.mV[VY]) : getRect().getHeight() - y;
	};
}

////////////////////////////////////////////////////////////////////////////////
// inherited from LLViewerMediaObserver
//virtual 
void LLMediaCtrl::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	switch(event)
	{
		case MEDIA_EVENT_CONTENT_UPDATED:
		{
			// LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CONTENT_UPDATED " << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_TIME_DURATION_UPDATED:
		{
			// LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_TIME_DURATION_UPDATED, time is " << self->getCurrentTime() << " of " << self->getDuration() << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_SIZE_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_SIZE_CHANGED " << LL_ENDL;
			LLRect r = getRect();
			reshape( r.getWidth(), r.getHeight(), FALSE );
		};
		break;
		
		case MEDIA_EVENT_CURSOR_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CURSOR_CHANGED, new cursor is " << self->getCursorName() << LL_ENDL;
		}
		break;
			
		case MEDIA_EVENT_NAVIGATE_BEGIN:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_BEGIN, url is " << self->getNavigateURI() << LL_ENDL;
			hideNotification();
		};
		break;
		
		case MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_COMPLETE, result string is: " << self->getNavigateResultString() << LL_ENDL;
			if(mHidingInitialLoad)
			{
				mHidingInitialLoad = false;
			}
		};
		break;

		case MEDIA_EVENT_PROGRESS_UPDATED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PROGRESS_UPDATED, loading at " << self->getProgressPercent() << "%" << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_STATUS_TEXT_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_STATUS_TEXT_CHANGED, new status text is: " << self->getStatusText() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_LOCATION_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_LOCATION_CHANGED, new uri is: " << self->getLocation() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_CLICK_LINK_HREF:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, target is \"" << self->getClickTarget() << "\", uri is " << self->getClickURL() << LL_ENDL;
			// retrieve the event parameters
			std::string url = self->getClickURL();
			std::string target = self->getClickTarget();
			std::string uuid = self->getClickUUID();

			LLNotification::Params notify_params;
			notify_params.name = "PopupAttempt";
			notify_params.payload = LLSD().with("target", target).with("url", url).with("uuid", uuid).with("media_id", mMediaTextureID);
			notify_params.functor.function = boost::bind(&LLMediaCtrl::onPopup, this, _1, _2);

			if (mTrusted)
			{
				LLNotifications::instance().forceResponse(notify_params, 0);
			}
			else
			{
				LLNotifications::instance().add(notify_params);
			}
			break;
		};

		case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is " << self->getClickURL() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED" << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED_LAUNCH" << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_NAME_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAME_CHANGED" << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_CLOSE_REQUEST:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLOSE_REQUEST" << LL_ENDL;
		}
		break;
		
		case MEDIA_EVENT_PICK_FILE_REQUEST:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PICK_FILE_REQUEST" << LL_ENDL;
		}
		break;
		
		case MEDIA_EVENT_GEOMETRY_CHANGE:
		{
			LL_DEBUGS("Media") << "Media event:  MEDIA_EVENT_GEOMETRY_CHANGE, uuid is " << self->getClickUUID() << LL_ENDL;
		}
		break;

		case MEDIA_EVENT_AUTH_REQUEST:
		{
			LLNotification::Params auth_request_params;
			auth_request_params.name = "AuthRequest";
			auth_request_params.payload = LLSD().with("media_id", mMediaTextureID);
			auth_request_params.functor.function = boost::bind(&LLMediaCtrl::onAuthSubmit, this, _1, _2);
			LLNotifications::instance().add(auth_request_params);
		};
		break;

		case MEDIA_EVENT_LINK_HOVERED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_LINK_HOVERED, hover text is: " << self->getHoverText() << LL_ENDL;
			mHoverTextChanged = true;
		};
		break;
	};

	// chain all events to any potential observers of this object.
	emitEvent(self, event);
}

////////////////////////////////////////////////////////////////////////////////
// 
std::string LLMediaCtrl::getCurrentNavUrl()
{
	return mCurrentNavUrl;
}

void LLMediaCtrl::onPopup(const LLSD& notification, const LLSD& response)
{
	if (response["open"])
	{
		LLWeb::loadURL(notification["payload"]["url"], notification["payload"]["target"], notification["payload"]["uuid"]);
	}
	else
	{
		// Make sure the opening instance knows its window open request was denied, so it can clean things up.
		LLViewerMedia::proxyWindowClosed(notification["payload"]["uuid"]);
	}
}

void LLMediaCtrl::onAuthSubmit(const LLSD& notification, const LLSD& response)
{
	if (response["ok"])
	{
		mMediaSource->getMediaPlugin()->sendAuthResponse(true, response["username"], response["password"]);
	}
	else
	{
		mMediaSource->getMediaPlugin()->sendAuthResponse(false, "", "");
	}
}


void LLMediaCtrl::showNotification(LLNotificationPtr notify)
{
	delete mWindowShade;

	LLWindowShade::Params params;
	params.rect = getLocalRect();
	params.follows.flags = FOLLOWS_ALL;
	params.notification = notify;
	//HACK: don't hardcode this
	if (notify->getName() == "PopupAttempt")
	{
		params.bg_image.name = "Yellow_Gradient";
	}

	mWindowShade = LLUICtrlFactory::create<LLWindowShade>(params);

	addChild(mWindowShade);
	mWindowShade->show();
}

void LLMediaCtrl::hideNotification()
{
	if (mWindowShade)
	{
		mWindowShade->hide();
	}
}
