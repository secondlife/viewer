/** 
 * @file llpreviewgesture.cpp
 * @brief Editing UI for inventory-based gestures.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llpreviewgesture.h"

#include "llagent.h"
#include "llanimstatelabels.h"
#include "llanimationstates.h"
#include "llappviewer.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldatapacker.h"
#include "lldelayedgestureerror.h"
#include "llfloaterreg.h"
#include "llgesturemgr.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llkeyboard.h"
#include "llmultigesture.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "llresmgr.h"
#include "lltrans.h"
#include "llfilesystem.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerassetupload.h"

std::string NONE_LABEL;
std::string SHIFT_LABEL;
std::string CTRL_LABEL;

void dialog_refresh_all();

// used for getting

class LLInventoryGestureAvailable : public LLInventoryCompletionObserver
{
public:
	LLInventoryGestureAvailable() {}

protected:
	virtual void done();
};

void LLInventoryGestureAvailable::done()
{
	for(uuid_vec_t::iterator it = mComplete.begin(); it != mComplete.end(); ++it)
	{
		LLPreviewGesture* preview = LLFloaterReg::findTypedInstance<LLPreviewGesture>("preview_gesture", *it);
		if(preview)
		{
			preview->refresh();
		}
	}
	gInventory.removeObserver(this);
	delete this;
}

// Used for sorting
struct SortItemPtrsByName
{
	bool operator()(const LLInventoryItem* i1, const LLInventoryItem* i2)
	{
		return (LLStringUtil::compareDict(i1->getName(), i2->getName()) < 0);
	}
};

// static
LLPreviewGesture* LLPreviewGesture::show(const LLUUID& item_id, const LLUUID& object_id)
{
	LLPreviewGesture* preview = LLFloaterReg::showTypedInstance<LLPreviewGesture>("preview_gesture", LLSD(item_id), TAKE_FOCUS_YES);
	if (!preview)
	{
		return NULL;
	}
	
	preview->setObjectID(object_id);
	
	// Start speculative download of sounds and animations
	const LLUUID animation_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ANIMATION);
	LLInventoryModelBackgroundFetch::instance().start(animation_folder_id);

	const LLUUID sound_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_SOUND);
	LLInventoryModelBackgroundFetch::instance().start(sound_folder_id);

	// this will call refresh when we have everything.
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)preview->getItem();
	if (item && !item->isFinished())
	{
		LLInventoryGestureAvailable* observer;
		observer = new LLInventoryGestureAvailable();
		observer->watchItem(item_id);
		gInventory.addObserver(observer);
		item->fetchFromServer();
	}
	else
	{
		// not sure this is necessary.
		preview->refresh();
	}

	return preview;
}

void LLPreviewGesture::draw()
{
	// Skip LLPreview::draw() to avoid description update
	LLFloater::draw();
}

// virtual
bool LLPreviewGesture::handleKeyHere(KEY key, MASK mask)
{
	if(('S' == key) && (MASK_CONTROL == (mask & MASK_CONTROL)))
	{
		saveIfNeeded();
		return true;
	}

	return LLPreview::handleKeyHere(key, mask);
}


// virtual
bool LLPreviewGesture::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{
	bool handled = true;
	switch(cargo_type)
	{
	case DAD_ANIMATION:
	case DAD_SOUND:
		{
			// TODO: Don't allow this if you can't transfer the sound/animation

			// make a script step
			LLInventoryItem* item = (LLInventoryItem*)cargo_data;
			if (item
				&& gInventory.getItem(item->getUUID()))
			{
				LLPermissions perm = item->getPermissions();
				if (!((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED))
				{
					*accept = ACCEPT_NO;
					if (tooltip_msg.empty())
					{
						tooltip_msg.assign("Only animations and sounds\n"
											"with unrestricted permissions\n"
											"can be added to a gesture.");
					}
					break;
				}
				else if (drop)
				{
					LLScrollListItem* line = NULL;
					if (cargo_type == DAD_ANIMATION)
					{
						line = addStep( STEP_ANIMATION );
						LLGestureStepAnimation* anim = (LLGestureStepAnimation*)line->getUserdata();
						anim->mAnimAssetID = item->getAssetUUID();
						anim->mAnimName = item->getName();
					}
					else if (cargo_type == DAD_SOUND)
					{
						line = addStep( STEP_SOUND );
						LLGestureStepSound* sound = (LLGestureStepSound*)line->getUserdata();
						sound->mSoundAssetID = item->getAssetUUID();
						sound->mSoundName = item->getName();
					}
					updateLabel(line);
					mDirty = true;
					refresh();
				}
				*accept = ACCEPT_YES_COPY_MULTI;
			}
			else
			{
				// Not in user's inventory means it was in object inventory
				*accept = ACCEPT_NO;
			}
			break;
		}
	default:
		*accept = ACCEPT_NO;
		if (tooltip_msg.empty())
		{
			tooltip_msg.assign("Only animations and sounds\n"
								"can be added to a gesture.");
		}
		break;
	}
	return handled;
}


// virtual
bool LLPreviewGesture::canClose()
{

	if(!mDirty || mForceClose)
	{
		return true;
	}
	else
	{
		if(!mSaveDialogShown)
		{
			mSaveDialogShown = true;
			// Bring up view-modal dialog: Save changes? Yes, No, Cancel
			LLNotificationsUtil::add("SaveChanges", LLSD(), LLSD(),
					boost::bind(&LLPreviewGesture::handleSaveChangesDialog, this, _1, _2) );
		}
		return false;
	}
}

// virtual
void LLPreviewGesture::onClose(bool app_quitting)
{
	LLGestureMgr::instance().stopGesture(mPreviewGesture);
}

// virtual
void LLPreviewGesture::onUpdateSucceeded()
{
	refresh();
}

void LLPreviewGesture::onVisibilityChanged ( const LLSD& new_visibility )
{
	if (new_visibility.asBoolean())
	{
		refresh();
	}
}


bool LLPreviewGesture::handleSaveChangesDialog(const LLSD& notification, const LLSD& response)
{
	mSaveDialogShown = false;
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:  // "Yes"
		LLGestureMgr::instance().stopGesture(mPreviewGesture);
		mCloseAfterSave = true;
		onClickSave(this);
		break;

	case 1:  // "No"
		LLGestureMgr::instance().stopGesture(mPreviewGesture);
		mDirty = false; // Force the dirty flag because user has clicked NO on confirm save dialog...
		closeFloater();
		break;

	case 2: // "Cancel"
	default:
		// If we were quitting, we didn't really mean it.
		LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}


LLPreviewGesture::LLPreviewGesture(const LLSD& key)
:	LLPreview(key),
	mTriggerEditor(NULL),
	mModifierCombo(NULL),
	mKeyCombo(NULL),
	mLibraryList(NULL),
	mAddBtn(NULL),
	mUpBtn(NULL),
	mDownBtn(NULL),
	mDeleteBtn(NULL),
	mStepList(NULL),
	mOptionsText(NULL),
	mAnimationRadio(NULL),
	mAnimationCombo(NULL),
	mSoundCombo(NULL),
	mChatEditor(NULL),
	mSaveBtn(NULL),
	mPreviewBtn(NULL),
	mPreviewGesture(NULL),
	mDirty(false)
{
	NONE_LABEL =  LLTrans::getString("---");
	SHIFT_LABEL = LLTrans::getString("KBShift");
	CTRL_LABEL = LLTrans::getString("KBCtrl");
}


LLPreviewGesture::~LLPreviewGesture()
{
	// Userdata for all steps is a LLGestureStep we need to clean up
	std::vector<LLScrollListItem*> data_list = mStepList->getAllData();
	std::vector<LLScrollListItem*>::iterator data_itor;
	for (data_itor = data_list.begin(); data_itor != data_list.end(); ++data_itor)
	{
		LLScrollListItem* item = *data_itor;
		LLGestureStep* step = (LLGestureStep*)item->getUserdata();
		delete step;
		step = NULL;
	}
}


bool LLPreviewGesture::postBuild()
{
	setVisibleCallback(boost::bind(&LLPreviewGesture::onVisibilityChanged, this, _2));
	
	LLLineEditor* edit;
	LLComboBox* combo;
	LLButton* btn;
	LLScrollListCtrl* list;
	LLTextBox* text;
	LLCheckBoxCtrl* check;

	edit = getChild<LLLineEditor>("desc");
	edit->setKeystrokeCallback(onKeystrokeCommit, this);

	edit = getChild<LLLineEditor>("trigger_editor");
	edit->setKeystrokeCallback(onKeystrokeCommit, this);
	edit->setCommitCallback(onCommitSetDirty, this);
	edit->setCommitOnFocusLost(true);
	edit->setIgnoreTab(true);
	mTriggerEditor = edit;

	text = getChild<LLTextBox>("replace_text");
	text->setEnabled(false);
	mReplaceText = text;

	edit = getChild<LLLineEditor>("replace_editor");
	edit->setEnabled(false);
	edit->setKeystrokeCallback(onKeystrokeCommit, this);
	edit->setCommitCallback(onCommitSetDirty, this);
	edit->setCommitOnFocusLost(true);
	edit->setIgnoreTab(true);
	mReplaceEditor = edit;

	combo = getChild<LLComboBox>( "modifier_combo");
	combo->setCommitCallback(boost::bind(&LLPreviewGesture::onCommitKeyorModifier, this));
	mModifierCombo = combo;

	combo = getChild<LLComboBox>( "key_combo");
	combo->setCommitCallback(boost::bind(&LLPreviewGesture::onCommitKeyorModifier, this));
	mKeyCombo = combo;

	list = getChild<LLScrollListCtrl>("library_list");
	list->setCommitCallback(onCommitLibrary, this);
	list->setDoubleClickCallback(onClickAdd, this);
	mLibraryList = list;

	btn = getChild<LLButton>( "add_btn");
	btn->setClickedCallback(onClickAdd, this);
	btn->setEnabled(false);
	mAddBtn = btn;

	btn = getChild<LLButton>( "up_btn");
	btn->setClickedCallback(onClickUp, this);
	btn->setEnabled(false);
	mUpBtn = btn;

	btn = getChild<LLButton>( "down_btn");
	btn->setClickedCallback(onClickDown, this);
	btn->setEnabled(false);
	mDownBtn = btn;

	btn = getChild<LLButton>( "delete_btn");
	btn->setClickedCallback(onClickDelete, this);
	btn->setEnabled(false);
	mDeleteBtn = btn;

	list = getChild<LLScrollListCtrl>("step_list");
	list->setCommitCallback(onCommitStep, this);
	mStepList = list;

	// Options
	mOptionsText = getChild<LLTextBox>("options_text");

	combo = getChild<LLComboBox>( "animation_list");
	combo->setVisible(false);
	combo->setCommitCallback(onCommitAnimation, this);
	mAnimationCombo = combo;

	LLRadioGroup* group;
	group = getChild<LLRadioGroup>("animation_trigger_type");
	group->setVisible(false);
	group->setCommitCallback(onCommitAnimationTrigger, this);
	mAnimationRadio = group;

	combo = getChild<LLComboBox>( "sound_list");
	combo->setVisible(false);
	combo->setCommitCallback(onCommitSound, this);
	mSoundCombo = combo;

	edit = getChild<LLLineEditor>("chat_editor");
	edit->setVisible(false);
	edit->setCommitCallback(onCommitChat, this);
	//edit->setKeystrokeCallback(onKeystrokeCommit, this);
	edit->setCommitOnFocusLost(true);
	edit->setIgnoreTab(true);
	mChatEditor = edit;

	check = getChild<LLCheckBoxCtrl>( "wait_anim_check");
	check->setVisible(false);
	check->setCommitCallback(onCommitWait, this);
	mWaitAnimCheck = check;

	check = getChild<LLCheckBoxCtrl>( "wait_time_check");
	check->setVisible(false);
	check->setCommitCallback(onCommitWait, this);
	mWaitTimeCheck = check;

	edit = getChild<LLLineEditor>("wait_time_editor");
	edit->setEnabled(false);
	edit->setVisible(false);
	edit->setPrevalidate(LLTextValidate::validateFloat);
//	edit->setKeystrokeCallback(onKeystrokeCommit, this);
	edit->setCommitOnFocusLost(true);
	edit->setCommitCallback(onCommitWaitTime, this);
	edit->setIgnoreTab(true);
	mWaitTimeEditor = edit;

	// Buttons at the bottom
	check = getChild<LLCheckBoxCtrl>( "active_check");
	check->setCommitCallback(onCommitActive, this);
	mActiveCheck = check;

	btn = getChild<LLButton>( "save_btn");
	btn->setClickedCallback(onClickSave, this);
	mSaveBtn = btn;

	btn = getChild<LLButton>( "preview_btn");
	btn->setClickedCallback(onClickPreview, this);
	mPreviewBtn = btn;


	// Populate the comboboxes
	addModifiers();
	addKeys();
	addAnimations();
	addSounds();

	const LLInventoryItem* item = getItem();

	if (item) 
	{
		getChild<LLUICtrl>("desc")->setValue(item->getDescription());
		getChild<LLLineEditor>("desc")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
	}

	return LLPreview::postBuild();
}


void LLPreviewGesture::addModifiers()
{
	LLComboBox* combo = mModifierCombo;

	combo->add( NONE_LABEL,  ADD_BOTTOM );
	combo->add( SHIFT_LABEL, ADD_BOTTOM );
	combo->add( CTRL_LABEL,  ADD_BOTTOM );
	combo->setCurrentByIndex(0);
}

void LLPreviewGesture::addKeys()
{
	LLComboBox* combo = mKeyCombo;

	combo->add( NONE_LABEL );
	for (KEY key = KEY_F2; key <= KEY_F12; key++)
	{
		combo->add( LLKeyboard::stringFromKey(key), ADD_BOTTOM );
	}
	combo->setCurrentByIndex(0);
}


// TODO: Sort the legacy and non-legacy together?
void LLPreviewGesture::addAnimations()
{
	LLComboBox* combo = mAnimationCombo;

	combo->removeall();
	
	std::string none_text = getString("none_text");

	combo->add(none_text, LLUUID::null);

	// Add all the default (legacy) animations
	S32 i;
	for (i = 0; i < gUserAnimStatesCount; ++i)
	{
		// Use the user-readable name
		std::string label = LLAnimStateLabels::getStateLabel( gUserAnimStates[i].mName );
		const LLUUID& id = gUserAnimStates[i].mID;
		combo->add(label, id);
	}

	// Get all inventory items that are animations
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLIsTypeWithPermissions is_copyable_animation(LLAssetType::AT_ANIMATION,
													PERM_ITEM_UNRESTRICTED,
													gAgent.getID(),
													gAgent.getGroupID());
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_copyable_animation);

	// Copy into something we can sort
	std::vector<LLInventoryItem*> animations;

	S32 count = items.size();
	for(i = 0; i < count; ++i)
	{
		animations.push_back( items.at(i) );
	}

	// Do the sort
	std::sort(animations.begin(), animations.end(), SortItemPtrsByName());

	// And load up the combobox
	std::vector<LLInventoryItem*>::iterator it;
	for (it = animations.begin(); it != animations.end(); ++it)
	{
		LLInventoryItem* item = *it;

		combo->add(item->getName(), item->getAssetUUID(), ADD_BOTTOM);
	}
}


void LLPreviewGesture::addSounds()
{
	LLComboBox* combo = mSoundCombo;
	combo->removeall();
	
	std::string none_text = getString("none_text");

	combo->add(none_text, LLUUID::null);

	// Get all inventory items that are sounds
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLIsTypeWithPermissions is_copyable_sound(LLAssetType::AT_SOUND,
													PERM_ITEM_UNRESTRICTED,
													gAgent.getID(),
													gAgent.getGroupID());
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_copyable_sound);

	// Copy sounds into something we can sort
	std::vector<LLInventoryItem*> sounds;

	S32 i;
	S32 count = items.size();
	for(i = 0; i < count; ++i)
	{
		sounds.push_back( items.at(i) );
	}

	// Do the sort
	std::sort(sounds.begin(), sounds.end(), SortItemPtrsByName());

	// And load up the combobox
	std::vector<LLInventoryItem*>::iterator it;
	for (it = sounds.begin(); it != sounds.end(); ++it)
	{
		LLInventoryItem* item = *it;

		combo->add(item->getName(), item->getAssetUUID(), ADD_BOTTOM);
	}
}


void LLPreviewGesture::refresh()
{
	LLPreview::refresh();
	// If previewing or item is incomplete, all controls are disabled
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)getItem();
	bool is_complete = (item && item->isFinished()) ? true : false;
	if (mPreviewGesture || !is_complete)
	{
		
		getChildView("desc")->setEnabled(false);
		//mDescEditor->setEnabled(false);
		mTriggerEditor->setEnabled(false);
		mReplaceText->setEnabled(false);
		mReplaceEditor->setEnabled(false);
		mModifierCombo->setEnabled(false);
		mKeyCombo->setEnabled(false);
		mLibraryList->setEnabled(false);
		mAddBtn->setEnabled(false);
		mUpBtn->setEnabled(false);
		mDownBtn->setEnabled(false);
		mDeleteBtn->setEnabled(false);
		mStepList->setEnabled(false);
		mOptionsText->setEnabled(false);
		mAnimationCombo->setEnabled(false);
		mAnimationRadio->setEnabled(false);
		mSoundCombo->setEnabled(false);
		mChatEditor->setEnabled(false);
		mWaitAnimCheck->setEnabled(false);
		mWaitTimeCheck->setEnabled(false);
		mWaitTimeEditor->setEnabled(false);
		mActiveCheck->setEnabled(false);
		mSaveBtn->setEnabled(false);

		// Make sure preview button is enabled, so we can stop it
		mPreviewBtn->setEnabled(true);
		return;
	}

	bool modifiable = item->getPermissions().allowModifyBy(gAgent.getID());

	getChildView("desc")->setEnabled(modifiable);
	mTriggerEditor->setEnabled(true);
	mLibraryList->setEnabled(modifiable);
	mStepList->setEnabled(modifiable);
	mOptionsText->setEnabled(modifiable);
	mAnimationCombo->setEnabled(modifiable);
	mAnimationRadio->setEnabled(modifiable);
	mSoundCombo->setEnabled(modifiable);
	mChatEditor->setEnabled(modifiable);
	mWaitAnimCheck->setEnabled(modifiable);
	mWaitTimeCheck->setEnabled(modifiable);
	mWaitTimeEditor->setEnabled(modifiable);
	mActiveCheck->setEnabled(true);

	const std::string& trigger = mTriggerEditor->getText();
	bool have_trigger = !trigger.empty();

	const std::string& replace = mReplaceEditor->getText();
	bool have_replace = !replace.empty();

	LLScrollListItem* library_item = mLibraryList->getFirstSelected();
	bool have_library = (library_item != NULL);

	LLScrollListItem* step_item = mStepList->getFirstSelected();
	S32 step_index = mStepList->getFirstSelectedIndex();
	S32 step_count = mStepList->getItemCount();
	bool have_step = (step_item != NULL);

	mReplaceText->setEnabled(have_trigger || have_replace);
	mReplaceEditor->setEnabled(have_trigger || have_replace);

	mModifierCombo->setEnabled(true);
	mKeyCombo->setEnabled(true);

	mAddBtn->setEnabled(modifiable && have_library);
	mUpBtn->setEnabled(modifiable && have_step && step_index > 0);
	mDownBtn->setEnabled(modifiable && have_step && step_index < step_count-1);
	mDeleteBtn->setEnabled(modifiable && have_step);

	// Assume all not visible
	mAnimationCombo->setVisible(false);
	mAnimationRadio->setVisible(false);
	mSoundCombo->setVisible(false);
	mChatEditor->setVisible(false);
	mWaitAnimCheck->setVisible(false);
	mWaitTimeCheck->setVisible(false);
	mWaitTimeEditor->setVisible(false);

	std::string optionstext;
	
	if (have_step)
	{
		// figure out the type, show proper options, update text
		LLGestureStep* step = (LLGestureStep*)step_item->getUserdata();
		EStepType type = step->getType();

		switch(type)
		{
		case STEP_ANIMATION:
			{
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				optionstext = getString("step_anim");
				mAnimationCombo->setVisible(true);
				mAnimationRadio->setVisible(true);
				mAnimationRadio->setSelectedIndex((anim_step->mFlags & ANIM_FLAG_STOP) ? 1 : 0);
				mAnimationCombo->setCurrentByID(anim_step->mAnimAssetID);
				break;
			}
		case STEP_SOUND:
			{
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				optionstext = getString("step_sound");
				mSoundCombo->setVisible(true);
				mSoundCombo->setCurrentByID(sound_step->mSoundAssetID);
				break;
			}
		case STEP_CHAT:
			{
				LLGestureStepChat* chat_step = (LLGestureStepChat*)step;
				optionstext = getString("step_chat");
				mChatEditor->setVisible(true);
				mChatEditor->setText(chat_step->mChatText);
				break;
			}
		case STEP_WAIT:
			{
				LLGestureStepWait* wait_step = (LLGestureStepWait*)step;
				optionstext = getString("step_wait");
				mWaitAnimCheck->setVisible(true);
				mWaitAnimCheck->set(wait_step->mFlags & WAIT_FLAG_ALL_ANIM);
				mWaitTimeCheck->setVisible(true);
				mWaitTimeCheck->set(wait_step->mFlags & WAIT_FLAG_TIME);
				mWaitTimeEditor->setVisible(true);
				std::string buffer = llformat("%.1f", (double)wait_step->mWaitSeconds);
				mWaitTimeEditor->setText(buffer);
				break;
			}
		default:
			break;
		}
	}
	
	mOptionsText->setText(optionstext);

	bool active = LLGestureMgr::instance().isGestureActive(mItemUUID);
	mActiveCheck->set(active);

	// Can only preview if there are steps
	mPreviewBtn->setEnabled(step_count > 0);

	// And can only save if changes have been made
	mSaveBtn->setEnabled(mDirty);
	addAnimations();
	addSounds();
}


void LLPreviewGesture::initDefaultGesture()
{
	LLScrollListItem* item;
	item = addStep( STEP_ANIMATION );
	LLGestureStepAnimation* anim = (LLGestureStepAnimation*)item->getUserdata();
	anim->mAnimAssetID = ANIM_AGENT_HELLO;
	anim->mAnimName = LLTrans::getString("Wave");
	updateLabel(item);

	item = addStep( STEP_WAIT );
	LLGestureStepWait* wait = (LLGestureStepWait*)item->getUserdata();
	wait->mFlags = WAIT_FLAG_ALL_ANIM;
	updateLabel(item);

	item = addStep( STEP_CHAT );
	LLGestureStepChat* chat_step = (LLGestureStepChat*)item->getUserdata();
	chat_step->mChatText =  LLTrans::getString("HelloAvatar");
	updateLabel(item);

	// Start with item list selected
	mStepList->selectFirstItem();

	// this is *new* content, so we are dirty
	mDirty = true;
}


void LLPreviewGesture::loadAsset()
{
	const LLInventoryItem* item = getItem();
	if (!item) 
	{
		// Don't set asset status here; we may not have set the item id yet
		// (e.g. when this gets called initially)
		//mAssetStatus = PREVIEW_ASSET_ERROR;
		return;
	}

	LLUUID asset_id = item->getAssetUUID();
	if (asset_id.isNull())
	{
		// Freshly created gesture, don't need to load asset.
		// Blank gesture will be fine.
		initDefaultGesture();
		refresh();
		mAssetStatus = PREVIEW_ASSET_LOADED;
		return;
	}

	// TODO: Based on item->getPermissions().allow*
	// could enable/disable UI.

	// Copy the UUID, because the user might close the preview
	// window if the download gets stalled.
	LLUUID* item_idp = new LLUUID(mItemUUID);

	const bool high_priority = true;
	gAssetStorage->getAssetData(asset_id,
								LLAssetType::AT_GESTURE,
								onLoadComplete,
								(void**)item_idp,
								high_priority);
	mAssetStatus = PREVIEW_ASSET_LOADING;
}


// static
void LLPreviewGesture::onLoadComplete(const LLUUID& asset_uuid,
									  LLAssetType::EType type,
									  void* user_data, S32 status, LLExtStat ext_status)
{
	LLUUID* item_idp = (LLUUID*)user_data;

	LLPreviewGesture* self = LLFloaterReg::findTypedInstance<LLPreviewGesture>("preview_gesture", *item_idp);
	if (self)
	{
		if (0 == status)
		{
			LLFileSystem file(asset_uuid, type, LLFileSystem::READ);
			S32 size = file.getSize();

			std::vector<char> buffer(size+1);
			file.read((U8*)&buffer[0], size);
			buffer[size] = '\0';

			LLMultiGesture* gesture = new LLMultiGesture();

			LLDataPackerAsciiBuffer dp(&buffer[0], size+1);
			bool ok = gesture->deserialize(dp);

			if (ok)
			{
				// Everything has been successful.  Load up the UI.
				self->loadUIFromGesture(gesture);

				self->mStepList->selectFirstItem();

				self->mDirty = false;
				self->refresh();
				self->refreshFromItem(); // to update description and title
			}
			else
			{
				LL_WARNS() << "Unable to load gesture" << LL_ENDL;
			}

			delete gesture;
			gesture = NULL;

			self->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLDelayedGestureError::gestureMissing( *item_idp );
			}
			else
			{
				LLDelayedGestureError::gestureFailedToLoad( *item_idp );
			}

			LL_WARNS() << "Problem loading gesture: " << status << LL_ENDL;
			self->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}
	delete item_idp;
	item_idp = NULL;
}


void LLPreviewGesture::loadUIFromGesture(LLMultiGesture* gesture)
{
	/*LLInventoryItem* item = getItem();


	
	if (item)
	{
		LLLineEditor* descEditor = getChild<LLLineEditor>("desc");
		descEditor->setText(item->getDescription());
	}*/

	mTriggerEditor->setText(gesture->mTrigger);

	mReplaceEditor->setText(gesture->mReplaceText);

	switch (gesture->mMask)
	{
	default:
	  case MASK_NONE:
		mModifierCombo->setSimple( NONE_LABEL );
		break;
	  case MASK_SHIFT:
		mModifierCombo->setSimple( SHIFT_LABEL );
		break;
	  case MASK_CONTROL:
		mModifierCombo->setSimple( CTRL_LABEL );
		break;
	}

	mModifierCombo->setEnabledByValue(CTRL_LABEL, gesture->mKey != KEY_F10);

	mKeyCombo->setCurrentByIndex(0);
	if (gesture->mKey != KEY_NONE)
	{
		mKeyCombo->setSimple(LLKeyboard::stringFromKey(gesture->mKey));
	}

	mKeyCombo->setEnabledByValue(LLKeyboard::stringFromKey(KEY_F10), gesture->mMask != MASK_CONTROL);

	// Make UI steps for each gesture step
	S32 i;
	S32 count = gesture->mSteps.size();
	for (i = 0; i < count; ++i)
	{
		LLGestureStep* step = gesture->mSteps[i];

		LLGestureStep* new_step = NULL;
		
		switch(step->getType())
		{
		case STEP_ANIMATION:
			{
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				LLGestureStepAnimation* new_anim_step =
					new LLGestureStepAnimation(*anim_step);
				new_step = new_anim_step;
				break;
			}
		case STEP_SOUND:
			{
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				LLGestureStepSound* new_sound_step =
					new LLGestureStepSound(*sound_step);
				new_step = new_sound_step;
				break;
			}
		case STEP_CHAT:
			{
				LLGestureStepChat* chat_step = (LLGestureStepChat*)step;
				LLGestureStepChat* new_chat_step =
					new LLGestureStepChat(*chat_step);
				new_step = new_chat_step;
				break;
			}
		case STEP_WAIT:
			{
				LLGestureStepWait* wait_step = (LLGestureStepWait*)step;
				LLGestureStepWait* new_wait_step =
					new LLGestureStepWait(*wait_step);
				new_step = new_wait_step;
				break;
			}
		default:
			{
				break;
			}
		}

		if (!new_step) continue;

		// Create an enabled item with this step
		LLSD row;
		row["columns"][0]["value"] = getLabel( new_step->getLabel());
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		LLScrollListItem* item = mStepList->addElement(row);
		item->setUserdata(new_step);
	}
}

