/** 
 * @file llfloatergesture.cpp
 * @brief Read-only list of gestures from your inventory.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatergesture.h"

#include "lldir.h"
#include "llinventory.h"
#include "llmultigesture.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llpreviewgesture.h"
#include "llresizehandle.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llvieweruictrlfactory.h"
#include "llviewergesture.h"
#include "llviewerimagelist.h"
#include "llviewerinventory.h"
#include "llvoavatar.h"
#include "llviewercontrol.h"

// static
LLFloaterGesture* LLFloaterGesture::sInstance = NULL;
LLFloaterGestureObserver* LLFloaterGesture::sObserver = NULL;

BOOL item_name_precedes( LLInventoryItem* a, LLInventoryItem* b )
{
	return LLString::precedesDict( a->getName(), b->getName() );
}

class LLFloaterGestureObserver : public LLGestureManagerObserver
{
public:
	LLFloaterGestureObserver() {}
	virtual ~LLFloaterGestureObserver() {}
	virtual void changed() { LLFloaterGesture::refreshAll(); }
};

//---------------------------------------------------------------------------
// LLFloaterGesture
//---------------------------------------------------------------------------
LLFloaterGesture::LLFloaterGesture()
:	LLFloater("Gesture Floater")
{
	sInstance = this;

	sObserver = new LLFloaterGestureObserver;
	gGestureManager.addObserver(sObserver);
}

// virtual
LLFloaterGesture::~LLFloaterGesture()
{
	gGestureManager.removeObserver(sObserver);
	delete sObserver;
	sObserver = NULL;

	sInstance = NULL;

	// Custom saving rectangle, since load must be done
	// after postBuild.
	gSavedSettings.setRect("FloaterGestureRect", mRect);
}

// virtual
BOOL LLFloaterGesture::postBuild()
{
	LLString label;

	// Translate title
	label = getTitle();
	
	setTitle(label);

	childSetCommitCallback("gesture_list", onCommitList, this);
	childSetDoubleClickCallback("gesture_list", onClickPlay);

	childSetAction("inventory_btn", onClickInventory, this);

	childSetAction("edit_btn", onClickEdit, this);

	childSetAction("play_btn", onClickPlay, this);
	childSetAction("stop_btn", onClickPlay, this);

	childSetAction("new_gesture_btn", onClickNew, this);

	childSetVisible("play_btn", true);
	childSetVisible("stop_btn", false);
	setDefaultBtn("play_btn");

	return TRUE;
}


// static
void LLFloaterGesture::show()
{
	if (sInstance)
	{
		sInstance->open();		/*Flawfinder: ignore*/
		return;
	}

	LLFloaterGesture *self = new LLFloaterGesture();

	// Builds and adds to gFloaterView
	gUICtrlFactory->buildFloater(self, "floater_gesture.xml");

	// Fix up rectangle
	LLRect rect = gSavedSettings.getRect("FloaterGestureRect");
	self->reshape(rect.getWidth(), rect.getHeight());
	self->setRect(rect);

	self->buildGestureList();

	self->childSetFocus("gesture_list");

	LLCtrlListInterface *list = self->childGetListInterface("gesture_list");
	if (list) list->selectFirstItem();
	
	self->mSelectedID = LLUUID::null;

	// Update button labels
	onCommitList(NULL, self);
	self->open();	/*Flawfinder: ignore*/
}

// static
void LLFloaterGesture::toggleVisibility()
{
	if(sInstance && sInstance->getVisible())
	{
		sInstance->close();
	}
	else
	{
		show();
	}
}

// static
void LLFloaterGesture::refreshAll()
{
	if (sInstance)
	{
		sInstance->buildGestureList();

		LLCtrlListInterface *list = sInstance->childGetListInterface("gesture_list");
		if (!list) return;

		if (sInstance->mSelectedID.isNull())
		{
			list->selectFirstItem();
		}
		else
		{
			if (list->setCurrentByID(sInstance->mSelectedID))
			{
				LLCtrlScrollInterface *scroll = sInstance->childGetScrollInterface("gesture_list");
				if (scroll) scroll->scrollToShowSelected();
			}
			else
			{
				list->selectFirstItem();
			}
		}

		// Update button labels
		onCommitList(NULL, sInstance);
	}
}

