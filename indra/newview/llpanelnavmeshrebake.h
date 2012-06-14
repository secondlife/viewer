/**
 * @file llpanelenavmeshrebake.h
 * @author prep
 * @brief handles the buttons for navmesh rebaking
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

#ifndef LL_NAVMESHREBAKE_H
#define LL_NAVMESHREBAKE_H


class LLPanelNavMeshRebake : public LLPanel
{

	LOG_CLASS(LLPanelNavMeshRebake);

public:

	typedef enum navmesh_rebake_mode_t
	{
		NMRM_Visible,
		NMRM_Visible_Waiting_Response,
		NVRM_Hiddeb
	} ESNavMeshRebakeMode;

	void reparent( LLView* rootp );
	void resetButtonStates();
	static LLPanelNavMeshRebake* getInstance();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible( BOOL visible );

	
	/*virtual*/ void draw(){/*updatePosition(); */LLPanel::draw();}
	/*virtual*/ BOOL handleToolTip( S32 x, S32 y, MASK mask );

protected:

	LLPanelNavMeshRebake();

private:
	static LLPanelNavMeshRebake* getPanel();
	void onNavMeshRebakeClick();
	//void updatePosition();

	LLButton* mNavMeshRebakeButton;
	LLButton* mNavMeshBakingButton;	

	LLHandle<LLPanel> mOriginalParent;	
	
};

#endif //LL_NAVMESHREBAKE_H

