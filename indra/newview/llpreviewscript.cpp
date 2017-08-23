/** 
 * @file llpreviewscript.cpp
 * @brief LLPreviewScript class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llpreviewscript.h"

#include "llassetstorage.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldir.h"
#include "llenvmanager.h"
#include "llexternaleditor.h"
#include "llfilepicker.h"
#include "llfloaterreg.h"
#include "llinventorydefines.h"
#include "llinventorymodel.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "lllivefile.h"
#include "llhelp.h"
#include "llnotificationsutil.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "llsdserialize.h"
#include "llslider.h"
#include "lltooldraganddrop.h"
#include "llvfile.h"

#include "llagent.h"
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
#include "llscripteditor.h"
#include "llselectmgr.h"
#include "lltooldraganddrop.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llslider.h"
#include "lldir.h"
#include "llcombobox.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"
#include "lluictrlfactory.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llfloatergotoline.h"
#include "llexperiencecache.h"
#include "llfloaterexperienceprofile.h"
#include "llviewerassetupload.h"

const std::string HELLO_LSL =
	"default\n"
	"{\n"
	"	state_entry()\n"
	"	{\n"
	"		llSay(0, \"Hello, Avatar!\");\n"
	"	}\n"
	"\n"
	"	touch_start(integer total_number)\n"
	"	{\n"
	"		llSay(0, \"Touched.\");\n"
	"	}\n"
	"}\n";
const std::string HELP_LSL_PORTAL_TOPIC = "LSL_Portal";

const std::string DEFAULT_SCRIPT_NAME = "New Script"; // *TODO:Translate?
const std::string DEFAULT_SCRIPT_DESC = "(No Description)"; // *TODO:Translate?

// Description and header information
const S32 MAX_HISTORY_COUNT = 10;
const F32 LIVE_HELP_REFRESH_TIME = 1.f;

static bool have_script_upload_cap(LLUUID& object_id)
{
	LLViewerObject* object = gObjectList.findObject(object_id);
	return object && (! object->getRegion()->getCapability("UpdateScriptTask").empty());
}

/// ---------------------------------------------------------------------------
/// LLLiveLSLFile
/// ---------------------------------------------------------------------------
class LLLiveLSLFile : public LLLiveFile
{
public:
	typedef boost::function<bool (const std::string& filename)> change_callback_t;

	LLLiveLSLFile(std::string file_path, change_callback_t change_cb);
	~LLLiveLSLFile();

	void ignoreNextUpdate() { mIgnoreNextUpdate = true; }

protected:
	/*virtual*/ bool loadFile();

	change_callback_t	mOnChangeCallback;
	bool				mIgnoreNextUpdate;
};

LLLiveLSLFile::LLLiveLSLFile(std::string file_path, change_callback_t change_cb)
:	mOnChangeCallback(change_cb)
,	mIgnoreNextUpdate(false)
,	LLLiveFile(file_path, 1.0)
{
	llassert(mOnChangeCallback);
}

LLLiveLSLFile::~LLLiveLSLFile()
{
	LLFile::remove(filename());
}

bool LLLiveLSLFile::loadFile()
{
	if (mIgnoreNextUpdate)
	{
		mIgnoreNextUpdate = false;
		return true;
	}

	return mOnChangeCallback(filename());
}

/// ---------------------------------------------------------------------------
/// LLFloaterScriptSearch
/// ---------------------------------------------------------------------------
class LLFloaterScriptSearch : public LLFloater
{
public:
	LLFloaterScriptSearch(LLScriptEdCore* editor_core);
	~LLFloaterScriptSearch();

	/*virtual*/	BOOL	postBuild();
	static void show(LLScriptEdCore* editor_core);
	static void onBtnSearch(void* userdata);
	void handleBtnSearch();

	static void onBtnReplace(void* userdata);
	void handleBtnReplace();

	static void onBtnReplaceAll(void* userdata);
	void handleBtnReplaceAll();

	LLScriptEdCore* getEditorCore() { return mEditorCore; }
	static LLFloaterScriptSearch* getInstance() { return sInstance; }

	virtual bool hasAccelerators() const;
	virtual BOOL handleKeyHere(KEY key, MASK mask);

private:

	LLScriptEdCore* mEditorCore;
	static LLFloaterScriptSearch*	sInstance;

protected:
	LLLineEditor*			mSearchBox;
	LLLineEditor*			mReplaceBox;
		void onSearchBoxCommit();
};

LLFloaterScriptSearch* LLFloaterScriptSearch::sInstance = NULL;

LLFloaterScriptSearch::LLFloaterScriptSearch(LLScriptEdCore* editor_core)
:	LLFloater(LLSD()),
	mSearchBox(NULL),
	mReplaceBox(NULL),
	mEditorCore(editor_core)
{
	buildFromFile("floater_script_search.xml");

	sInstance = this;
	
	// find floater in which script panel is embedded
	LLView* viewp = (LLView*)editor_core;
	while(viewp)
	{
		LLFloater* floaterp = dynamic_cast<LLFloater*>(viewp);
		if (floaterp)
		{
			floaterp->addDependentFloater(this);
			break;
		}
		viewp = viewp->getParent();
	}
}

BOOL LLFloaterScriptSearch::postBuild()
{
	mReplaceBox = getChild<LLLineEditor>("replace_text");
	mSearchBox = getChild<LLLineEditor>("search_text");
	mSearchBox->setCommitCallback(boost::bind(&LLFloaterScriptSearch::onSearchBoxCommit, this));
	mSearchBox->setCommitOnFocusLost(FALSE);
	childSetAction("search_btn", onBtnSearch,this);
	childSetAction("replace_btn", onBtnReplace,this);
	childSetAction("replace_all_btn", onBtnReplaceAll,this);

	setDefaultBtn("search_btn");

	return TRUE;
}

