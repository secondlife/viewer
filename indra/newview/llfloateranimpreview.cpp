/** 
 * @file llfloateranimpreview.cpp
 * @brief LLFloaterAnimPreview class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloateranimpreview.h"

#include "llbvhloader.h"
#include "lldatapacker.h"
#include "lldir.h"
#include "lleconomy.h"
#include "llnotificationsutil.h"
#include "llvfile.h"
#include "llapr.h"
#include "llstring.h"

#include "llagent.h"
#include "llanimationstates.h"
#include "llbbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llrender.h"
#include "llface.h"
#include "llfocusmgr.h"
#include "llkeyframemotion.h"
#include "lllineeditor.h"
#include "llfloaterperms.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "llvoavatar.h"
#include "pipeline.h"
#include "lluictrlfactory.h"
#include "lltrans.h"

const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREF_BUTTON_HEIGHT = 16;
const S32 PREVIEW_TEXTURE_HEIGHT = 300;

const F32 PREVIEW_CAMERA_DISTANCE = 4.f;

const F32 MIN_CAMERA_ZOOM = 0.5f;
const F32 MAX_CAMERA_ZOOM = 10.f;

const F32 BASE_ANIM_TIME_OFFSET = 5.f;

std::string STATUS[] =
{
	"E_ST_OK",
	"E_ST_EOF",
	"E_ST_NO_CONSTRAINT",
	"E_ST_NO_FILE",
	"E_ST_NO_HIER",
	"E_ST_NO_JOINT",
	"E_ST_NO_NAME",
	"E_ST_NO_OFFSET",
	"E_ST_NO_CHANNELS",
	"E_ST_NO_ROTATION",
	"E_ST_NO_AXIS",
	"E_ST_NO_MOTION",
	"E_ST_NO_FRAMES",
	"E_ST_NO_FRAME_TIME",
	"E_ST_NO_POS",
	"E_ST_NO_ROT",
	"E_ST_NO_XLT_FILE",
	"E_ST_NO_XLT_HEADER",
	"E_ST_NO_XLT_NAME",
	"E_ST_NO_XLT_IGNORE",
	"E_ST_NO_XLT_RELATIVE",
	"E_ST_NO_XLT_OUTNAME",
	"E_ST_NO_XLT_MATRIX",
	"E_ST_NO_XLT_MERGECHILD",
	"E_ST_NO_XLT_MERGEPARENT",
	"E_ST_NO_XLT_PRIORITY",
	"E_ST_NO_XLT_LOOP",
	"E_ST_NO_XLT_EASEIN",
	"E_ST_NO_XLT_EASEOUT",
	"E_ST_NO_XLT_HAND",
	"E_ST_NO_XLT_EMOTE",
	"E_ST_BAD_ROOT"
};

//-----------------------------------------------------------------------------
// LLFloaterAnimPreview()
//-----------------------------------------------------------------------------
LLFloaterAnimPreview::LLFloaterAnimPreview(const std::string& filename) : 
	LLFloaterNameDesc(filename)
{
	mLastMouseX = 0;
	mLastMouseY = 0;

	mIDList["Standing"] = ANIM_AGENT_STAND;
	mIDList["Walking"] = ANIM_AGENT_FEMALE_WALK;
	mIDList["Sitting"] = ANIM_AGENT_SIT_FEMALE;
	mIDList["Flying"] = ANIM_AGENT_HOVER;

	mIDList["[None]"] = LLUUID::null;
	mIDList["Aaaaah"] = ANIM_AGENT_EXPRESS_OPEN_MOUTH;
	mIDList["Afraid"] = ANIM_AGENT_EXPRESS_AFRAID;
	mIDList["Angry"] = ANIM_AGENT_EXPRESS_ANGER;
	mIDList["Big Smile"] = ANIM_AGENT_EXPRESS_TOOTHSMILE;
	mIDList["Bored"] = ANIM_AGENT_EXPRESS_BORED;
	mIDList["Cry"] = ANIM_AGENT_EXPRESS_CRY;
	mIDList["Disdain"] = ANIM_AGENT_EXPRESS_DISDAIN;
	mIDList["Embarrassed"] = ANIM_AGENT_EXPRESS_EMBARRASSED;
	mIDList["Frown"] = ANIM_AGENT_EXPRESS_FROWN;
	mIDList["Kiss"] = ANIM_AGENT_EXPRESS_KISS;
	mIDList["Laugh"] = ANIM_AGENT_EXPRESS_LAUGH;
	mIDList["Plllppt"] = ANIM_AGENT_EXPRESS_TONGUE_OUT;
	mIDList["Repulsed"] = ANIM_AGENT_EXPRESS_REPULSED;
	mIDList["Sad"] = ANIM_AGENT_EXPRESS_SAD;
	mIDList["Shrug"] = ANIM_AGENT_EXPRESS_SHRUG;
	mIDList["Smile"] = ANIM_AGENT_EXPRESS_SMILE;
	mIDList["Surprise"] = ANIM_AGENT_EXPRESS_SURPRISE;
	mIDList["Wink"] = ANIM_AGENT_EXPRESS_WINK;
	mIDList["Worry"] = ANIM_AGENT_EXPRESS_WORRY;
}

//-----------------------------------------------------------------------------
// setAnimCallbacks()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::setAnimCallbacks()
{
	childSetCommitCallback("playback_slider", onSliderMove, this);
	
	childSetCommitCallback("preview_base_anim", onCommitBaseAnim, this);
	childSetValue("preview_base_anim", "Standing");

	childSetCommitCallback("priority", onCommitPriority, this);
	childSetCommitCallback("loop_check", onCommitLoop, this);
	childSetCommitCallback("loop_in_point", onCommitLoopIn, this);
	childSetValidate("loop_in_point", boost::bind(&LLFloaterAnimPreview::validateLoopIn, this, _1));
	childSetCommitCallback("loop_out_point", onCommitLoopOut, this);
	childSetValidate("loop_out_point", boost::bind(&LLFloaterAnimPreview::validateLoopOut, this, _1));

	childSetCommitCallback("hand_pose_combo", onCommitHandPose, this);
	
	childSetCommitCallback("emote_combo", onCommitEmote, this);
	childSetValue("emote_combo", "[None]");

	childSetCommitCallback("ease_in_time", onCommitEaseIn, this);
	childSetValidate("ease_in_time", boost::bind(&LLFloaterAnimPreview::validateEaseIn, this, _1));
	childSetCommitCallback("ease_out_time", onCommitEaseOut, this);
	childSetValidate("ease_out_time", boost::bind(&LLFloaterAnimPreview::validateEaseOut, this, _1));
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::postBuild()
{
	LLKeyframeMotion* motionp = NULL;
	LLBVHLoader* loaderp = NULL;

	if (!LLFloaterNameDesc::postBuild())
	{
		return FALSE;
	}

	childSetCommitCallback("name_form", onCommitName, this);

	childSetAction("ok_btn", onBtnOK, this);
	setDefaultBtn();

	mPreviewRect.set(PREVIEW_HPAD, 
		PREVIEW_TEXTURE_HEIGHT,
		getRect().getWidth() - PREVIEW_HPAD, 
		PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
	mPreviewImageRect.set(0.f, 1.f, 1.f, 0.f);

	mPlayButton = getChild<LLButton>( "play_btn");
	mPlayButton->setClickedCallback(onBtnPlay, this);
	mPlayButton->setVisible(true);

	mPauseButton = getChild<LLButton>( "pause_btn");
	mPauseButton->setClickedCallback(onBtnPause, this);
	mPauseButton->setVisible(false);
	
	mStopButton = getChild<LLButton>( "stop_btn");
	mStopButton->setClickedCallback(onBtnStop, this);

	childHide("bad_animation_text");

	std::string exten = gDirUtilp->getExtension(mFilename);
	if (exten == "bvh")
	{
		// loading a bvh file

		// now load bvh file
		S32 file_size;
		
		LLAPRFile infile ;
		infile.open(mFilenameAndPath, LL_APR_RB, NULL, &file_size);
		
		if (!infile.getFileHandle())
		{
			llwarns << "Can't open BVH file:" << mFilename << llendl;	
		}
		else
		{
			char*	file_buffer;

			file_buffer = new char[file_size + 1];

			if (file_size == infile.read(file_buffer, file_size))
			{
				file_buffer[file_size] = '\0';
				llinfos << "Loading BVH file " << mFilename << llendl;
				ELoadStatus load_status = E_ST_OK;
				S32 line_number = 0; 
				loaderp = new LLBVHLoader(file_buffer, load_status, line_number);
				std::string status = getString(STATUS[load_status]);
				
				if(load_status == E_ST_NO_XLT_FILE)
				{
					llwarns << "NOTE: No translation table found." << llendl;
				}
				else
				{
					llwarns << "ERROR: [line: " << line_number << "] " << status << llendl;
				}
			}

			infile.close() ;
			delete[] file_buffer;
		}
	}

	if (loaderp && loaderp->isInitialized() && loaderp->getDuration() <= MAX_ANIM_DURATION)
	{
		// generate unique id for this motion
		mTransactionID.generate();
		mMotionID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());

		mAnimPreview = new LLPreviewAnimation(256, 256);

		// motion will be returned, but it will be in a load-pending state, as this is a new motion
		// this motion will not request an asset transfer until next update, so we have a chance to 
		// load the keyframe data locally
		motionp = (LLKeyframeMotion*)mAnimPreview->getDummyAvatar()->createMotion(mMotionID);

		// create data buffer for keyframe initialization
		S32 buffer_size = loaderp->getOutputSize();
		U8* buffer = new U8[buffer_size];

		LLDataPackerBinaryBuffer dp(buffer, buffer_size);

		// pass animation data through memory buffer
		loaderp->serialize(dp);
		dp.reset();
		BOOL success = motionp && motionp->deserialize(dp);

		delete []buffer;

		if (success)
		{
			setAnimCallbacks() ;
			
			const LLBBoxLocal &pelvis_bbox = motionp->getPelvisBBox();

			LLVector3 temp = pelvis_bbox.getCenter();
			// only consider XY?
			//temp.mV[VZ] = 0.f;
			F32 pelvis_offset = temp.magVec();

			temp = pelvis_bbox.getExtent();
			//temp.mV[VZ] = 0.f;
			F32 pelvis_max_displacement = pelvis_offset + (temp.magVec() * 0.5f) + 1.f;
			
			F32 camera_zoom = LLViewerCamera::getInstance()->getDefaultFOV() / (2.f * atan(pelvis_max_displacement / PREVIEW_CAMERA_DISTANCE));
		
			mAnimPreview->setZoom(camera_zoom);

			motionp->setName(childGetValue("name_form").asString());
			mAnimPreview->getDummyAvatar()->startMotion(mMotionID);
			
			getChild<LLSlider>("playback_slider")->setMinValue(0.0);
			getChild<LLSlider>("playback_slider")->setMaxValue(1.0);

			childSetValue("loop_check", LLSD(motionp->getLoop()));
			childSetValue("loop_in_point", LLSD(motionp->getLoopIn() / motionp->getDuration() * 100.f));
			childSetValue("loop_out_point", LLSD(motionp->getLoopOut() / motionp->getDuration() * 100.f));
			childSetValue("priority", LLSD((F32)motionp->getPriority()));
			childSetValue("hand_pose_combo", LLHandMotion::getHandPoseName(motionp->getHandPose()));
			childSetValue("ease_in_time", LLSD(motionp->getEaseInDuration()));
			childSetValue("ease_out_time", LLSD(motionp->getEaseOutDuration()));
			setEnabled(TRUE);
			std::string seconds_string;
			seconds_string = llformat(" - %.2f seconds", motionp->getDuration());

			setTitle(mFilename + std::string(seconds_string));
		}
		else
		{
			mAnimPreview = NULL;
			mMotionID.setNull();
			childSetValue("bad_animation_text", getString("failed_to_initialize"));
		}
	}
	else
	{
		if ( loaderp )
		{
			if (loaderp->getDuration() > MAX_ANIM_DURATION)
			{
				LLUIString out_str = getString("anim_too_long");
				out_str.setArg("[LENGTH]", llformat("%.1f", loaderp->getDuration()));
				out_str.setArg("[MAX_LENGTH]", llformat("%.1f", MAX_ANIM_DURATION));
				childSetValue("bad_animation_text", out_str.getString());
			}
			else
			{
				LLUIString out_str = getString("failed_file_read");
				out_str.setArg("[STATUS]", getString(STATUS[loaderp->getStatus()])); 
				childSetValue("bad_animation_text", out_str.getString());
			}
		}

		//setEnabled(FALSE);
		mMotionID.setNull();
		mAnimPreview = NULL;
	}

	refresh();

	delete loaderp;

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLFloaterAnimPreview()
//-----------------------------------------------------------------------------
LLFloaterAnimPreview::~LLFloaterAnimPreview()
{
	mAnimPreview = NULL;

	setEnabled(FALSE);
}

//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::draw()
{
	LLFloater::draw();
	LLRect r = getRect();

	refresh();

	if (mMotionID.notNull() && mAnimPreview)
	{
		gGL.color3f(1.f, 1.f, 1.f);

		gGL.getTexUnit(0)->bind(mAnimPreview);

		gGL.begin( LLRender::QUADS );
		{
			gGL.texCoord2f(0.f, 1.f);
			gGL.vertex2i(PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex2i(PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
		}
		gGL.end();

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		if (!avatarp->areAnimationsPaused())
		{
			mAnimPreview->requestUpdate();
		}
	}
}

//-----------------------------------------------------------------------------
// resetMotion()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::resetMotion()
{
	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	BOOL paused = avatarp->areAnimationsPaused();

	// *TODO: Fix awful casting hack
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
	
	// Set emotion
	std::string emote = childGetValue("emote_combo").asString();
	motionp->setEmote(mIDList[emote]);
	
	LLUUID base_id = mIDList[childGetValue("preview_base_anim").asString()];
	avatarp->deactivateAllMotions();
	avatarp->startMotion(mMotionID, 0.0f);
	avatarp->startMotion(base_id, BASE_ANIM_TIME_OFFSET);
	childSetValue("playback_slider", 0.0f);

	// Set pose
	std::string handpose = childGetValue("hand_pose_combo").asString();
	avatarp->startMotion( ANIM_AGENT_HAND_MOTION, 0.0f );
	motionp->setHandPose(LLHandMotion::getHandPose(handpose));

	if (paused)
	{
		mPauseRequest = avatarp->requestPause();
	}
	else
	{
		mPauseRequest = NULL;	
	}
}

//-----------------------------------------------------------------------------
// handleMouseDown()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mPreviewRect.pointInRect(x, y))
	{
		bringToFront( x, y );
		gFocusMgr.setMouseCapture(this);
		gViewerWindow->hideCursor();
		mLastMouseX = x;
		mLastMouseY = y;
		return TRUE;
	}

	return LLFloater::handleMouseDown(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleMouseUp()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gFocusMgr.setMouseCapture(FALSE);
	gViewerWindow->showCursor();
	return LLFloater::handleMouseUp(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleHover()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::handleHover(S32 x, S32 y, MASK mask)
{
	MASK local_mask = mask & ~MASK_ALT;

	if (mAnimPreview && hasMouseCapture())
	{
		if (local_mask == MASK_PAN)
		{
			// pan here
			mAnimPreview->pan((F32)(x - mLastMouseX) * -0.005f, (F32)(y - mLastMouseY) * -0.005f);
		}
		else if (local_mask == MASK_ORBIT)
		{
			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 pitch_radians = (F32)(y - mLastMouseY) * 0.02f;
			
			mAnimPreview->rotate(yaw_radians, pitch_radians);
		}
		else 
		{
			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 zoom_amt = (F32)(y - mLastMouseY) * 0.02f;
			
			mAnimPreview->rotate(yaw_radians, 0.f);
			mAnimPreview->zoom(zoom_amt);
		}

		mAnimPreview->requestUpdate();

		LLUI::setMousePositionLocal(this, mLastMouseX, mLastMouseY);
	}

	if (!mPreviewRect.pointInRect(x, y) || !mAnimPreview)
	{
		return LLFloater::handleHover(x, y, mask);
	}
	else if (local_mask == MASK_ORBIT)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (local_mask == MASK_PAN)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// handleScrollWheel()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	mAnimPreview->zoom((F32)clicks * -0.2f);
	mAnimPreview->requestUpdate();

	return TRUE;
}

//-----------------------------------------------------------------------------
// onMouseCaptureLost()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onMouseCaptureLost()
{
	gViewerWindow->showCursor();
}

//-----------------------------------------------------------------------------
// onBtnPlay()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onBtnPlay(void* user_data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)user_data;
	if (!previewp->getEnabled())
		return;

	if (previewp->mMotionID.notNull() && previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
		
		if(!avatarp->isMotionActive(previewp->mMotionID))
		{
			previewp->resetMotion();
			previewp->mPauseRequest = NULL;
		}
		else if (avatarp->areAnimationsPaused())
		{			
			previewp->mPauseRequest = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// onBtnPause()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onBtnPause(void* user_data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)user_data;
	if (!previewp->getEnabled())
		return;
	
	if (previewp->mMotionID.notNull() && previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();

		if(avatarp->isMotionActive(previewp->mMotionID))
		{
			if (!avatarp->areAnimationsPaused())
			{
				previewp->mPauseRequest = avatarp->requestPause();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// onBtnStop()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onBtnStop(void* user_data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)user_data;
	if (!previewp->getEnabled())
		return;

	if (previewp->mMotionID.notNull() && previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
		previewp->resetMotion();
		previewp->mPauseRequest = avatarp->requestPause();
	}
}

//-----------------------------------------------------------------------------
// onSliderMove()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onSliderMove(LLUICtrl* ctrl, void*user_data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)user_data;
	if (!previewp->getEnabled())
		return;

	if (previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
		F32 slider_value = (F32)previewp->childGetValue("playback_slider").asReal();
		LLUUID base_id = previewp->mIDList[previewp->childGetValue("preview_base_anim").asString()];
		LLMotion* motionp = avatarp->findMotion(previewp->mMotionID);
		F32 duration = motionp->getDuration();// + motionp->getEaseOutDuration();
		F32 delta_time = duration * slider_value;
		avatarp->deactivateAllMotions();
		avatarp->startMotion(base_id, delta_time + BASE_ANIM_TIME_OFFSET);
		avatarp->startMotion(previewp->mMotionID, delta_time);
		previewp->mPauseRequest = avatarp->requestPause();
		previewp->refresh();
	}

}

//-----------------------------------------------------------------------------
// onCommitBaseAnim()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitBaseAnim(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	if (previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();

		BOOL paused = avatarp->areAnimationsPaused();

		// stop all other possible base motions
		avatarp->stopMotion(previewp->mIDList["Standing"], TRUE);
		avatarp->stopMotion(previewp->mIDList["Walking"], TRUE);
		avatarp->stopMotion(previewp->mIDList["Sitting"], TRUE);
		avatarp->stopMotion(previewp->mIDList["Flying"], TRUE);

		previewp->resetMotion();

		if (!paused)
		{
			previewp->mPauseRequest = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// onCommitLoop()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitLoop(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;
	
	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	if (motionp)
	{
		motionp->setLoop(previewp->childGetValue("loop_check").asBoolean());
		motionp->setLoopIn((F32)previewp->childGetValue("loop_in_point").asReal() * 0.01f * motionp->getDuration());
		motionp->setLoopOut((F32)previewp->childGetValue("loop_out_point").asReal() * 0.01f * motionp->getDuration());
	}
}

//-----------------------------------------------------------------------------
// onCommitLoopIn()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitLoopIn(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	if (motionp)
	{
		motionp->setLoopIn((F32)previewp->childGetValue("loop_in_point").asReal() / 100.f);
		previewp->resetMotion();
		previewp->childSetValue("loop_check", LLSD(TRUE));
		onCommitLoop(ctrl, data);
	}
}

//-----------------------------------------------------------------------------
// onCommitLoopOut()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitLoopOut(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	if (motionp)
	{
		motionp->setLoopOut((F32)previewp->childGetValue("loop_out_point").asReal() * 0.01f * motionp->getDuration());
		previewp->resetMotion();
		previewp->childSetValue("loop_check", LLSD(TRUE));
		onCommitLoop(ctrl, data);
	}
}

//-----------------------------------------------------------------------------
// onCommitName()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitName(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	if (motionp)
	{
		motionp->setName(previewp->childGetValue("name_form").asString());
	}

	previewp->doCommit();
}

//-----------------------------------------------------------------------------
// onCommitHandPose()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitHandPose(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	previewp->resetMotion(); // sets hand pose
}

//-----------------------------------------------------------------------------
// onCommitEmote()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitEmote(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	previewp->resetMotion(); // ssts emote
}

//-----------------------------------------------------------------------------
// onCommitPriority()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitPriority(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	motionp->setPriority(llfloor((F32)previewp->childGetValue("priority").asReal()));
}

//-----------------------------------------------------------------------------
// onCommitEaseIn()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitEaseIn(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	motionp->setEaseIn((F32)previewp->childGetValue("ease_in_time").asReal());
	previewp->resetMotion();
}

//-----------------------------------------------------------------------------
// onCommitEaseOut()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitEaseOut(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->getEnabled())
		return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	motionp->setEaseOut((F32)previewp->childGetValue("ease_out_time").asReal());
	previewp->resetMotion();
}

//-----------------------------------------------------------------------------
// validateEaseIn()
//-----------------------------------------------------------------------------
bool LLFloaterAnimPreview::validateEaseIn(const LLSD& data)
{
	if (!getEnabled())
		return false;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
	
	if (!motionp->getLoop())
	{
		F32 new_ease_in = llclamp((F32)childGetValue("ease_in_time").asReal(), 0.f, motionp->getDuration() - motionp->getEaseOutDuration());
		childSetValue("ease_in_time", LLSD(new_ease_in));
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// validateEaseOut()
//-----------------------------------------------------------------------------
bool LLFloaterAnimPreview::validateEaseOut(const LLSD& data)
{
	if (!getEnabled())
		return false;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
	
	if (!motionp->getLoop())
	{
		F32 new_ease_out = llclamp((F32)childGetValue("ease_out_time").asReal(), 0.f, motionp->getDuration() - motionp->getEaseInDuration());
		childSetValue("ease_out_time", LLSD(new_ease_out));
	}

	return true;
}

//-----------------------------------------------------------------------------
// validateLoopIn()
//-----------------------------------------------------------------------------
bool LLFloaterAnimPreview::validateLoopIn(const LLSD& data)
{
	if (!getEnabled())
		return false;

	F32 loop_in_value = (F32)childGetValue("loop_in_point").asReal();
	F32 loop_out_value = (F32)childGetValue("loop_out_point").asReal();

	if (loop_in_value < 0.f)
	{
		loop_in_value = 0.f;
	}
	else if (loop_in_value > 100.f)
	{
		loop_in_value = 100.f;
	}
	else if (loop_in_value > loop_out_value)
	{
		loop_in_value = loop_out_value;
	}

	childSetValue("loop_in_point", LLSD(loop_in_value));
	return true;
}

//-----------------------------------------------------------------------------
// validateLoopOut()
//-----------------------------------------------------------------------------
bool LLFloaterAnimPreview::validateLoopOut(const LLSD& data)
{
	if (!getEnabled())
		return false;

	F32 loop_out_value = (F32)childGetValue("loop_out_point").asReal();
	F32 loop_in_value = (F32)childGetValue("loop_in_point").asReal();

	if (loop_out_value < 0.f)
	{
		loop_out_value = 0.f;
	}
	else if (loop_out_value > 100.f)
	{
		loop_out_value = 100.f;
	}
	else if (loop_out_value < loop_in_value)
	{
		loop_out_value = loop_in_value;
	}

	childSetValue("loop_out_point", LLSD(loop_out_value));
	return true;
}


//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::refresh()
{
	// Are we showing the play button (default) or the pause button?
	bool show_play = true;
	if (!mAnimPreview)
	{
		childShow("bad_animation_text");
		// play button visible but disabled
		mPlayButton->setEnabled(FALSE);
		mStopButton->setEnabled(FALSE);
		childDisable("ok_btn");
	}
	else
	{
		childHide("bad_animation_text");
		// re-enabled in case previous animation was bad
		mPlayButton->setEnabled(TRUE);
		mStopButton->setEnabled(TRUE);
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		if (avatarp->isMotionActive(mMotionID))
		{
			mStopButton->setEnabled(TRUE);
			LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
			if (!avatarp->areAnimationsPaused())
			{
				// animation is playing
				if (motionp)
				{
					F32 fraction_complete = motionp->getLastUpdateTime() / motionp->getDuration();
					childSetValue("playback_slider", fraction_complete);
				}
				show_play = false;
			}
		}
		else
		{
			// Motion just finished playing
			mPauseRequest = avatarp->requestPause();
		}
		childEnable("ok_btn");
		mAnimPreview->requestUpdate();
	}
	mPlayButton->setVisible(show_play);
	mPauseButton->setVisible(!show_play);
}

//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onBtnOK(void* userdata)
{
	LLFloaterAnimPreview* floaterp = (LLFloaterAnimPreview*)userdata;
	if (!floaterp->getEnabled()) return;

	if (floaterp->mAnimPreview)
	{
		LLKeyframeMotion* motionp = (LLKeyframeMotion*)floaterp->mAnimPreview->getDummyAvatar()->findMotion(floaterp->mMotionID);

		S32 file_size = motionp->getFileSize();
		U8* buffer = new U8[file_size];

		LLDataPackerBinaryBuffer dp(buffer, file_size);
		if (motionp->serialize(dp))
		{
			LLVFile file(gVFS, motionp->getID(), LLAssetType::AT_ANIMATION, LLVFile::APPEND);

			S32 size = dp.getCurrentSize();
			file.setMaxSize(size);
			if (file.write((U8*)buffer, size))
			{
				std::string name = floaterp->childGetValue("name_form").asString();
				std::string desc = floaterp->childGetValue("description_form").asString();
				LLAssetStorage::LLStoreAssetCallback callback = NULL;
				S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
				void *userdata = NULL;
				upload_new_resource(floaterp->mTransactionID, // tid
						    LLAssetType::AT_ANIMATION,
						    name,
						    desc,
						    0,
						    LLFolderType::FT_NONE,
						    LLInventoryType::IT_ANIMATION,
						    LLFloaterPerms::getNextOwnerPerms(), LLFloaterPerms::getGroupPerms(), LLFloaterPerms::getEveryonePerms(),
						    name,
						    callback, expected_upload_cost, userdata);
			}
			else
			{
				llwarns << "Failure writing animation data." << llendl;
				LLNotificationsUtil::add("WriteAnimationFail");
			}
		}

		delete [] buffer;
		// clear out cache for motion data
		floaterp->mAnimPreview->getDummyAvatar()->removeMotion(floaterp->mMotionID);
		LLKeyframeDataCache::removeKeyframeData(floaterp->mMotionID);
	}

	floaterp->closeFloater(false);
}

//-----------------------------------------------------------------------------
// LLPreviewAnimation
//-----------------------------------------------------------------------------
LLPreviewAnimation::LLPreviewAnimation(S32 width, S32 height) : LLViewerDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE)
{
	mNeedsUpdate = TRUE;
	mCameraDistance = PREVIEW_CAMERA_DISTANCE;
	mCameraYaw = 0.f;
	mCameraPitch = 0.f;
	mCameraZoom = 1.f;

	mDummyAvatar = (LLVOAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR, gAgent.getRegion());
	mDummyAvatar->createDrawable(&gPipeline);
	mDummyAvatar->mIsDummy = TRUE;
	mDummyAvatar->mSpecialRenderMode = 1;
	mDummyAvatar->setPositionAgent(LLVector3::zero);
	mDummyAvatar->slamPosition();
	mDummyAvatar->updateJointLODs();
	mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	mDummyAvatar->startMotion(ANIM_AGENT_STAND, BASE_ANIM_TIME_OFFSET);
	mDummyAvatar->hideSkirt();
	gPipeline.markVisible(mDummyAvatar->mDrawable, *LLViewerCamera::getInstance());

	// stop extraneous animations
	mDummyAvatar->stopMotion( ANIM_AGENT_HEAD_ROT, TRUE );
	mDummyAvatar->stopMotion( ANIM_AGENT_EYE, TRUE );
	mDummyAvatar->stopMotion( ANIM_AGENT_BODY_NOISE, TRUE );
	mDummyAvatar->stopMotion( ANIM_AGENT_BREATHE_ROT, TRUE );
}

//-----------------------------------------------------------------------------
// LLPreviewAnimation()
//-----------------------------------------------------------------------------
LLPreviewAnimation::~LLPreviewAnimation()
{
	mDummyAvatar->markDead();
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
BOOL	LLPreviewAnimation::render()
{
	mNeedsUpdate = FALSE;
	LLVOAvatar* avatarp = mDummyAvatar;
	
	glMatrixMode(GL_PROJECTION);
	gGL.pushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mFullWidth, 0.0f, mFullHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	gGL.pushMatrix();
	glLoadIdentity();

	LLGLSUIDefault def;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.color4f(0.15f, 0.2f, 0.3f, 1.f);

	gl_rect_2d_simple( mFullWidth, mFullHeight );

	glMatrixMode(GL_PROJECTION);
	gGL.popMatrix();

	glMatrixMode(GL_MODELVIEW);
	gGL.popMatrix();

	gGL.flush();

	LLVector3 target_pos = avatarp->mRoot.getWorldPosition();

	LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) * 
		LLQuaternion(mCameraYaw, LLVector3::z_axis);

	LLQuaternion av_rot = avatarp->mRoot.getWorldRotation() * camera_rot;
	LLViewerCamera::getInstance()->setOriginAndLookAt(
		target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + mCameraOffset) * av_rot),		// camera
		LLVector3::z_axis,																	// up
		target_pos + (mCameraOffset  * av_rot) );											// point of interest

	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / mCameraZoom);
	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, FALSE);

	mCameraRelPos = LLViewerCamera::getInstance()->getOrigin() - avatarp->mHeadp->getWorldPosition();

	//avatarp->setAnimationData("LookAtPoint", (void *)&mCameraRelPos);

	//SJB: Animation is updated in LLVOAvatar::updateCharacter
	
	if (avatarp->mDrawable.notNull())
	{
		avatarp->updateLOD();
		
		LLVertexBuffer::unbind();
		LLGLDepthTest gls_depth(GL_TRUE);

		LLDrawPoolAvatar *avatarPoolp = (LLDrawPoolAvatar *)avatarp->mDrawable->getFace(0)->getPool();
		avatarp->dirtyMesh();
		avatarPoolp->renderAvatars(avatarp);  // renders only one avatar
	}

	gGL.color4f(1,1,1,1);
	return TRUE;
}

//-----------------------------------------------------------------------------
// requestUpdate()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::requestUpdate()
{ 
	mNeedsUpdate = TRUE; 
}

//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::rotate(F32 yaw_radians, F32 pitch_radians)
{
	mCameraYaw = mCameraYaw + yaw_radians;

	mCameraPitch = llclamp(mCameraPitch + pitch_radians, F_PI_BY_TWO * -0.8f, F_PI_BY_TWO * 0.8f);
}

//-----------------------------------------------------------------------------
// zoom()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::zoom(F32 zoom_delta)
{
	setZoom(mCameraZoom + zoom_delta);
}

//-----------------------------------------------------------------------------
// setZoom()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::setZoom(F32 zoom_amt)
{
	mCameraZoom	= llclamp(zoom_amt, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);
}

//-----------------------------------------------------------------------------
// pan()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::pan(F32 right, F32 up)
{
	mCameraOffset.mV[VY] = llclamp(mCameraOffset.mV[VY] + right * mCameraDistance / mCameraZoom, -1.f, 1.f);
	mCameraOffset.mV[VZ] = llclamp(mCameraOffset.mV[VZ] + up * mCameraDistance / mCameraZoom, -1.f, 1.f);
}