// Helpful structure so we can look up the inventory item
// after the save finishes.
struct LLSaveInfo
{
	LLSaveInfo(const LLUUID& item_id, const LLUUID& object_id, const std::string& desc,
				const LLTransactionID tid)
		: mItemUUID(item_id), mObjectUUID(object_id), mDesc(desc), mTransactionID(tid)
	{
	}

	LLUUID mItemUUID;
	LLUUID mObjectUUID;
	std::string mDesc;
	LLTransactionID mTransactionID;
};


void LLPreviewGesture::finishInventoryUpload(LLUUID itemId, LLUUID newAssetId)
{
    // If this gesture is active, then we need to update the in-memory
    // active map with the new pointer.				
    if (LLGestureMgr::instance().isGestureActive(itemId))
    {
        // Active gesture edited from menu.
        LLGestureMgr::instance().replaceGesture(itemId, newAssetId);
        gInventory.notifyObservers();
    }

    //gesture will have a new asset_id
    LLPreviewGesture* previewp = LLFloaterReg::findTypedInstance<LLPreviewGesture>("preview_gesture", LLSD(itemId));
    if (previewp)
    {
        previewp->onUpdateSucceeded();
    }
}


void LLPreviewGesture::saveIfNeeded()
{
	if (!gAssetStorage)
	{
		LL_WARNS() << "Can't save gesture, no asset storage system." << LL_ENDL;
		return;
	}

	if (!mDirty)
	{
		return;
	}

    // Copy the UI into a gesture
    LLMultiGesture* gesture = createGesture();

    // Serialize the gesture
    S32 maxSize = gesture->getMaxSerialSize();
    char* buffer = new char[maxSize];

    LLDataPackerAsciiBuffer dp(buffer, maxSize);

    bool ok = gesture->serialize(dp);

    if (dp.getCurrentSize() > 1000)
    {
        LLNotificationsUtil::add("GestureSaveFailedTooManySteps");

        delete gesture;
        gesture = NULL;
        return;
    }
    else if (!ok)
    {
        LLNotificationsUtil::add("GestureSaveFailedTryAgain");
        delete gesture;
        gesture = NULL;
        return;
    }

    LLAssetID assetId;
    LLPreview::onCommit();
    bool delayedUpload(false);

    LLViewerInventoryItem* item = (LLViewerInventoryItem*) getItem();
    if (item)
    {
        const LLViewerRegion* region = gAgent.getRegion();
        if (!region)
        {
            LL_WARNS() << "Not connected to a region, cannot save gesture." << LL_ENDL;
            return;
        }
        std::string agent_url = region->getCapability("UpdateGestureAgentInventory");
        std::string task_url = region->getCapability("UpdateGestureTaskInventory");

        if (!agent_url.empty() && !task_url.empty())
        {
            std::string url;
            LLResourceUploadInfo::ptr_t uploadInfo;

            if (mObjectUUID.isNull() && !agent_url.empty())
            {
                //need to disable the preview floater so item
                //isn't re-saved before new asset arrives
                //fake out refresh.
                item->setComplete(false);
                refresh();
                item->setComplete(true);

                uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(mItemUUID, LLAssetType::AT_GESTURE, buffer,
                    [](LLUUID itemId, LLUUID newAssetId, LLUUID, LLSD)
                    {
                        LLPreviewGesture::finishInventoryUpload(itemId, newAssetId);
                    },
                    nullptr);
                url = agent_url;
            }
            else if (!mObjectUUID.isNull() && !task_url.empty())
            {
                uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(mObjectUUID, mItemUUID, LLAssetType::AT_GESTURE, buffer, nullptr, nullptr);
                url = task_url;
            }

            if (!url.empty() && uploadInfo)
            {
                delayedUpload = true;

                LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
            }

        }
        else if (gAssetStorage)
        {
            // Every save gets a new UUID.  Yup.
            LLTransactionID tid;
            tid.generate();
            assetId = tid.makeAssetID(gAgent.getSecureSessionID());

            LLFileSystem file(assetId, LLAssetType::AT_GESTURE, LLFileSystem::APPEND);

            S32 size = dp.getCurrentSize();
            file.write((U8*)buffer, size);

            LLLineEditor* descEditor = getChild<LLLineEditor>("desc");
            LLSaveInfo* info = new LLSaveInfo(mItemUUID, mObjectUUID, descEditor->getText(), tid);
            gAssetStorage->storeAssetData(tid, LLAssetType::AT_GESTURE, onSaveComplete, info, false);
        }

    }

    // If this gesture is active, then we need to update the in-memory
    // active map with the new pointer.
    if (!delayedUpload && LLGestureMgr::instance().isGestureActive(mItemUUID))
    {
        // gesture manager now owns the pointer
        LLGestureMgr::instance().replaceGesture(mItemUUID, gesture, assetId);

        // replaceGesture may deactivate other gestures so let the
        // inventory know.
        gInventory.notifyObservers();
    }
    else
    {
        // we're done with this gesture
        delete gesture;
        gesture = NULL;
    }

    mDirty = false;
    // refresh will be called when callback
    // if triggered when delayedUpload
    if(!delayedUpload)
    {
        refresh();
    }

}


