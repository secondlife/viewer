/** 
 * @file llpanelnearbymedia.cpp
 * @brief Management interface for muting and controlling nearby media
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llpanelnearbymedia.h"

#include "llaudioengine.h"
#include "llbase64.h"
#include "llcheckboxctrl.h"
#include "llclipboard.h"
#include "llcombobox.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llagentui.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llviewermedia.h"
#include "llviewerparcelaskplay.h"
#include "llviewerparcelmedia.h"
#include "llviewerregion.h"
#include "llviewermediafocus.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "llvovolume.h"
#include "llstatusbar.h"
#include "llsdutil.h"
#include "lltoggleablemenu.h"
#include "llvieweraudio.h"
#include "llviewermenu.h"

#include "llfloaterreg.h"
#include "llfloaterpreference.h" // for the gear icon
#include "lltabcontainer.h"

#include <stringize.h>

extern LLControlGroup gSavedSettings;

static const LLUUID PARCEL_MEDIA_LIST_ITEM_UUID = LLUUID("CAB5920F-E484-4233-8621-384CF373A321");
static const LLUUID PARCEL_AUDIO_LIST_ITEM_UUID = LLUUID("DF4B020D-8A24-4B95-AB5D-CA970D694822");

//
// LLPanelNearByMedia
//


LLPanelNearByMedia::LLPanelNearByMedia()
:	mMediaList(NULL),
	  mEnableAllCtrl(NULL),
	  mDebugInfoVisible(false),
	  mParcelMediaItem(NULL),
	  mParcelAudioItem(NULL),
      mMoreLessBtn(NULL)
{
    // This is just an initial value, mParcelAudioAutoStart does not affect ParcelMediaAutoPlayEnable
    mParcelAudioAutoStart = gSavedSettings.getS32("ParcelMediaAutoPlayEnable") != 0
                            && gSavedSettings.getBOOL("MediaTentativeAutoPlay");

    gSavedSettings.getControl("ParcelMediaAutoPlayEnable")->getSignal()->connect(boost::bind(&LLPanelNearByMedia::handleMediaAutoPlayChanged, this, _2));

	mCommitCallbackRegistrar.add("MediaListCtrl.EnableAll",		boost::bind(&LLPanelNearByMedia::onClickEnableAll, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.DisableAll",		boost::bind(&LLPanelNearByMedia::onClickDisableAll, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.GoMediaPrefs", boost::bind(&LLPanelNearByMedia::onAdvancedButtonClick, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.MoreLess", boost::bind(&LLPanelNearByMedia::onMoreLess, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Stop",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaStop, this));	
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Play",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaPlay, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Pause",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaPause, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Mute",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaMute, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Volume",	boost::bind(&LLPanelNearByMedia::onCommitSelectedMediaVolume, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Zoom",		boost::bind(&LLPanelNearByMedia::onClickSelectedMediaZoom, this));
	mCommitCallbackRegistrar.add("SelectedMediaCtrl.Unzoom",	boost::bind(&LLPanelNearByMedia::onClickSelectedMediaUnzoom, this));

    // Context menu handler.
    mCommitCallbackRegistrar.add("SelectedMediaCtrl.Action",
                                 [this](LLUICtrl* ctrl, const LLSD& data)
                                 {
                                     onMenuAction(data);
                                 });
    mEnableCallbackRegistrar.add("SelectedMediaCtrl.Visible",
                                 [this](LLUICtrl* ctrl, const LLSD& data)
                                 {
                                     return onMenuVisible(data);
                                 });

	buildFromFile( "panel_nearby_media.xml");
}

LLPanelNearByMedia::~LLPanelNearByMedia()
{
}

bool LLPanelNearByMedia::postBuild()
{
	LLPanelPulldown::postBuild();

	const S32 RESIZE_BAR_THICKNESS = 6;
	LLResizeBar::Params p;
	p.rect = LLRect(0, RESIZE_BAR_THICKNESS, getRect().getWidth(), 0);
	p.name = "resizebar_bottom";
	p.min_size = getRect().getHeight();
	p.side = LLResizeBar::BOTTOM;
	p.resizing_view = this;
	addChild( LLUICtrlFactory::create<LLResizeBar>(p) );

	p.rect = LLRect( 0, getRect().getHeight(), RESIZE_BAR_THICKNESS, 0);
	p.name = "resizebar_left";
	p.min_size = getRect().getWidth();
	p.side = LLResizeBar::LEFT;
	addChild( LLUICtrlFactory::create<LLResizeBar>(p) );
	
	LLResizeHandle::Params resize_handle_p;
	resize_handle_p.rect = LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 );
	resize_handle_p.mouse_opaque(false);
	resize_handle_p.min_width(getRect().getWidth());
	resize_handle_p.min_height(getRect().getHeight());
	resize_handle_p.corner(LLResizeHandle::LEFT_BOTTOM);
	addChild(LLUICtrlFactory::create<LLResizeHandle>(resize_handle_p));

	mNearbyMediaPanel = getChild<LLUICtrl>("nearby_media_panel");
	mMediaList = getChild<LLScrollListCtrl>("media_list");
	mEnableAllCtrl = getChild<LLUICtrl>("all_nearby_media_enable_btn");
	mDisableAllCtrl = getChild<LLUICtrl>("all_nearby_media_disable_btn");
	mShowCtrl = getChild<LLComboBox>("show_combo");

	// Dynamic (selection-dependent) controls
	mStopCtrl = getChild<LLUICtrl>("stop");
	mPlayCtrl = getChild<LLUICtrl>("play");
	mPauseCtrl = getChild<LLUICtrl>("pause");
	mMuteCtrl = getChild<LLUICtrl>("mute");
	mVolumeSliderCtrl = getChild<LLUICtrl>("volume_slider_ctrl");
	mZoomCtrl = getChild<LLUICtrl>("zoom");
	mUnzoomCtrl = getChild<LLUICtrl>("unzoom");
	mVolumeSlider = getChild<LLSlider>("volume_slider");
	mMuteBtn = getChild<LLButton>("mute_btn");
    mMoreLessBtn = getChild<LLButton>("more_btn");

	mEmptyNameString = getString("empty_item_text");
	mParcelMediaName = getString("parcel_media_name");
	mParcelAudioName = getString("parcel_audio_name");
	mPlayingString = getString("playing_suffix");
	
	mMediaList->setDoubleClickCallback(onZoomMedia, this);
	mMediaList->sortByColumnIndex(PROXIMITY_COLUMN, true);
	mMediaList->sortByColumnIndex(VISIBILITY_COLUMN, false);
	
	refreshList();
	updateControls();
	updateColumns();
	
	LLView* minimized_controls = getChildView("minimized_controls");
	mMoreRect = getRect();
	mLessRect = getRect();
	mLessRect.mBottom = minimized_controls->getRect().mBottom;

    mMoreLessBtn->setVisible(false);
	onMoreLess();

    mContextMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
        "menu_nearby_media.xml",
        gMenuHolder,
        LLViewerMenuHolderGL::child_registry_t::instance());
	
	return true;
}

void LLPanelNearByMedia::handleMediaAutoPlayChanged(const LLSD& newvalue)
{
	// update mParcelAudioAutoStartMode if "ParcelMediaAutoPlayEnable" changes
    S32 value = gSavedSettings.getS32("ParcelMediaAutoPlayEnable");
    mParcelAudioAutoStart = value != 0
                            && gSavedSettings.getBOOL("MediaTentativeAutoPlay");

    LLViewerParcelAskPlay *inst = LLViewerParcelAskPlay::getInstance();
    if (value == 2 && !inst->hasData())
    {
        // Init if nessesary
        inst->loadSettings();
    }
    inst->cancelNotification();
}

/*virtual*/
void LLPanelNearByMedia::reshape(S32 width, S32 height, bool called_from_parent)
{
	LLPanelPulldown::reshape(width, height, called_from_parent);

	if (mMoreLessBtn && mMoreLessBtn->getValue().asBoolean())
	{
		mMoreRect = getRect();
	}

}

