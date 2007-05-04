/** 
 * @file llpreviewscript.cpp
 * @brief LLPreviewScript class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpreviewscript.h"

#include "llassetstorage.h"
#include "llassetuploadresponders.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldir.h"
#include "llinventorymodel.h"
#include "llkeyboard.h"
#include "lllineeditor.h"

#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llscrolllistctrl.h"
#include "llslider.h"
#include "lscript_rt_interface.h"
#include "lscript_export.h"
#include "lltextbox.h"
#include "lltooldraganddrop.h"
#include "llvfile.h"

#include "llagent.h"
#include "llnotify.h"
#include "llmenugl.h"
#include "roles_constants.h"
#include "llselectmgr.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llkeyboard.h"
#include "llscrollcontainer.h"
#include "llcheckboxctrl.h"
#include "llselectmgr.h"
#include "lltooldraganddrop.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llslider.h"
#include "viewer.h"
#include "lldir.h"
#include "llcombobox.h"
//#include "llfloaterchat.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llvieweruictrlfactory.h"
#include "llwebbrowserctrl.h"
#include "lluictrlfactory.h"

#include "viewer.h"

#include "llpanelinventory.h"


const char HELLO_LSL[] =
	"default\n"
	"{\n"
	"    state_entry()\n"
    "    {\n"
    "        llSay(0, \"Hello, Avatar!\");\n"
    "    }\n"
	"\n"
	"    touch_start(integer total_number)\n"
	"    {\n"
	"        llSay(0, \"Touched.\");\n"
	"    }\n"
	"}\n";
const char HELP_LSL[] = "lsl_guide.html";

const char DEFAULT_SCRIPT_NAME[] = "New Script";
const char DEFAULT_SCRIPT_DESC[] = "(No Description)";

const char ENABLED_RUNNING_CHECKBOX_LABEL[] = "Running";
const char DISABLED_RUNNING_CHECKBOX_LABEL[] = "Public Objects cannot run scripts";

// Description and header information

const S32 SCRIPT_BORDER = 4;
const S32 SCRIPT_PAD = 5;
const S32 SCRIPT_BUTTON_WIDTH = 128;
const S32 SCRIPT_BUTTON_HEIGHT = 24;	// HACK: Use BTN_HEIGHT where possible.
const S32 LINE_COLUMN_HEIGHT = 14;
const S32 BTN_PAD = 8;

const S32 SCRIPT_EDITOR_MIN_HEIGHT = 2 * SCROLLBAR_SIZE + 2 * LLPANEL_BORDER_WIDTH + 128;

const S32 SCRIPT_MIN_WIDTH = 
	2 * SCRIPT_BORDER + 
	2 * SCRIPT_BUTTON_WIDTH + 
	SCRIPT_PAD + RESIZE_HANDLE_WIDTH +
	SCRIPT_PAD;

const S32 SCRIPT_MIN_HEIGHT = 
	2 * SCRIPT_BORDER +
	3*(SCRIPT_BUTTON_HEIGHT + SCRIPT_PAD) +
	LINE_COLUMN_HEIGHT +
	SCRIPT_EDITOR_MIN_HEIGHT;

const S32 MAX_EXPORT_SIZE = 1000;

const S32 SCRIPT_SEARCH_WIDTH = 300;
const S32 SCRIPT_SEARCH_HEIGHT = 120;
const S32 SCRIPT_SEARCH_LABEL_WIDTH = 50;
const S32 SCRIPT_SEARCH_BUTTON_WIDTH = 80;
const S32 TEXT_EDIT_COLUMN_HEIGHT = 16;
const S32 MAX_HISTORY_COUNT = 10;
const F32 LIVE_HELP_REFRESH_TIME = 1.f;

/// ---------------------------------------------------------------------------
/// LLFloaterScriptSearch
/// ---------------------------------------------------------------------------
class LLFloaterScriptSearch : public LLFloater
{
public:
	LLFloaterScriptSearch(std::string title, LLRect rect, LLScriptEdCore* editor_core);
	~LLFloaterScriptSearch();

	static void show(LLScriptEdCore* editor_core);
	static void onBtnSearch(void* userdata);
	void handleBtnSearch();

	static void onBtnReplace(void* userdata);
	void handleBtnReplace();

	static void onBtnReplaceAll(void* userdata);
	void handleBtnReplaceAll();

	LLScriptEdCore* getEditorCore() { return mEditorCore; }
	static LLFloaterScriptSearch* getInstance() { return sInstance; }

	void open();		/*Flawfinder: ignore*/

private:

	LLScriptEdCore* mEditorCore;

	static LLFloaterScriptSearch*	sInstance;
};

LLFloaterScriptSearch* LLFloaterScriptSearch::sInstance = NULL;

LLFloaterScriptSearch::LLFloaterScriptSearch(std::string title, LLRect rect, LLScriptEdCore* editor_core)
		: LLFloater("script search",rect,title), mEditorCore(editor_core)
{
	
	gUICtrlFactory->buildFloater(this,"floater_script_search.xml");

	childSetAction("search_btn", onBtnSearch,this);
	childSetAction("replace_btn", onBtnReplace,this);
	childSetAction("replace_all_btn", onBtnReplaceAll,this);
	
	setDefaultBtn("search_btn");

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}
	
	sInstance = this;

	childSetFocus("search_text", TRUE);

	// find floater in which script panel is embedded
	LLView* viewp = (LLView*)editor_core;
	while(viewp)
	{
		if (viewp->getWidgetType() == WIDGET_TYPE_FLOATER)
		{
			((LLFloater*)viewp)->addDependentFloater(this);
			break;
		}
		viewp = viewp->getParent();
	}
}

//static 
void LLFloaterScriptSearch::show(LLScriptEdCore* editor_core)
{
	if (sInstance && sInstance->mEditorCore && sInstance->mEditorCore != editor_core)
	{
		sInstance->close();
		delete sInstance;
	}

	if (!sInstance)
	{
		S32 left = 0;
		S32 top = 0;
		gFloaterView->getNewFloaterPosition(&left,&top);

		// sInstance will be assigned in the constructor.
		new LLFloaterScriptSearch("Script Search",LLRect(left,top,left + SCRIPT_SEARCH_WIDTH,top - SCRIPT_SEARCH_HEIGHT),editor_core);
	}

	sInstance->open();		/*Flawfinder: ignore*/
}

LLFloaterScriptSearch::~LLFloaterScriptSearch()
{
	sInstance = NULL;
}

// static 
void LLFloaterScriptSearch::onBtnSearch(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnSearch();
}