// TODO: This is very similar to LLPreviewNotecard::onSaveComplete.
// Could merge code.
// static
void LLPreviewGesture::onSaveComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLSaveInfo* info = (LLSaveInfo*)user_data;
	if (info && (status == 0))
	{
		if(info->mObjectUUID.isNull())
		{
			// Saving into user inventory
			LLViewerInventoryItem* item;
			item = (LLViewerInventoryItem*)gInventory.getItem(info->mItemUUID);
			if(item)
			{
				LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
				new_item->setDescription(info->mDesc);
				new_item->setTransactionID(info->mTransactionID);
				new_item->setAssetUUID(asset_uuid);
				new_item->updateServer(false);
				gInventory.updateItem(new_item);
				gInventory.notifyObservers();
			}
			else
			{
				LL_WARNS() << "Inventory item for gesture " << info->mItemUUID
						<< " is no longer in agent inventory." << LL_ENDL;
			}
		}
		else
		{
			// Saving into in-world object inventory
			LLViewerObject* object = gObjectList.findObject(info->mObjectUUID);
			LLViewerInventoryItem* item = NULL;
			if(object)
			{
				item = (LLViewerInventoryItem*)object->getInventoryObject(info->mItemUUID);
			}
			if(object && item)
			{
				item->setDescription(info->mDesc);
				item->setAssetUUID(asset_uuid);
				item->setTransactionID(info->mTransactionID);
				object->updateInventory(item, TASK_INVENTORY_ITEM_KEY, false);
				dialog_refresh_all();
			}
			else
			{
				LLNotificationsUtil::add("GestureSaveFailedObjectNotFound");
			}
		}

		// Find our window and close it if requested.
		LLPreviewGesture* previewp = LLFloaterReg::findTypedInstance<LLPreviewGesture>("preview_gesture", info->mItemUUID);
		if (previewp && previewp->mCloseAfterSave)
		{
			previewp->closeFloater();
		}
	}
	else
	{
		LL_WARNS() << "Problem saving gesture: " << status << LL_ENDL;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("GestureSaveFailedReason", args);
	}
	delete info;
	info = NULL;
}