//static 
void LLFloaterScriptSearch::show(LLScriptEdCore* editor_core)
{
	LLSD::String search_text;
	LLSD::String replace_text;
	if (sInstance && sInstance->mEditorCore && sInstance->mEditorCore != editor_core)
	{
		search_text=sInstance->mSearchBox->getValue().asString();
		replace_text=sInstance->mReplaceBox->getValue().asString();
		sInstance->closeFloater();
		delete sInstance;
	}

	if (!sInstance)
	{
		// sInstance will be assigned in the constructor.
		new LLFloaterScriptSearch(editor_core);
		sInstance->mSearchBox->setValue(search_text);
		sInstance->mReplaceBox->setValue(replace_text);
	}

	sInstance->openFloater();
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
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->selectNext(mSearchBox->getValue().asString(), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplace(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplace();
}

void LLFloaterScriptSearch::handleBtnReplace()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceText(mSearchBox->getValue().asString(), mReplaceBox->getValue().asString(), caseChk->get());
}

// static 
void LLFloaterScriptSearch::onBtnReplaceAll(void *userdata)
{
	LLFloaterScriptSearch* self = (LLFloaterScriptSearch*)userdata;
	self->handleBtnReplaceAll();
}

void LLFloaterScriptSearch::handleBtnReplaceAll()
{
	LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
	mEditorCore->mEditor->replaceTextAll(mSearchBox->getValue().asString(), mReplaceBox->getValue().asString(), caseChk->get());
}

bool LLFloaterScriptSearch::hasAccelerators() const
{
	if (mEditorCore)
	{
		return mEditorCore->hasAccelerators();
	}
	return FALSE;
}

BOOL LLFloaterScriptSearch::handleKeyHere(KEY key, MASK mask)
{
	if (mEditorCore)
	{
		BOOL handled = mEditorCore->handleKeyHere(key, mask);
		if (!handled)
		{
			LLFloater::handleKeyHere(key, mask);
		}
	}

	return FALSE;
}

void LLFloaterScriptSearch::onSearchBoxCommit()
{
	if (mEditorCore && mEditorCore->mEditor)
	{
		LLCheckBoxCtrl* caseChk = getChild<LLCheckBoxCtrl>("case_text");
		mEditorCore->mEditor->selectNext(mSearchBox->getValue().asString(), caseChk->get());
	}
}

/// ---------------------------------------------------------------------------
/// LLScriptEdCore
/// ---------------------------------------------------------------------------

struct LLSECKeywordCompare
{
	bool operator()(const std::string& lhs, const std::string& rhs)
	{
		return (LLStringUtil::compareDictInsensitive( lhs, rhs ) < 0 );
	}
};

LLScriptEdCore::LLScriptEdCore(
	LLScriptEdContainer* container,
	const std::string& sample,
	const LLHandle<LLFloater>& floater_handle,
	void (*load_callback)(void*),
	void (*save_callback)(void*, BOOL),
	void (*search_replace_callback) (void* userdata),
	void* userdata,
	bool live,
	S32 bottom_pad)
	:
	LLPanel(),
	mSampleText(sample),
	mEditor( NULL ),
	mLoadCallback( load_callback ),
	mSaveCallback( save_callback ),
	mSearchReplaceCallback( search_replace_callback ),
	mUserdata( userdata ),
	mForceClose( FALSE ),
	mLastHelpToken(NULL),
	mLiveHelpHistorySize(0),
	mEnableSave(FALSE),
	mLiveFile(NULL),
	mLive(live),
	mContainer(container),
	mHasScriptData(FALSE),
	mScriptRemoved(FALSE)
{
	setFollowsAll();
	setBorderVisible(FALSE);

	setXMLFilename("panel_script_ed.xml");
	llassert_always(mContainer != NULL);
}

LLScriptEdCore::~LLScriptEdCore()
{
	deleteBridges();

	// If the search window is up for this editor, close it.
	LLFloaterScriptSearch* script_search = LLFloaterScriptSearch::getInstance();
	if (script_search && script_search->getEditorCore() == this)
	{
		script_search->closeFloater();
		delete script_search;
	}

	delete mLiveFile;
	if (mSyntaxIDConnection.connected())
	{
		mSyntaxIDConnection.disconnect();
	}
}

void LLLiveLSLEditor::experienceChanged()
{
	if(mScriptEd->getAssociatedExperience() != mExperiences->getSelectedValue().asUUID())
	{
		mScriptEd->enableSave(getIsModifiable());
		//getChildView("Save_btn")->setEnabled(TRUE);
		mScriptEd->setAssociatedExperience(mExperiences->getSelectedValue().asUUID());
		updateExperiencePanel();
	}
}

void LLLiveLSLEditor::onViewProfile( LLUICtrl *ui, void* userdata )
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;

	LLUUID id;
	if(self->mExperienceEnabled->get())
	{
		id=self->mScriptEd->getAssociatedExperience();
		if(id.notNull())
		{
			 LLFloaterReg::showInstance("experience_profile", id, true);
		}
	}

}

void LLLiveLSLEditor::onToggleExperience( LLUICtrl *ui, void* userdata )
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;

	LLUUID id;
	if(self->mExperienceEnabled->get())
	{
		if(self->mScriptEd->getAssociatedExperience().isNull())
		{
			id=self->mExperienceIds.beginArray()->asUUID();
		}
	}

	if(id != self->mScriptEd->getAssociatedExperience())
	{
		self->mScriptEd->enableSave(self->getIsModifiable());
	}
	self->mScriptEd->setAssociatedExperience(id);

	self->updateExperiencePanel();
}

BOOL LLScriptEdCore::postBuild()
{
	mErrorList = getChild<LLScrollListCtrl>("lsl errors");

	mFunctions = getChild<LLComboBox>("Insert...");

	childSetCommitCallback("Insert...", &LLScriptEdCore::onBtnInsertFunction, this);

	mEditor = getChild<LLScriptEditor>("Script Editor");

	childSetCommitCallback("lsl errors", &LLScriptEdCore::onErrorList, this);
	childSetAction("Save_btn", boost::bind(&LLScriptEdCore::doSave,this,FALSE));
	childSetAction("Edit_btn", boost::bind(&LLScriptEdCore::openInExternalEditor, this));

	initMenu();

	mSyntaxIDConnection = LLSyntaxIdLSL::getInstance()->addSyntaxIDCallback(boost::bind(&LLScriptEdCore::processKeywords, this));

	// Intialise keyword highlighting for the current simulator's version of LSL
	LLSyntaxIdLSL::getInstance()->initialize();
	processKeywords();

	return TRUE;
}

void LLScriptEdCore::processKeywords()
{
	LL_DEBUGS("SyntaxLSL") << "Processing keywords" << LL_ENDL;
	mEditor->clearSegments();
	mEditor->initKeywords();
	mEditor->loadKeywords();
	
	string_vec_t primary_keywords;
	string_vec_t secondary_keywords;
	LLKeywordToken *token;
	LLKeywords::keyword_iterator_t token_it;
	for (token_it = mEditor->keywordsBegin(); token_it != mEditor->keywordsEnd(); ++token_it)
	{
		token = token_it->second;
		if (token->getType() == LLKeywordToken::TT_FUNCTION)
		{
			primary_keywords.push_back( wstring_to_utf8str(token->getToken()) );
		}
		else
		{
			secondary_keywords.push_back( wstring_to_utf8str(token->getToken()) );
		}
	}
	for (string_vec_t::const_iterator iter = primary_keywords.begin();
		 iter!= primary_keywords.end(); ++iter)
	{
		mFunctions->add(*iter);
	}
	for (string_vec_t::const_iterator iter = secondary_keywords.begin();
		 iter!= secondary_keywords.end(); ++iter)
	{
		mFunctions->add(*iter);
	}
}

void LLScriptEdCore::initMenu()
{
	// *TODO: Skinning - make these callbacks data driven
	LLMenuItemCallGL* menuItem;
	
	menuItem = getChild<LLMenuItemCallGL>("Save");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::doSave, this, FALSE));
	menuItem->setEnableCallback(boost::bind(&LLScriptEdCore::hasChanged, this));
	
	menuItem = getChild<LLMenuItemCallGL>("Revert All Changes");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnUndoChanges, this));
	menuItem->setEnableCallback(boost::bind(&LLScriptEdCore::hasChanged, this));

	menuItem = getChild<LLMenuItemCallGL>("Undo");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::undo, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canUndo, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Redo");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::redo, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canRedo, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Cut");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::cut, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canCut, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Copy");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::copy, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canCopy, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Paste");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::paste, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canPaste, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Select All");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::selectAll, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canSelectAll, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Deselect");
	menuItem->setClickCallback(boost::bind(&LLTextEditor::deselect, mEditor));
	menuItem->setEnableCallback(boost::bind(&LLTextEditor::canDeselect, mEditor));

	menuItem = getChild<LLMenuItemCallGL>("Search / Replace...");
	menuItem->setClickCallback(boost::bind(&LLFloaterScriptSearch::show, this));

	menuItem = getChild<LLMenuItemCallGL>("Go to line...");
	menuItem->setClickCallback(boost::bind(&LLFloaterGotoLine::show, this));

	menuItem = getChild<LLMenuItemCallGL>("Help...");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnHelp, this));

	menuItem = getChild<LLMenuItemCallGL>("Keyword Help...");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnDynamicHelp, this));

	menuItem = getChild<LLMenuItemCallGL>("LoadFromFile");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnLoadFromFile, this));
	menuItem->setEnableCallback(boost::bind(&LLScriptEdCore::enableLoadFromFileMenu, this));

	menuItem = getChild<LLMenuItemCallGL>("SaveToFile");
	menuItem->setClickCallback(boost::bind(&LLScriptEdCore::onBtnSaveToFile, this));
	menuItem->setEnableCallback(boost::bind(&LLScriptEdCore::enableSaveToFileMenu, this));
}

