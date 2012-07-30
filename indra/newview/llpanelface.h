/** 
 * @file llpanelface.h
 * @brief Panel in the tools floater for editing face textures, colors, etc.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLPANELFACE_H
#define LL_LLPANELFACE_H

#include "v4color.h"
#include "llpanel.h"

class LLButton;
class LLCheckBoxCtrl;
class LLColorSwatchCtrl;
class LLComboBox;
class LLInventoryItem;
class LLLineEditor;
class LLSpinCtrl;
class LLTextBox;
class LLTextureCtrl;
class LLUICtrl;
class LLViewerObject;
class LLFloater;

class LLPanelFace : public LLPanel
{
public:
	virtual BOOL	postBuild();
	LLPanelFace();
	virtual ~LLPanelFace();

	void			refresh();
	void			setMediaURL(const std::string& url);
	void			setMediaType(const std::string& mime_type);

protected:
	void			getState();

	void			sendTexture();			// applies and sends texture
	void			sendTextureInfo();		// applies and sends texture scale, offset, etc.
	void			sendColor();			// applies and sends color
	void			sendAlpha();			// applies and sends transparency
	void			sendBump();				// applies and sends bump map
	void			sendTexGen();				// applies and sends bump map
	void			sendShiny();			// applies and sends shininess
	void			sendFullbright();		// applies and sends full bright
	void            sendGlow();
	void			sendMedia();

	// this function is to return TRUE if the drag should succeed.
	static BOOL onDragTexture(LLUICtrl* ctrl, LLInventoryItem* item);

	void 	onCommitTexture(const LLSD& data);
	void 	onCancelTexture(const LLSD& data);
	void 	onSelectTexture(const LLSD& data);
	void 	onCommitColor(const LLSD& data);
	void 	onCommitAlpha(const LLSD& data);
	void 	onCancelColor(const LLSD& data);
	void 	onSelectColor(const LLSD& data);
	
	static 	void onCommitTextureInfo( 		LLUICtrl* ctrl, void* userdata);
	static void		onCommitBump(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitTexGen(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitShiny(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitFullbright(		LLUICtrl* ctrl, void* userdata);
	static void     onCommitGlow(           LLUICtrl* ctrl, void *userdata);
	static void		onCommitPlanarAlign(	LLUICtrl* ctrl, void* userdata);
	
	static void		onClickApply(void*);
	static void		onClickAutoFix(void*);
	static F32      valueGlow(LLViewerObject* object, S32 face);

private:

	/*
	 * Checks whether the selected texture from the LLFloaterTexturePicker can be applied to the currently selected object.
	 * If agent selects texture which is not allowed to be applied for the currently selected object,
	 * all controls of the floater texture picker which allow to apply the texture will be disabled.
	 */
	void onTextureSelectionChanged(LLInventoryItem* itemp);

};

#endif