LLMultiGesture* LLPreviewGesture::createGesture()
{
	LLMultiGesture* gesture = new LLMultiGesture();

	gesture->mTrigger = mTriggerEditor->getText();
	gesture->mReplaceText = mReplaceEditor->getText();

	const std::string& modifier = mModifierCombo->getSimple();
	if (modifier == CTRL_LABEL)
	{
		gesture->mMask = MASK_CONTROL;
	}
	else if (modifier == SHIFT_LABEL)
	{
		gesture->mMask = MASK_SHIFT;
	}
	else
	{
		gesture->mMask = MASK_NONE;
	}

	if (mKeyCombo->getCurrentIndex() == 0)
	{
		gesture->mKey = KEY_NONE;
	}
	else
	{
		const std::string& key_string = mKeyCombo->getSimple();
		LLKeyboard::keyFromString(key_string, &(gesture->mKey));
	}

	std::vector<LLScrollListItem*> data_list = mStepList->getAllData();
	std::vector<LLScrollListItem*>::iterator data_itor;
	for (data_itor = data_list.begin(); data_itor != data_list.end(); ++data_itor)
	{
		LLScrollListItem* item = *data_itor;
		LLGestureStep* step = (LLGestureStep*)item->getUserdata();

		switch(step->getType())
		{
		case STEP_ANIMATION:
			{
				// Copy UI-generated step into actual gesture step
				LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
				LLGestureStepAnimation* new_anim_step =
					new LLGestureStepAnimation(*anim_step);
				gesture->mSteps.push_back(new_anim_step);
				break;
			}
		case STEP_SOUND:
			{
				// Copy UI-generated step into actual gesture step
				LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
				LLGestureStepSound* new_sound_step =
					new LLGestureStepSound(*sound_step);
				gesture->mSteps.push_back(new_sound_step);
				break;
			}
		case STEP_CHAT:
			{
				// Copy UI-generated step into actual gesture step
				LLGestureStepChat* chat_step = (LLGestureStepChat*)step;
				LLGestureStepChat* new_chat_step =
					new LLGestureStepChat(*chat_step);
				gesture->mSteps.push_back(new_chat_step);
				break;
			}
		case STEP_WAIT:
			{
				// Copy UI-generated step into actual gesture step
				LLGestureStepWait* wait_step = (LLGestureStepWait*)step;
				LLGestureStepWait* new_wait_step =
					new LLGestureStepWait(*wait_step);
				gesture->mSteps.push_back(new_wait_step);
				break;
			}
		default:
			{
				break;
			}
		}
	}

	return gesture;
}