void LLScriptEdCore::setScriptText(const std::string& text, BOOL is_valid)
{
	if (mEditor)
	{
		mEditor->setText(text);
		mHasScriptData = is_valid;
	}
}

bool LLScriptEdCore::loadScriptText(const std::string& filename)
{
	if (filename.empty())
	{
		LL_WARNS() << "Empty file name" << LL_ENDL;
		return false;
	}

	LLFILE* file = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
	if (!file)
	{
		LL_WARNS() << "Error opening " << filename << LL_ENDL;
		return false;
	}

	// read in the whole file
	fseek(file, 0L, SEEK_END);
	size_t file_length = (size_t) ftell(file);
	fseek(file, 0L, SEEK_SET);
	char* buffer = new char[file_length+1];
	size_t nread = fread(buffer, 1, file_length, file);
	if (nread < file_length)
	{
		LL_WARNS() << "Short read" << LL_ENDL;
	}
	buffer[nread] = '\0';
	fclose(file);

	mEditor->setText(LLStringExplicit(buffer));
	delete[] buffer;

	return true;
}

bool LLScriptEdCore::writeToFile(const std::string& filename)
{
	LLFILE* fp = LLFile::fopen(filename, "wb");
	if (!fp)
	{
		LL_WARNS() << "Unable to write to " << filename << LL_ENDL;

		LLSD row;
		row["columns"][0]["value"] = "Error writing to local file. Is your hard drive full?";
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		mErrorList->addElement(row);
		return false;
	}

	std::string utf8text = mEditor->getText();

	// Special case for a completely empty script - stuff in one space so it can store properly.  See SL-46889
	if (utf8text.size() == 0)
	{
		utf8text = " ";
	}

	fputs(utf8text.c_str(), fp);
	fclose(fp);
	return true;
}

void LLScriptEdCore::sync()
{
	// Sync with external editor.
	std::string tmp_file = mContainer->getTmpFileName();
	llstat s;
	if (LLFile::stat(tmp_file, &s) == 0) // file exists
	{
		if (mLiveFile) mLiveFile->ignoreNextUpdate();
		writeToFile(tmp_file);
	}
}

bool LLScriptEdCore::hasChanged()
{
	if (!mEditor) return false;

	return ((!mEditor->isPristine() || mEnableSave) && mHasScriptData);
}

void LLScriptEdCore::draw()
{
	BOOL script_changed	= hasChanged();
	getChildView("Save_btn")->setEnabled(script_changed && !mScriptRemoved);

	if( mEditor->hasFocus() )
	{
		S32 line = 0;
		S32 column = 0;
		mEditor->getCurrentLineAndColumn( &line, &column, FALSE );  // don't include wordwrap
		LLStringUtil::format_map_t args;
		std::string cursor_pos;
		args["[LINE]"] = llformat ("%d", line);
		args["[COLUMN]"] = llformat ("%d", column);
		cursor_pos = LLTrans::getString("CursorPos", args);
		getChild<LLUICtrl>("line_col")->setValue(cursor_pos);
	}
	else
	{
		getChild<LLUICtrl>("line_col")->setValue(LLStringUtil::null);
	}

	updateDynamicHelp();

	LLPanel::draw();
}

void LLScriptEdCore::updateDynamicHelp(BOOL immediate)
{
	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;

	// update back and forward buttons
	LLButton* fwd_button = help_floater->getChild<LLButton>("fwd_btn");
	LLButton* back_button = help_floater->getChild<LLButton>("back_btn");
	LLMediaCtrl* browser = help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	back_button->setEnabled(browser->canNavigateBack());
	fwd_button->setEnabled(browser->canNavigateForward());

	if (!immediate && !gSavedSettings.getBOOL("ScriptHelpFollowsCursor"))
	{
		return;
	}

	LLTextSegmentPtr segment = NULL;
	std::vector<LLTextSegmentPtr> selected_segments;
	mEditor->getSelectedSegments(selected_segments);
	LLKeywordToken* token;
	// try segments in selection range first
	std::vector<LLTextSegmentPtr>::iterator segment_iter;
	for (segment_iter = selected_segments.begin(); segment_iter != selected_segments.end(); ++segment_iter)
	{
		token = (*segment_iter)->getToken();
		if(token && isKeyword(token))
		{
			segment = *segment_iter;
			break;
		}
	}

	// then try previous segment in case we just typed it
	if (!segment)
	{
		const LLTextSegmentPtr test_segment = mEditor->getPreviousSegment();
		token = test_segment->getToken();
		if(token && isKeyword(token))
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
			std::string help_string = mEditor->getText().substr(segment->getStart(), segment->getEnd() - segment->getStart());
			setHelpPage(help_string);
			mLiveHelpTimer.stop();
		}
	}
	else
	{
		if (immediate)
		{
			setHelpPage(LLStringUtil::null);
		}
	}
}

bool LLScriptEdCore::isKeyword(LLKeywordToken* token)
{
	switch(token->getType())
	{
		case LLKeywordToken::TT_CONSTANT:
		case LLKeywordToken::TT_CONTROL:
		case LLKeywordToken::TT_EVENT:
		case LLKeywordToken::TT_FUNCTION:
		case LLKeywordToken::TT_SECTION:
		case LLKeywordToken::TT_TYPE:
		case LLKeywordToken::TT_WORD:
			return true;

		default:
			return false;
	}
}

void LLScriptEdCore::setHelpPage(const std::string& help_string)
{
	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;
	
	LLMediaCtrl* web_browser = help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
	if (!web_browser) return;

	LLComboBox* history_combo = help_floater->getChild<LLComboBox>("history_combo");
	if (!history_combo) return;

	LLUIString url_string = gSavedSettings.getString("LSLHelpURL");

	url_string.setArg("[LSL_STRING]", help_string);

	addHelpItemToHistory(help_string);

	web_browser->navigateTo(url_string);

}


void LLScriptEdCore::addHelpItemToHistory(const std::string& help_string)
{
	if (help_string.empty()) return;

	LLFloater* help_floater = mLiveHelpHandle.get();
	if (!help_floater) return;

	LLComboBox* history_combo = help_floater->getChild<LLComboBox>("history_combo");
	if (!history_combo) return;

	// separate history items from full item list
	if (mLiveHelpHistorySize == 0)
	{
		history_combo->addSeparator(ADD_TOP);
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
	if(mForceClose || !hasChanged() || mScriptRemoved)
	{
		return TRUE;
	}
	else
	{
		// Bring up view-modal dialog: Save changes? Yes, No, Cancel
		LLNotificationsUtil::add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLScriptEdCore::handleSaveChangesDialog, this, _1, _2));
		return FALSE;
	}
}

void LLScriptEdCore::setEnableEditing(bool enable)
{
	mEditor->setEnabled(enable);
	getChildView("Edit_btn")->setEnabled(enable);
}

bool LLScriptEdCore::handleSaveChangesDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch( option )
	{
	case 0:  // "Yes"
		// close after saving
			doSave( TRUE );
		break;

	case 1:  // "No"
		mForceClose = TRUE;
		// This will close immediately because mForceClose is true, so we won't
		// infinite loop with these dialogs. JC
		((LLFloater*) getParent())->closeFloater();
		break;

	case 2: // "Cancel"
	default:
		// If we were quitting, we didn't really mean it.
		LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}

void LLScriptEdCore::onBtnHelp()
{
	LLUI::sHelpImpl->showTopic(HELP_LSL_PORTAL_TOPIC);
}

