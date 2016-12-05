/** 
 * @file llpreviewscript.h
 * @brief LLPreviewScript class definition
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

#ifndef LL_LLPREVIEWSCRIPT_H
#define LL_LLPREVIEWSCRIPT_H

#include "llpreview.h"
#include "lltabcontainer.h"
#include "llinventory.h"
#include "llcombobox.h"
#include "lliconctrl.h"
#include "llframetimer.h"
#include "llfloatergotoline.h"
#include "llsyntaxid.h"

class LLLiveLSLFile;
class LLMessageSystem;
class LLScriptEditor;
class LLButton;
class LLCheckBoxCtrl;
class LLScrollListCtrl;
class LLViewerObject;
struct 	LLEntryAndEdCore;
class LLMenuBarGL;
class LLFloaterScriptSearch;
class LLKeywordToken;
class LLVFS;
class LLViewerInventoryItem;
class LLScriptEdContainer;
class LLFloaterGotoLine;
class LLFloaterExperienceProfile;

// Inner, implementation class.  LLPreviewScript and LLLiveLSLEditor each own one of these.
class LLScriptEdCore : public LLPanel
{
	friend class LLPreviewScript;
	friend class LLPreviewLSL;
	friend class LLLiveLSLEditor;
	friend class LLFloaterScriptSearch;
	friend class LLScriptEdContainer;
	friend class LLFloaterGotoLine;

protected:
	// Supposed to be invoked only by the container.
	LLScriptEdCore(
		LLScriptEdContainer* container,
		const std::string& sample,
		const LLHandle<LLFloater>& floater_handle,
		void (*load_callback)(void* userdata),
		void (*save_callback)(void* userdata, BOOL close_after_save),
		void (*search_replace_callback)(void* userdata),
		void* userdata,
		bool live,
		S32 bottom_pad = 0);	// pad below bottom row of buttons
public:
	~LLScriptEdCore();
	
	void			initializeKeywords();
	void			initMenu();
	void			processKeywords();
	void			processLoaded();

	virtual void	draw();
	/*virtual*/	BOOL	postBuild();
	BOOL			canClose();
	void			setEnableEditing(bool enable);
	bool			canLoadOrSaveToFile( void* userdata );

	void            setScriptText(const std::string& text, BOOL is_valid);
	bool			loadScriptText(const std::string& filename);
	bool			writeToFile(const std::string& filename);
	void			sync();
	
	void			doSave( BOOL close_after_save );

	bool			handleSaveChangesDialog(const LLSD& notification, const LLSD& response);
	bool			handleReloadFromServerDialog(const LLSD& notification, const LLSD& response);

	void			openInExternalEditor();

	static void		onCheckLock(LLUICtrl*, void*);
	static void		onHelpComboCommit(LLUICtrl* ctrl, void* userdata);
	static void		onClickBack(void* userdata);
	static void		onClickForward(void* userdata);
	static void		onBtnInsertSample(void*);
	static void		onBtnInsertFunction(LLUICtrl*, void*);
	static void		onBtnLoadFromFile(void*);
    static void		onBtnSaveToFile(void*);

	static bool		enableSaveToFileMenu(void* userdata);
	static bool		enableLoadFromFileMenu(void* userdata);

    virtual bool	hasAccelerators() const { return true; }
	LLUUID 			getAssociatedExperience()const;
	void            setAssociatedExperience( const LLUUID& experience_id );

	void 			setScriptName(const std::string& name){mScriptName = name;};

	void 			setItemRemoved(bool script_removed){mScriptRemoved = script_removed;};

private:
	void		onBtnHelp();
	void		onBtnDynamicHelp();
	void		onBtnUndoChanges();

	bool		hasChanged();

	void selectFirstError();

	virtual BOOL handleKeyHere(KEY key, MASK mask);
	
	void enableSave(BOOL b) {mEnableSave = b;}

protected:
	void deleteBridges();
	void setHelpPage(const std::string& help_string);
	void updateDynamicHelp(BOOL immediate = FALSE);
	bool isKeyword(LLKeywordToken* token);
	void addHelpItemToHistory(const std::string& help_string);
	static void onErrorList(LLUICtrl*, void* user_data);

	bool			mLive;

private:
	std::string		mSampleText;
	std::string		mScriptName;
	LLScriptEditor*	mEditor;
	void			(*mLoadCallback)(void* userdata);
	void			(*mSaveCallback)(void* userdata, BOOL close_after_save);
	void			(*mSearchReplaceCallback) (void* userdata);
    void*			mUserdata;
    LLComboBox		*mFunctions;
	BOOL			mForceClose;
	LLPanel*		mCodePanel;
	LLScrollListCtrl* mErrorList;
	std::vector<LLEntryAndEdCore*> mBridges;
	LLHandle<LLFloater>	mLiveHelpHandle;
	LLKeywordToken* mLastHelpToken;
	LLFrameTimer	mLiveHelpTimer;
	S32				mLiveHelpHistorySize;
	BOOL			mEnableSave;
	BOOL			mHasScriptData;
	LLLiveLSLFile*	mLiveFile;
	LLUUID			mAssociatedExperience;
	BOOL			mScriptRemoved;

	LLScriptEdContainer* mContainer; // parent view