void LLPreviewGesture::onCommitKeyorModifier()
{
	// SL-14139: ctrl-F10 is currently used to access top menu,
	// so don't allow to bound gestures to this combination.

	mKeyCombo->setEnabledByValue(LLKeyboard::stringFromKey(KEY_F10), mModifierCombo->getSimple() != CTRL_LABEL);
	mModifierCombo->setEnabledByValue(CTRL_LABEL, mKeyCombo->getSimple() != LLKeyboard::stringFromKey(KEY_F10));
	mDirty = true;
	refresh();
}

// static
void LLPreviewGesture::updateLabel(LLScrollListItem* item)
{
	LLGestureStep* step = (LLGestureStep*)item->getUserdata();

	LLScrollListCell* cell = item->getColumn(0);
	LLScrollListText* text_cell = (LLScrollListText*)cell;
	std::string label = getLabel( step->getLabel());
	text_cell->setText(label);
}

// static
void LLPreviewGesture::onCommitSetDirty(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;
	self->mDirty = true;
	self->refresh();
}

// static
void LLPreviewGesture::onCommitLibrary(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* library_item = self->mLibraryList->getFirstSelected();
	if (library_item)
	{
		self->mStepList->deselectAllItems();
		self->refresh();
	}
}


// static
void LLPreviewGesture::onCommitStep(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* step_item = self->mStepList->getFirstSelected();
	if (!step_item) return;

	self->mLibraryList->deselectAllItems();
	self->refresh();
}


