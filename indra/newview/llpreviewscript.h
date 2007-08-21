/** 
 * @file llpreviewscript.h
 * @brief LLPreviewScript class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPREVIEWSCRIPT_H
#define LL_LLPREVIEWSCRIPT_H

#include "lldarray.h"
#include "llpreview.h"
#include "lltabcontainer.h"
#include "llinventory.h"
#include "llcombobox.h"
#include "lliconctrl.h"
#include "llframetimer.h"


class LLMessageSystem;
class LLTextEditor;
class LLButton;
class LLCheckBoxCtrl;
class LLScrollListCtrl;
class LLViewerObject;
struct 	LLEntryAndEdCore;
class LLMenuBarGL;
class LLFloaterScriptSearch;
class LLKeywordToken;

// Inner, implementation class.  LLPreviewScript and LLLiveScriptEditor each own one of these.
class LLScriptEdCore : public LLPanel
{
	friend class LLPreviewScript;
	friend class LLPreviewLSL;
	friend class LLLiveScriptEditor;
	friend class LLLiveLSLEditor;
	friend class LLFloaterScriptSearch;

public:
	LLScriptEdCore(
		const std::string& name,
		const LLRect& rect,
		const std::string& sample,
		const std::string& help,
		const LLViewHandle& floater_handle,
		void (*load_callback)(void* userdata),
		void (*save_callback)(void* userdata, BOOL close_after_save),
		void (*search_replace_callback)(void* userdata),
		void* userdata,
		S32 bottom_pad = 0);	// pad below bottom row of buttons
	~LLScriptEdCore();
	
	void			initMenu();

	virtual void	draw();

	BOOL			canClose();

	static void		handleSaveChangesDialog(S32 option, void* userdata);
	static void		handleReloadFromServerDialog(S32 option, void* userdata);

	static void		onHelpWebDialog(S32 option, void* userdata);
	static void		onBtnHelp(void* userdata);
	static void		onBtnDynamicHelp(void* userdata);
	static void		onCheckLock(LLUICtrl*, void*);
	static void		onHelpComboCommit(LLUICtrl* ctrl, void* userdata);
	static void		onClickBack(void* userdata);
	static void		onClickForward(void* userdata);
	static void		onBtnInsertSample(void*);
	static void		onBtnInsertFunction(LLUICtrl*, void*);
	static void		doSave( void* userdata, BOOL close_after_save );
	static void		onBtnSave(void*);
	static void		onBtnUndoChanges(void*);
	static void		onSearchMenu(void* userdata);

	static void		onUndoMenu(void* userdata);
	static void		onRedoMenu(void* userdata);
	static void		onCutMenu(void* userdata);
	static void		onCopyMenu(void* userdata);
	static void		onPasteMenu(void* userdata);
	static void		onSelectAllMenu(void* userdata);
	static void		onDeselectMenu(void* userdata);

	static BOOL		enableUndoMenu(void* userdata);
	static BOOL		enableRedoMenu(void* userdata);
	static BOOL		enableCutMenu(void* userdata);
	static BOOL		enableCopyMenu(void* userdata);
	static BOOL		enablePasteMenu(void* userdata);
	static BOOL		enableSelectAllMenu(void* userdata);
	static BOOL		enableDeselectMenu(void* userdata);

	static BOOL		hasChanged(void* userdata);

	void selectFirstError();

	virtual BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);

protected:
	void deleteBridges();
	void setHelpPage(const LLString& help_string);
	void updateDynamicHelp(BOOL immediate = FALSE);
	void addHelpItemToHistory(const LLString& help_string);

	static void onErrorList(LLUICtrl*, void* user_data);

private:
	LLString		mSampleText;
	std::string		mHelpFile;
	LLTextEditor*	mEditor;
	void			(*mLoadCallback)(void* userdata);
	void			(*mSaveCallback)(void* userdata, BOOL close_after_save);
	void			(*mSearchReplaceCallback) (void* userdata);
	void*			mUserdata;
	LLComboBox		*mFunctions;
	BOOL			mForceClose;
	//LLPanel*		mGuiPanel;
	LLPanel*		mCodePanel;
	LLScrollListCtrl* mErrorList;
	LLDynamicArray<LLEntryAndEdCore*> mBridges;
	LLViewHandle	mLiveHelpHandle;
	LLKeywordToken* mLastHelpToken;
	LLFrameTimer	mLiveHelpTimer;
	S32				mLiveHelpHistorySize;
};


// Used to view and edit a LSL from your inventory.
class LLPreviewLSL : public LLPreview
{
public:
	LLPreviewLSL(const std::string& name, const LLRect& rect, const std::string& title,
				 const LLUUID& item_uuid );
	virtual void callbackLSLCompileSucceeded();
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

	/*virtual*/ void open();		/*Flawfinder: ignore*/