void LLFloaterScriptSearch::handleBtnSearch()
{
	LLCheckBoxCtrl* caseChk = LLUICtrlFactory::getCheckBoxByName(this,"case_text");
	mEditorCore->mEditor->selectNext(childGetText("search_text"), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplace(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplace();
}

void LLFloaterScriptSearch::handleBtnReplace()
{
	LLCheckBoxCtrl* caseChk = LLUICtrlFactory::getCheckBoxByName(this,"case_text");
	mEditorCore->mEditor->replaceText(childGetText("search_text"), childGetText("replace_text"), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplaceAll(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplaceAll();
}

void LLFloaterScriptSearch::handleBtnReplaceAll()
{
	LLCheckBoxCtrl* caseChk = LLUICtrlFactory::getCheckBoxByName(this,"case_text");
	mEditorCore->mEditor->replaceTextAll(childGetText("search_text"), childGetText("replace_text"), caseChk->get());
}

void LLFloaterScriptSearch::open()		/*Flawfinder: ignore*/
{
	LLFloater::open();		/*Flawfinder: ignore*/
	childSetFocus("search_text", TRUE); 
}

/// ---------------------------------------------------------------------------
/// LLScriptEdCore
/// ---------------------------------------------------------------------------

LLScriptEdCore::LLScriptEdCore(
	const std::string& name,
	const LLRect& rect,
	const std::string& sample,
	const std::string& help,
	const LLViewHandle& floater_handle,
	void (*load_callback)(void*),
	void (*save_callback)(void*, BOOL),
	void* userdata,
	S32 bottom_pad)
	:
	LLPanel( "name", rect ),
	mSampleText(sample),
	mHelpFile ( help ),
	mEditor( NULL ),
	mLoadCallback( load_callback ),
	mSaveCallback( save_callback ),
	mUserdata( userdata ),
	mForceClose( FALSE ),
	mLastHelpToken(NULL),
	mLiveHelpHistorySize(0)
{
	setFollowsAll();
	setBorderVisible(FALSE);

	
	gUICtrlFactory->buildPanel(this, "floater_script_ed_panel.xml");
	
	mErrorList = LLUICtrlFactory::getScrollListByName(this, "lsl errors");

	mFunctions = LLUICtrlFactory::getComboBoxByName(this, "Insert...");
	
	childSetCommitCallback("Insert...", &LLScriptEdCore::onBtnInsertFunction, this);
	mFunctions->setLabel("Insert...");

	mEditor = LLViewerUICtrlFactory::getViewerTextEditorByName(this, "Script Editor");
	mEditor->setReadOnlyBgColor(gColors.getColor( "ScriptBgReadOnlyColor" ) );
	mEditor->setFollowsAll();
	mEditor->setHandleEditKeysDirectly(TRUE);
	mEditor->setEnabled(TRUE);
	mEditor->setWordWrap(TRUE);

	LLDynamicArray<const char*> funcs;
	LLDynamicArray<const char*> tooltips;
	for (S32 i = 0; i < gScriptLibrary.mNextNumber; i++)
	{
		// Make sure this isn't a god only function, or the agent is a god.
		if (!gScriptLibrary.mFunctions[i]->mGodOnly || gAgent.isGodlike())
		{
			funcs.put(gScriptLibrary.mFunctions[i]->mName);
			tooltips.put(gScriptLibrary.mFunctions[i]->mDesc);
		}
	}
	LLColor3 color(0.5f, 0.0f, 0.15f);
		
	mEditor->loadKeywords(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"keywords.ini"), funcs, tooltips, color);

	
	LLKeywordToken *token;
	LLKeywords::word_token_map_t::iterator token_it;
	for (token_it = mEditor->mKeywords.mWordTokenMap.begin(); token_it != mEditor->mKeywords.mWordTokenMap.end(); ++token_it)
	{
		token = token_it->second;
		if (token->mColor == color)
			mFunctions->add(wstring_to_utf8str(token->mToken));
	}

	for (token_it = mEditor->mKeywords.mWordTokenMap.begin(); token_it != mEditor->mKeywords.mWordTokenMap.end(); ++token_it)
	{
		token = token_it->second;
		if (token->mColor != color)
			mFunctions->add(wstring_to_utf8str(token->mToken));
	}


	childSetCommitCallback("lsl errors", &LLScriptEdCore::onErrorList, this);
	childSetAction("Save_btn", onBtnSave,this);

	initMenu();
		
	// Do the work that addTabPanel() normally does.
	//LLRect tab_panel_rect( 0, mRect.getHeight(), mRect.getWidth(), 0 );
	//tab_panel_rect.stretch( -LLPANEL_BORDER_WIDTH );
	//mCodePanel->setFollowsAll();
	//mCodePanel->translate( tab_panel_rect.mLeft - mCodePanel->getRect().mLeft, tab_panel_rect.mBottom - mCodePanel->getRect().mBottom);
	//mCodePanel->reshape( tab_panel_rect.getWidth(), tab_panel_rect.getHeight(), TRUE );
	
}

LLScriptEdCore::~LLScriptEdCore()
{
	deleteBridges();

	// If the search window is up for this editor, close it.
	LLFloaterScriptSearch* script_search = LLFloaterScriptSearch::getInstance();
	if (script_search && script_search->getEditorCore() == this)
	{
		script_search->close();
		delete script_search;
	}
}

void LLScriptEdCore::initMenu()
{

	LLMenuItemCallGL* menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Save");
	menuItem->setMenuCallback(onBtnSave, this);
	menuItem->setEnabledCallback(hasChanged);
	
	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Revert All Changes");
	menuItem->setMenuCallback(onBtnUndoChanges, this);
	menuItem->setEnabledCallback(hasChanged);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Undo");
	menuItem->setMenuCallback(onUndoMenu, this);
	menuItem->setEnabledCallback(enableUndoMenu);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Redo");
	menuItem->setMenuCallback(onRedoMenu, this);
	menuItem->setEnabledCallback(enableRedoMenu);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Cut");
	menuItem->setMenuCallback(onCutMenu, this);
	menuItem->setEnabledCallback(enableCutMenu);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Copy");
	menuItem->setMenuCallback(onCopyMenu, this);
	menuItem->setEnabledCallback(enableCopyMenu);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Paste");
	menuItem->setMenuCallback(onPasteMenu, this);
	menuItem->setEnabledCallback(enablePasteMenu);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Select All");
	menuItem->setMenuCallback(onSelectAllMenu, this);
	menuItem->setEnabledCallback(enableSelectAllMenu);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Search / Replace...");
	menuItem->setMenuCallback(onSearchMenu, this);
	menuItem->setEnabledCallback(NULL);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "Help...");
	menuItem->setMenuCallback(onBtnHelp, this);
	menuItem->setEnabledCallback(NULL);

	menuItem = LLUICtrlFactory::getMenuItemCallByName(this, "LSL Wiki Help...");
	menuItem->setMenuCallback(onBtnDynamicHelp, this);
	menuItem->setEnabledCallback(NULL);
}

BOOL LLScriptEdCore::hasChanged(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;

	return !self->mEditor->isPristine();
}

void LLScriptEdCore::draw()
{
	BOOL script_changed = !mEditor->isPristine();
	childSetEnabled("Save_btn", script_changed);

	if( mEditor->hasFocus() )
	{
		S32 line = 0;
		S32 column = 0;
		mEditor->getCurrentLineAndColumn( &line, &column, FALSE );  // don't include wordwrap
		char cursor_pos[STD_STRING_BUF_SIZE];		/*Flawfinder: ignore*/
		snprintf( cursor_pos, STD_STRING_BUF_SIZE, "Line %d, Column %d", line, column );			/* Flawfinder: ignore */
		childSetText("line_col", cursor_pos);
	}
	else
	{
		childSetText("line_col", "");
	}

	updateDynamicHelp();

	LLPanel::draw();
}

void LLScriptEdCore::updateDynamicHelp(BOOL immediate)
{
	LLFloater* help_floater = LLFloater::getFloaterByHandle(mLiveHelpHandle);
	if (!help_floater) return;

#if LL_LIBXUL_ENABLED
	// update back and forward buttons
	LLButton* fwd_button = LLUICtrlFactory::getButtonByName(help_floater, "fwd_btn");
	LLButton* back_button = LLUICtrlFactory::getButtonByName(help_floater, "back_btn");
	LLWebBrowserCtrl* browser = LLUICtrlFactory::getWebBrowserCtrlByName(help_floater, "lsl_guide_html");
	back_button->setEnabled(browser->canNavigateBack());
	fwd_button->setEnabled(browser->canNavigateForward());
#endif // LL_LIBXUL_ENABLED

	if (!immediate && !gSavedSettings.getBOOL("ScriptHelpFollowsCursor"))
	{
		return;
	}

	LLTextSegment* segment = NULL;
	std::vector<LLTextSegment*> selected_segments;
	mEditor->getSelectedSegments(selected_segments);

	// try segments in selection range first
	std::vector<LLTextSegment*>::iterator segment_iter;
	for (segment_iter = selected_segments.begin(); segment_iter != selected_segments.end(); ++segment_iter)
	{
		if((*segment_iter)->getToken() && (*segment_iter)->getToken()->getType() == LLKeywordToken::WORD)
		{
			segment = *segment_iter;
			break;
		}
	}

	// then try previous segment in case we just typed it
	if (!segment)
	{
		LLTextSegment* test_segment = mEditor->getPreviousSegment();
		if(test_segment->getToken() && test_segment->getToken()->getType() == LLKeywordToken::WORD)
		{
			segment = test_segment;
		}
	}

	if (segment)
	{
		if (segment->getToken() != mLastHelpToken)
		{
			mLastHelpToken = segment->getToken();
			mLiveHelpTimer.start();
		}
		if (immediate || (mLiveHelpTimer.getStarted() && mLiveHelpTimer.getElapsedTimeF32() > LIVE_HELP_REFRESH_TIME))
		{
			LLString help_string = mEditor->getText().substr(segment->getStart(), segment->getEnd() - segment->getStart());
			setHelpPage(help_string);
			mLiveHelpTimer.stop();
		}
	}
	else if (immediate)
	{
		setHelpPage("");
	}
}

void LLScriptEdCore::setHelpPage(const LLString& help_string)
{
	LLFloater* help_floater = LLFloater::getFloaterByHandle(mLiveHelpHandle);
	if (!help_floater) return;
	
	LLWebBrowserCtrl* web_browser = gUICtrlFactory->getWebBrowserCtrlByName(help_floater, "lsl_guide_html");
	if (!web_browser) return;

	LLComboBox* history_combo = gUICtrlFactory->getComboBoxByName(help_floater, "history_combo");
	if (!history_combo) return;

	LLUIString url_string = gSavedSettings.getString("LSLHelpURL");
	url_string.setArg("[APP_DIRECTORY]", gDirUtilp->getWorkingDir());
	url_string.setArg("[LSL_STRING]", help_string);

	addHelpItemToHistory(help_string);
#if LL_LIBXUL_ENABLED
	web_browser->navigateTo(url_string);
#endif // LL_LIBXUL_ENABLED
}

void LLScriptEdCore::addHelpItemToHistory(const LLString& help_string)
{
	if (help_string.empty()) return;

	LLFloater* help_floater = LLFloater::getFloaterByHandle(mLiveHelpHandle);
	if (!help_floater) return;

	LLComboBox* history_combo = gUICtrlFactory->getComboBoxByName(help_floater, "history_combo");
	if (!history_combo) return;

	// separate history items from full item list
	if (mLiveHelpHistorySize == 0)
	{
		LLSD row;
		row["columns"][0]["type"] = "separator";
		history_combo->addElement(row, ADD_TOP);
	}
	// delete all history items over history limit
	while(mLiveHelpHistorySize > MAX_HISTORY_COUNT - 1)
	{
		history_combo->remove(mLiveHelpHistorySize - 1);
		mLiveHelpHistorySize--;
	}

	history_combo->setSimple(help_string);
	S32 index = history_combo->getCurrentIndex();

	// if help string exists in the combo box
	if (index >= 0)
	{
		S32 cur_index = history_combo->getCurrentIndex();
		if (cur_index < mLiveHelpHistorySize)
		{
			// item found in history, bubble up to top
			history_combo->remove(history_combo->getCurrentIndex());
			mLiveHelpHistorySize--;
		}
	}
	history_combo->add(help_string, LLSD(help_string), ADD_TOP);
	history_combo->selectFirstItem();
	mLiveHelpHistorySize++;
}

BOOL LLScriptEdCore::canClose()
{
	if(mForceClose || mEditor->isPristine())
	{
		return TRUE;
	}
	else
	{
		// Bring up view-modal dialog: Save changes? Yes, No, Cancel
		gViewerWindow->alertXml("SaveChanges", LLScriptEdCore::handleSaveChangesDialog, this);
		return FALSE;
	}
}

// static
void LLScriptEdCore::handleSaveChangesDialog( S32 option, void* userdata )
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;
	switch( option )
	{
	case 0:  // "Yes"
		// close after saving
		LLScriptEdCore::doSave( self, TRUE );
		break;

	case 1:  // "No"
		self->mForceClose = TRUE;
		// This will close immediately because mForceClose is true, so we won't
		// infinite loop with these dialogs. JC
		((LLFloater*) self->getParent())->close();
		break;

	case 2: // "Cancel"
	default:
		// If we were quitting, we didn't really mean it.
		app_abort_quit();
		break;
	}
}

// static 
void LLScriptEdCore::onHelpWebDialog(S32 option, void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;

	switch(option)
	{
	case 0:
		load_url_local_file(corep->mHelpFile.c_str());
		break;
	default:
		break;
	}
}

// static 
void LLScriptEdCore::onBtnHelp(void* userdata)
{
		gViewerWindow->alertXml("WebLaunchLSLGuide",
			onHelpWebDialog,
			userdata);
}

// static 
void LLScriptEdCore::onBtnDynamicHelp(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;

	LLFloater* live_help_floater = LLFloater::getFloaterByHandle(corep->mLiveHelpHandle);
	if (live_help_floater)
	{
		live_help_floater->setFocus(TRUE);
		corep->updateDynamicHelp(TRUE);

		return;
	}

	live_help_floater = new LLFloater("lsl_help");
	gUICtrlFactory->buildFloater(live_help_floater, "floater_lsl_guide.xml");
	((LLFloater*)corep->getParent())->addDependentFloater(live_help_floater, TRUE);
	live_help_floater->childSetCommitCallback("lock_check", onCheckLock, userdata);
	live_help_floater->childSetValue("lock_check", gSavedSettings.getBOOL("ScriptHelpFollowsCursor"));
	live_help_floater->childSetCommitCallback("history_combo", onHelpComboCommit, userdata);
	live_help_floater->childSetAction("back_btn", onClickBack, userdata);
	live_help_floater->childSetAction("fwd_btn", onClickForward, userdata);

#if LL_LIBXUL_ENABLED
	LLWebBrowserCtrl* browser = LLUICtrlFactory::getWebBrowserCtrlByName(live_help_floater, "lsl_guide_html");
	browser->setAlwaysRefresh(TRUE);
#endif // LL_LIBXUL_ENABLED

	LLComboBox* help_combo = LLUICtrlFactory::getComboBoxByName(live_help_floater, "history_combo");
	LLKeywordToken *token;
	LLKeywords::word_token_map_t::iterator token_it;
	for (token_it = corep->mEditor->mKeywords.mWordTokenMap.begin(); 
		token_it != corep->mEditor->mKeywords.mWordTokenMap.end(); 
		++token_it)
	{
		token = token_it->second;
		help_combo->add(wstring_to_utf8str(token->mToken));
	}
	help_combo->sortByName();

	// re-initialize help variables
	corep->mLastHelpToken = NULL;
	corep->mLiveHelpHandle = live_help_floater->getHandle();
	corep->mLiveHelpHistorySize = 0;
	corep->updateDynamicHelp(TRUE);
}

//static 
void LLScriptEdCore::onClickBack(void* userdata)
{
#if LL_LIBXUL_ENABLED
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = LLFloater::getFloaterByHandle(corep->mLiveHelpHandle);
	if (live_help_floater)
	{
		LLWebBrowserCtrl* browserp = LLUICtrlFactory::getWebBrowserCtrlByName(live_help_floater, "lsl_guide_html");
		if (browserp)
		{
			browserp->navigateBack();
		}
	}
#endif // LL_LIBXUL_ENABLED
}

//static 
void LLScriptEdCore::onClickForward(void* userdata)
{
#if LL_LIBXUL_ENABLED
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = LLFloater::getFloaterByHandle(corep->mLiveHelpHandle);
	if (live_help_floater)
	{
		LLWebBrowserCtrl* browserp = LLUICtrlFactory::getWebBrowserCtrlByName(live_help_floater, "lsl_guide_html");
		if (browserp)
		{
			browserp->navigateForward();
		}
	}
#endif // LL_LIBXUL_ENABLED
}

// static
void LLScriptEdCore::onCheckLock(LLUICtrl* ctrl, void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;

	// clear out token any time we lock the frame, so we will refresh web page immediately when unlocked
	gSavedSettings.setBOOL("ScriptHelpFollowsCursor", ctrl->getValue().asBoolean());

	corep->mLastHelpToken = NULL;
}

// static 
void LLScriptEdCore::onBtnInsertSample(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;

	// Insert sample code
	self->mEditor->selectAll();
	self->mEditor->cut();
	self->mEditor->insertText(self->mSampleText);
}

// static 
void LLScriptEdCore::onHelpComboCommit(LLUICtrl* ctrl, void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;

	LLFloater* live_help_floater = LLFloater::getFloaterByHandle(corep->mLiveHelpHandle);
	if (live_help_floater)
	{
		LLString help_string = ctrl->getValue().asString();

		corep->addHelpItemToHistory(help_string);

#if LL_LIBXUL_ENABLED
		LLWebBrowserCtrl* web_browser = gUICtrlFactory->getWebBrowserCtrlByName(live_help_floater, "lsl_guide_html");
		LLUIString url_string = gSavedSettings.getString("LSLHelpURL");
		url_string.setArg("[APP_DIRECTORY]", gDirUtilp->getWorkingDir());
		url_string.setArg("[LSL_STRING]", help_string);
		web_browser->navigateTo(url_string);
#endif // LL_LIBXUL_ENABLED
	}
}

// static 
void LLScriptEdCore::onBtnInsertFunction(LLUICtrl *ui, void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;

	// Insert sample code
	if(self->mEditor->getEnabled())
	{
		self->mEditor->insertText(self->mFunctions->getSimple());
	}
	self->mEditor->setFocus(TRUE);
	self->setHelpPage(self->mFunctions->getSimple());
}

// static 
void LLScriptEdCore::doSave( void* userdata, BOOL close_after_save )
{
	if( gViewerStats )
	{
		gViewerStats->incStat( LLViewerStats::ST_LSL_SAVE_COUNT );
	}

	LLScriptEdCore* self = (LLScriptEdCore*) userdata;

	if( self->mSaveCallback )
	{
		self->mSaveCallback( self->mUserdata, close_after_save );
	}
}

// static
void LLScriptEdCore::onBtnSave(void* data)
{
	// do the save, but don't close afterwards
	doSave(data, FALSE);
}

// static
void LLScriptEdCore::onBtnUndoChanges( void* userdata )
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;
	if( !self->mEditor->tryToRevertToPristineState() )
	{
		gViewerWindow->alertXml("ScriptCannotUndo",
			 LLScriptEdCore::handleReloadFromServerDialog, self);
	}
}