/*virtual*/
void LLPanelNearByMedia::draw()
{
	// keep bottom of panel on screen
	LLRect screen_rect = calcScreenRect();
	if (screen_rect.mBottom < 0)
	{
		LLRect new_rect = getRect();
		new_rect.mBottom += 0 - screen_rect.mBottom;
		setShape(new_rect);
	}

	refreshList();
	updateControls();

	LLPanelPulldown::draw();
}

/*virtual*/
bool LLPanelNearByMedia::handleHover(S32 x, S32 y, MASK mask)
{
	LLPanelPulldown::handleHover(x, y, mask);
	
	// If we are hovering over this panel, make sure to clear any hovered media
	// ID.  Note that the more general solution would be to clear this ID when
	// the mouse leaves the in-scene view, but that proved to be problematic.
	// See EXT-5517
	LLViewerMediaFocus::getInstance()->clearHover();
		
	// Always handle
	return true;
}

bool LLPanelNearByMedia::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
    S32 x_list, y_list;
    localPointToOtherView(x, y, &x_list, &y_list, mMediaList);
    if (mMoreLessBtn->getToggleState()
        && mMediaList->pointInView(x_list, y_list)
        && mMediaList->selectItemAt(x_list, y_list, mask))
    {
        if (mContextMenu)
        {
            mContextMenu->buildDrawLabels();
            mContextMenu->updateParent(LLMenuGL::sMenuContainer);
            LLMenuGL::showPopup(this, mContextMenu, x, y);
            return true;
        }
    }

    return LLPanelPulldown::handleRightMouseDown(x, y, mask);
}


void LLPanelNearByMedia::onVisibilityChange(bool new_visibility)
{
    if (!new_visibility && mContextMenu->getVisible())
    {
        gMenuHolder->hideMenus();
    }
    LLPanelPulldown::onVisibilityChange(new_visibility);
}

bool LLPanelNearByMedia::getParcelAudioAutoStart()
{
	return mParcelAudioAutoStart;
}

