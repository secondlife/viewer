/** 
 * @file llui.cpp
 * @brief UI implementation
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

// Utilities functions the user interface needs

#include "linden_common.h"

#include <string>
#include <map>

// Linden library includes
#include "v2math.h"
#include "m3math.h"
#include "v4color.h"
#include "llrender.h"
#include "llrect.h"
#include "lldir.h"
#include "llgl.h"
#include "llsd.h"

// Project includes
#include "llcommandmanager.h"
#include "llcontrol.h"
#include "llui.h"
#include "lluicolortable.h"
#include "llview.h"
#include "lllineeditor.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llmenugl.h"
#include "llmenubutton.h"
#include "llloadingindicator.h"
#include "llwindow.h"

// for registration
#include "llfiltereditor.h"
#include "llflyoutbutton.h"
#include "llsearcheditor.h"
#include "lltoolbar.h"
#include "llcleanup.h"

// for XUIParse
#include "llquaternion.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>

//
// Globals
//

// Language for UI construction
std::map<std::string, std::string> gTranslation;
std::list<std::string> gUntranslated;

// register filter editor here
static LLDefaultChildRegistry::Register<LLFilterEditor> register_filter_editor("filter_editor");
static LLDefaultChildRegistry::Register<LLFlyoutButton> register_flyout_button("flyout_button");
static LLDefaultChildRegistry::Register<LLSearchEditor> register_search_editor("search_editor");

// register other widgets which otherwise may not be linked in
static LLDefaultChildRegistry::Register<LLLoadingIndicator> register_loading_indicator("loading_indicator");
static LLDefaultChildRegistry::Register<LLToolBar> register_toolbar("toolbar");

//
// Functions
//

LLUUID find_ui_sound(const char * namep)
{
	std::string name = ll_safe_string(namep);
	LLUUID uuid = LLUUID(NULL);
	LLUI *ui_inst = LLUI::getInstance();
	if (!ui_inst->mSettingGroups["config"]->controlExists(name))
	{
		LL_WARNS() << "tried to make UI sound for unknown sound name: " << name << LL_ENDL;	
	}
	else
	{
		uuid = LLUUID(ui_inst->mSettingGroups["config"]->getString(name));
		if (uuid.isNull())
		{
			if (ui_inst->mSettingGroups["config"]->getString(name) == LLUUID::null.asString())
			{
				if (ui_inst->mSettingGroups["config"]->getBOOL("UISndDebugSpamToggle"))
				{
					LL_INFOS() << "UI sound name: " << name << " triggered but silent (null uuid)" << LL_ENDL;	
				}				
			}
			else
			{
				LL_WARNS() << "UI sound named: " << name << " does not translate to a valid uuid" << LL_ENDL;	
			}
		}
		else if (ui_inst->mAudioCallback != NULL)
		{
			if (ui_inst->mSettingGroups["config"]->getBOOL("UISndDebugSpamToggle"))
			{
				LL_INFOS() << "UI sound name: " << name << LL_ENDL;	
			}
		}
	}

	return uuid;
}

void make_ui_sound(const char* namep)
{
	LLUUID soundUUID = find_ui_sound(namep);
	if(soundUUID.notNull())
	{
		LLUI::getInstance()->mAudioCallback(soundUUID);
	}
}

void make_ui_sound_deferred(const char* namep)
{
	LLUUID soundUUID = find_ui_sound(namep);
	if(soundUUID.notNull())
	{
		LLUI::getInstance()->mDeferredAudioCallback(soundUUID);
	}
}

LLUI::LLUI(const settings_map_t& settings,
				 LLImageProviderInterface* image_provider,
				 LLUIAudioCallback audio_callback,
				 LLUIAudioCallback deferred_audio_callback)
: mSettingGroups(settings),
mAudioCallback(audio_callback),
mDeferredAudioCallback(deferred_audio_callback),
mWindow(NULL), // set later in startup
mRootView(NULL),
mHelpImpl(NULL)
{
	LLRender2D::initParamSingleton(image_provider);

	if ((get_ptr_in_map(mSettingGroups, std::string("config")) == NULL) ||
		(get_ptr_in_map(mSettingGroups, std::string("floater")) == NULL) ||
		(get_ptr_in_map(mSettingGroups, std::string("ignores")) == NULL))
	{
		LL_ERRS() << "Failure to initialize configuration groups" << LL_ENDL;
	}

	LLFontGL::sShadowColor = LLUIColorTable::instance().getColor("ColorDropShadow");

	LLUICtrl::CommitCallbackRegistry::Registrar& reg = LLUICtrl::CommitCallbackRegistry::defaultRegistrar();

	// Callbacks for associating controls with floater visibility:
	reg.add("Floater.Toggle", boost::bind(&LLFloaterReg::toggleInstance, _2, LLSD()));
	reg.add("Floater.ToggleOrBringToFront", boost::bind(&LLFloaterReg::toggleInstanceOrBringToFront, _2, LLSD()));
	reg.add("Floater.Show", boost::bind(&LLFloaterReg::showInstance, _2, LLSD(), false));
	reg.add("Floater.ShowOrBringToFront", boost::bind(&LLFloaterReg::showInstanceOrBringToFront, _2, LLSD()));
	reg.add("Floater.Hide", boost::bind(&LLFloaterReg::hideInstance, _2, LLSD()));
	
	// Button initialization callback for toggle buttons
	reg.add("Button.SetFloaterToggle", boost::bind(&LLButton::setFloaterToggle, _1, _2));
	
	// Button initialization callback for toggle buttons on dockable floaters
	reg.add("Button.SetDockableFloaterToggle", boost::bind(&LLButton::setDockableFloaterToggle, _1, _2));

	// Display the help topic for the current context
	reg.add("Button.ShowHelp", boost::bind(&LLButton::showHelp, _1, _2));

	// Currently unused, but kept for reference:
	reg.add("Button.ToggleFloater", boost::bind(&LLButton::toggleFloaterAndSetToggleState, _1, _2));
	
	// Used by menus along with Floater.Toggle to display visibility as a check-mark
	LLUICtrl::EnableCallbackRegistry::defaultRegistrar().add("Floater.Visible", boost::bind(&LLFloaterReg::instanceVisible, _2, LLSD()));
	LLUICtrl::EnableCallbackRegistry::defaultRegistrar().add("Floater.IsOpen", boost::bind(&LLFloaterReg::instanceVisible, _2, LLSD()));

	// Parse the master list of commands
	LLCommandManager::load();
}

void LLUI::setPopupFuncs(const add_popup_t& add_popup, const remove_popup_t& remove_popup,  const clear_popups_t& clear_popups)
{
	mAddPopupFunc = add_popup;
	mRemovePopupFunc = remove_popup;
	mClearPopupsFunc = clear_popups;
}

void LLUI::setMousePositionScreen(S32 x, S32 y)
{
#if defined(LL_DARWIN)
    S32 screen_x = ll_round(((F32)x * getScaleFactor().mV[VX]) / LLView::getWindow()->getSystemUISize());
    S32 screen_y = ll_round(((F32)y * getScaleFactor().mV[VY]) / LLView::getWindow()->getSystemUISize());
#else
    S32 screen_x = ll_round((F32)x * getScaleFactor().mV[VX]);
    S32 screen_y = ll_round((F32)y * getScaleFactor().mV[VY]);
#endif
	
	LLView::getWindow()->setCursorPosition(LLCoordGL(screen_x, screen_y).convert());
}

void LLUI::getMousePositionScreen(S32 *x, S32 *y)
{
	LLCoordWindow cursor_pos_window;
	getWindow()->getCursorPosition(&cursor_pos_window);
	LLCoordGL cursor_pos_gl(cursor_pos_window.convert());
	*x = ll_round((F32)cursor_pos_gl.mX / getScaleFactor().mV[VX]);
	*y = ll_round((F32)cursor_pos_gl.mY / getScaleFactor().mV[VY]);
}

void LLUI::setMousePositionLocal(const LLView* viewp, S32 x, S32 y)
{
	S32 screen_x, screen_y;
	viewp->localPointToScreen(x, y, &screen_x, &screen_y);

	setMousePositionScreen(screen_x, screen_y);
}

void LLUI::getMousePositionLocal(const LLView* viewp, S32 *x, S32 *y)
{
	S32 screen_x, screen_y;
	getMousePositionScreen(&screen_x, &screen_y);
	viewp->screenPointToLocal(screen_x, screen_y, x, y);
}


// On Windows, the user typically sets the language when they install the
// app (by running it with a shortcut that sets InstallLanguage).  On Mac,
// or on Windows if the SecondLife.exe executable is run directly, the 
// language follows the OS language.  In all cases the user can override
// the language manually in preferences. JC
std::string LLUI::getUILanguage()
{
	std::string language = "en";
	if (mSettingGroups["config"])
	{
		language = mSettingGroups["config"]->getString("Language");
		if (language.empty() || language == "default")
		{
			language = mSettingGroups["config"]->getString("InstallLanguage");
		}
		if (language.empty() || language == "default")
		{
			language = mSettingGroups["config"]->getString("SystemLanguage");
		}
		if (language.empty() || language == "default")
		{
			language = "en";
		}
	}
	return language;
}

// static
std::string LLUI::getLanguage()
{
    // Note: lldateutil_test redefines this function
    return LLUI::getInstance()->getUILanguage();
}

struct SubDir : public LLInitParam::Block<SubDir>
{
	Mandatory<std::string> value;

	SubDir()
	:	value("value")
	{}
};

struct Directory : public LLInitParam::Block<Directory>
{
	Multiple<SubDir, AtLeast<1> > subdirs;

	Directory()
	:	subdirs("subdir")
	{}
};

struct Paths : public LLInitParam::Block<Paths>
{
	Multiple<Directory, AtLeast<1> > directories;

	Paths()
	:	directories("directory")
	{}
};


//static
std::string LLUI::locateSkin(const std::string& filename)
{
	std::string found_file = filename;
	if (gDirUtilp->fileExists(found_file))
	{
		return found_file;
	}

	found_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename); // Should be CUSTOM_SKINS?
	if (gDirUtilp->fileExists(found_file))
	{
		return found_file;
	}

	found_file = gDirUtilp->findSkinnedFilename(LLDir::XUI, filename);
	if (! found_file.empty())
	{
		return found_file;
	}

	found_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, filename);
	if (gDirUtilp->fileExists(found_file))
	{
		return found_file;
	}
	LL_WARNS("LLUI") << "Can't find '" << filename
					 << "' in user settings, any skin directory or app_settings" << LL_ENDL;
	return "";
}

LLVector2 LLUI::getWindowSize()
{
	LLCoordWindow window_rect;
	mWindow->getSize(&window_rect);

	return LLVector2(window_rect.mX / getScaleFactor().mV[VX], window_rect.mY / getScaleFactor().mV[VY]);
}

void LLUI::screenPointToGL(S32 screen_x, S32 screen_y, S32 *gl_x, S32 *gl_y)
{
	*gl_x = ll_round((F32)screen_x * getScaleFactor().mV[VX]);
	*gl_y = ll_round((F32)screen_y * getScaleFactor().mV[VY]);
}

void LLUI::glPointToScreen(S32 gl_x, S32 gl_y, S32 *screen_x, S32 *screen_y)
{
	*screen_x = ll_round((F32)gl_x / getScaleFactor().mV[VX]);
	*screen_y = ll_round((F32)gl_y / getScaleFactor().mV[VY]);
}

void LLUI::screenRectToGL(const LLRect& screen, LLRect *gl)
{
	screenPointToGL(screen.mLeft, screen.mTop, &gl->mLeft, &gl->mTop);
	screenPointToGL(screen.mRight, screen.mBottom, &gl->mRight, &gl->mBottom);
}

void LLUI::glRectToScreen(const LLRect& gl, LLRect *screen)
{
	glPointToScreen(gl.mLeft, gl.mTop, &screen->mLeft, &screen->mTop);
	glPointToScreen(gl.mRight, gl.mBottom, &screen->mRight, &screen->mBottom);
}


LLControlGroup& LLUI::getControlControlGroup (const std::string& controlname)
{
	for (settings_map_t::iterator itor = mSettingGroups.begin();
		 itor != mSettingGroups.end(); ++itor)
	{
		LLControlGroup* control_group = itor->second;
		if(control_group != NULL)
		{
			if (control_group->controlExists(controlname))
				return *control_group;
		}
	}

	return *mSettingGroups["config"]; // default group
}

void LLUI::addPopup(LLView* viewp)
{
	if (mAddPopupFunc)
	{
		mAddPopupFunc(viewp);
	}
}

void LLUI::removePopup(LLView* viewp)
{
	if (mRemovePopupFunc)
	{
		mRemovePopupFunc(viewp);
	}
}

void LLUI::clearPopups()
{
	if (mClearPopupsFunc)
	{
		mClearPopupsFunc();
	}
}

void LLUI::reportBadKeystroke()
{
	make_ui_sound("UISndBadKeystroke");
}

// spawn_x and spawn_y are top left corner of view in screen GL coordinates
void LLUI::positionViewNearMouse(LLView* view, S32 spawn_x, S32 spawn_y)
{
	const S32 CURSOR_HEIGHT = 16;		// Approximate "normal" cursor size
	const S32 CURSOR_WIDTH = 8;

	LLView* parent = view->getParent();

	S32 mouse_x;
	S32 mouse_y;
	getMousePositionScreen(&mouse_x, &mouse_y);

	// If no spawn location provided, use mouse position
	if (spawn_x == S32_MAX || spawn_y == S32_MAX)
	{
		spawn_x = mouse_x + CURSOR_WIDTH;
		spawn_y = mouse_y - CURSOR_HEIGHT;
	}

	LLRect virtual_window_rect = parent->getLocalRect();

	LLRect mouse_rect;
	const S32 MOUSE_CURSOR_PADDING = 1;
	mouse_rect.setLeftTopAndSize(mouse_x - MOUSE_CURSOR_PADDING, 
								mouse_y + MOUSE_CURSOR_PADDING, 
								CURSOR_WIDTH + MOUSE_CURSOR_PADDING * 2, 
								CURSOR_HEIGHT + MOUSE_CURSOR_PADDING * 2);

	S32 local_x, local_y;
	// convert screen coordinates to tooltip view-local coordinates
	parent->screenPointToLocal(spawn_x, spawn_y, &local_x, &local_y);

	// Start at spawn position (using left/top)
	view->setOrigin( local_x, local_y - view->getRect().getHeight());
	// Make sure we're on-screen and not overlapping the mouse
	view->translateIntoRectWithExclusion( virtual_window_rect, mouse_rect );
}

LLView* LLUI::resolvePath(LLView* context, const std::string& path)
{
	// Nothing about resolvePath() should require non-const LLView*. If caller
	// wants non-const, call the const flavor and then cast away const-ness.
	return const_cast<LLView*>(resolvePath(const_cast<const LLView*>(context), path));
}

const LLView* LLUI::resolvePath(const LLView* context, const std::string& path)
{
	// Create an iterator over slash-separated parts of 'path'. Dereferencing
	// this iterator returns an iterator_range over the substring. Unlike
	// LLStringUtil::getTokens(), this split_iterator doesn't combine adjacent
	// delimiters: leading/trailing slash produces an empty substring, double
	// slash produces an empty substring. That's what we need.
	boost::split_iterator<std::string::const_iterator> ti(path, boost::first_finder("/")), tend;

	if (ti == tend)
	{
		// 'path' is completely empty, no navigation
		return context;
	}

	// leading / means "start at root"
	if (ti->empty())
	{
		context = getRootView();
		++ti;
	}

	bool recurse = false;
	for (; ti != tend && context; ++ti)
	{
		if (ti->empty()) 
		{
			recurse = true;
		}
		else
		{
			std::string part(ti->begin(), ti->end());
			context = context->findChildView(LLURI::unescape(part), recurse);
			recurse = false;
		}
	}

	return context;
}

//static
LLVector2& LLUI::getScaleFactor()
{
    return LLRender::sUIGLScaleFactor;
}

//static
void LLUI::setScaleFactor(const LLVector2& scale_factor)
{
    LLRender::sUIGLScaleFactor = scale_factor;
}


// LLLocalClipRect and LLScreenClipRect moved to lllocalcliprect.h/cpp

namespace LLInitParam
{
	ParamValue<LLUIColor>::ParamValue(const LLUIColor& color)
	:	super_t(color),
		red("red"),
		green("green"),
		blue("blue"),
		alpha("alpha"),
		control("")
	{
		updateBlockFromValue(false);
	}

	void ParamValue<LLUIColor>::updateValueFromBlock()
	{
		if (control.isProvided() && !control().empty())
		{
			updateValue(LLUIColorTable::instance().getColor(control));
		}
		else
		{
			updateValue(LLColor4(red, green, blue, alpha));
		}
	}
	
	void ParamValue<LLUIColor>::updateBlockFromValue(bool make_block_authoritative)
	{
		LLColor4 color = getValue();
		red.set(color.mV[VRED], make_block_authoritative);
		green.set(color.mV[VGREEN], make_block_authoritative);
		blue.set(color.mV[VBLUE], make_block_authoritative);
		alpha.set(color.mV[VALPHA], make_block_authoritative);
		control.set("", make_block_authoritative);
	}

	bool ParamCompare<const LLFontGL*, false>::equals(const LLFontGL* a, const LLFontGL* b)
	{
		return !(a->getFontDesc() < b->getFontDesc())
			&& !(b->getFontDesc() < a->getFontDesc());
	}

	ParamValue<const LLFontGL*>::ParamValue(const LLFontGL* fontp)
	:	super_t(fontp),
		name("name"),
		size("size"),
		style("style")
	{
		if (!fontp)
		{
			updateValue(LLFontGL::getFontDefault());
		}
		addSynonym(name, "");
		updateBlockFromValue(false);
	}

	void ParamValue<const LLFontGL*>::updateValueFromBlock()
	{
		const LLFontGL* res_fontp = LLFontGL::getFontByName(name);
		if (res_fontp)
		{
			updateValue(res_fontp);
			return;
		}

		U8 fontstyle = 0;
		fontstyle = LLFontGL::getStyleFromString(style());
		LLFontDescriptor desc(name(), size(), fontstyle);
		const LLFontGL* fontp = LLFontGL::getFont(desc);
		if (fontp)
		{
			updateValue(fontp);
		}
		else
		{
			updateValue(LLFontGL::getFontDefault());
		}
	}
	
	void ParamValue<const LLFontGL*>::updateBlockFromValue(bool make_block_authoritative)
	{
		if (getValue())
		{
			name.set(LLFontGL::nameFromFont(getValue()), make_block_authoritative);
			size.set(LLFontGL::sizeFromFont(getValue()), make_block_authoritative);
			style.set(LLFontGL::getStringFromStyle(getValue()->getFontDesc().getStyle()), make_block_authoritative);
		}
	}

	ParamValue<LLRect>::ParamValue(const LLRect& rect)
	:	super_t(rect),
		left("left"),
		top("top"),
		right("right"),
		bottom("bottom"),
		width("width"),
		height("height")
	{
		updateBlockFromValue(false);
	}

	void ParamValue<LLRect>::updateValueFromBlock()
	{
		LLRect rect;

		//calculate from params
		// prefer explicit left and right
		if (left.isProvided() && right.isProvided())
		{
			rect.mLeft = left;
			rect.mRight = right;
		}
		// otherwise use width along with specified side, if any
		else if (width.isProvided())
		{
			// only right + width provided
			if (right.isProvided())
			{
				rect.mRight = right;
				rect.mLeft = right - width;
			}
			else // left + width, or just width
			{
				rect.mLeft = left;
				rect.mRight = left + width;
			}
		}
		// just left, just right, or none
		else
		{
			rect.mLeft = left;
			rect.mRight = right;
		}

		// prefer explicit bottom and top
		if (bottom.isProvided() && top.isProvided())
		{
			rect.mBottom = bottom;
			rect.mTop = top;
		}
		// otherwise height along with specified side, if any
		else if (height.isProvided())
		{
			// top + height provided
			if (top.isProvided())
			{
				rect.mTop = top;
				rect.mBottom = top - height;
			}
			// bottom + height or just height
			else
			{
				rect.mBottom = bottom;
				rect.mTop = bottom + height;
			}
		}
		// just bottom, just top, or none
		else
		{
			rect.mBottom = bottom;
			rect.mTop = top;
		}
		updateValue(rect);
	}
	
	void ParamValue<LLRect>::updateBlockFromValue(bool make_block_authoritative)
	{
		// because of the ambiguity in specifying a rect by position and/or dimensions
		// we use the lowest priority pairing so that any valid pairing in xui 
		// will override those calculated from the rect object
		// in this case, that is left+width and bottom+height
		LLRect& value = getValue();

		right.set(value.mRight, false);
		left.set(value.mLeft, make_block_authoritative);
		width.set(value.getWidth(), make_block_authoritative);

		top.set(value.mTop, false);
		bottom.set(value.mBottom, make_block_authoritative);
		height.set(value.getHeight(), make_block_authoritative);
	}

	ParamValue<LLCoordGL>::ParamValue(const LLCoordGL& coord)
	:	super_t(coord),
		x("x"),
		y("y")
	{
		updateBlockFromValue(false);
	}

	void ParamValue<LLCoordGL>::updateValueFromBlock()
	{
		updateValue(LLCoordGL(x, y));
	}
	
	void ParamValue<LLCoordGL>::updateBlockFromValue(bool make_block_authoritative)
	{
		x.set(getValue().mX, make_block_authoritative);
		y.set(getValue().mY, make_block_authoritative);
	}


	void TypeValues<LLFontGL::HAlign>::declareValues()
	{
		declare("left", LLFontGL::LEFT);
		declare("right", LLFontGL::RIGHT);
		declare("center", LLFontGL::HCENTER);
	}

	void TypeValues<LLFontGL::VAlign>::declareValues()
	{
		declare("top", LLFontGL::TOP);
		declare("center", LLFontGL::VCENTER);
		declare("baseline", LLFontGL::BASELINE);
		declare("bottom", LLFontGL::BOTTOM);
	}

	void TypeValues<LLFontGL::ShadowType>::declareValues()
	{
		declare("none", LLFontGL::NO_SHADOW);
		declare("hard", LLFontGL::DROP_SHADOW);
		declare("soft", LLFontGL::DROP_SHADOW_SOFT);
	}
}