void LLScriptEdCore::onBtnDynamicHelp()
{
	LLFloater* live_help_floater = mLiveHelpHandle.get();
	if (!live_help_floater)
	{
		live_help_floater = new LLFloater(LLSD());
		live_help_floater->buildFromFile("floater_lsl_guide.xml");
		LLFloater* parent = dynamic_cast<LLFloater*>(getParent());
		llassert(parent);
		if (parent)
			parent->addDependentFloater(live_help_floater, TRUE);
		live_help_floater->childSetCommitCallback("lock_check", onCheckLock, this);
		live_help_floater->getChild<LLUICtrl>("lock_check")->setValue(gSavedSettings.getBOOL("ScriptHelpFollowsCursor"));
		live_help_floater->childSetCommitCallback("history_combo", onHelpComboCommit, this);
		live_help_floater->childSetAction("back_btn", onClickBack, this);
		live_help_floater->childSetAction("fwd_btn", onClickForward, this);

		LLMediaCtrl* browser = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		browser->setAlwaysRefresh(TRUE);

		LLComboBox* help_combo = live_help_floater->getChild<LLComboBox>("history_combo");
		LLKeywordToken *token;
		LLKeywords::keyword_iterator_t token_it;
		for (token_it = mEditor->keywordsBegin(); 
			 token_it != mEditor->keywordsEnd(); 
			 ++token_it)
		{
			token = token_it->second;
			help_combo->add(wstring_to_utf8str(token->getToken()));
		}
		help_combo->sortByName();

		// re-initialize help variables
		mLastHelpToken = NULL;
		mLiveHelpHandle = live_help_floater->getHandle();
		mLiveHelpHistorySize = 0;
	}

	BOOL visible = TRUE;
	BOOL take_focus = TRUE;
	live_help_floater->setVisible(visible);
	live_help_floater->setFrontmost(take_focus);

	updateDynamicHelp(TRUE);
}

//static 
void LLScriptEdCore::onClickBack(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		LLMediaCtrl* browserp = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		if (browserp)
		{
			browserp->navigateBack();
		}
	}
}

//static 
void LLScriptEdCore::onClickForward(void* userdata)
{
	LLScriptEdCore* corep = (LLScriptEdCore*)userdata;
	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		LLMediaCtrl* browserp = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		if (browserp)
		{
			browserp->navigateForward();
		}
	}
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

	LLFloater* live_help_floater = corep->mLiveHelpHandle.get();
	if (live_help_floater)
	{
		std::string help_string = ctrl->getValue().asString();

		corep->addHelpItemToHistory(help_string);

		LLMediaCtrl* web_browser = live_help_floater->getChild<LLMediaCtrl>("lsl_guide_html");
		LLUIString url_string = gSavedSettings.getString("LSLHelpURL");
		url_string.setArg("[LSL_STRING]", help_string);
		web_browser->navigateTo(url_string);
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

void LLScriptEdCore::doSave( BOOL close_after_save )
{
	add(LLStatViewer::LSL_SAVES, 1);

	if( mSaveCallback )
	{
		mSaveCallback( mUserdata, close_after_save );
	}
}

void LLScriptEdCore::openInExternalEditor()
{
	delete mLiveFile; // deletes file

	// Save the script to a temporary file.
	std::string filename = mContainer->getTmpFileName();
	writeToFile(filename);

	// Start watching file changes.
	mLiveFile = new LLLiveLSLFile(filename, boost::bind(&LLScriptEdContainer::onExternalChange, mContainer, _1));
	mLiveFile->addToEventTimer();

	// Open it in external editor.
	{
		LLExternalEditor ed;
		LLExternalEditor::EErrorCode status;
		std::string msg;

		status = ed.setCommand("LL_SCRIPT_EDITOR");
		if (status != LLExternalEditor::EC_SUCCESS)
		{
			if (status == LLExternalEditor::EC_NOT_SPECIFIED) // Use custom message for this error.
			{
				msg = getString("external_editor_not_set");
			}
			else
			{
				msg = LLExternalEditor::getErrorMessage(status);
			}

			LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", msg));
			return;
		}

		status = ed.run(filename);
		if (status != LLExternalEditor::EC_SUCCESS)
		{
			msg = LLExternalEditor::getErrorMessage(status);
			LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", msg));
		}
	}
}

void LLScriptEdCore::onBtnUndoChanges()
{
	if( !mEditor->tryToRevertToPristineState() )
	{
		LLNotificationsUtil::add("ScriptCannotUndo", LLSD(), LLSD(), boost::bind(&LLScriptEdCore::handleReloadFromServerDialog, this, _1, _2));
	}
}

// static
void LLScriptEdCore::onErrorList(LLUICtrl*, void* user_data)
{
	LLScriptEdCore* self = (LLScriptEdCore*)user_data;
	LLScrollListItem* item = self->mErrorList->getFirstSelected();
	if(item)
	{
		// *FIX: replace with boost grep
		S32 row = 0;
		S32 column = 0;
		const LLScrollListCell* cell = item->getColumn(0);
		std::string line(cell->getValue().asString());
		line.erase(0, 1);
		LLStringUtil::replaceChar(line, ',',' ');
		LLStringUtil::replaceChar(line, ')',' ');
		sscanf(line.c_str(), "%d %d", &row, &column);
		//LL_INFOS() << "LLScriptEdCore::onErrorList() - " << row << ", "
		//<< column << LL_ENDL;
		self->mEditor->setCursor(row, column);
		self->mEditor->setFocus(TRUE);
	}
}

bool LLScriptEdCore::handleReloadFromServerDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch( option )
	{
	case 0: // "Yes"
		if( mLoadCallback )
		{
			setScriptText(getString("loading"), FALSE);
			mLoadCallback(mUserdata);
		}
		break;

	case 1: // "No"
		break;

	default:
		llassert(0);
		break;
	}
	return false;
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
	S32 count = mBridges.size();
	LLEntryAndEdCore* eandc;
	for(S32 i = 0; i < count; i++)
	{
		eandc = mBridges.at(i);
		delete eandc;
		mBridges[i] = NULL;
	}
	mBridges.clear();
}

// virtual
BOOL LLScriptEdCore::handleKeyHere(KEY key, MASK mask)
{
	bool just_control = MASK_CONTROL == (mask & MASK_MODIFIERS);

	if(('S' == key) && just_control)
	{
		if(mSaveCallback)
		{
			// don't close after saving
			mSaveCallback(mUserdata, FALSE);
		}

		return TRUE;
	}

	if(('F' == key) && just_control)
	{
		if(mSearchReplaceCallback)
		{
			mSearchReplaceCallback(mUserdata);
		}

		return TRUE;
	}

	return FALSE;
}

void LLScriptEdCore::onBtnLoadFromFile( void* data )
{
	LLScriptEdCore* self = (LLScriptEdCore*) data;

	// TODO Maybe add a dialogue warning here if the current file has unsaved changes.
	LLFilePicker& file_picker = LLFilePicker::instance();
	if( !file_picker.getOpenFile( LLFilePicker::FFLOAD_SCRIPT ) )
	{
		//File picking cancelled by user, so nothing to do.
		return;
	}

	std::string filename = file_picker.getFirstFile();

	std::ifstream fin(filename.c_str());

	std::string line;
	std::string text;
	std::string linetotal;
	while (!fin.eof())
	{ 
		getline(fin,line);
		text += line;
		if (!fin.eof())
		{
			text += "\n";
		}
	}
	fin.close();

	// Only replace the script if there is something to replace with.
	if (text.length() > 0)
	{
		self->mEditor->selectAll();
		LLWString script(utf8str_to_wstring(text));
		self->mEditor->insertText(script);
	}
}