// static
void LLPreviewGesture::onCommitAnimation(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* step_item = self->mStepList->getFirstSelected();
	if (step_item)
	{
		LLGestureStep* step = (LLGestureStep*)step_item->getUserdata();
		if (step->getType() == STEP_ANIMATION)
		{
			// Assign the animation name
			LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
			if (self->mAnimationCombo->getCurrentIndex() == 0)
			{
				anim_step->mAnimName.clear();
				anim_step->mAnimAssetID.setNull();
			}
			else
			{
				anim_step->mAnimName = self->mAnimationCombo->getSimple();
				anim_step->mAnimAssetID = self->mAnimationCombo->getCurrentID();
			}
			//anim_step->mFlags = 0x0;

			// Update the UI label in the list
			updateLabel(step_item);

			self->mDirty = true;
			self->refresh();
		}
	}
}

// static
void LLPreviewGesture::onCommitAnimationTrigger(LLUICtrl* ctrl, void *data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* step_item = self->mStepList->getFirstSelected();
	if (step_item)
	{
		LLGestureStep* step = (LLGestureStep*)step_item->getUserdata();
		if (step->getType() == STEP_ANIMATION)
		{
			LLGestureStepAnimation* anim_step = (LLGestureStepAnimation*)step;
			if (self->mAnimationRadio->getSelectedIndex() == 0)
			{
				// start
				anim_step->mFlags &= ~ANIM_FLAG_STOP;
			}
			else
			{
				// stop
				anim_step->mFlags |= ANIM_FLAG_STOP;
			}
			// Update the UI label in the list
			updateLabel(step_item);

			self->mDirty = true;
			self->refresh();
		}
	}
}

