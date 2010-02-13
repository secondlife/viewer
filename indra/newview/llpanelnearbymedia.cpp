/** 
 * @file llpanelnearbymedia.cpp
 * @brief Management interface for muting and controlling nearby media
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llpanelnearbymedia.h"

#include "llaudioengine.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llresizebar.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llagentui.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llviewerregion.h"
#include "llviewermediafocus.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "llvovolume.h"
#include "llstatusbar.h"
#include "llsdutil.h"

#include "llfloaterreg.h"
#include "llfloaterpreference.h" // for the gear icon
#include "lltabcontainer.h"

#include <stringize.h>

extern LLControlGroup gSavedSettings;

// Ugh, isInternetStreamPlaying() returns not a bool, but an *int*, with
// 0 = stopped, 1 = playing, 2 = paused.
static const int PARCEL_AUDIO_STOPPED = 0;
static const int PARCEL_AUDIO_PLAYING = 1;
static const int PARCEL_AUDIO_PAUSED = 2;

//
// LLPanelNearByMedia
//

LLPanelNearByMedia::LLPanelNearByMedia()
:	mMediaList(NULL),
	  mEnableAllCtrl(NULL),
	  mEnableParcelMediaCtrl(NULL),	  
	  mAllMediaDisabled(false),
	  mDebugInfoVisible(false)
{
	mParcelAudioAutoStart = gSavedSettings.getBOOL(LLViewerMedia::AUTO_PLAY_MEDIA_SETTING);

	mCommitCallbackRegistrar.add("MediaListCtrl.EnableAll",		boost::bind(&LLPanelNearByMedia::onClickEnableAll, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.DisableAll",		boost::bind(&LLPanelNearByMedia::onClickDisableAll, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.GoMediaPrefs", boost::bind(&LLPanelNearByMedia::onAdvancedButtonClick, this));
	mCommitCallbackRegistrar.add("MediaListCtrl.MoreLess", boost::bind(&LLPanelNearByMedia::onMoreLess, this));
	mCommitCallbackRegistrar.add("ParcelMediaCtrl.ParcelMediaVolume",		boost::bind(&LLPanelNearByMedia::onParcelMediaVolumeSlider, this));
	mCommitCallbackRegistrar.add("ParcelMediaCtrl.MuteParcelMedia",		boost::bind(&LLPanelNearByMedia::onClickMuteParcelMedia, this));
	mCommitCallbackRegistrar.add("ParcelMediaCtrl.EnableParcelMedia",		boost::bind(&LLPanelNearByMedia::onClickEnableParcelMedia, this));
	mCommitCallbackRegistrar.add("ParcelMediaCtrl.DisableParcelMedia",		boost::bind(&LLPanelNearByMedia::onClickDisableParcelMedia, this));
	mCommitCallbackRegistrar.add("ParcelMediaCtrl.Play",		boost::bind(&LLPanelNearByMedia::onClickParcelMediaPlay, this));
	mCommitCallbackRegistrar.add("ParcelMediaCtrl.Stop",		boost::bind(&LLPanelNearByMedia::onClickParcelMediaStop, this));
	mCommitCallbackRegistrar.add("ParcelMediaCtrl.Pause",		boost::bind(&LLPanelNearByMedia::onClickParcelMediaPause, this));
	mCommitCallbackRegistrar.add("ParcelAudioCtrl.Play",		boost::bind(&LLPanelNearByMedia::onClickParcelAudioPlay, this));
	mCommitCallbackRegistrar.add("ParcelAudioCtrl.Stop",		boost::bind(&LLPanelNearByMedia::onClickParcelAudioStop, this));
	mCommitCallbackRegistrar.add("ParcelAudioCtrl.Pause",		boost::bind(&LLPanelNearByMedia::onClickParcelAudioPause, this));
	LLUICtrlFactory::instance().buildPanel(this, "panel_nearby_media.xml");
}

LLPanelNearByMedia::~LLPanelNearByMedia()
{
}

BOOL LLPanelNearByMedia::postBuild()
{
	LLPanel::postBuild();

	const S32 RESIZE_BAR_THICKNESS = 6;
	LLResizeBar::Params p;
	p.rect = LLRect(0, RESIZE_BAR_THICKNESS, getRect().getWidth(), 0);
	p.name = "resizebar_bottom";
	p.min_size = getRect().getHeight();
	p.side = LLResizeBar::BOTTOM;
	p.resizing_view = this;
	addChild( LLUICtrlFactory::create<LLResizeBar>(p) );

	mNearbyMediaPanel = getChild<LLUICtrl>("nearby_media_panel");
	mMediaList = getChild<LLScrollListCtrl>("media_list");
	mEnableAllCtrl = getChild<LLUICtrl>("all_nearby_media_enable_btn");
	mDisableAllCtrl = getChild<LLUICtrl>("all_nearby_media_disable_btn");
	mParcelMediaVolumeSlider = getChild<LLSliderCtrl>("parcel_media_volume");
	mParcelMediaMuteCtrl = getChild<LLButton>("parcel_media_mute");
	mEnableParcelMediaCtrl = getChild<LLUICtrl>("parcel_media_enable_btn");
	mDisableParcelMediaCtrl = getChild<LLUICtrl>("parcel_media_disable_btn");
	mParcelMediaText = getChild<LLTextBox>("parcel_media_name");
	mItemCountText = getChild<LLTextBox>("media_item_count");
	mParcelMediaPlayCtrl = getChild<LLButton>("parcel_media_play_btn");
	mParcelMediaPauseCtrl = getChild<LLButton>("parcel_media_pause_btn");
	mParcelMediaCtrl = getChild<LLUICtrl>("parcel_media_ctrls");
	mParcelAudioPlayCtrl = getChild<LLButton>("parcel_audio_play_btn");
	mParcelAudioPauseCtrl = getChild<LLButton>("parcel_audio_pause_btn");
	mParcelAudioCtrl = getChild<LLUICtrl>("parcel_audio_ctrls");
	mShowCtrl = getChild<LLComboBox>("show_combo");
	
	mEmptyNameString = getString("empty_item_text");
	mDefaultParcelMediaName = getString("default_parcel_media_name");
	mPlayingString = getString("playing_suffix");
	
	mMediaList->setDoubleClickCallback(onZoomMedia, this);
	mMediaList->sortByColumnIndex(PROXIMITY_COLUMN, TRUE);
	mMediaList->sortByColumnIndex(VISIBILITY_COLUMN, FALSE);

	std::string url = LLViewerParcelMedia::getURL();
	if (!url.empty())
	{
		std::string name = LLViewerParcelMedia::getName();
		mParcelMediaText->setValue(name.empty()?url:name);
		mParcelMediaText->setToolTip(url);	
	}
	refreshList();
	updateColumns();
	
	LLView* minimized_controls = getChildView("minimized_controls");
	mMoreHeight = getRect().getHeight();
	mLessHeight = getRect().getHeight() - minimized_controls->getRect().mBottom;
	getChild<LLUICtrl>("more_less_btn")->setValue(false);
	onMoreLess();
	
	return TRUE;
}

/*virtual*/
void LLPanelNearByMedia::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mHoverTimer.stop();
	LLPanel::onMouseEnter(x,y,mask);
}