protected:
	virtual BOOL canClose();
	void closeIfNeeded();
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual void loadAsset();
	void saveIfNeeded();
	void uploadAssetViaCaps(const std::string& url,
							const std::string& filename, 
							const LLUUID& item_id);
	void uploadAssetLegacy(const std::string& filename,
							const LLUUID& item_id,
							const LLTransactionID& tid);

	static void onSearchReplace(void* userdata);
	static void onLoad(void* userdata);
	static void onSave(void* userdata, BOOL close_after_save);
	
	static void onLoadComplete(LLVFS *vfs, const LLUUID& uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveComplete(const LLUUID& uuid, void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status);
	static LLPreviewLSL* getInstance(const LLUUID& uuid);

	static void* createScriptEdPanel(void* userdata);


protected:
	LLScriptEdCore* mScriptEd;
	// Can safely close only after both text and bytecode are uploaded
	S32 mPendingUploads;
};


// Used to view and edit an LSL that is attached to an object.
class LLLiveLSLEditor : public LLPreview
{
public: 
	LLLiveLSLEditor(const std::string& name, const LLRect& rect,
					const std::string& title,
					const LLUUID& object_id, const LLUUID& item_id);
	~LLLiveLSLEditor();


	static LLLiveLSLEditor* show(const LLUUID& item_id, const LLUUID& object_id);
	static void hide(const LLUUID& item_id, const LLUUID& object_id);
	static LLLiveLSLEditor* find(const LLUUID& item_id, const LLUUID& object_id);

	static void processScriptRunningReply(LLMessageSystem* msg, void**);

	virtual void callbackLSLCompileSucceeded(const LLUUID& task_id,
											const LLUUID& item_id,
											bool is_script_running);
	virtual void callbackLSLCompileFailed(const LLSD& compile_errors);

protected:
	virtual BOOL canClose();
	void closeIfNeeded();
	virtual void draw();
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual void loadAsset();
	void loadAsset(BOOL is_new);
	void saveIfNeeded();
	void uploadAssetViaCaps(const std::string& url,
							const std::string& filename, 
							const LLUUID& task_id,
							const LLUUID& item_id,
							BOOL is_running);
	void uploadAssetLegacy(const std::string& filename,
						   LLViewerObject* object,
						   const LLTransactionID& tid,
						   BOOL is_running);

	static void onSearchReplace(void* userdata);
	static void onLoad(void* userdata);
	static void onSave(void* userdata, BOOL close_after_save);

	static void onLoadComplete(LLVFS *vfs, const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveTextComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveBytecodeComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status);
	static void onRunningCheckboxClicked(LLUICtrl*, void* userdata);
	static void onReset(void* userdata);

	void loadScriptText(const char* filename);
	void loadScriptText(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type);

	static void onErrorList(LLUICtrl*, void* user_data);

	static void* createScriptEdPanel(void* userdata);


protected:
	LLUUID mObjectID;
	LLUUID mItemID; // The inventory item this script is associated with
	BOOL mIsNew;
	LLScriptEdCore* mScriptEd;
	//LLUUID mTransmitID;
	LLCheckBoxCtrl	*mRunningCheckbox;
	BOOL mAskedForRunningInfo;
	BOOL mHaveRunningInfo;
	LLButton		*mResetButton;
	LLPointer<LLViewerInventoryItem> mItem;
	BOOL mCloseAfterSave;
	// need to save both text and script, so need to decide when done
	S32 mPendingUploads;

	static LLMap<LLUUID, LLLiveLSLEditor*> sInstances;
};

// name of help file for lsl
extern const char HELP_LSL[];

#endif  // LL_LLPREVIEWSCRIPT_H