LLScrollListItem* LLPanelNearByMedia::addListItem(const LLUUID &id)
{
	if (NULL == mMediaList) return NULL;
	
	// Just set up the columns -- the values will be filled in by updateListItem().
	
	LLSD row;
	row["id"] = id;
	
	LLSD &columns = row["columns"];
	
	columns[CHECKBOX_COLUMN]["column"] = "media_checkbox_ctrl";
	columns[CHECKBOX_COLUMN]["type"] = "checkbox";	
	//if(mDebugInfoVisible)
	{
	columns[PROXIMITY_COLUMN]["column"] = "media_proximity";
	columns[PROXIMITY_COLUMN]["value"] = "";
		columns[VISIBILITY_COLUMN]["column"] = "media_visibility";
		columns[VISIBILITY_COLUMN]["value"] = "";
		columns[CLASS_COLUMN]["column"] = "media_class";
		columns[CLASS_COLUMN]["type"] = "text";
		columns[CLASS_COLUMN]["value"] = "";
	}
	columns[NAME_COLUMN]["column"] = "media_name";
	columns[NAME_COLUMN]["type"] = "text";
	columns[NAME_COLUMN]["value"] = "";
	//if(mDebugInfoVisible)
	{
		columns[DEBUG_COLUMN]["column"] = "media_debug";
		columns[DEBUG_COLUMN]["type"] = "text";
		columns[DEBUG_COLUMN]["value"] = "";
	}
	
	LLScrollListItem* new_item = mMediaList->addElement(row);
	if (NULL != new_item)
	{
		LLScrollListCheck* scroll_list_check = dynamic_cast<LLScrollListCheck*>(new_item->getColumn(CHECKBOX_COLUMN));
		if (scroll_list_check)
		{
			LLCheckBoxCtrl *check = scroll_list_check->getCheckBox();
			check->setCommitCallback(boost::bind(&LLPanelNearByMedia::onCheckItem, this, _1, id));
		}
	}	
	return new_item;
}

void LLPanelNearByMedia::updateListItem(LLScrollListItem* item, LLViewerMediaImpl* impl)
{
	std::string item_name;
	std::string item_tooltip;		
	std::string debug_str;
	LLPanelNearByMedia::MediaClass media_class = MEDIA_CLASS_ALL;
	
	getNameAndUrlHelper(impl, item_name, item_tooltip, mEmptyNameString);
	// Focused
	if (impl->hasFocus())
	{
		media_class = MEDIA_CLASS_FOCUSED;
	}
	// Is attached to another avatar?
	else if (impl->isAttachedToAnotherAvatar())
	{
		media_class = MEDIA_CLASS_ON_OTHERS;
	}
	// Outside agent parcel
	else if (!impl->isInAgentParcel())
	{
		media_class = MEDIA_CLASS_OUTSIDE_PARCEL;
	}
	else {
		// inside parcel
		media_class = MEDIA_CLASS_WITHIN_PARCEL;
	}
	
	if(mDebugInfoVisible)
	{
		debug_str += llformat("%g/", (float)impl->getInterest());
		
		// proximity distance is actually distance squared -- display it as straight distance.
		debug_str += llformat("%g/", (F32) sqrt(impl->getProximityDistance()));
		
		//			s += llformat("%g/", (float)impl->getCPUUsage());
		//			s += llformat("%g/", (float)impl->getApproximateTextureInterest());
		debug_str += llformat("%g/", (float)(NULL == impl->getSomeObject()) ? 0.0 : impl->getSomeObject()->getPixelArea());
		
		debug_str += LLPluginClassMedia::priorityToString(impl->getPriority());
		
		if(impl->hasMedia())
		{
			debug_str += '@';
		}
		else if(impl->isPlayable())
		{
			debug_str += '+';
		}
		else if(impl->isForcedUnloaded())
		{
			debug_str += '!';
		}
	}
	
	updateListItem(item,
				   item_name,
				   item_tooltip,
				   impl->getProximity(),
				   impl->isMediaDisabled(),
				   impl->hasMedia(),
				   impl->isMediaTimeBased() &&	impl->isMediaPlaying(),
				   media_class,
				   debug_str);
}

void LLPanelNearByMedia::updateListItem(LLScrollListItem* item,
										  const std::string &item_name,
										  const std::string &item_tooltip,
										  S32 proximity,
										  bool is_disabled,
										  bool has_media,
										  bool is_time_based_and_playing,
										  LLPanelNearByMedia::MediaClass media_class,
										  const std::string &debug_str)
{
	LLScrollListCell* cell = item->getColumn(PROXIMITY_COLUMN);
	if(cell)
	{
		// since we are forced to sort by text, encode sort order as string
		std::string proximity_string = STRINGIZE(proximity);
		std::string old_proximity_string = cell->getValue().asString();
		if(proximity_string != old_proximity_string)
		{
			cell->setValue(proximity_string);
			mMediaList->setNeedsSort(true);
		}
	}
	
	cell = item->getColumn(CHECKBOX_COLUMN);
	if(cell)
	{
		cell->setValue(!is_disabled);
	}
	
	cell = item->getColumn(VISIBILITY_COLUMN);
	if(cell)
	{
		S32 old_visibility = cell->getValue();
		// *HACK ALERT: force ordering of Media before Audio before the rest of the list
		S32 new_visibility = 
			item->getUUID() == PARCEL_MEDIA_LIST_ITEM_UUID ? 3
			: item->getUUID() == PARCEL_AUDIO_LIST_ITEM_UUID ? 2
			: (has_media) ? 1 
			: ((is_disabled) ? 0
			: -1);
		cell->setValue(STRINGIZE(new_visibility));
		if (new_visibility != old_visibility)
		{			
			mMediaList->setNeedsSort(true);
		}
	}
	
	cell = item->getColumn(NAME_COLUMN);
	if(cell)
	{
		std::string name = item_name;
		std::string old_name = cell->getValue().asString();
		if (has_media) 
		{
			name += " " + mPlayingString;
		}
		if (name != old_name)
		{
			cell->setValue(name);
		}
		cell->setToolTip(item_tooltip);
		
		// *TODO: Make these font styles/colors configurable via XUI
		U8 font_style = LLFontGL::NORMAL;
		LLColor4 cell_color = LLColor4::white;
		
		// Only colorize by class in debug
		if (mDebugInfoVisible)
		{
			switch (media_class) {
				case MEDIA_CLASS_FOCUSED:
					cell_color = LLColor4::yellow;
					break;
				case MEDIA_CLASS_ON_OTHERS:
					cell_color = LLColor4::red;
					break;
				case MEDIA_CLASS_OUTSIDE_PARCEL:
					cell_color = LLColor4::orange;
					break;
				case MEDIA_CLASS_WITHIN_PARCEL:
				default:
					break;
			}
		}
		if (is_disabled)
		{
			if (mDebugInfoVisible)
			{
				font_style |= LLFontGL::ITALIC;
				cell_color = LLColor4::black;
			}
			else {
				// Dim it if it is disabled
				cell_color.setAlpha(0.25);
			}
		}
		// Dim it if it isn't "showing"
		else if (!has_media)
		{
			cell_color.setAlpha(0.25);
		}
		// Bold it if it is time-based media and it is playing
		else if (is_time_based_and_playing)
		{
			if (mDebugInfoVisible) font_style |= LLFontGL::BOLD;
		}
		cell->setColor(cell_color);
		LLScrollListText *text_cell = dynamic_cast<LLScrollListText*> (cell);
		if (text_cell)
		{
			text_cell->setFontStyle(font_style);
		}
	}
	
	cell = item->getColumn(CLASS_COLUMN);
	if(cell)
	{
		// TODO: clean this up!
		cell->setValue(STRINGIZE(media_class));
	}
	
	if(mDebugInfoVisible)
	{
		cell = item->getColumn(DEBUG_COLUMN);
		if(cell)
		{
			cell->setValue(debug_str);
		}
	}
}
						 