/*virtual*/
void LLPanelNearByMedia::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mHoverTimer.start();
	LLPanel::onMouseLeave(x,y,mask);
}

/*virtual*/ 
void LLPanelNearByMedia::handleVisibilityChange ( BOOL new_visibility )
{
	if (new_visibility)	
	{
		mHoverTimer.start(); // timer will be stopped when mouse hovers over panel
		//gFocusMgr.setTopCtrl(this);
	}
	else
	{
		mHoverTimer.stop();
		//if (gFocusMgr.getTopCtrl() == this)
		//{
		//	gFocusMgr.setTopCtrl(NULL);
		//}
	}
}

/*virtual*/ 
void LLPanelNearByMedia::onTopLost ()
{
	//LLUICtrl* new_top = gFocusMgr.getTopCtrl();
	//if (!new_top || !new_top->hasAncestor(this))
	//{
	//	setVisible(FALSE);
	//}
}

/*virtual*/
void LLPanelNearByMedia::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);

	LLButton* more_less_btn = getChild<LLButton>("more_less_btn");
	if (more_less_btn->getValue().asBoolean())
	{
		mMoreHeight = getRect().getHeight();
	}

}

const F32 AUTO_CLOSE_FADE_TIME_START= 4.0f;
const F32 AUTO_CLOSE_FADE_TIME_END = 5.0f;