// static
void LLPreviewGesture::onCommitSound(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* step_item = self->mStepList->getFirstSelected();
	if (step_item)
	{
		LLGestureStep* step = (LLGestureStep*)step_item->getUserdata();
		if (step->getType() == STEP_SOUND)
		{
			// Assign the sound name
			LLGestureStepSound* sound_step = (LLGestureStepSound*)step;
			sound_step->mSoundName = self->mSoundCombo->getSimple();
			sound_step->mSoundAssetID = self->mSoundCombo->getCurrentID();
			sound_step->mFlags = 0x0;

			// Update the UI label in the list
			updateLabel(step_item);

			self->mDirty = true;
			self->refresh();
		}
	}
}

// static
void LLPreviewGesture::onCommitChat(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* step_item = self->mStepList->getFirstSelected();
	if (!step_item) return;

	LLGestureStep* step = (LLGestureStep*)step_item->getUserdata();
	if (step->getType() != STEP_CHAT) return;

	LLGestureStepChat* chat_step = (LLGestureStepChat*)step;
	chat_step->mChatText = self->mChatEditor->getText();
	chat_step->mFlags = 0x0;

	// Update the UI label in the list
	updateLabel(step_item);

	self->mDirty = true;
	self->refresh();
}

// static
void LLPreviewGesture::onCommitWait(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* step_item = self->mStepList->getFirstSelected();
	if (!step_item) return;

	LLGestureStep* step = (LLGestureStep*)step_item->getUserdata();
	if (step->getType() != STEP_WAIT) return;

	LLGestureStepWait* wait_step = (LLGestureStepWait*)step;
	U32 flags = 0x0;
	if (self->mWaitAnimCheck->get()) flags |= WAIT_FLAG_ALL_ANIM;
	if (self->mWaitTimeCheck->get()) flags |= WAIT_FLAG_TIME;
	wait_step->mFlags = flags;

	{
		LLLocale locale(LLLocale::USER_LOCALE);

		F32 wait_seconds = (F32)atof(self->mWaitTimeEditor->getText().c_str());
		if (wait_seconds < 0.f) wait_seconds = 0.f;
		if (wait_seconds > 3600.f) wait_seconds = 3600.f;
		wait_step->mWaitSeconds = wait_seconds;
	}

	// Enable the input area if necessary
	self->mWaitTimeEditor->setEnabled(self->mWaitTimeCheck->get());

	// Update the UI label in the list
	updateLabel(step_item);

	self->mDirty = true;
	self->refresh();
}