void LLScriptEdCore::onBtnSaveToFile( void* userdata )
{
	add(LLStatViewer::LSL_SAVES, 1);

	LLScriptEdCore* self = (LLScriptEdCore*) userdata;

	if( self->mSaveCallback )
	{
		LLFilePicker& file_picker = LLFilePicker::instance();
		if( file_picker.getSaveFile( LLFilePicker::FFSAVE_SCRIPT, self->mScriptName ) )
		{
			std::string filename = file_picker.getFirstFile();
			std::string scriptText=self->mEditor->getText();
			std::ofstream fout(filename.c_str());
			fout<<(scriptText);
			fout.close();
			self->mSaveCallback( self->mUserdata, FALSE );
		}
	}
}

bool LLScriptEdCore::canLoadOrSaveToFile( void* userdata )
{
	LLScriptEdCore* self = (LLScriptEdCore*) userdata;
	return self->mEditor->canLoadOrSaveToFile();
}

// static
bool LLScriptEdCore::enableSaveToFileMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	if (!self || !self->mEditor) return FALSE;
	return self->mEditor->canLoadOrSaveToFile();
}

// static 
bool LLScriptEdCore::enableLoadFromFileMenu(void* userdata)
{
	LLScriptEdCore* self = (LLScriptEdCore*)userdata;
	return (self && self->mEditor) ? self->mEditor->canLoadOrSaveToFile() : FALSE;
}

LLUUID LLScriptEdCore::getAssociatedExperience()const
{
	return mAssociatedExperience;
}

void LLLiveLSLEditor::setExperienceIds( const LLSD& experience_ids )
{
	mExperienceIds=experience_ids;
	updateExperiencePanel();
}


void LLLiveLSLEditor::updateExperiencePanel()
{
	if(mScriptEd->getAssociatedExperience().isNull())
	{
		mExperienceEnabled->set(FALSE);
		mExperiences->setVisible(FALSE);
		if(mExperienceIds.size()>0)
		{
			mExperienceEnabled->setEnabled(TRUE);
			mExperienceEnabled->setToolTip(getString("add_experiences"));
		}
		else
		{
			mExperienceEnabled->setEnabled(FALSE);
			mExperienceEnabled->setToolTip(getString("no_experiences"));
		}
		getChild<LLButton>("view_profile")->setVisible(FALSE);
	}
	else
	{
		mExperienceEnabled->setToolTip(getString("experience_enabled"));
		mExperienceEnabled->setEnabled(getIsModifiable());
		mExperiences->setVisible(TRUE);
		mExperienceEnabled->set(TRUE);
		getChild<LLButton>("view_profile")->setToolTip(getString("show_experience_profile"));
		buildExperienceList();
	}
}

void LLLiveLSLEditor::buildExperienceList()
{
	mExperiences->clearRows();
	bool foundAssociated=false;
	const LLUUID& associated = mScriptEd->getAssociatedExperience();
	LLUUID last;
	LLScrollListItem* item;
	for(LLSD::array_const_iterator it = mExperienceIds.beginArray(); it != mExperienceIds.endArray(); ++it)
	{
		LLUUID id = it->asUUID();
		EAddPosition position = ADD_BOTTOM;
		if(id == associated)
		{
			foundAssociated = true;
			position = ADD_TOP;
		}
		
        const LLSD& experience = LLExperienceCache::instance().get(id);
		if(experience.isUndefined())
		{
			mExperiences->add(getString("loading"), id, position);
			last = id;
		}
		else
		{
			std::string experience_name_string = experience[LLExperienceCache::NAME].asString();
			if (experience_name_string.empty())
			{
				experience_name_string = LLTrans::getString("ExperienceNameUntitled");
			}
			mExperiences->add(experience_name_string, id, position);
		} 
	}

	if(!foundAssociated )
	{
        const LLSD& experience = LLExperienceCache::instance().get(associated);
		if(experience.isDefined())
		{
			std::string experience_name_string = experience[LLExperienceCache::NAME].asString();
			if (experience_name_string.empty())
			{
				experience_name_string = LLTrans::getString("ExperienceNameUntitled");
			}
			item=mExperiences->add(experience_name_string, associated, ADD_TOP);
		} 
		else
		{
			item=mExperiences->add(getString("loading"), associated, ADD_TOP);
			last = associated;
		}
		item->setEnabled(FALSE);
	}

	if(last.notNull())
	{
		mExperiences->setEnabled(FALSE);
        LLExperienceCache::instance().get(last, boost::bind(&LLLiveLSLEditor::buildExperienceList, this));
	}
	else
	{
		mExperiences->setEnabled(TRUE);
		mExperiences->sortByName(TRUE);
		mExperiences->setCurrentByIndex(mExperiences->getCurrentIndex());
		getChild<LLButton>("view_profile")->setVisible(TRUE);
	}
}


void LLScriptEdCore::setAssociatedExperience( const LLUUID& experience_id )
{
	mAssociatedExperience = experience_id;
}



void LLLiveLSLEditor::requestExperiences()
{
	if (!getIsModifiable())
	{
		return;
	}

	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string lookup_url=region->getCapability("GetCreatorExperiences"); 
		if(!lookup_url.empty())
		{
            LLCoreHttpUtil::HttpCoroutineAdapter::completionCallback_t success =
                boost::bind(&LLLiveLSLEditor::receiveExperienceIds, _1, getDerivedHandle<LLLiveLSLEditor>());

            LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpGet(lookup_url, success);
		}
	}
}

/*static*/ 
void LLLiveLSLEditor::receiveExperienceIds(LLSD result, LLHandle<LLLiveLSLEditor> hparent)
{
    LLLiveLSLEditor* parent = hparent.get();
    if (!parent)
        return;

    parent->setExperienceIds(result["experience_ids"]);
}


/// ---------------------------------------------------------------------------
/// LLScriptEdContainer
/// ---------------------------------------------------------------------------

LLScriptEdContainer::LLScriptEdContainer(const LLSD& key) :
	LLPreview(key)
,	mScriptEd(NULL)
{
}

std::string LLScriptEdContainer::getTmpFileName()
{
	// Take script inventory item id (within the object inventory)
	// to consideration so that it's possible to edit multiple scripts
	// in the same object inventory simultaneously (STORM-781).
	std::string script_id = mObjectUUID.asString() + "_" + mItemUUID.asString();

	// Use MD5 sum to make the file name shorter and not exceed maximum path length.
	char script_id_hash_str[33];			   /* Flawfinder: ignore */
	LLMD5 script_id_hash((const U8 *)script_id.c_str());
	script_id_hash.hex_digest(script_id_hash_str);

	return std::string(LLFile::tmpdir()) + "sl_script_" + script_id_hash_str + ".lsl";
}

bool LLScriptEdContainer::onExternalChange(const std::string& filename)
{
	if (!mScriptEd->loadScriptText(filename))
	{
		return false;
	}

	// Disable sync to avoid recursive load->save->load calls.
	saveIfNeeded(false);
	return true;
}

/// ---------------------------------------------------------------------------
/// LLPreviewLSL
/// ---------------------------------------------------------------------------

struct LLScriptSaveInfo
{
	LLUUID mItemUUID;
	std::string mDescription;
	LLTransactionID mTransactionID;

	LLScriptSaveInfo(const LLUUID& uuid, const std::string& desc, LLTransactionID tid) :
		mItemUUID(uuid), mDescription(desc),  mTransactionID(tid) {}
};



//static
void* LLPreviewLSL::createScriptEdPanel(void* userdata)
{
	
	LLPreviewLSL *self = (LLPreviewLSL*)userdata;

	self->mScriptEd =  new LLScriptEdCore(
								   self,
								   HELLO_LSL,
								   self->getHandle(),
								   LLPreviewLSL::onLoad,
								   LLPreviewLSL::onSave,
								   LLPreviewLSL::onSearchReplace,
								   self,
								   false,
								   0);
	return self->mScriptEd;
}