void LLFloaterGesture::buildGestureList()
{
	LLCtrlListInterface *list = childGetListInterface("gesture_list");
	if (!list) return;

	list->operateOnAll(LLCtrlListInterface::OP_DELETE);

	LLGestureManager::item_map_t::iterator it;
	for (it = gGestureManager.mActive.begin(); it != gGestureManager.mActive.end(); ++it)
	{
		const LLUUID& item_id = (*it).first;
		LLMultiGesture* gesture = (*it).second;

		// Note: Can have NULL item if inventory hasn't arrived yet.
		std::string item_name = "Loading...";
		LLInventoryItem* item = gInventory.getItem(item_id);
		if (item)
		{
			item_name = item->getName();
		}

		LLString font_style = "NORMAL";
		// If gesture is playing, bold it

		LLSD element;
		element["id"] = item_id;

		if (gesture)
		{
			if (gesture->mPlaying)
			{
				font_style = "BOLD";
			}

			element["columns"][0]["column"] = "trigger";
			element["columns"][0]["value"] = gesture->mTrigger;
			element["columns"][0]["font"] = "SANSSERIF";
			element["columns"][0]["font-style"] = font_style;

			LLString key_string = LLKeyboard::stringFromKey(gesture->mKey);
			LLString buffer;

			{
				if (gesture->mKey == KEY_NONE)
				{
					buffer = "---";
					key_string = "~~~";		// alphabetize to end
				}
				else
				{
					if (gesture->mMask & MASK_CONTROL) buffer.append("Ctrl-");
					if (gesture->mMask & MASK_ALT) buffer.append("Alt-");
					if (gesture->mMask & MASK_SHIFT) buffer.append("Shift-");
					if ((gesture->mMask & (MASK_CONTROL|MASK_ALT|MASK_SHIFT)) &&
						(key_string[0] == '-' || key_string[0] == '='))
					{
						buffer.append(" ");
					}
					buffer.append(key_string);
				}
			}
			element["columns"][1]["column"] = "shortcut";
			element["columns"][1]["value"] = buffer;
			element["columns"][1]["font"] = "SANSSERIF";
			element["columns"][1]["font-style"] = font_style;

			// hidden column for sorting
			element["columns"][2]["column"] = "key";
			element["columns"][2]["value"] = key_string;
			element["columns"][2]["font"] = "SANSSERIF";
			element["columns"][2]["font-style"] = font_style;

			// Only add "playing" if we've got the name, less confusing. JC
			if (item && gesture->mPlaying)
			{
				item_name += " (Playing)";
			}
			element["columns"][3]["column"] = "name";
			element["columns"][3]["value"] = item_name;
			element["columns"][3]["font"] = "SANSSERIF";
			element["columns"][3]["font-style"] = font_style;
		}
		else
		{
			element["columns"][0]["column"] = "trigger";
			element["columns"][0]["value"] = "";
			element["columns"][0]["font"] = "SANSSERIF";
			element["columns"][0]["font-style"] = font_style;
			element["columns"][0]["column"] = "trigger";
			element["columns"][0]["value"] = "---";
			element["columns"][0]["font"] = "SANSSERIF";
			element["columns"][0]["font-style"] = font_style;
			element["columns"][2]["column"] = "key";
			element["columns"][2]["value"] = "~~~";
			element["columns"][2]["font"] = "SANSSERIF";
			element["columns"][2]["font-style"] = font_style;
			element["columns"][3]["column"] = "name";
			element["columns"][3]["value"] = item_name;
			element["columns"][3]["font"] = "SANSSERIF";
			element["columns"][3]["font-style"] = font_style;
		}
		list->addElement(element, ADD_BOTTOM);
	}
}

// static
void LLFloaterGesture::onClickInventory(void* data)
{
	LLFloaterGesture* self = (LLFloaterGesture*)data;

	LLCtrlListInterface *list = self->childGetListInterface("gesture_list");
	if (!list) return;
	const LLUUID& item_id = list->getCurrentID();

	LLInventoryView* inv = LLInventoryView::showAgentInventory();
	if (!inv) return;
	inv->getPanel()->setSelection(item_id, TRUE);
}

// static
void LLFloaterGesture::onClickPlay(void* data)
{
	LLFloaterGesture* self = (LLFloaterGesture*)data;

	LLCtrlListInterface *list = self->childGetListInterface("gesture_list");
	if (!list) return;
	const LLUUID& item_id = list->getCurrentID();

	if (gGestureManager.isGesturePlaying(item_id))
	{
		gGestureManager.stopGesture(item_id);
	}
	else
	{
		gGestureManager.playGesture(item_id);
	}
}

class GestureShowCallback : public LLInventoryCallback
{
public:
	GestureShowCallback(LLString &title)
	{
		mTitle = title;
	}
	~GestureShowCallback() {}
	void fire(const LLUUID &inv_item)
	{
		LLPreviewGesture::show(mTitle.c_str(), inv_item, LLUUID::null);
	}
private:
	LLString mTitle;
};

// static
void LLFloaterGesture::onClickNew(void* data)
{
	LLString title("Gesture: ");
	title.append("New Gesture");
	LLPointer<LLInventoryCallback> cb = new GestureShowCallback(title);
	create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
		LLUUID::null, LLTransactionID::tnull, "New Gesture", "", LLAssetType::AT_GESTURE,
		LLInventoryType::IT_GESTURE, NOT_WEARABLE, PERM_MOVE | PERM_TRANSFER, cb);
}


// static
void LLFloaterGesture::onClickEdit(void* data)
{
	LLFloaterGesture* self = (LLFloaterGesture*)data;

	LLCtrlListInterface *list = self->childGetListInterface("gesture_list");
	if (!list) return;
	const LLUUID& item_id = list->getCurrentID();

	LLInventoryItem* item = gInventory.getItem(item_id);
	if (!item) return;

	LLString title("Gesture: ");
	title.append(item->getName());

	LLPreviewGesture* previewp = LLPreviewGesture::show(title, item_id, LLUUID::null);
	if (!previewp->getHost())
	{
		previewp->setRect(gFloaterView->findNeighboringPosition(self, previewp));
	}
}

// static
void LLFloaterGesture::onCommitList(LLUICtrl* ctrl, void* data)
{
	LLFloaterGesture* self = (LLFloaterGesture*)data;

	const LLUUID& item_id = self->childGetValue("gesture_list").asUUID();

	self->mSelectedID = item_id;
	if (gGestureManager.isGesturePlaying(item_id))
	{
		self->childSetVisible("play_btn", false);
		self->childSetVisible("stop_btn", true);
	}
	else
	{
		self->childSetVisible("play_btn", true);
		self->childSetVisible("stop_btn", false);
	}
}