void LLPanelNearByMedia::removeListItem(const LLUUID &id)
{
	if (NULL == mMediaList) return;
	
	mMediaList->deleteSingleItem(mMediaList->getItemIndex(id));
	mMediaList->updateLayout();
}

void LLPanelNearByMedia::refreshParcelItems()
{
	//
	// First add/remove the "fake" items Parcel Media and Parcel Audio.
	// These items will have special UUIDs 
	//    PARCEL_MEDIA_LIST_ITEM_UUID
	//    PARCEL_AUDIO_LIST_ITEM_UUID
	//
	// Get the filter choice.
	const LLSD &choice_llsd = mShowCtrl->getSelectedValue();
	MediaClass choice = (MediaClass)choice_llsd.asInteger();
	// Only show "special parcel items" if "All" or "Within" filter
	// (and if media is "enabled")
	bool should_include = (choice == MEDIA_CLASS_ALL || choice == MEDIA_CLASS_WITHIN_PARCEL);
	LLViewerMedia* media_inst = LLViewerMedia::getInstance();

	// First Parcel Media: add or remove it as necessary
	if (gSavedSettings.getBOOL("AudioStreamingMedia") && should_include && media_inst->hasParcelMedia())
	{
		// Yes, there is parcel media.
		if (NULL == mParcelMediaItem)
		{
			mParcelMediaItem = addListItem(PARCEL_MEDIA_LIST_ITEM_UUID);
			mMediaList->setNeedsSort(true);
		}
	}
	else {
		if (NULL != mParcelMediaItem) {
			removeListItem(PARCEL_MEDIA_LIST_ITEM_UUID);
			mParcelMediaItem = NULL;
			mMediaList->setNeedsSort(true);	
		}
	}
	
	// ... then update it
	if (NULL != mParcelMediaItem)
	{
		std::string name, url, tooltip;
		getNameAndUrlHelper(LLViewerParcelMedia::getInstance()->getParcelMedia(), name, url, "");
		if (name.empty() || name == url)
		{
			tooltip = url;
		}
		else
		{
			tooltip = name + " : " + url;
		}
		LLViewerMediaImpl *impl = LLViewerParcelMedia::getInstance()->getParcelMedia();
		updateListItem(mParcelMediaItem,
					   mParcelMediaName,
					   tooltip,
					   -2, // Proximity closer than anything else, before Parcel Audio
					   impl == NULL || impl->isMediaDisabled(),
					   impl != NULL && !LLViewerParcelMedia::getInstance()->getURL().empty(),
					   impl != NULL && impl->isMediaTimeBased() &&	impl->isMediaPlaying(),
					   MEDIA_CLASS_ALL,
					   "parcel media");
	}
	
	// Next Parcel Audio: add or remove it as necessary (don't show if disabled in prefs)
	if (should_include && media_inst->hasParcelAudio() && gSavedSettings.getBOOL("AudioStreamingMusic"))
	{
		// Yes, there is parcel audio.
		if (NULL == mParcelAudioItem)
		{
			mParcelAudioItem = addListItem(PARCEL_AUDIO_LIST_ITEM_UUID);
			mMediaList->setNeedsSort(true);
		}
	}
	else {
		if (NULL != mParcelAudioItem) {
			removeListItem(PARCEL_AUDIO_LIST_ITEM_UUID);
			mParcelAudioItem = NULL;
			mMediaList->setNeedsSort(true);
		}
	}
	
	// ... then update it
	if (NULL != mParcelAudioItem)
	{
		bool is_playing = media_inst->isParcelAudioPlaying();
	
		std::string url;
		url = media_inst->getParcelAudioURL();

		updateListItem(mParcelAudioItem,
					   mParcelAudioName,
					   url,
					   -1, // Proximity after Parcel Media, but closer than anything else
					   (!is_playing),
					   is_playing,
					   is_playing,
					   MEDIA_CLASS_ALL,
					   "parcel audio");
	}
}