void LLScriptEdCore::onSearchMenu(void* userdata)
{
	LLScriptEdCore* sec = (LLScriptEdCore*)userdata;
	LLFloaterScriptSearch::show(sec);
}

// static 
void LLScriptEdCore::onUndoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->undo();
}

// static 
void LLScriptEdCore::onRedoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->redo();
}

// static 
void LLScriptEdCore::onCutMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->cut();
}

// static 
void LLScriptEdCore::onCopyMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->copy();
}

// static 
void LLScriptEdCore::onPasteMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->paste();
}

// static 
void LLScriptEdCore::onSelectAllMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->selectAll();
}

// static 
void LLScriptEdCore::onDeselectMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return;
	self->mEditor->deselect();
}

// static 
BOOL LLScriptEdCore::enableUndoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canUndo();
}

// static 
BOOL LLScriptEdCore::enableRedoMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canRedo();
}

// static 
BOOL LLScriptEdCore::enableCutMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canCut();
}

// static 
BOOL LLScriptEdCore::enableCopyMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canCopy();
}

// static 
BOOL LLScriptEdCore::enablePasteMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canPaste();
}

// static 
BOOL LLScriptEdCore::enableSelectAllMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canSelectAll();
}

// static 
BOOL LLScriptEdCore::enableDeselectMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canDeselect();
}