LLPreviewLSL::LLPreviewLSL(const LLSD& key )
:	LLScriptEdContainer(key),
	mPendingUploads(0)
{
	mFactoryMap["script panel"] = LLCallbackMap(LLPreviewLSL::createScriptEdPanel, this);
}

// virtual
BOOL LLPreviewLSL::postBuild()
{
	const LLInventoryItem* item = getItem();

	llassert(item);
	if (item)
	{
		getChild<LLUICtrl>("desc")->setValue(item->getDescription());
	}
	childSetCommitCallback("desc", LLPreview::onText, this);
	getChild<LLLineEditor>("desc")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);

	return LLPreview::postBuild();
}

void LLPreviewLSL::draw()
{
	const LLInventoryItem* item = getItem();
	if(!item)
	{
		setTitle(LLTrans::getString("ScriptWasDeleted"));
		mScriptEd->setItemRemoved(TRUE);
	}

	LLPreview::draw();
}
// virtual
void LLPreviewLSL::callbackLSLCompileSucceeded()
{
	LL_INFOS() << "LSL Bytecode saved" << LL_ENDL;
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessful"));
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
	closeIfNeeded();
}

// virtual
void LLPreviewLSL::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	LL_INFOS() << "Compile failed!" << LL_ENDL;

	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		std::string error_message = line->asString();
		LLStringUtil::stripNonprintable(error_message);
		row["columns"][0]["value"] = error_message;
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
	const LLInventoryItem* item = gInventory.getItem(mItemUUID);
	BOOL is_library = item
		&& !gInventory.isObjectDescendentOf(mItemUUID,
											gInventory.getRootFolderID());
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
			gAssetStorage->getInvItemAsset(LLHost(),
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
			mScriptEd->setScriptText(mScriptEd->getString("can_not_view"), FALSE);
			mScriptEd->mEditor->makePristine();
			mScriptEd->mFunctions->setEnabled(FALSE);
			mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		getChildView("lock")->setVisible( !is_modifiable);
		mScriptEd->getChildView("Insert...")->setEnabled(is_modifiable);
	}
	else
	{
		mScriptEd->setScriptText(std::string(HELLO_LSL), TRUE);
		mScriptEd->setEnableEditing(TRUE);
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
		closeFloater();
	}
}

void LLPreviewLSL::onSearchReplace(void* userdata)
{
	LLPreviewLSL* self = (LLPreviewLSL*)userdata;
	LLScriptEdCore* sec = self->mScriptEd; 
	LLFloaterScriptSearch::show(sec);
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

/*static*/
void LLPreviewLSL::finishedLSLUpload(LLUUID itemId, LLSD response)
{
    // Find our window and close it if requested.
    LLPreviewLSL* preview = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", LLSD(itemId));
    if (preview)
    {
        // Bytecode save completed
        if (response["compiled"])
        {
            preview->callbackLSLCompileSucceeded();
        }
        else
        {
            preview->callbackLSLCompileFailed(response["errors"]);
        }
    }
}

// Save needs to compile the text in the buffer. If the compile
// succeeds, then save both assets out to the database. If the compile
// fails, go ahead and save the text anyway.
void LLPreviewLSL::saveIfNeeded(bool sync /*= true*/)
{
    if (!mScriptEd->hasChanged())
    {
        return;
    }

    mPendingUploads = 0;
    mScriptEd->mErrorList->deleteAllItems();
    mScriptEd->mEditor->makePristine();

    if (sync)
    {
        mScriptEd->sync();
    }

    if (!gAgent.getRegion()) return;
    const LLInventoryItem *inv_item = getItem();
    // save it out to asset server
    std::string url = gAgent.getRegion()->getCapability("UpdateScriptAgent");
    if(inv_item)
    {
        getWindow()->incBusyCount();
        mPendingUploads++;
        if (!url.empty())
        {
            std::string buffer(mScriptEd->mEditor->getText());
            LLBufferedAssetUploadInfo::invnUploadFinish_f proc = boost::bind(&LLPreviewLSL::finishedLSLUpload, _1, _4);

            LLResourceUploadInfo::ptr_t uploadInfo(new LLScriptAssetUpload(mItemUUID, buffer, proc));

            LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
        }
    }
}

// static
void LLPreviewLSL::onLoadComplete( LLVFS *vfs, const LLUUID& asset_uuid, LLAssetType::EType type,
								   void* user_data, S32 status, LLExtStat ext_status)
{
	LL_DEBUGS() << "LLPreviewLSL::onLoadComplete: got uuid " << asset_uuid
		 << LL_ENDL;
	LLUUID* item_uuid = (LLUUID*)user_data;
	LLPreviewLSL* preview = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", *item_uuid);
	if( preview )
	{
		if(0 == status)
		{
			LLVFile file(vfs, asset_uuid, type);
			S32 file_length = file.getSize();

			std::vector<char> buffer(file_length+1);
			file.read((U8*)&buffer[0], file_length);

			// put a EOS at the end
			buffer[file_length] = 0;
			preview->mScriptEd->setScriptText(LLStringExplicit(&buffer[0]), TRUE);
			preview->mScriptEd->mEditor->makePristine();
			LLInventoryItem* item = gInventory.getItem(*item_uuid);
			BOOL is_modifiable = FALSE;
			if(item
			   && gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
				   					GP_OBJECT_MANIPULATE))
			{
				is_modifiable = TRUE;		
			}
			preview->mScriptEd->setEnableEditing(is_modifiable);
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotificationsUtil::add("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotificationsUtil::add("ScriptNoPermissions");
			}
			else
			{
				LLNotificationsUtil::add("UnableToLoadScript");
			}

			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
			LL_WARNS() << "Problem loading script: " << status << LL_ENDL;
		}
	}
	delete item_uuid;
}


/// ---------------------------------------------------------------------------
/// LLLiveLSLEditor
/// ---------------------------------------------------------------------------


//static 
void* LLLiveLSLEditor::createScriptEdPanel(void* userdata)
{
	LLLiveLSLEditor *self = (LLLiveLSLEditor*)userdata;

	self->mScriptEd =  new LLScriptEdCore(
								   self,
								   HELLO_LSL,
								   self->getHandle(),
								   &LLLiveLSLEditor::onLoad,
								   &LLLiveLSLEditor::onSave,
								   &LLLiveLSLEditor::onSearchReplace,
								   self,
								   true,
								   0);
	return self->mScriptEd;
}


LLLiveLSLEditor::LLLiveLSLEditor(const LLSD& key) :
	LLScriptEdContainer(key),
	mAskedForRunningInfo(FALSE),
	mHaveRunningInfo(FALSE),
	mCloseAfterSave(FALSE),
	mPendingUploads(0),
	mIsModifiable(FALSE),
	mIsNew(false),
	mIsSaving(FALSE)
{
	mFactoryMap["script ed panel"] = LLCallbackMap(LLLiveLSLEditor::createScriptEdPanel, this);
}

BOOL LLLiveLSLEditor::postBuild()
{
	childSetCommitCallback("running", LLLiveLSLEditor::onRunningCheckboxClicked, this);
	getChildView("running")->setEnabled(FALSE);

	childSetAction("Reset",&LLLiveLSLEditor::onReset,this);
	getChildView("Reset")->setEnabled(TRUE);

	mMonoCheckbox =	getChild<LLCheckBoxCtrl>("mono");
	childSetCommitCallback("mono", &LLLiveLSLEditor::onMonoCheckboxClicked, this);
	getChildView("mono")->setEnabled(FALSE);

	mScriptEd->mEditor->makePristine();
	mScriptEd->mEditor->setFocus(TRUE);


	mExperiences = getChild<LLComboBox>("Experiences...");
	mExperiences->setCommitCallback(boost::bind(&LLLiveLSLEditor::experienceChanged, this));
	
	mExperienceEnabled = getChild<LLCheckBoxCtrl>("enable_xp");
	
	childSetCommitCallback("enable_xp", onToggleExperience, this);
	childSetCommitCallback("view_profile", onViewProfile, this);
	

	return LLPreview::postBuild();
}