void LLPanelNearByMedia::draw()
{
	//LLUICtrl* new_top = gFocusMgr.getTopCtrl();
	//if (new_top != this)
	//{
	//	// reassert top ctrl
	//	gFocusMgr.setTopCtrl(this);
	//}

	// keep bottom of panel on screen
	LLRect screen_rect = calcScreenRect();
	if (screen_rect.mBottom < 0)
	{
		LLRect new_rect = getRect();
		new_rect.mBottom += 0 - screen_rect.mBottom;
		setShape(new_rect);
	}

	mItemCountText->setValue(llformat(getString("media_item_count_format").c_str(), mMediaList->getItemCount()));
	
//	refreshParcelMediaUI();
//	refreshParcelAudioUI();
	refreshList();
	
	F32 alpha = mHoverTimer.getStarted() 
		? clamp_rescale(mHoverTimer.getElapsedTimeF32(), AUTO_CLOSE_FADE_TIME_START, AUTO_CLOSE_FADE_TIME_END, 1.f, 0.f)
		: 1.0f;
	LLViewDrawContext context(alpha);

	LLPanel::draw();

	if (alpha == 0.f)
	{
		setVisible(false);
	}
}

bool LLPanelNearByMedia::getParcelAudioAutoStart()
{
	return mParcelAudioAutoStart;
}

void LLPanelNearByMedia::addMediaItem(const LLUUID &id)
{
	if (NULL == mMediaList) return;
	
	// Just set up the columns -- the values will be filled in by updateMediaItem().
	
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
	LLScrollListCheck* scroll_list_check = dynamic_cast<LLScrollListCheck*>(new_item->getColumn(CHECKBOX_COLUMN));
	if (scroll_list_check)
	{
		LLCheckBoxCtrl *check = scroll_list_check->getCheckBox();
		check->setCommitCallback(boost::bind(&LLPanelNearByMedia::onCheckItem, this, _1, id));
	}
}

void LLPanelNearByMedia::updateMediaItem(LLScrollListItem* item, LLViewerMediaImpl* impl)
{
	LLScrollListCell* cell = item->getColumn(PROXIMITY_COLUMN);
	if(cell)
	{
		// since we are forced to sort by text, encode sort order as string
		// proximity of -1 means "closest"
		S32 proximity = impl->isParcelMedia() ? -1 : impl->getProximity();
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
		cell->setValue(!impl->isMediaDisabled());
	}
	
	cell = item->getColumn(VISIBILITY_COLUMN);
	if(cell)
	{
		S32 old_visibility = cell->getValue();
		S32 new_visibility = (impl->hasMedia()) ? 1 : ((impl->isMediaDisabled()) ? 0 : -1);
		cell->setValue(STRINGIZE(new_visibility));
		if (new_visibility != old_visibility)
		{			
			mMediaList->setNeedsSort(true);
		}
	}
		
	S32 media_class = -1;
	cell = item->getColumn(NAME_COLUMN);
	if(cell)
	{
		std::string name;
		std::string url;
		std::string old_name = cell->getValue().asString();
		
		getNameAndUrlHelper(impl, name, url, mEmptyNameString);
		
		if (impl->isParcelMedia())
		{
			cell->setToolTip(name + " : " + url);
			name = mDefaultParcelMediaName;
		}
		else {
		cell->setToolTip(url);
		}
		if (impl->hasMedia()) name += " " + mPlayingString;
		if (name != old_name)
		{
			cell->setValue(name);
		}
		
		// *TODO: Make these font styles/colors configurable via XUI
		LLColor4 cell_color = LLColor4::white;
		U8 font_style = LLFontGL::NORMAL;
		
		// Focused
		if (impl->hasFocus())
		{
			if (mDebugInfoVisible) cell_color = LLColor4::yellow;
			media_class = MEDIA_CLASS_FOCUSED;
		}
		// Is attached to another avatar?
		else if (impl->isAttachedToAnotherAvatar())
		{
			if (mDebugInfoVisible) cell_color = LLColor4::red;
			media_class = MEDIA_CLASS_ON_OTHERS;
		}
		// Outside agent parcel
		else if (!impl->isInAgentParcel())
		{
			if (mDebugInfoVisible) cell_color = LLColor4::orange;
			media_class = MEDIA_CLASS_OUTSIDE_PARCEL;
		}
		else {
			// inside parcel
			media_class = MEDIA_CLASS_WITHIN_PARCEL;
		}
		if (impl->isMediaDisabled())
		{
			//font_style |= LLFontGL::ITALIC;
			//cell_color = LLColor4::black;
			// Dim it if it is disabled
			cell_color.setAlpha(0.25);
		}
		// Dim it if it isn't "showing"
		else if (!impl->hasMedia())
		{
			cell_color.setAlpha(0.25);
		}
		// Bold it if it is time-based media and it is playing
		else if (impl->isMediaTimeBased() &&
				 impl->isMediaPlaying())
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
			std::string s;
			
			s += llformat("%g/", (float)impl->getInterest());

			// proximity distance is actually distance squared -- display it as straight distance.
			s += llformat("%g/", fsqrtf(impl->getProximityDistance()));

//			s += llformat("%g/", (float)impl->getCPUUsage());
//			s += llformat("%g/", (float)impl->getApproximateTextureInterest());
			s += llformat("%g/", (float)(NULL == impl->getSomeObject()) ? 0.0 : impl->getSomeObject()->getPixelArea());
			
			s += LLPluginClassMedia::priorityToString(impl->getPriority());
			
			if(impl->hasMedia())
			{
				s += '@';
			}
			else if(impl->isPlayable())
			{
				s += '+';
			}
			else if(impl->isForcedUnloaded())
			{
				s += '!';
			}
				
			cell->setValue(s);
		}
	}
}