// static
void LLScriptEdCore::onErrorList(LLUICtrl*, void* user_data)
{
	LLScriptEdCore* self = (LLScriptEdCore*)user_data;
	LLScrollListItem* item = self->mErrorList->getFirstSelected();
	if(item)
	{
		// *FIX: This fucked up little hack is here because we don't
		// have a grep library. This is very brittle code.
		S32 row = 0;
		S32 column = 0;
		const LLScrollListCell* cell = item->getColumn(0);
		LLString line(cell->getText());
		line.erase(0, 1);
		LLString::replaceChar(line, ',',' ');
		LLString::replaceChar(line, ')',' ');
		sscanf(line.c_str(), "%d %d", &row, &column);
		//llinfos << "LLScriptEdCore::onErrorList() - " << row << ", "
		//<< column << llendl;
		self->mEditor->setCursor(row, column);
		self->mEditor->setFocus(TRUE);
	}
}

// static
void LLScriptEdCore::handleReloadFromServerDialog( S32 option, void* userdata )
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;
	switch( option )
	{
	case 0: // "Yes"
		if( self->mLoadCallback )
		{
			self->mEditor->setText( "Loading..." );
			self->mLoadCallback( self->mUserdata );
		}
		break;

	case 1: // "No"
		break;

	default:
		llassert(0);
		break;
	}
}

void LLScriptEdCore::selectFirstError()
{
	// Select the first item;
	mErrorList->selectFirstItem();
	onErrorList(mErrorList, this);
}


struct LLEntryAndEdCore
{
	LLScriptEdCore* mCore;
	LLEntryAndEdCore(LLScriptEdCore* core) :
		mCore(core)
		{}
};

void LLScriptEdCore::deleteBridges()
{
	S32 count = mBridges.count();
	LLEntryAndEdCore* eandc;
	for(S32 i = 0; i < count; i++)
	{
		eandc = mBridges.get(i);
		delete eandc;
		mBridges[i] = NULL;
	}
	mBridges.reset();
}

// virtual
BOOL LLScriptEdCore::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if(getVisible() && getEnabled())
	{
		if(('S' == key) && (MASK_CONTROL == (mask & MASK_CONTROL)))
		{
			if(mSaveCallback)
			{
				// don't close after saving
				mSaveCallback(mUserdata, FALSE);
			}
	
			return TRUE;
		}
	}
	return FALSE;
}

/// ---------------------------------------------------------------------------
/// LLPreviewLSL
/// ---------------------------------------------------------------------------

struct LLScriptSaveInfo
{
	LLUUID mItemUUID;
	LLString mDescription;
	LLTransactionID mTransactionID;

	LLScriptSaveInfo(const LLUUID& uuid, const LLString& desc, LLTransactionID tid) :
		mItemUUID(uuid), mDescription(desc),  mTransactionID(tid) {}
};



//static
void* LLPreviewLSL::createScriptEdPanel(void* userdata)
{
	
	LLPreviewLSL *self = (LLPreviewLSL*)userdata;

	self->mScriptEd =  new LLScriptEdCore("script panel",
								   LLRect(),
								   HELLO_LSL,
								   HELP_LSL,
								   self->mViewHandle,
								   LLPreviewLSL::onLoad,
								   LLPreviewLSL::onSave,
								   self,
								   0);

	return self->mScriptEd;
}


LLPreviewLSL::LLPreviewLSL(const std::string& name, const LLRect& rect,
						   const std::string& title, const LLUUID& item_id )
:	LLPreview( name, rect, title, item_id, LLUUID::null, TRUE,
			   SCRIPT_MIN_WIDTH, SCRIPT_MIN_HEIGHT ),
   mPendingUploads(0)
{

	LLRect curRect = rect;


	LLCallbackMap::map_t factory_map;
	factory_map["script panel"] = LLCallbackMap(LLPreviewLSL::createScriptEdPanel, this);


	gUICtrlFactory->buildFloater(this,"floater_script_preview.xml", &factory_map);

	moveResizeHandleToFront();

	
	LLInventoryItem* item = getItem();	

	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);

	LLMultiFloater* hostp = getHost();

	if (!sHostp && !hostp && getAssetStatus() == PREVIEW_ASSET_UNLOADED)
	{
		loadAsset();
	}
	
	setTitle(title);
	
	if (!getHost())
	{
		reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
		setRect(curRect);
	}
}

// virtual
void LLPreviewLSL::callbackLSLCompileSucceeded()
{
	llinfos << "LSL Bytecode saved" << llendl;
	mScriptEd->mErrorList->addSimpleItem("Compile successful!");
	mScriptEd->mErrorList->addSimpleItem("Save complete.");
	closeIfNeeded();
}

// virtual
void LLPreviewLSL::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	llinfos << "Compile failed!" << llendl;

	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		row["columns"][0]["value"] = line->asString();
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	closeIfNeeded();
}