// virtual
void LLLiveLSLEditor::callbackLSLCompileSucceeded(const LLUUID& task_id,
												  const LLUUID& item_id,
												  bool is_script_running)
{
	LL_DEBUGS() << "LSL Bytecode saved" << LL_ENDL;
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("CompileSuccessful"));
	mScriptEd->mErrorList->setCommentText(LLTrans::getString("SaveComplete"));
	getChild<LLCheckBoxCtrl>("running")->set(is_script_running);
	mIsSaving = FALSE;
	closeIfNeeded();
}

// virtual
void LLLiveLSLEditor::callbackLSLCompileFailed(const LLSD& compile_errors)
{
	LL_DEBUGS() << "Compile failed!" << LL_ENDL;
	for(LLSD::array_const_iterator line = compile_errors.beginArray();
		line < compile_errors.endArray();
		line++)
	{
		LLSD row;
		std::string error_message = line->asString();
		LLStringUtil::stripNonprintable(error_message);
		row["columns"][0]["value"] = error_message;
		// *TODO: change to "MONOSPACE" and change llfontgl.cpp?
		row["columns"][0]["font"] = "OCRA";
		mScriptEd->mErrorList->addElement(row);
	}
	mScriptEd->selectFirstError();
	mIsSaving = FALSE;
	closeIfNeeded();
}

void LLLiveLSLEditor::loadAsset()
{
	//LL_INFOS() << "LLLiveLSLEditor::loadAsset()" << LL_ENDL;
	if(!mIsNew)
	{
		LLViewerObject* object = gObjectList.findObject(mObjectUUID);
		if(object)
		{
			LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemUUID));

			if(item)
			{
				LLViewerRegion* region = object->getRegion();
				std::string url = std::string();
				if(region)
				{
					url = region->getCapability("GetMetadata");
				}
				LLExperienceCache::instance().fetchAssociatedExperience(item->getParentUUID(), item->getUUID(), url,
					boost::bind(&LLLiveLSLEditor::setAssociatedExperience, getDerivedHandle<LLLiveLSLEditor>(), _1));

				bool isGodlike = gAgent.isGodlike();
				bool copyManipulate = gAgent.allowOperation(PERM_COPY, item->getPermissions(), GP_OBJECT_MANIPULATE);
				mIsModifiable = gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE);
				
				if(!isGodlike && (!copyManipulate || !mIsModifiable))
				{
					mItem = new LLViewerInventoryItem(item);
					mScriptEd->setScriptText(getString("not_allowed"), FALSE);
					mScriptEd->mEditor->makePristine();
					mScriptEd->enableSave(FALSE);
					mAssetStatus = PREVIEW_ASSET_LOADED;
				}
				else if(copyManipulate || isGodlike)
				{
					mItem = new LLViewerInventoryItem(item);
					// request the text from the object
				LLSD* user_data = new LLSD();
				user_data->with("taskid", mObjectUUID).with("itemid", mItemUUID);
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
					msg->addUUIDFast(_PREHASH_ObjectID, mObjectUUID);
					msg->addUUIDFast(_PREHASH_ItemID, mItemUUID);
					msg->sendReliable(object->getRegion()->getHost());
					mAskedForRunningInfo = TRUE;
					mAssetStatus = PREVIEW_ASSET_LOADING;
				}
			}
			
			if(mItem.isNull())
			{
				mScriptEd->setScriptText(LLStringUtil::null, FALSE);
				mScriptEd->mEditor->makePristine();
				mAssetStatus = PREVIEW_ASSET_LOADED;
				mIsModifiable = FALSE;
			}

			refreshFromItem();
			// This is commented out, because we don't completely
			// handle script exports yet.
			/*
			// request the exports from the object
			gMessageSystem->newMessage("GetScriptExports");
			gMessageSystem->nextBlock("ScriptBlock");
			gMessageSystem->addUUID("AgentID", gAgent.getID());
			U32 local_id = object->getLocalID();
			gMessageSystem->addData("LocalID", &local_id);
			gMessageSystem->addUUID("ItemID", mItemUUID);
			LLHost host(object->getRegion()->getIP(),
						object->getRegion()->getPort());
			gMessageSystem->sendReliable(host);
			*/
		}
	}
	else
	{
		mScriptEd->setScriptText(std::string(HELLO_LSL), TRUE);
		mScriptEd->enableSave(FALSE);
		LLPermissions perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, gAgent.getGroupID());
		perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, PERM_MOVE | PERM_TRANSFER);
		mItem = new LLViewerInventoryItem(mItemUUID,
										  mObjectUUID,
										  perm,
										  LLUUID::null,
										  LLAssetType::AT_LSL_TEXT,
										  LLInventoryType::IT_LSL,
										  DEFAULT_SCRIPT_NAME,
										  DEFAULT_SCRIPT_DESC,
										  LLSaleInfo::DEFAULT,
										  LLInventoryItemFlags::II_FLAGS_NONE,
										  time_corrected());
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}

	requestExperiences();
}

// static
void LLLiveLSLEditor::onLoadComplete(LLVFS *vfs, const LLUUID& asset_id,
									 LLAssetType::EType type,
									 void* user_data, S32 status, LLExtStat ext_status)
{
	LL_DEBUGS() << "LLLiveLSLEditor::onLoadComplete: got uuid " << asset_id
		 << LL_ENDL;
	LLSD* floater_key = (LLSD*)user_data;
	
	LLLiveLSLEditor* instance = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", *floater_key);
	
	if(instance )
	{
		if( LL_ERR_NOERR == status )
		{
			instance->loadScriptText(vfs, asset_id, type);
			instance->mScriptEd->setEnableEditing(TRUE);
			instance->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotificationsUtil::add("ScriptMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotificationsUtil::add("ScriptNoPermissions");
			}
			else
			{
				LLNotificationsUtil::add("UnableToLoadScript");
			}
			instance->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}

	delete floater_key;
}

void LLLiveLSLEditor::loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type)
{
	LLVFile file(vfs, uuid, type);
	S32 file_length = file.getSize();
	std::vector<char> buffer(file_length + 1);
	file.read((U8*)&buffer[0], file_length);

	if (file.getLastBytesRead() != file_length ||
		file_length <= 0)
	{
		LL_WARNS() << "Error reading " << uuid << ":" << type << LL_ENDL;
	}

	buffer[file_length] = '\0';

	mScriptEd->setScriptText(LLStringExplicit(&buffer[0]), TRUE);
	mScriptEd->mEditor->makePristine();
	mScriptEd->setScriptName(getItem()->getName());
}


void LLLiveLSLEditor::onRunningCheckboxClicked( LLUICtrl*, void* userdata )
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;
	LLViewerObject* object = gObjectList.findObject( self->mObjectUUID );
	LLCheckBoxCtrl* runningCheckbox = self->getChild<LLCheckBoxCtrl>("running");
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
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectUUID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemUUID);
		msg->addBOOLFast(_PREHASH_Running, running);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		runningCheckbox->set(!running);
		LLNotificationsUtil::add("CouldNotStartStopScript");
	}
}

