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
#include "llmaterial.h"

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
class LLMaterialID;

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
	void 	onCommitMaterialTexture(const LLSD& data);
	void 	onCancelMaterialTexture(const LLSD& data);
	void 	onSelectMaterialTexture(const LLSD& data);
	void 	onCommitColor(const LLSD& data);
	void 	onCommitShinyColor(const LLSD& data);
	void 	onCommitAlpha(const LLSD& data);
	void 	onCancelColor(const LLSD& data);
	void 	onSelectColor(const LLSD& data);
	void    onMaterialLoaded(const LLMaterialPtr material);
	void    updateMaterial();
	
	static 	void onCommitTextureInfo( 		LLUICtrl* ctrl, void* userdata);
	static void		onCommitMaterial(		LLUICtrl* ctrl, void* userdata);
	static void		onCommitMaterialsMedia(		LLUICtrl* ctrl, void* userdata);
	static void		onCommitMaterialType(		LLUICtrl* ctrl, void* userdata);
	static void		onCommitBump(			LLUICtrl* ctrl, void* userdata);
	static void		onCommitTexGen(			LLUICtrl* ctrl, void* userdata);
	static void		updateShinyControls(		LLUICtrl* ctrl, void* userdata, bool mess_with_combobox = false);
	static void		updateBumpyControls(		LLUICtrl* ctrl, void* userdata, bool mess_with_combobox = false);
	static void		onCommitShiny(			LLUICtrl* ctrl, void* userdata);
	static void		updateAlphaControls(		LLUICtrl* ctrl, void* userdata);
	static void		onCommitAlphaMode(		LLUICtrl* ctrl, void* userdata);
	static void		onCommitFullbright(		LLUICtrl* ctrl, void* userdata);
	static void     onCommitGlow(           LLUICtrl* ctrl, void *userdata);
	static void		onCommitPlanarAlign(	LLUICtrl* ctrl, void* userdata);
	
	static void		onCommitRepeatsPerMeter(	LLUICtrl* ctrl, void* userinfo);
	static void		onClickAutoFix(void*);
	static F32      valueGlow(LLViewerObject* object, S32 face);

private:

	/*
	 * Checks whether the selected texture from the LLFloaterTexturePicker can be applied to the currently selected object.
	 * If agent selects texture which is not allowed to be applied for the currently selected object,
	 * all controls of the floater texture picker which allow to apply the texture will be disabled.
	 */
	void onTextureSelectionChanged(LLInventoryItem* itemp);

	LLMaterialPtr mMaterial;
	bool mIsAlpha;
	
	/* These variables interlock processing of materials updates sent to
	 * the sim.  mUpdateInFlight is set to flag that an update has been
	 * sent to the sim and not acknowledged yet, and cleared when an
	 * update is received from the sim.  mUpdatePending is set when
	 * there's an update in flight and another UI change has been made
	 * that needs to be sent as a materials update, and cleared when the
	 * update is sent.  This prevents the sim from getting spammed with
	 * update messages when, for example, the user holds down the
	 * up-arrow on a spinner, and avoids running afoul of its throttle.
	 */
	bool mUpdateInFlight;
	bool mUpdatePending;
};

#endif