void LLPreviewLSL::loadAsset()
{
	// *HACK: we poke into inventory to see if it's there, and if so,
	// then it might be part of the inventory library. If it's in the
	// library, then you can see the script, but not modify it.
	LLInventoryItem* item = gInventory.getItem(mItemUUID);
	BOOL is_library = item
		&& !gInventory.isObjectDescendentOf(mItemUUID,
											gAgent.getInventoryRootID());
	if(!item)
	{
		// do the more generic search.
		getItem();
	}
	if(item)
	{
		BOOL is_copyable = gAgent.allowOperation(PERM_COPY, 
								item->getPermissions(), GP_OBJECT_MANIPULATE);
		BOOL is_modifiable = gAgent.allowOperation(PERM_MODIFY,
								item->getPermissions(), GP_OBJECT_MANIPULATE);
		if (gAgent.isGodlike() || (is_copyable && (is_modifiable || is_library)))
		{
			LLUUID* new_uuid = new LLUUID(mItemUUID);
			gAssetStorage->getInvItemAsset(LLHost::invalid,
										gAgent.getID(),
										gAgent.getSessionID(),
										item->getPermissions().getOwner(),
										LLUUID::null,
										item->getUUID(),
										item->getAssetUUID(),
										item->getType(),
										&LLPreviewLSL::onLoadComplete,
										(void*)new_uuid,
										TRUE);
			mAssetStatus = PREVIEW_ASSET_LOADING;
		}
		else
		{
			mScriptEd->mEditor->setText("You are not allowed to view this script.");
			mScriptEd->mEditor->makePristine();
			mScriptEd->mEditor->setEnabled(FALSE);
			mScriptEd->mFunctions->setEnabled(FALSE);
			mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		childSetVisible("lock", !is_modifiable);
		mScriptEd->childSetEnabled("Insert...", is_modifiable);
	}
	else
	{
		mScriptEd->mEditor->setText(HELLO_LSL);
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
}


BOOL LLPreviewLSL::canClose()
{
	return mScriptEd->canClose();
}

void LLPreviewLSL::closeIfNeeded()
{
	// Find our window and close it if requested.
	getWindow()->decBusyCount();
	mPendingUploads--;
	if (mPendingUploads <= 0 && mCloseAfterSave)
	{
		close();
	}
}

//override the llpreview open which attempts to load asset, load after xml ui made
void LLPreviewLSL::open()		/*Flawfinder: ignore*/
{
	LLFloater::open();		/*Flawfinder: ignore*/
}

// static
void LLPreviewLSL::onLoad(void* userdata)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	self->loadAsset();
}

// static
void LLPreviewLSL::onSave(void* userdata, BOOL close_after_save)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	self->mCloseAfterSave = close_after_save;
	self->saveIfNeeded();
}

// Save needs to compile the text in the buffer. If the compile
// succeeds, then save both assets out to the database. If the compile
// fails, go ahead and save the text anyway so that the user doesn't
// get too fucked.
void LLPreviewLSL::saveIfNeeded()
{
	// llinfos << "LLPreviewLSL::saveIfNeeded()" << llendl;
	if(mScriptEd->mEditor->isPristine())
	{
		return;
	}

	mPendingUploads = 0;
	mScriptEd->mErrorList->deleteAllItems();
	mScriptEd->mEditor->makePristine();

	// save off asset into file
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = llformat("%s.lsl", filepath.c_str());

	FILE* fp = LLFile::fopen(filename.c_str(), "wb");
	if(!fp)
	{
		llwarns << "Unable to write to " << filename << llendl;

		LLSD row;
		row["columns"][0]["value"] = "Error writing to local file. Is your hard drive full?";
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		mScriptEd->mErrorList->addElement(row);
		return;
	}

	LLString utf8text = mScriptEd->mEditor->getText();
	fputs(utf8text.c_str(), fp);
	fclose(fp);
	fp = NULL;

	LLInventoryItem *inv_item = getItem();
	// save it out to asset server
	std::string url = gAgent.getRegion()->getCapability("UpdateScriptAgentInventory");
	if(inv_item)
	{
		getWindow()->incBusyCount();
		mPendingUploads++;
		if (!url.empty())
		{
			uploadAssetViaCaps(url, filename, mItemUUID);
		}
		else if (gAssetStorage)
		{
			uploadAssetLegacy(filename, mItemUUID, tid);
		}
	}
}

void LLPreviewLSL::uploadAssetViaCaps(const std::string& url,
									  const std::string& filename,
									  const LLUUID& item_id)
{
	llinfos << "Update Agent Inventory via capability" << llendl;
	LLSD body;
	body["item_id"] = item_id;
	LLHTTPClient::post(url, body, new LLUpdateAgentInventoryResponder(body, filename));
}

void LLPreviewLSL::uploadAssetLegacy(const std::string& filename,
									  const LLUUID& item_id,
									  const LLTransactionID& tid)
{
	LLLineEditor* descEditor = LLUICtrlFactory::getLineEditorByName(this, "desc");
	LLScriptSaveInfo* info = new LLScriptSaveInfo(item_id,
								descEditor->getText(),
								tid);
	gAssetStorage->storeAssetData(filename.c_str(),	tid,
								  LLAssetType::AT_LSL_TEXT,
								  &LLPreviewLSL::onSaveComplete,
								  info);

	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string dst_filename = llformat("%s.lso", filepath.c_str());
	std::string err_filename = llformat("%s.out", filepath.c_str());

	if(!lscript_compile(filename.c_str(),
						dst_filename.c_str(),
						err_filename.c_str(),
						gAgent.isGodlike()))
	{
		llinfos << "Compile failed!" << llendl;
		//char command[256];
		//sprintf(command, "type %s\n", err_filename);
		//system(command);

		// load the error file into the error scrolllist
		FILE* fp = LLFile::fopen(err_filename.c_str(), "r");
		if(fp)
		{
			char buffer[MAX_STRING];		/*Flawfinder: ignore*/
			LLString line;
			while(!feof(fp)) 
			{
				fgets(buffer, MAX_STRING, fp);
				if(feof(fp))
				{
					break;
				}
				else
				{
					line.assign(buffer);
					LLString::stripNonprintable(line);

					LLSD row;
					row["columns"][0]["value"] = line;
					row["columns"][0]["font"] = "OCRA";
					mScriptEd->mErrorList->addElement(row);
				}
			}
			fclose(fp);
			mScriptEd->selectFirstError();
		}
	}
	else
	{
		llinfos << "Compile worked!" << llendl;
		if(gAssetStorage)
		{
			getWindow()->incBusyCount();
			mPendingUploads++;
			LLUUID* this_uuid = new LLUUID(mItemUUID);
			gAssetStorage->storeAssetData(dst_filename.c_str(),
										  tid,
										  LLAssetType::AT_LSL_BYTECODE,
										  &LLPreviewLSL::onSaveBytecodeComplete,
										  (void**)this_uuid);
		}
	}

	// get rid of any temp files left lying around
	LLFile::remove(filename.c_str());
	LLFile::remove(err_filename.c_str());
	LLFile::remove(dst_filename.c_str());
}


// static
void LLPreviewLSL::onSaveComplete(const LLUUID& asset_uuid, void* user_data, S32 status) // StoreAssetData callback (fixed)
{
	LLScriptSaveInfo* info = reinterpret_cast<LLScriptSaveInfo*>(user_data);
	if(0 == status)
	{
		if (info)
		{
			LLViewerInventoryItem* item;
			item = (LLViewerInventoryItem*)gInventory.getItem(info->mItemUUID);
			if(item)
			{
				LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
				new_item->setAssetUUID(asset_uuid);
				new_item->setTransactionID(info->mTransactionID);
				new_item->updateServer(FALSE);
				gInventory.updateItem(new_item);
				gInventory.notifyObservers();
			}
			else
			{
				llwarns << "Inventory item for script " << info->mItemUUID
					<< " is no longer in agent inventory." << llendl
			}

			// Find our window and close it if requested.
			LLPreviewLSL* self = (LLPreviewLSL*)LLPreview::find(info->mItemUUID);
			if (self)
			{
				getWindow()->decBusyCount();
				self->mPendingUploads--;
				if (self->mPendingUploads <= 0
					&& self->mCloseAfterSave)
				{
					self->close();
				}
			}
		}
	}
	else
	{
		llwarns << "Problem saving script: " << status << llendl;
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("SaveScriptFailReason", args);
	}
	delete info;
}

// static
void LLPreviewLSL::onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status) // StoreAssetData callback (fixed)
{
	LLUUID* instance_uuid = (LLUUID*)user_data;
	LLPreviewLSL* self = NULL;
	if(instance_uuid)
	{
		self = LLPreviewLSL::getInstance(*instance_uuid);
	}
	if (0 == status)
	{
		if (self)
		{
			LLSD row;
			row["columns"][0]["value"] = "Compile successful!";
			row["columns"][0]["font"] = "SANSSERIF_SMALL";
			self->mScriptEd->mErrorList->addElement(row);

			// Find our window and close it if requested.
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->close();
			}
		}
	}
	else
	{
		llwarns << "Problem saving LSL Bytecode (Preview)" << llendl;
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("SaveBytecodeFailReason", args);
	}
	delete instance_uuid;
}