void LLLiveLSLEditor::onReset(void *userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*) userdata;

	LLViewerObject* object = gObjectList.findObject( self->mObjectUUID );
	if(object)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ScriptReset);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Script);
		msg->addUUIDFast(_PREHASH_ObjectID, self->mObjectUUID);
		msg->addUUIDFast(_PREHASH_ItemID, self->mItemUUID);
		msg->sendReliable(object->getRegion()->getHost());
	}
	else
	{
		LLNotificationsUtil::add("CouldNotStartStopScript"); 
	}
}

void LLLiveLSLEditor::draw()
{
	LLViewerObject* object = gObjectList.findObject(mObjectUUID);
	LLCheckBoxCtrl* runningCheckbox = getChild<LLCheckBoxCtrl>( "running");
	if(object && mAskedForRunningInfo && mHaveRunningInfo)
	{
		if(object->permAnyOwner())
		{
			runningCheckbox->setLabel(getString("script_running"));
			runningCheckbox->setEnabled(!mIsSaving);
		}
		else
		{
			runningCheckbox->setLabel(getString("public_objects_can_not_run"));
			runningCheckbox->setEnabled(FALSE);

			// *FIX: Set it to false so that the ui is correct for
			// a box that is released to public. It could be
			// incorrect after a release/claim cycle, but will be
			// correct after clicking on it.
			runningCheckbox->set(FALSE);
			mMonoCheckbox->set(FALSE);
		}
	}
	else if(!object)
	{
		// HACK: Display this information in the title bar.
		// Really ought to put in main window.
		setTitle(LLTrans::getString("ObjectOutOfRange"));
		runningCheckbox->setEnabled(FALSE);
		mMonoCheckbox->setEnabled(FALSE);
		// object may have fallen out of range.
		mHaveRunningInfo = FALSE;
	}

	LLPreview::draw();
}


void LLLiveLSLEditor::onSearchReplace(void* userdata)
{
	LLLiveLSLEditor* self = (LLLiveLSLEditor*)userdata;

	LLScriptEdCore* sec = self->mScriptEd; 
	LLFloaterScriptSearch::show(sec);
}

struct LLLiveLSLSaveData
{
	LLLiveLSLSaveData(const LLUUID& id, const LLViewerInventoryItem* item, BOOL active);
	LLUUID mSaveObjectID;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL mActive;
};

LLLiveLSLSaveData::LLLiveLSLSaveData(const LLUUID& id,
									 const LLViewerInventoryItem* item,
									 BOOL active) :
	mSaveObjectID(id),
	mActive(active)
{
	llassert(item);
	mItem = new LLViewerInventoryItem(item);
}


/*static*/
void LLLiveLSLEditor::finishLSLUpload(LLUUID itemId, LLUUID taskId, LLUUID newAssetId, LLSD response, bool isRunning)
{
    LLSD floater_key;
    floater_key["taskid"] = taskId;
    floater_key["itemid"] = itemId;

    LLLiveLSLEditor* preview = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", floater_key);
    if (preview)
    {
        preview->mItem->setAssetUUID(newAssetId);

        // Bytecode save completed
        if (response["compiled"])
        {
            preview->callbackLSLCompileSucceeded(taskId, itemId, isRunning);
        }
        else
        {
            preview->callbackLSLCompileFailed(response["errors"]);
        }
    }

}


// virtual
void LLLiveLSLEditor::saveIfNeeded(bool sync /*= true*/)
{
	LLViewerObject* object = gObjectList.findObject(mObjectUUID);
	if(!object)
	{
		LLNotificationsUtil::add("SaveScriptFailObjectNotFound");
		return;
	}

    if (mItem.isNull() || !mItem->isFinished())
	{
		// $NOTE: While the error message may not be exactly correct,
		// it's pretty close.
		LLNotificationsUtil::add("SaveScriptFailObjectNotFound");
		return;
	}

    // get the latest info about it. We used to be losing the script
    // name on save, because the viewer object version of the item,
    // and the editor version would get out of synch. Here's a good
    // place to synch them back up.
    LLInventoryItem* inv_item = dynamic_cast<LLInventoryItem*>(object->getInventoryObject(mItemUUID));
    if (inv_item)
    {
        mItem->copyItem(inv_item);
    }

    // Don't need to save if we're pristine
    if(!mScriptEd->hasChanged())
    {
        return;
    }

    mPendingUploads = 0;

    // save the script
    mScriptEd->enableSave(FALSE);
    mScriptEd->mEditor->makePristine();
    mScriptEd->mErrorList->deleteAllItems();
    mScriptEd->mEditor->makePristine();

    if (sync)
    {
        mScriptEd->sync();
    }
    bool isRunning = getChild<LLCheckBoxCtrl>("running")->get();
    getWindow()->incBusyCount();
    mPendingUploads++;

    std::string url = object->getRegion()->getCapability("UpdateScriptTask");

    if (!url.empty())
    {
        std::string buffer(mScriptEd->mEditor->getText());
        LLBufferedAssetUploadInfo::taskUploadFinish_f proc = boost::bind(&LLLiveLSLEditor::finishLSLUpload, _1, _2, _3, _4, isRunning);

        LLResourceUploadInfo::ptr_t uploadInfo(new LLScriptAssetUpload(mObjectUUID, mItemUUID, 
                monoChecked() ? LLScriptAssetUpload::MONO : LLScriptAssetUpload::LSL2, 
                isRunning, mScriptEd->getAssociatedExperience(), buffer, proc));

        LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
    }
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
		closeFloater();
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
	if(self)
	{
		self->mCloseAfterSave = close_after_save;
		self->mScriptEd->mErrorList->setCommentText("");
		self->saveIfNeeded();
	}
}


// static
void LLLiveLSLEditor::processScriptRunningReply(LLMessageSystem* msg, void**)
{
	LLUUID item_id;
	LLUUID object_id;
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_Script, _PREHASH_ItemID, item_id);

	LLSD floater_key;
	floater_key["taskid"] = object_id;
	floater_key["itemid"] = item_id;
	LLLiveLSLEditor* instance = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", floater_key);
	if(instance)
	{
		instance->mHaveRunningInfo = TRUE;
		BOOL running;
		msg->getBOOLFast(_PREHASH_Script, _PREHASH_Running, running);
		LLCheckBoxCtrl* runningCheckbox = instance->getChild<LLCheckBoxCtrl>("running");
		runningCheckbox->set(running);
		BOOL mono;
		msg->getBOOLFast(_PREHASH_Script, "Mono", mono);
		LLCheckBoxCtrl* monoCheckbox = instance->getChild<LLCheckBoxCtrl>("mono");
		monoCheckbox->setEnabled(instance->getIsModifiable() && have_script_upload_cap(object_id));
		monoCheckbox->set(mono);
	}
}


void LLLiveLSLEditor::onMonoCheckboxClicked(LLUICtrl*, void* userdata)
{
	LLLiveLSLEditor* self = static_cast<LLLiveLSLEditor*>(userdata);
	self->mMonoCheckbox->setEnabled(have_script_upload_cap(self->mObjectUUID));
	self->mScriptEd->enableSave(self->getIsModifiable());
}

BOOL LLLiveLSLEditor::monoChecked() const
{
	if(NULL != mMonoCheckbox)
	{
		return mMonoCheckbox->getValue()? TRUE : FALSE;
	}
	return FALSE;
}

void LLLiveLSLEditor::setAssociatedExperience( LLHandle<LLLiveLSLEditor> editor, const LLSD& experience )
{
	LLLiveLSLEditor* scriptEd = editor.get();
	if(scriptEd)
	{
		LLUUID id;
		if(experience.has(LLExperienceCache::EXPERIENCE_ID))
		{
			id=experience[LLExperienceCache::EXPERIENCE_ID].asUUID();
		}
		scriptEd->mScriptEd->setAssociatedExperience(id);
		scriptEd->updateExperiencePanel();
	}
}