void LLPanelNearByMedia::removeMediaItem(const LLUUID &id)
{
	if (NULL == mMediaList) return;
	
	mMediaList->deleteSingleItem(mMediaList->getItemIndex(id));
}

void LLPanelNearByMedia::refreshParcelMediaUI()
{	
	std::string url = LLViewerParcelMedia::getURL();
	LLStyle::Params style_params;
	if (url.empty())
	{	
		style_params.font.style = "ITALIC";
		mParcelMediaText->setText(mDefaultParcelMediaName, style_params);
		mEnableParcelMediaCtrl->setEnabled(false);
		mDisableParcelMediaCtrl->setEnabled(false);
	}
	else {
		std::string name = LLViewerParcelMedia::getName();
		if (name.empty()) name = url;
		mParcelMediaText->setText(name, style_params);
		mParcelMediaText->setToolTip(url);
		mEnableParcelMediaCtrl->setEnabled(true);
		mDisableParcelMediaCtrl->setEnabled(true);
	}
	
	// Set up the default play controls state
	mParcelMediaPauseCtrl->setEnabled(false);
	mParcelMediaPauseCtrl->setVisible(false);
	mParcelMediaPlayCtrl->setEnabled(true);
	mParcelMediaPlayCtrl->setVisible(true);
	mParcelMediaCtrl->setEnabled(false);
	
	if (LLViewerParcelMedia::getParcelMedia())
	{
		if (LLViewerParcelMedia::getParcelMedia()->getMediaPlugin() &&
			LLViewerParcelMedia::getParcelMedia()->getMediaPlugin()->pluginSupportsMediaTime())
		{
			mParcelMediaCtrl->setEnabled(true);
			
			switch(LLViewerParcelMedia::getParcelMedia()->getMediaPlugin()->getStatus())
			{
				case LLPluginClassMediaOwner::MEDIA_PLAYING:
					mParcelMediaPlayCtrl->setEnabled(false);
					mParcelMediaPlayCtrl->setVisible(false);
					mParcelMediaPauseCtrl->setEnabled(true);
					mParcelMediaPauseCtrl->setVisible(true);
					break;
				case LLPluginClassMediaOwner::MEDIA_PAUSED:
				default:
					// default play status is kosher
					break;
			}
		}
	}
}

