/** 
 * @file llpanelcontents.h
 * @brief Object contents panel in the tools floater.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELCONTENTS_H
#define LL_LLPANELCONTENTS_H

#include "v3math.h"
#include "llpanel.h"

class LLButton;
class LLPanelInventory;
class LLViewerObject;
class LLCheckBoxCtrl;
class LLSpinCtrl;

class LLPanelContents : public LLPanel
{
public:
	virtual	BOOL postBuild();
	LLPanelContents(const std::string& name);
	virtual ~LLPanelContents();

	void			refresh();

	static void		onClickNewScript(		void* userdata);

protected:
	void			getState(LLViewerObject *object);

public:
	LLPanelInventory* mPanelInventory;
};

#endif