void LLPanelNearByMedia::refreshList()
{
	bool all_items_deleted = false;
		
	if(!mMediaList)
	{
		// None of this makes any sense if the media list isn't there.
		return;
	}
	
	// Check whether the debug column has been shown/hidden.
	bool debug_info_visible = gSavedSettings.getBOOL("MediaPerformanceManagerDebug");
	if(debug_info_visible != mDebugInfoVisible)
	{
		mDebugInfoVisible = debug_info_visible;

		// Clear all items so the list gets regenerated.
		mMediaList->deleteAllItems();
		mParcelAudioItem = NULL;
		mParcelMediaItem = NULL;
		all_items_deleted = true;
		
		updateColumns();
	}
	
	refreshParcelItems();
	
	// Get the canonical list from LLViewerMedia
	LLViewerMedia* media_inst = LLViewerMedia::getInstance();
	LLViewerMedia::impl_list impls = media_inst->getPriorityList();
	LLViewerMedia::impl_list::iterator priority_iter;
	
	U32 disabled_count = 0;
	
	// iterate over the impl list, creating rows as necessary.
	for(priority_iter = impls.begin(); priority_iter != impls.end(); priority_iter++)
	{
		LLViewerMediaImpl *impl = *priority_iter;
		
		// If we just emptied out the list, every flag needs to be reset.
		if(all_items_deleted)
		{
			impl->setInNearbyMediaList(false);
		}

		if (!impl->isParcelMedia())
		{
			LLUUID media_id = impl->getMediaTextureID();
			S32 proximity = impl->getProximity();
			// This is expensive (i.e. a linear search) -- don't use it here.  We now use mInNearbyMediaList instead.
			//S32 index = mMediaList->getItemIndex(media_id);
			if (proximity < 0 || !shouldShow(impl))
			{
				if (impl->getInNearbyMediaList())
				{
					// There's a row for this impl -- remove it.
					removeListItem(media_id);
					impl->setInNearbyMediaList(false);
				}
			}
			else
			{
				if (!impl->getInNearbyMediaList())
				{
					// We don't have a row for this impl -- add one.
					addListItem(media_id);
					impl->setInNearbyMediaList(true);
				}
			}
			// Update counts
			if (impl->isMediaDisabled())
			{
				disabled_count++;
			}
		}
	}
	mDisableAllCtrl->setEnabled((gSavedSettings.getBOOL("AudioStreamingMusic") || 
								 gSavedSettings.getBOOL("AudioStreamingMedia")) &&
								(media_inst->isAnyMediaShowing() || 
								 media_inst->isParcelMediaPlaying() ||
								 media_inst->isParcelAudioPlaying()));

	mEnableAllCtrl->setEnabled( (gSavedSettings.getBOOL("AudioStreamingMusic") ||
								gSavedSettings.getBOOL("AudioStreamingMedia")) &&
							   (disabled_count > 0 ||
								// parcel media (if we have it, and it isn't playing, enable "start")
								(media_inst->hasParcelMedia() && ! media_inst->isParcelMediaPlaying()) ||
								// parcel audio (if we have it, and it isn't playing, enable "start")
								(media_inst->hasParcelAudio() && ! media_inst->isParcelAudioPlaying())));

	// Iterate over the rows in the control, updating ones whose impl exists, and deleting ones whose impl has gone away.
	std::vector<LLScrollListItem*> items = mMediaList->getAllData();

	for (std::vector<LLScrollListItem*>::iterator item_it = items.begin();
		item_it != items.end();
		++item_it)
	{
		LLScrollListItem* item = (*item_it);
		LLUUID row_id = item->getUUID();
		
		if (row_id != PARCEL_MEDIA_LIST_ITEM_UUID &&
			row_id != PARCEL_AUDIO_LIST_ITEM_UUID)
		{
			LLViewerMediaImpl* impl = media_inst->getMediaImplFromTextureID(row_id);
			if(impl)
			{
				updateListItem(item, impl);
			}
			else
			{
				// This item's impl has been deleted -- remove the row.
				// Removing the row won't throw off our iteration, since we have a local copy of the array.
				// We just need to make sure we don't access this item after the delete.
				removeListItem(row_id);
			}
		}
	}
	
	// Set the selection to whatever media impl the media focus/hover is on. 
	// This is an experiment, and can be removed by ifdefing out these 4 lines.
	LLUUID media_target = LLViewerMediaFocus::getInstance()->getControlsMediaID();
	if(media_target.notNull())
	{
		mMediaList->selectByID(media_target);
	}
}