// static
void LLPreviewGesture::onCommitWaitTime(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* step_item = self->mStepList->getFirstSelected();
	if (!step_item) return;

	LLGestureStep* step = (LLGestureStep*)step_item->getUserdata();
	if (step->getType() != STEP_WAIT) return;

	self->mWaitTimeCheck->set(true);
	onCommitWait(ctrl, data);
}


// static
void LLPreviewGesture::onKeystrokeCommit(LLLineEditor* caller,
										 void* data)
{
	// Just commit every keystroke
	onCommitSetDirty(caller, data);
}

// static
void LLPreviewGesture::onClickAdd(void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* library_item = self->mLibraryList->getFirstSelected();
	if (!library_item) return;

	S32 library_item_index = self->mLibraryList->getFirstSelectedIndex();

	const LLScrollListCell* library_cell = library_item->getColumn(0);
	const std::string& library_text = library_cell->getValue().asString();

	if( library_item_index >= STEP_EOF )
	{
		LL_ERRS() << "Unknown step type: " << library_text << LL_ENDL;
		return;
	}

	self->addStep( (EStepType)library_item_index );
	self->mDirty = true;
	self->refresh();
}

LLScrollListItem* LLPreviewGesture::addStep( const EStepType step_type )
{
	// Order of enum EStepType MUST match the library_list element in floater_preview_gesture.xml

	LLGestureStep* step = NULL;
	switch( step_type)
	{
		case STEP_ANIMATION:
			step = new LLGestureStepAnimation();

			break;
		case STEP_SOUND:
			step = new LLGestureStepSound();
			break;
		case STEP_CHAT:
			step = new LLGestureStepChat();	
			break;
		case STEP_WAIT:
			step = new LLGestureStepWait();			
			break;
		default:
			LL_ERRS() << "Unknown step type: " << (S32)step_type << LL_ENDL;
			return NULL;
	}


	// Create an enabled item with this step
	LLSD row;
	row["columns"][0]["value"] = getLabel(step->getLabel());
	row["columns"][0]["font"] = "SANSSERIF_SMALL";
	LLScrollListItem* step_item = mStepList->addElement(row);
	step_item->setUserdata(step);

	// And move selection to the list on the right
	mLibraryList->deselectAllItems();
	mStepList->deselectAllItems();

	step_item->setSelected(true);

	return step_item;
}

// static
std::string LLPreviewGesture::getLabel(std::vector<std::string> labels)
{
	std::vector<std::string> v_labels = labels ;
	std::string result("");
	
	if( v_labels.size() != 2)
	{
		return result;
	}
	
	if(v_labels[0]=="Chat")
	{
		result=LLTrans::getString("Chat Message");
	}
    else if(v_labels[0]=="Sound")	
	{
		result=LLTrans::getString("Sound");
	}
	else if(v_labels[0]=="Wait")
	{
		result=LLTrans::getString("Wait");
	}
	else if(v_labels[0]=="AnimFlagStop")
	{
		result=LLTrans::getString("AnimFlagStop");
	}
	else if(v_labels[0]=="AnimFlagStart")
	{
		result=LLTrans::getString("AnimFlagStart");
	}

	// lets localize action value
	std::string action = v_labels[1];
	if ("None" == action)
	{
		action = LLTrans::getString("GestureActionNone");
	}
	else if ("until animations are done" == action)
	{
		action = LLFloaterReg::getInstance("preview_gesture")->getChild<LLCheckBoxCtrl>("wait_anim_check")->getLabel();
	}
	result.append(action);
	return result;
	
}
// static
void LLPreviewGesture::onClickUp(void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	S32 selected_index = self->mStepList->getFirstSelectedIndex();
	if (selected_index > 0)
	{
		self->mStepList->swapWithPrevious(selected_index);
		self->mDirty = true;
		self->refresh();
	}
}

// static
void LLPreviewGesture::onClickDown(void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	S32 selected_index = self->mStepList->getFirstSelectedIndex();
	if (selected_index < 0) return;

	S32 count = self->mStepList->getItemCount();
	if (selected_index < count-1)
	{
		self->mStepList->swapWithNext(selected_index);
		self->mDirty = true;
		self->refresh();
	}
}

// static
void LLPreviewGesture::onClickDelete(void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	LLScrollListItem* item = self->mStepList->getFirstSelected();
	S32 selected_index = self->mStepList->getFirstSelectedIndex();
	if (item && selected_index >= 0)
	{
		LLGestureStep* step = (LLGestureStep*)item->getUserdata();
		delete step;
		step = NULL;

		self->mStepList->deleteSingleItem(selected_index);

		self->mDirty = true;
		self->refresh();
	}
}

// static
void LLPreviewGesture::onCommitActive(LLUICtrl* ctrl, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;
	if (!LLGestureMgr::instance().isGestureActive(self->mItemUUID))
	{
		LLGestureMgr::instance().activateGesture(self->mItemUUID);
	}
	else
	{
		LLGestureMgr::instance().deactivateGesture(self->mItemUUID);
	}

	// Make sure the (active) label in the inventory gets updated.
	LLViewerInventoryItem* item = gInventory.getItem(self->mItemUUID);
	if (item)
	{
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}

	self->refresh();
}

// static
void LLPreviewGesture::onClickSave(void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;
	self->saveIfNeeded();
}

// static
void LLPreviewGesture::onClickPreview(void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	if (!self->mPreviewGesture)
	{
		// make temporary gesture
		self->mPreviewGesture = self->createGesture();

		// add a callback
		self->mPreviewGesture->mDoneCallback = onDonePreview;
		self->mPreviewGesture->mCallbackData = self;

		// set the button title
		self->mPreviewBtn->setLabel(self->getString("stop_txt"));

		// play it, and delete when done
		LLGestureMgr::instance().playGesture(self->mPreviewGesture);

		self->refresh();
	}
	else
	{
		// Will call onDonePreview() below
		LLGestureMgr::instance().stopGesture(self->mPreviewGesture);

		self->refresh();
	}
}


// static
void LLPreviewGesture::onDonePreview(LLMultiGesture* gesture, void* data)
{
	LLPreviewGesture* self = (LLPreviewGesture*)data;

	self->mPreviewBtn->setLabel(self->getString("preview_txt"));

	delete self->mPreviewGesture;
	self->mPreviewGesture = NULL;

	self->refresh();
}
