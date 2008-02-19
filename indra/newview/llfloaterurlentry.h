/** 
 * @file llfloaternamedesc.h
 * @brief LLFloaterNameDesc class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERURLENTRY_H
#define LL_LLFLOATERURLENTRY_H

#include "llfloater.h"
#include "llpanellandmedia.h"

class LLLineEditor;
class LLComboBox;

class LLFloaterURLEntry : public LLFloater
{
public:
	// Can only be shown by LLPanelLandMedia, and pushes data back into
	// that panel via the handle.
	static LLHandle<LLFloater> show(LLHandle<LLPanel> panel_land_media_handle);

	void updateFromLandMediaPanel();

	void headerFetchComplete(U32 status, const std::string& mime_type);
	
	bool addURLToCombobox(const std::string& media_url);

private:
	LLFloaterURLEntry(LLHandle<LLPanel> parent);
	/*virtual*/ ~LLFloaterURLEntry();
	void buildURLHistory();

private:
	LLComboBox*		mMediaURLEdit;
	LLHandle<LLPanel> mPanelLandMediaHandle;

	static void		onBtnOK(void*);
	static void		onBtnCancel(void*);
	static void		onBtnClear(void*);
	static void		callback_clear_url_list(S32 option, void* userdata);
};

#endif  // LL_LLFLOATERURLENTRY_H