void LLPanelNearByMedia::refreshParcelAudioUI()
{	
	bool parcel_audio_enabled = !getParcelAudioURL().empty();

	mParcelAudioCtrl->setToolTip(getParcelAudioURL());
	
	if (gAudiop && parcel_audio_enabled)
	{
		mParcelAudioCtrl->setEnabled(true);

		if (PARCEL_AUDIO_PLAYING == gAudiop->isInternetStreamPlaying())
		{
			mParcelAudioPlayCtrl->setEnabled(false);
			mParcelAudioPlayCtrl->setVisible(false);
			mParcelAudioPauseCtrl->setEnabled(true);
			mParcelAudioPauseCtrl->setVisible(true);
		}
		else {
			mParcelAudioPlayCtrl->setEnabled(true);
			mParcelAudioPlayCtrl->setVisible(true);
			mParcelAudioPauseCtrl->setEnabled(false);
			mParcelAudioPauseCtrl->setVisible(false);
		}
	}
	else {
		mParcelAudioCtrl->setEnabled(false);
		mParcelAudioPlayCtrl->setEnabled(true);
		mParcelAudioPlayCtrl->setVisible(true);
		mParcelAudioPauseCtrl->setEnabled(false);
		mParcelAudioPauseCtrl->setVisible(false);
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
		all_items_deleted = true;
		
		updateColumns();
	}
	
	// Get the canonical list from LLViewerMedia
	LLViewerMedia::impl_list impls = LLViewerMedia::getPriorityList();
	LLViewerMedia::impl_list::iterator priority_iter;
	
	U32 enabled_count = 0;
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
		
		{
			bool remove_item = false;
			LLUUID media_id = impl->getMediaTextureID();
			if (impl->isParcelMedia())
			{
				remove_item = LLViewerParcelMedia::getURL().empty();
			}
			else {
				S32 proximity = impl->getProximity();
			// This is expensive (i.e. a linear search) -- don't use it here.  We now use mInNearbyMediaList instead.
//			S32 index = mMediaList->getItemIndex(media_id);
				remove_item = (proximity < 0 || !shouldShow(impl));
			}
			if (remove_item)
			{
				// This isn't inworld media -- don't show it in the list.
				if (impl->getInNearbyMediaList())
				{
					// There's a row for this impl -- remove it.
					removeMediaItem(media_id);
					impl->setInNearbyMediaList(false);
				}
			}
			else
			{
				if (!impl->getInNearbyMediaList())
				{
					// We don't have a row for this impl -- add one.
					addMediaItem(media_id);
					impl->setInNearbyMediaList(true);
				}
			}
			// Update counts
			if (impl->isMediaDisabled())
			{
				disabled_count++;
			}
			else {
				enabled_count++;
		}
	}
	}
	mDisableAllCtrl->setEnabled(LLViewerMedia::isAnyMediaShowing());
	mEnableAllCtrl->setEnabled(disabled_count > 0);

	// Iterate over the rows in the control, updating ones whose impl exists, and deleting ones whose impl has gone away.
	std::vector<LLScrollListItem*> items = mMediaList->getAllData();

	for (std::vector<LLScrollListItem*>::iterator item_it = items.begin();
		item_it != items.end();
		++item_it)
	{
		LLScrollListItem* item = (*item_it);
		LLUUID row_id = item->getUUID();
		
		LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(row_id);
		if(impl)
		{
			updateMediaItem(item, impl);
		}
		else
		{
			// This item's impl has been deleted -- remove the row.
			// Removing the row won't throw off our iteration, since we have a local copy of the array.
			// We just need to make sure we don't access this item after the delete.
			removeMediaItem(row_id);
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
		if (mMediaList->getColumn(VISIBILITY_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(PROXIMITY_COLUMN)) mMediaList->getColumn(PROXIMITY_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(CLASS_COLUMN)) mMediaList->getColumn(CLASS_COLUMN)->setWidth(-1);
		if (mMediaList->getColumn(DEBUG_COLUMN)) mMediaList->getColumn(DEBUG_COLUMN)->setWidth(-1);
	}
	else {
		if (mMediaList->getColumn(VISIBILITY_COLUMN)) mMediaList->getColumn(VISIBILITY_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(PROXIMITY_COLUMN)) mMediaList->getColumn(PROXIMITY_COLUMN)->setWidth(30);
		if (mMediaList->getColumn(CLASS_COLUMN)) mMediaList->getColumn(CLASS_COLUMN)->setWidth(20);
		if (mMediaList->getColumn(DEBUG_COLUMN)) mMediaList->getColumn(DEBUG_COLUMN)->setWidth(200);
	}
}

void LLPanelNearByMedia::onClickEnableAll()
	{
	LLViewerMedia::setAllMediaEnabled(true);
	// Parcel Audio, too
	onClickParcelAudioPlay();
	}

void LLPanelNearByMedia::onClickDisableAll()
	{
	LLViewerMedia::setAllMediaEnabled(false);
	// Parcel Audio, too
		onClickParcelAudioStop();
	}

void LLPanelNearByMedia::onClickEnableParcelMedia()
{	
		LLViewerParcelMedia::play(LLViewerParcelMgr::getInstance()->getAgentParcel());
	}

void LLPanelNearByMedia::onClickDisableParcelMedia()
{	
		// This actually unloads the impl, as opposed to "stop"ping the media
		LLViewerParcelMedia::stop();
	}

void LLPanelNearByMedia::onCheckItem(LLUICtrl* ctrl, const LLUUID &row_id)
{	
	LLCheckBoxCtrl* check = static_cast<LLCheckBoxCtrl*>(ctrl);

	setDisabled(row_id, ! check->getValue());
}

bool LLPanelNearByMedia::setDisabled(const LLUUID &row_id, bool disabled)
{
	LLViewerMediaImpl* impl = LLViewerMedia::getMediaImplFromTextureID(row_id);
	if(impl)
	{
		impl->setDisabled(disabled);
		return true;
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

void LLPanelNearByMedia::onClickMuteParcelMedia()
{
	if (LLViewerParcelMedia::getParcelMedia())
	{
		bool muted = mParcelMediaMuteCtrl->getValue();
		LLViewerParcelMedia::getParcelMedia()->setVolume(muted ? (F32)0 : mParcelMediaVolumeSlider->getValueF32() );
	}
}

void LLPanelNearByMedia::onParcelMediaVolumeSlider()
{
	if (LLViewerParcelMedia::getParcelMedia())
	{
		LLViewerParcelMedia::getParcelMedia()->setVolume(mParcelMediaVolumeSlider->getValueF32());
	}
}

void LLPanelNearByMedia::onClickParcelMediaPlay()
{
	LLViewerParcelMedia::play(LLViewerParcelMgr::getInstance()->getAgentParcel());
}

void LLPanelNearByMedia::onClickParcelMediaStop()
{	
	if (LLViewerParcelMedia::getParcelMedia())
	{
		// This stops the media playing, as opposed to unloading it like
		// LLViewerParcelMedia::stop() does
		LLViewerParcelMedia::getParcelMedia()->stop();
	}
}

void LLPanelNearByMedia::onClickParcelMediaPause()
{
	LLViewerParcelMedia::pause();
}

void LLPanelNearByMedia::onClickParcelAudioPlay()
{
	// User *explicitly* started the internet stream, so keep the stream
	// playing and updated as they cross to other parcels etc.
	mParcelAudioAutoStart = true;

	if (!gAudiop)
		return;

	if (PARCEL_AUDIO_PAUSED == gAudiop->isInternetStreamPlaying())
	{
		// 'false' means unpause
		gAudiop->pauseInternetStream(false);
	}
	else {
		gAudiop->startInternetStream(getParcelAudioURL());
	}
}

void LLPanelNearByMedia::onClickParcelAudioStop()
{
	// User *explicitly* stopped the internet stream, so don't
	// re-start audio when i.e. they move to another parcel, until
	// they explicitly start it again.
	mParcelAudioAutoStart = false;

	if (!gAudiop)
		return;

	gAudiop->stopInternetStream();
}

void LLPanelNearByMedia::onClickParcelAudioPause()
{
	if (!gAudiop)
		return;

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
	bool is_more = getChild<LLUICtrl>("more_less_btn")->getValue();
	mNearbyMediaPanel->setVisible(is_more);

	// enable resizing when expanded
	getChildView("resizebar_bottom")->setEnabled(is_more);

	S32 new_height = is_more ? mMoreHeight : mLessHeight;

	LLRect new_rect = getRect();
	new_rect.mBottom = new_rect.mTop - new_height;

	setShape(new_rect);
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

// static
std::string LLPanelNearByMedia::getParcelAudioURL()
{
	return LLViewerParcelMgr::getInstance()->getAgentParcel()->getMusicURL();
}