public:
	boost::signals2::connection mSyntaxIDConnection;

};

class LLScriptEdContainer : public LLPreview
{
	friend class LLScriptEdCore;

public:
	LLScriptEdContainer(const LLSD& key);
	LLScriptEdContainer(const LLSD& key, const bool live);

protected:
	std::string		getTmpFileName();
	bool			onExternalChange(const std::string& filename);
	virtual void	saveIfNeeded(bool sync = true) = 0;

	LLScriptEdCore*		mScriptEd;
};

// Used to view and edit an LSL script from your inventory.
class LLPreviewLSL : public LLScriptEdContainer
{
public:
	LLPreviewLSL(const LLSD& key );
	virtual void callbackLSLCompileSucceeded();
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

	/*virtual*/ BOOL postBuild();

protected:
	virtual void draw();
	virtual BOOL canClose();
	void closeIfNeeded();

	virtual void loadAsset();
	/*virtual*/ void saveIfNeeded(bool sync = true);

	static void onSearchReplace(void* userdata);
	static void onLoad(void* userdata);
	static void onSave(void* userdata, BOOL close_after_save);
	
	static void onLoadComplete(LLVFS *vfs, const LLUUID& uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);

protected:
	static void* createScriptEdPanel(void* userdata);

    static void finishedLSLUpload(LLUUID itemId, LLSD response);
protected:

	// Can safely close only after both text and bytecode are uploaded
	S32 mPendingUploads;

};


// Used to view and edit an LSL script that is attached to an object.
class LLLiveLSLEditor : public LLScriptEdContainer
{
	friend class LLLiveLSLFile;
public: 
	LLLiveLSLEditor(const LLSD& key);


	static void processScriptRunningReply(LLMessageSystem* msg, void**);

	virtual void callbackLSLCompileSucceeded(const LLUUID& task_id,
											const LLUUID& item_id,
											bool is_script_running);
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

	/*virtual*/ BOOL postBuild();
	
    void setIsNew() { mIsNew = TRUE; }

	static void setAssociatedExperience( LLHandle<LLLiveLSLEditor> editor, const LLSD& experience );
	static void onToggleExperience(LLUICtrl *ui, void* userdata);
	static void onViewProfile(LLUICtrl *ui, void* userdata);

	void setExperienceIds(const LLSD& experience_ids);
	void buildExperienceList();
	void updateExperiencePanel();
	void requestExperiences();
	void experienceChanged();
	void addAssociatedExperience(const LLSD& experience);
	
private:
	virtual BOOL canClose();
	void closeIfNeeded();
	virtual void draw();

	virtual void loadAsset();
	void loadAsset(BOOL is_new);
	/*virtual*/ void saveIfNeeded(bool sync = true);
	BOOL monoChecked() const;


	static void onSearchReplace(void* userdata);
	static void onLoad(void* userdata);
	static void onSave(void* userdata, BOOL close_after_save);

	static void onLoadComplete(LLVFS *vfs, const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);
	static void onRunningCheckboxClicked(LLUICtrl*, void* userdata);
	static void onReset(void* userdata);

	void loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type);

	static void onErrorList(LLUICtrl*, void* user_data);

	static void* createScriptEdPanel(void* userdata);

	static void	onMonoCheckboxClicked(LLUICtrl*, void* userdata);

    static void finishLSLUpload(LLUUID itemId, LLUUID taskId, LLUUID newAssetId, LLSD response, bool isRunning);
    static void receiveExperienceIds(LLSD result, LLHandle<LLLiveLSLEditor> parent);

private:
	bool				mIsNew;
	//LLUUID mTransmitID;
	LLCheckBoxCtrl*		mRunningCheckbox;
	BOOL				mAskedForRunningInfo;
	BOOL				mHaveRunningInfo;
	LLButton*			mResetButton;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL				mCloseAfterSave;
	// need to save both text and script, so need to decide when done
	S32					mPendingUploads;

	BOOL                mIsSaving;

	BOOL getIsModifiable() const { return mIsModifiable; } // Evaluated on load assert

	LLCheckBoxCtrl*	mMonoCheckbox;
	BOOL mIsModifiable;


	LLComboBox*		mExperiences;
	LLCheckBoxCtrl*	mExperienceEnabled;
	LLSD			mExperienceIds;

	LLHandle<LLFloater> mExperienceProfile;
};

#endif  // LL_LLPREVIEWSCRIPT_H