void LLPanelNearByMedia::updateColumns()
{
	if (!mDebugInfoVisible)
	{
		if (mMediaList->getColumn(CHECKBOX_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(VISIBILITY_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(PROXIMITY_COLUMN)) mMediaList->getColumn(PROXIMITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(CLASS_COLUMN)) mMediaList->getColumn(CLASS_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(DEBUG_COLUMN)) mMediaList->getColumn(DEBUG_COLUMN)->setWidth(-1);
	}
	else {
		if (mMediaList->getColumn(CHECKBOX_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(VISIBILITY_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(PROXIMITY_COLUMN)) mMediaList->getColumn(PROXIMITY_COLUMN)->setWidth(30);
		if (mMediaList->getColumn(CLASS_COLUMN)) mMediaList->getColumn(CLASS_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(DEBUG_COLUMN)) mMediaList->getColumn(DEBUG_COLUMN)->setWidth(200);
	}
}

void LLPanelNearByMedia::onClickEnableAll()
{
	LLViewerMedia::getInstance()->setAllMediaEnabled(true);
}

void LLPanelNearByMedia::onClickDisableAll()
{
	LLViewerMedia::getInstance()->setAllMediaEnabled(false);
}

void LLPanelNearByMedia::onClickEnableParcelMedia()
{	
	if ( ! LLViewerMedia::getInstance()->isParcelMediaPlaying() )
	{
		LLViewerParcelMedia::getInstance()->play(LLViewerParcelMgr::getInstance()->getAgentParcel());
	}
}

void LLPanelNearByMedia::onClickDisableParcelMedia()
{	
	// This actually unloads the impl, as opposed to "stop"ping the media
	LLViewerParcelMedia::getInstance()->stop();
}

void LLPanelNearByMedia::onCheckItem(LLUICtrl* ctrl, const LLUUID &row_id)
{	
	LLCheckBoxCtrl* check = static_cast<LLCheckBoxCtrl*>(ctrl);

	setDisabled(row_id, ! check->getValue());
}

bool LLPanelNearByMedia::setDisabled(const LLUUID &row_id, bool disabled)
{
	if (row_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		if (disabled)
		{
			onClickParcelAudioStop();
		}
		else
		{
			onClickParcelAudioPlay();
		}
		return true;
	}
	else if (row_id == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		if (disabled)
		{
			onClickDisableParcelMedia();
		}
		else
		{
			onClickEnableParcelMedia();
		}
		return true;
	}
	else {
		LLViewerMediaImpl* impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(row_id);
		if(impl)
		{
			impl->setDisabled(disabled, true);
			return true;
		}
	}
	return false;
}
								
//static
void LLPanelNearByMedia::onZoomMedia(void* user_data)
{
	LLPanelNearByMedia* panelp = (LLPanelNearByMedia*)user_data;
	LLUUID media_id = panelp->mMediaList->getValue().asUUID();
	
	LLViewerMediaFocus::getInstance()->focusZoomOnMedia(media_id);
}

void LLPanelNearByMedia::onClickParcelMediaPlay()
{
	LLViewerParcelMedia::getInstance()->play(LLViewerParcelMgr::getInstance()->getAgentParcel());
}

void LLPanelNearByMedia::onClickParcelMediaStop()
{	
	if (LLViewerParcelMedia::getInstance()->getParcelMedia())
	{
		// This stops the media playing, as opposed to unloading it like
		// LLViewerParcelMedia::stop() does
		LLViewerParcelMedia::getInstance()->getParcelMedia()->stop();
	}
}

void LLPanelNearByMedia::onClickParcelMediaPause()
{
	LLViewerParcelMedia::getInstance()->pause();
}

void LLPanelNearByMedia::onClickParcelAudioPlay()
{
	// User *explicitly* started the internet stream, so keep the stream
	// playing and updated as they cross to other parcels etc.
	mParcelAudioAutoStart = true;
	if (!gAudiop)
	{
		LL_WARNS("AudioEngine") << "LLAudioEngine instance doesn't exist!" << LL_ENDL;
		return;
	}

	if (LLAudioEngine::AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
	{
		// 'false' means unpause
		gAudiop->pauseInternetStream(false);
	}
	else
	{
		LLViewerAudio::getInstance()->startInternetStreamWithAutoFade(LLViewerMedia::getInstance()->getParcelAudioURL());
	}
}

void LLPanelNearByMedia::onClickParcelAudioStop()
{
	// User *explicitly* stopped the internet stream, so don't
	// re-start audio when i.e. they move to another parcel, until
	// they explicitly start it again.
	mParcelAudioAutoStart = false;
	if (!gAudiop)
	{
		LL_WARNS("AudioEngine") << "LLAudioEngine instance doesn't exist!" << LL_ENDL;
		return;
	}

	LLViewerAudio::getInstance()->stopInternetStreamWithAutoFade();
}

void LLPanelNearByMedia::onClickParcelAudioPause()
{
	if (!gAudiop)
	{
		LL_WARNS("AudioEngine") << "LLAudioEngine instance doesn't exist!" << LL_ENDL;
		return;
	}

	// 'true' means pause
	gAudiop->pauseInternetStream(true);
}

bool LLPanelNearByMedia::shouldShow(LLViewerMediaImpl* impl)
{	
	const LLSD &choice_llsd = mShowCtrl->getSelectedValue();
	MediaClass choice = (MediaClass)choice_llsd.asInteger();

	switch (choice)
	{	
		case MEDIA_CLASS_ALL:
			return true;
			break;
		case MEDIA_CLASS_WITHIN_PARCEL:
			return impl->isInAgentParcel();
			break;
		case MEDIA_CLASS_OUTSIDE_PARCEL:
			return ! impl->isInAgentParcel();
			break;
		case MEDIA_CLASS_ON_OTHERS:
			return impl->isAttachedToAnotherAvatar();
			break;
		default:
			break;
	}
	return true;
}

void LLPanelNearByMedia::onAdvancedButtonClick()
{	
	// bring up the prefs floater
	LLFloaterPreference* prefsfloater = dynamic_cast<LLFloaterPreference*>(LLFloaterReg::showInstance("preferences"));
	if (prefsfloater)
	{
		// grab the 'audio' panel from the preferences floater and
		// bring it the front!
		LLTabContainer* tabcontainer = prefsfloater->getChild<LLTabContainer>("pref core");
		LLPanel* audiopanel = prefsfloater->getChild<LLPanel>("audio");
		if (tabcontainer && audiopanel)
		{
			tabcontainer->selectTabPanel(audiopanel);
		}
	}
}

void LLPanelNearByMedia::onMoreLess()
{
	bool is_more = mMoreLessBtn->getToggleState();
	mNearbyMediaPanel->setVisible(is_more);

	// enable resizing when expanded
	getChildView("resizebar_bottom")->setEnabled(is_more);

	LLRect new_rect = is_more ? mMoreRect : mLessRect;
	new_rect.translate(getRect().mRight - new_rect.mRight, getRect().mTop - new_rect.mTop);

	setShape(new_rect);

    mMoreLessBtn->setVisible(true);
}

void LLPanelNearByMedia::updateControls()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	LLViewerMedia* media_inst = LLViewerMedia::getInstance();
	
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		if (!media_inst->getInstance()->hasParcelAudio() || !gSavedSettings.getBOOL("AudioStreamingMusic"))
		{
			// disable controls if audio streaming music is disabled from preference
			showDisabledControls();
		}
		else {
			showTimeBasedControls(media_inst->isParcelAudioPlaying(),
							  false, // include_zoom
							  false, // is_zoomed
							  gSavedSettings.getBOOL("MuteMusic"), 
							  gSavedSettings.getF32("AudioLevelMusic") );
		}
	}
	else if (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
	{
		if (!media_inst->hasParcelMedia() || !gSavedSettings.getBOOL("AudioStreamingMedia"))
		{
			// disable controls if audio streaming media is disabled from preference
			showDisabledControls();
		}
		else {
			LLViewerMediaImpl* impl = LLViewerParcelMedia::getInstance()->getParcelMedia();
			if (NULL == impl)
			{
				// Just means it hasn't started yet
				showBasicControls(false, false, false, false, 0);
			}
			else if (impl->isMediaTimeBased())
			{
				showTimeBasedControls(impl->isMediaPlaying(), 
									  false, // include_zoom
									  false, // is_zoomed
									  impl->getVolume() == 0.0,
									  impl->getVolume() );
			}
			else {
				// non-time-based parcel media
				showBasicControls(media_inst->isParcelMediaPlaying(), 
							      false, 
								  false, 
								  impl->getVolume() == 0.0, 
								  impl->getVolume());
			}
		}
	}
	else {
		LLViewerMediaImpl* impl = media_inst->getMediaImplFromTextureID(selected_media_id);
		
		if (NULL == impl || !gSavedSettings.getBOOL("AudioStreamingMedia"))
		{
			showDisabledControls();
		}
		else {
			if (impl->isMediaTimeBased())
			{
				showTimeBasedControls(impl->isMediaPlaying(), 
									  ! impl->isParcelMedia(),  // include_zoom
									  LLViewerMediaFocus::getInstance()->isZoomed(),
									  impl->getVolume() == 0.0,
									  impl->getVolume());
			}
			else {
				showBasicControls(!impl->isMediaDisabled(), 
								  ! impl->isParcelMedia(),  // include_zoom
								  LLViewerMediaFocus::getInstance()->isZoomedOnMedia(impl->getMediaTextureID()),
								  impl->getVolume() == 0.0,
								  impl->getVolume());
			}
		}
	}
}

void LLPanelNearByMedia::showBasicControls(bool playing, bool include_zoom, bool is_zoomed, bool muted, F32 volume)
{
	mStopCtrl->setVisible(playing);
	mPlayCtrl->setVisible(!playing);
	mPauseCtrl->setVisible(false);
	mVolumeSliderCtrl->setVisible(true);
	mMuteCtrl->setVisible(true);
	mMuteBtn->setValue(muted);
	mVolumeSlider->setValue(volume);
	mZoomCtrl->setVisible(include_zoom && !is_zoomed);
	mUnzoomCtrl->setVisible(include_zoom && is_zoomed);	
	mStopCtrl->setEnabled(true);
	mZoomCtrl->setEnabled(true);
}

void LLPanelNearByMedia::showTimeBasedControls(bool playing, bool include_zoom, bool is_zoomed, bool muted, F32 volume)
{
	mStopCtrl->setVisible(true);
	mPlayCtrl->setVisible(!playing);
	mPauseCtrl->setVisible(playing);
	mMuteCtrl->setVisible(true);
	mVolumeSliderCtrl->setVisible(true);
	mZoomCtrl->setVisible(include_zoom);
	mZoomCtrl->setVisible(include_zoom && !is_zoomed);
	mUnzoomCtrl->setVisible(include_zoom && is_zoomed);	
	mStopCtrl->setEnabled(true);
	mZoomCtrl->setEnabled(true);
	mMuteBtn->setValue(muted);
	mVolumeSlider->setValue(volume);
}

void LLPanelNearByMedia::showDisabledControls()
{
	mStopCtrl->setVisible(true);
	mPlayCtrl->setVisible(false);
	mPauseCtrl->setVisible(false);
	mMuteCtrl->setVisible(false);
	mVolumeSliderCtrl->setVisible(false);
	mZoomCtrl->setVisible(true);
	mUnzoomCtrl->setVisible(false);	
	mStopCtrl->setEnabled(false);
	mZoomCtrl->setEnabled(false);
}

void LLPanelNearByMedia::onClickSelectedMediaStop()
{
	setDisabled(mMediaList->getValue().asUUID(), true);
}

void LLPanelNearByMedia::onClickSelectedMediaPlay()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	
	// First enable it
	setDisabled(selected_media_id, false);
	
	// Special code to make play "unpause" if time-based and playing
	if (selected_media_id != PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		LLViewerMediaImpl *impl = (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID) ?
			((LLViewerMediaImpl*)LLViewerParcelMedia::getInstance()->getParcelMedia()) : LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl)
		{
			if (impl->isMediaTimeBased() && impl->isMediaPaused())
			{
				// Aha!  It's really time-based media that's paused, so unpause
				impl->play();
				return;
			}
			else if (impl->isParcelMedia())
			{
				LLViewerParcelMedia::getInstance()->play(LLViewerParcelMgr::getInstance()->getAgentParcel());
			}
		}
	}	
}

void LLPanelNearByMedia::onClickSelectedMediaPause()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		onClickParcelAudioPause();
	}
	else if (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID) 
	{
		onClickParcelMediaPause();
	}
	else {
		LLViewerMediaImpl* impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl && impl->isMediaTimeBased() && impl->isMediaPlaying())
		{
			impl->pause();
		}
	}
}