// static
void LLPreviewLSL::onLoadComplete( LLVFS *vfs, const LLUUID& asset_uuid, LLAssetType::EType type,
								   void* user_data, S32 status)
{
	lldebugs << "LLPreviewLSL::onLoadComplete: got uuid " << asset_uuid
		 << llendl;
	LLUUID* item_uuid = (LLUUID*)user_data;
	LLPreviewLSL* preview = LLPreviewLSL::getInstance(*item_uuid);
	if( preview )
	{
		if(0 == status)
		{
			LLVFile file(vfs, asset_uuid, type);
			S32 file_length = file.getSize();

			char* buffer = new char[file_length+1];
			file.read((U8*)buffer, file_length);		/*Flawfinder: ignore*/

			// put a EOS at the end
			buffer[file_length] = 0;
			preview->mScriptEd->mEditor->setText(buffer);
			preview->mScriptEd->mEditor->makePristine();
			delete [] buffer;
			LLInventoryItem* item = gInventory.getItem(*item_uuid);
			BOOL is_modifiable = FALSE;
			if(item
			   && gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
				   					GP_OBJECT_MANIPULATE))
			{
				is_modifiable = TRUE;		
			}
			preview->mScriptEd->mEditor->setEnabled(is_modifiable);
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			if( gViewerStats )
			{
				gViewerStats->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
			}

			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotifyBox::showXml("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotifyBox::showXml("ScriptNoPermissions");
			}
			else
			{
				LLNotifyBox::showXml("UnableToLoadScript");
			}

			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
			llwarns << "Problem loading script: " << status << llendl;
		}
	}
	delete item_uuid;
}

// static
LLPreviewLSL* LLPreviewLSL::getInstance( const LLUUID& item_uuid )
{
	LLPreview* instance = NULL;
	preview_map_t::iterator found_it = LLPreview::sInstances.find(item_uuid);
	if(found_it != LLPreview::sInstances.end())
	{
		instance = found_it->second;
	}
	return (LLPreviewLSL*)instance;
}

void LLPreviewLSL::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPreview::reshape( width, height, called_from_parent );

	if( !isMinimized() )
	{
		// So that next time you open a script it will have the same height and width 
		// (although not the same position).
		gSavedSettings.setRect("PreviewScriptRect", mRect);
	}
}

/// ---------------------------------------------------------------------------
/// LLLiveLSLEditor
/// ---------------------------------------------------------------------------

LLMap<LLUUID, LLLiveLSLEditor*> LLLiveLSLEditor::sInstances;



//static 
void* LLLiveLSLEditor::createScriptEdPanel(void* userdata)
{
	
	LLLiveLSLEditor *self = (LLLiveLSLEditor*)userdata;

	self->mScriptEd =  new LLScriptEdCore("script ed panel",
								   LLRect(),
								   HELLO_LSL,
								   HELP_LSL,
								   self->mViewHandle,
								   &LLLiveLSLEditor::onLoad,
								   &LLLiveLSLEditor::onSave,
								   self,
								   0);

	return self->mScriptEd;
}


LLLiveLSLEditor::LLLiveLSLEditor(const std::string& name,
								 const LLRect& rect,
								 const std::string& title,
								 const LLUUID& object_id,
								 const LLUUID& item_id) :
	LLPreview(name, rect, title, item_id, object_id, TRUE, SCRIPT_MIN_WIDTH, SCRIPT_MIN_HEIGHT),
	mObjectID(object_id),
	mItemID(item_id),
	mScriptEd(NULL),
	mAskedForRunningInfo(FALSE),
	mHaveRunningInfo(FALSE),
	mCloseAfterSave(FALSE),
	mPendingUploads(0)
{

	
	BOOL is_new = FALSE;
	if(mItemID.isNull())
	{
		mItemID.generate();
		is_new = TRUE;
	}


	LLLiveLSLEditor::sInstances.addData(mItemID ^ mObjectID, this);



	LLCallbackMap::map_t factory_map;
	factory_map["script ed panel"] = LLCallbackMap(LLLiveLSLEditor::createScriptEdPanel, this);

	moveResizeHandleToFront();

	gUICtrlFactory->buildFloater(this,"floater_live_lsleditor.xml", &factory_map);


	childSetCommitCallback("running", LLLiveLSLEditor::onRunningCheckboxClicked, this);
	childSetEnabled("running", FALSE);

	childSetAction("Reset",&LLLiveLSLEditor::onReset,this);
	childSetEnabled("Reset", TRUE);


	mScriptEd->mEditor->makePristine();
	loadAsset(is_new);
	mScriptEd->mEditor->setFocus(TRUE);

	
	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}

	
	setTitle(title);

}

LLLiveLSLEditor::~LLLiveLSLEditor()
{
	LLLiveLSLEditor::sInstances.removeData(mItemID ^ mObjectID);
}

// this is called via LLPreview::loadAsset() virtual method
void LLLiveLSLEditor::loadAsset()
{
	loadAsset(FALSE);
}

// virtual
void LLLiveLSLEditor::callbackLSLCompileSucceeded(const LLUUID& task_id,
												  const LLUUID& item_id,
												  bool is_script_running)
{
	lldebugs << "LSL Bytecode saved" << llendl;
	mScriptEd->mErrorList->addSimpleItem("Compile successful!");
	mScriptEd->mErrorList->addSimpleItem("Save complete.");
	closeIfNeeded();
}

// virtual
void LLLiveLSLEditor::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	lldebugs << "Compile failed!" << llendl;
	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		row["columns"][0]["value"] = line->asString();
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	closeIfNeeded();
}

void LLLiveLSLEditor::loadAsset(BOOL is_new)
{
	//llinfos << "LLLiveLSLEditor::loadAsset()" << llendl;
	if(!is_new)
	{
		LLViewerObject* object = gObjectList.findObject(mObjectID);
		if(object)
		{
			// HACK! we "know" that mItemID refers to a LLViewerInventoryItem...
			LLViewerInventoryItem* item = (LLViewerInventoryItem*)object->getInventoryObject(mItemID);
			if(item
			   && (gAgent.allowOperation(PERM_COPY, item->getPermissions(),
					   						GP_OBJECT_MANIPULATE)
				   || gAgent.isGodlike()))
			{
				mItem = new LLViewerInventoryItem(item);
				//llinfos << "asset id " << mItem->getAssetUUID() << llendl;
			}

			if(!gAgent.isGodlike()
			   && (item
				   && (!gAgent.allowOperation(PERM_COPY, item->getPermissions(),
						   					GP_OBJECT_MANIPULATE)
					   || !gAgent.allowOperation(PERM_MODIFY, 
						   			item->getPermissions(), GP_OBJECT_MANIPULATE))))
				   
			{
				mScriptEd->mEditor->setText("You are not allowed to view this script.");
				mScriptEd->mEditor->makePristine();
				mScriptEd->mEditor->setEnabled(FALSE);
				mAssetStatus = PREVIEW_ASSET_LOADED;
			}
			else if(item && mItem.notNull())
			{
				// request the text from the object
				LLUUID* user_data = new LLUUID(mItemID ^ mObjectID);
				gAssetStorage->getInvItemAsset(object->getRegion()->getHost(),
											gAgent.getID(),
											gAgent.getSessionID(),
											item->getPermissions().getOwner(),
											object->getID(),
											item->getUUID(),
											item->getAssetUUID(),
											item->getType(),
											&LLLiveLSLEditor::onLoadComplete,
											(void*)user_data,
											TRUE);
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_GetScriptRunning);
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, mObjectID);
				msg->addUUIDFast(_PREHASH_ItemID, mItemID);
				msg->sendReliable(object->getRegion()->getHost());
				mAskedForRunningInfo = TRUE;
				mAssetStatus = PREVIEW_ASSET_LOADING;
			}
			else
			{
				mScriptEd->mEditor->setText("");
				mScriptEd->mEditor->makePristine();
				mAssetStatus = PREVIEW_ASSET_LOADED;
			}

			if(item
			   && !gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
				   						GP_OBJECT_MANIPULATE))
			{
				mScriptEd->mEditor->setEnabled(FALSE);
			}
			// This is commented out, because we don't completely
			// handle script exports yet.
			/*
			// request the exports from the object
			gMessageSystem->newMessage("GetScriptExports");
			gMessageSystem->nextBlock("ScriptBlock");
			gMessageSystem->addUUID("AgentID", gAgent.getID());
			U32 local_id = object->getLocalID();
			gMessageSystem->addData("LocalID", &local_id);
			gMessageSystem->addUUID("ItemID", mItemID);
			LLHost host(object->getRegion()->getIP(),
						object->getRegion()->getPort());
			gMessageSystem->sendReliable(host);
			*/
		}
	}
	else
	{
		mScriptEd->mEditor->setText(HELLO_LSL);
		//mScriptEd->mEditor->setText("");
		//mScriptEd->mEditor->makePristine();
		LLPermissions perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, gAgent.getGroupID());
		perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, PERM_MOVE | PERM_TRANSFER);
		mItem = new LLViewerInventoryItem(mItemID,
									mObjectID,
									perm,
									LLUUID::null,
									LLAssetType::AT_LSL_TEXT,
									LLInventoryType::IT_LSL,
									DEFAULT_SCRIPT_NAME,
									DEFAULT_SCRIPT_DESC,
									LLSaleInfo::DEFAULT,
									LLInventoryItem::II_FLAGS_NONE,
									time_corrected());
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
}

// static
void LLLiveLSLEditor::onLoadComplete(LLVFS *vfs, const LLUUID& asset_id,
									 LLAssetType::EType type,
									 void* user_data, S32 status)
{
	lldebugs << "LLLiveLSLEditor::onLoadComplete: got uuid " << asset_id
		 << llendl;
	LLLiveLSLEditor* instance = NULL;
	LLUUID* xored_id = (LLUUID*)user_data;

	if( LLLiveLSLEditor::sInstances.checkData(*xored_id) )
	{
		instance = LLLiveLSLEditor::sInstances[*xored_id];
		if( LL_ERR_NOERR == status )
		{
			instance->loadScriptText(vfs, asset_id, type);
			instance->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			if( gViewerStats )
			{
				gViewerStats->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
			}

			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotifyBox::showXml("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotifyBox::showXml("ScriptNoPermissions");
			}
			else
			{
				LLNotifyBox::showXml("UnableToLoadScript");
			}
			instance->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}

	delete xored_id;
}

void LLLiveLSLEditor::loadScriptText(const char* filename)
{
	if(!filename)
	{
		llerrs << "Filename is Empty!" << llendl;
		return;
	}
	FILE* file = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
	if(file)
	{
		// read in the whole file
		fseek(file, 0L, SEEK_END);
		S32 file_length = ftell(file);
		fseek(file, 0L, SEEK_SET);
		char* buffer = new char[file_length+1];
		fread(buffer, file_length, 1, file);
		fclose(file);
		buffer[file_length] = 0;
		mScriptEd->mEditor->setText(buffer);
		mScriptEd->mEditor->makePristine();
		delete[] buffer;
	}
	else
	{
		llwarns << "Error opening " << filename << llendl;
	}
}

void LLLiveLSLEditor::loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type)
{
	LLVFile file(vfs, uuid, type);
	S32 file_length = file.getSize();
	char *buffer = new char[file_length + 1];
	file.read((U8*)buffer, file_length);		/*Flawfinder: ignore*/

	if (file.getLastBytesRead() != file_length ||
		file_length <= 0)
	{
		llwarns << "Error reading " << uuid << ":" << type << llendl;
	}

	buffer[file_length] = '\0';

	mScriptEd->mEditor->setText(buffer);
	mScriptEd->mEditor->makePristine();
	delete[] buffer;

}


void LLLiveLSLEditor::onRunningCheckboxClicked( LLUICtrl*, void* userdata )
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;
	LLViewerObject* object = gObjectList.findObject( self->mObjectID );
	LLCheckBoxCtrl* runningCheckbox = LLUICtrlFactory::getCheckBoxByName(self, "running");
	BOOL running =  runningCheckbox->get();
	//self->mRunningCheckbox->get();
	if( object )
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_SetScriptRunning);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Script);
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemID);
		msg->addBOOLFast(_PREHASH_Running, running);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		runningCheckbox->set(!running);
		gViewerWindow->alertXml("CouldNotStartStopScript");
	}
}

void LLLiveLSLEditor::onReset(void *userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;

	LLViewerObject* object = gObjectList.findObject( self->mObjectID );
	if(object)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ScriptReset);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Script);
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemID);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		gViewerWindow->alertXml("CouldNotStartStopScript"); 
	}
}

void LLLiveLSLEditor::draw()
{
	if(getVisible())
	{
		LLViewerObject* object = gObjectList.findObject(mObjectID);
		LLCheckBoxCtrl* runningCheckbox = LLUICtrlFactory::getCheckBoxByName(this, "running");
			if(object && mAskedForRunningInfo && mHaveRunningInfo)
		{
			if(object->permAnyOwner())
			{
				runningCheckbox->setLabel(ENABLED_RUNNING_CHECKBOX_LABEL);
				runningCheckbox->setEnabled(TRUE);
			}
			else
			{
				runningCheckbox->setLabel(DISABLED_RUNNING_CHECKBOX_LABEL);
				runningCheckbox->setEnabled(FALSE);
				// *FIX: Set it to false so that the ui is correct for
				// a box that is released to public. It could be
				// incorrect after a release/claim cycle, but will be
				// correct after clicking on it.
				runningCheckbox->set(FALSE);
			}
		}
		else if(!object)
		{
			// HACK: Display this information in the title bar.
			// Really ought to put in main window.
			setTitle("Script (object out of range)");
			runningCheckbox->setEnabled(FALSE);
			// object may have fallen out of range.
			mHaveRunningInfo = FALSE;
		}
		LLFloater::draw();
	}
}

struct LLLiveLSLSaveData
{
	LLLiveLSLSaveData(const LLUUID& id, const LLViewerInventoryItem* item, BOOL active);
	LLUUID mObjectID;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL mActive;
};

LLLiveLSLSaveData::LLLiveLSLSaveData(const LLUUID& id,
									 const LLViewerInventoryItem* item,
									 BOOL active) :
	mObjectID(id),
	mActive(active)
{
	llassert(item);
	mItem = new LLViewerInventoryItem(item);
}

void LLLiveLSLEditor::saveIfNeeded()
{
	llinfos << "LLLiveLSLEditor::saveIfNeeded()" << llendl;
	LLViewerObject* object = gObjectList.findObject(mObjectID);
	if(!object)
	{
		gViewerWindow->alertXml("SaveScriptFailObjectNotFound");
		return;
	}

	// get the latest info about it. We used to be losing the script
	// name on save, because the viewer object version of the item,
	// and the editor version would get out of synch. Here's a good
	// place to synch them back up.
	// HACK! we "know" that mItemID refers to a LLInventoryItem...
	LLInventoryItem* inv_item = (LLInventoryItem*)object->getInventoryObject(mItemID);
	if(inv_item)
	{
		mItem->copy(inv_item);
	}

	// Don't need to save if we're pristine
	if(mScriptEd->mEditor->isPristine())
	{
		return;
	}

	mPendingUploads = 0;

	// save the script
	mScriptEd->mEditor->makePristine();
	mScriptEd->mErrorList->deleteAllItems();

	// set up the save on the local machine.
	mScriptEd->mEditor->makePristine();
	LLTransactionID tid;
	tid.generate();
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string filename = llformat("%s.lsl", filepath.c_str());

	mItem->setAssetUUID(asset_id);
	mItem->setTransactionID(tid);

	// write out the data, and store it in the asset database
	FILE* fp = LLFile::fopen(filename.c_str(), "wb");
	if(!fp)
	{
		llwarns << "Unable to write to " << filename << llendl;

		LLSD row;
		row["columns"][0]["value"] = "Error writing to local file. Is your hard drive full?";
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		mScriptEd->mErrorList->addElement(row);
		return;
	}
	LLString utf8text = mScriptEd->mEditor->getText();
	fputs(utf8text.c_str(), fp);
	fclose(fp);
	fp = NULL;
	
	// save it out to asset server
	std::string url = gAgent.getRegion()->getCapability("UpdateScriptTaskInventory");
	getWindow()->incBusyCount();
	mPendingUploads++;
	BOOL is_running = LLUICtrlFactory::getCheckBoxByName(this, "running")->get();
	if (!url.empty())
	{
		uploadAssetViaCaps(url, filename, mObjectID,
						   mItemID, is_running);
	}
	else if (gAssetStorage)
	{
		uploadAssetLegacy(filename, object, tid, is_running);
	}
}

void LLLiveLSLEditor::uploadAssetViaCaps(const std::string& url,
										 const std::string& filename,
										 const LLUUID& task_id,
										 const LLUUID& item_id,
										 BOOL is_running)
{
	llinfos << "Update Task Inventory via capability" << llendl;
	LLSD body;
	body["task_id"] = task_id;
	body["item_id"] = item_id;
	body["is_script_running"] = is_running;
	LLHTTPClient::post(url, body,
		new LLUpdateTaskInventoryResponder(body, filename));
}