void LLPanelNearByMedia::onClickSelectedMediaMute()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		gSavedSettings.setBOOL("MuteMusic", mMuteBtn->getValue());
	}
	else {
		LLViewerMediaImpl* impl = (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID) ?
			((LLViewerMediaImpl*)LLViewerParcelMedia::getInstance()->getParcelMedia()) : LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl)
		{
			F32 volume = impl->getVolume();
			if(volume > 0.0)
			{
				impl->setMute(true);
			}
			else if (mVolumeSlider->getValueF32() == 0.0)
			{
				impl->setMute(false);
				mVolumeSlider->setValue(impl->getVolume());
			}
			else 
			{
				impl->setVolume(mVolumeSlider->getValueF32());
			}
		}
	}
}

void LLPanelNearByMedia::onCommitSelectedMediaVolume()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
	{
		F32 vol = mVolumeSlider->getValueF32();
		gSavedSettings.setF32("AudioLevelMusic", vol);
	}
	else {
		LLViewerMediaImpl* impl = (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID) ?
			((LLViewerMediaImpl*)LLViewerParcelMedia::getInstance()->getParcelMedia()) : LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_media_id);
		if (NULL != impl)
		{
			impl->setVolume(mVolumeSlider->getValueF32());
		}
	}
}

void LLPanelNearByMedia::onClickSelectedMediaZoom()
{
	LLUUID selected_media_id = mMediaList->getValue().asUUID();
	if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID || selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
		return;
	LLViewerMediaFocus::getInstance()->focusZoomOnMedia(selected_media_id);
}