void LLLiveLSLEditor::uploadAssetLegacy(const std::string& filename,
										LLViewerObject* object,
										const LLTransactionID& tid,
										BOOL is_running)
{
	LLLiveLSLSaveData* data = new LLLiveLSLSaveData(mObjectID,
													mItem,
													is_running);
	gAssetStorage->storeAssetData(filename.c_str(), tid,
								  LLAssetType::AT_LSL_TEXT,
								  &onSaveTextComplete,
								  (void*)data,
								  FALSE);

	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id.asString());
	std::string dst_filename = llformat("%s.lso", filepath.c_str());
	std::string err_filename = llformat("%s.out", filepath.c_str());

	FILE *fp;
	if(!lscript_compile(filename.c_str(),
						dst_filename.c_str(),
						err_filename.c_str(),
						gAgent.isGodlike()))
	{
		// load the error file into the error scrolllist
		llinfos << "Compile failed!" << llendl;
		if(NULL != (fp = LLFile::fopen(err_filename.c_str(), "r")))
		{
			char buffer[MAX_STRING];		/*Flawfinder: ignore*/
			LLString line;
			while(!feof(fp)) 
			{
				
				fgets(buffer, MAX_STRING, fp);
				if(feof(fp))
				{
					break;
				}
				else
				{
					line.assign(buffer);
					LLString::stripNonprintable(line);
				
					LLSD row;
					row["columns"][0]["value"] = line;
					row["columns"][0]["font"] = "OCRA";
					mScriptEd->mErrorList->addElement(row);
				}
			}
			fclose(fp);
			mScriptEd->selectFirstError();
			// don't set the asset id, because we want to save the
			// script, even though the compile failed.
			//mItem->setAssetUUID(LLUUID::null);
			object->saveScript(mItem, FALSE, false);
			dialog_refresh_all();
		}
	}
	else
	{
		llinfos << "Compile worked!" << llendl;
		mScriptEd->mErrorList->addSimpleItem("Compile successful, saving...");
		if(gAssetStorage)
		{
			llinfos << "LLLiveLSLEditor::saveAsset "
					<< mItem->getAssetUUID() << llendl;
			getWindow()->incBusyCount();
			mPendingUploads++;
			LLLiveLSLSaveData* data = NULL;
			data = new LLLiveLSLSaveData(mObjectID,
										 mItem,
										 is_running);
			gAssetStorage->storeAssetData(dst_filename.c_str(),
										  tid,
										  LLAssetType::AT_LSL_BYTECODE,
										  &LLLiveLSLEditor::onSaveBytecodeComplete,
										  (void*)data);
			dialog_refresh_all();
		}
	}

	// get rid of any temp files left lying around
	LLFile::remove(filename.c_str());
	LLFile::remove(err_filename.c_str());
	LLFile::remove(dst_filename.c_str());

	// If we successfully saved it, then we should be able to check/uncheck the running box!
	LLCheckBoxCtrl* runningCheckbox = LLUICtrlFactory::getCheckBoxByName(this, "running");
	runningCheckbox->setLabel(ENABLED_RUNNING_CHECKBOX_LABEL);
	runningCheckbox->setEnabled(TRUE);
}

void LLLiveLSLEditor::onSaveTextComplete(const LLUUID& asset_uuid, void* user_data, S32 status) // StoreAssetData callback (fixed)
{
	LLLiveLSLSaveData* data = (LLLiveLSLSaveData*)user_data;

	if (status)
	{
		llwarns << "Unable to save text for a script." << llendl;
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("CompileQueueSaveText", args);
	}
	else
	{
		LLLiveLSLEditor* self = sInstances.getIfThere(data->mItem->getUUID() ^ data->mObjectID);
		if (self)
		{
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->close();
			}
		}
	}
	delete data;
	data = NULL;
}


void LLLiveLSLEditor::onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status) // StoreAssetData callback (fixed)
{
	LLLiveLSLSaveData* data = (LLLiveLSLSaveData*)user_data;
	if(!data)
		return;
	if(0 ==status)
	{
		llinfos << "LSL Bytecode saved" << llendl;
		LLUUID xor_id = data->mItem->getUUID() ^ data->mObjectID;
		LLLiveLSLEditor* self = sInstances.getIfThere(xor_id);
		if(self)
		{
			// Tell the user that the compile worked.
			self->mScriptEd->mErrorList->addSimpleItem("Save complete.");
			// close the window if this completes both uploads
			self->getWindow()->decBusyCount();
			self->mPendingUploads--;
			if (self->mPendingUploads <= 0
				&& self->mCloseAfterSave)
			{
				self->close();
			}
		}
		LLViewerObject* object = gObjectList.findObject(data->mObjectID);
		if(object)
		{
			object->saveScript(data->mItem, data->mActive, false);
			dialog_refresh_all();
			//LLToolDragAndDrop::dropScript(object, ids->first,
			//						  LLAssetType::AT_LSL_TEXT, FALSE);
		}
	}
	else
	{
		llinfos << "Problem saving LSL Bytecode (Live Editor)" << llendl;
		llwarns << "Unable to save a compiled script." << llendl;

		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("CompileQueueSaveBytecode", args);
	}

	std::string filepath = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_uuid.asString());
	std::string dst_filename = llformat("%s.lso", filepath.c_str());
	LLFile::remove(dst_filename.c_str());
	delete data;
}

BOOL LLLiveLSLEditor::canClose()
{
	return (mScriptEd->canClose());
}

void LLLiveLSLEditor::closeIfNeeded()
{
	getWindow()->decBusyCount();
	mPendingUploads--;
	if (mPendingUploads <= 0 && mCloseAfterSave)
	{
		close();
	}
}

// static
void LLLiveLSLEditor::onLoad(void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	self->loadAsset();
}

// static
void LLLiveLSLEditor::onSave(void* userdata, BOOL close_after_save)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;
	self->mCloseAfterSave = close_after_save;
	self->saveIfNeeded();
}

// static
LLLiveLSLEditor* LLLiveLSLEditor::show(const LLUUID& script_id, const LLUUID& object_id)
{
	LLLiveLSLEditor* instance = NULL;
	LLUUID xored_id = script_id ^ object_id;
	if(LLLiveLSLEditor::sInstances.checkData(xored_id))
	{
		// Move the existing view to the front
		instance = LLLiveLSLEditor::sInstances[xored_id];
		instance->open();		/*Flawfinder: ignore*/
	}
	return instance;
}

// static
void LLLiveLSLEditor::hide(const LLUUID& script_id, const LLUUID& object_id)
{
	LLUUID xored_id = script_id ^ object_id;
	if( LLLiveLSLEditor::sInstances.checkData( xored_id ) )
	{
		LLLiveLSLEditor* instance = LLLiveLSLEditor::sInstances[xored_id];
		if(instance->getParent())
		{
			instance->getParent()->removeChild(instance);
		}
		delete instance;
	}
}
// static
LLLiveLSLEditor* LLLiveLSLEditor::find(const LLUUID& script_id, const LLUUID& object_id)
{
	LLUUID xored_id = script_id ^ object_id;
	return sInstances.getIfThere(xored_id);
}


// static
void LLLiveLSLEditor::processScriptRunningReply(LLMessageSystem* msg, void**)
{
	LLUUID item_id;
	LLUUID object_id;
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ItemID, item_id);
	LLUUID xored_id = item_id ^ object_id;
	if(LLLiveLSLEditor::sInstances.checkData(xored_id))
	{
		LLLiveLSLEditor* instance = LLLiveLSLEditor::sInstances[xored_id];
		instance->mHaveRunningInfo = TRUE;
		BOOL running;
		msg->getBOOLFast(_PREHASH_Script, _PREHASH_Running, running);
		LLCheckBoxCtrl* runningCheckbox = LLUICtrlFactory::getCheckBoxByName(instance, "running");
		runningCheckbox->set(running);
	}
}

void LLLiveLSLEditor::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLFloater::reshape( width, height, called_from_parent );

	if( !isMinimized() )
	{
		// So that next time you open a script it will have the same height and width 
		// (although not the same position).
		gSavedSettings.setRect("PreviewScriptRect", mRect);
	}
}