void LLPanelNearByMedia::onClickSelectedMediaUnzoom()
{
	LLViewerMediaFocus::getInstance()->unZoom();
}

void LLPanelNearByMedia::onMenuAction(const LLSD& userdata)
{
    const std::string command_name = userdata.asString();
    if ("copy_url" == command_name)
    {
        LLClipboard::instance().reset();
        std::string url = getSelectedUrl();

        if (!url.empty())
        {
            LLClipboard::instance().copyToClipboard(utf8str_to_wstring(url), 0, url.size());
        }
    }
    else if ("copy_data" == command_name)
    {
        LLClipboard::instance().reset();
        std::string url = getSelectedUrl();
        static const std::string encoding_specifier = "base64,";
        size_t pos = url.find(encoding_specifier);
        if (pos != std::string::npos)
        {
            pos += encoding_specifier.size();
            std::string res = LLBase64::decodeAsString(url.substr(pos));
            LLClipboard::instance().copyToClipboard(utf8str_to_wstring(res), 0, res.size());
        }
        else
        {
            url = LLURI::unescape(url);
            LLClipboard::instance().copyToClipboard(utf8str_to_wstring(url), 0, url.size());
        }
    }
}

bool LLPanelNearByMedia::onMenuVisible(const LLSD& userdata)
{
    const std::string command_name = userdata.asString();
    if ("copy_data" == command_name)
    {
        std::string url = getSelectedUrl();
        if (url.rfind("data:", 0) == 0)
        {
            // might be a a good idea to permit text/html only
            return true;
        }
    }
    return false;
}

// static
void LLPanelNearByMedia::getNameAndUrlHelper(LLViewerMediaImpl* impl, std::string& name, std::string & url, const std::string &defaultName)
{
	if (NULL == impl) return;
	
	name = impl->getName();
	url = impl->getCurrentMediaURL();	// This is the URL the media impl actually has loaded
	if (url.empty())
	{
		url = impl->getMediaEntryURL();	// This is the current URL from the media data
	}
	if (url.empty())
	{
		url = impl->getHomeURL();		// This is the home URL from the media data
	}
	if (name.empty())
	{
		name = url;
	}
	if (name.empty())
	{
		name = defaultName;
	}
}

std::string LLPanelNearByMedia::getSelectedUrl()
{
    std::string url;
    LLUUID selected_media_id = mMediaList->getValue().asUUID();
    if (selected_media_id == PARCEL_AUDIO_LIST_ITEM_UUID)
    {
        url = LLViewerMedia::getInstance()->getParcelAudioURL();
    }
    else if (selected_media_id == PARCEL_MEDIA_LIST_ITEM_UUID)
    {
        url = LLViewerParcelMedia::getInstance()->getURL();
    }
    else
    {
        LLViewerMediaImpl* impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_media_id);
        if (NULL != impl)
        {
            std::string name;
            getNameAndUrlHelper(impl, name, url, mEmptyNameString);
        }
    }
    return url;
}

