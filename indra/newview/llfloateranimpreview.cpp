/** 
 * @file llfloateranimpreview.cpp
 * @brief LLFloaterAnimPreview class implementation
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloateranimpreview.h"

#include "llbvhloader.h"
#include "lldatapacker.h"
#include "lldir.h"
#include "llvfile.h"
#include "llapr.h"

#include "llagent.h"
#include "llbbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
#include "llkeyframemotion.h"
#include "lllineeditor.h"
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
#include "viewer.h"
#include "llvieweruictrlfactory.h"

S32 LLFloaterAnimPreview::sUploadAmount = 10;

const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREF_BUTTON_HEIGHT = 16;
const S32 PREVIEW_TEXTURE_HEIGHT = 300;

const F32 PREVIEW_CAMERA_DISTANCE = 4.f;

const F32 MIN_CAMERA_ZOOM = 0.5f;
const F32 MAX_CAMERA_ZOOM = 10.f;

//-----------------------------------------------------------------------------
// LLFloaterAnimPreview()
//-----------------------------------------------------------------------------
LLFloaterAnimPreview::LLFloaterAnimPreview(const char* filename) : 
	LLFloaterNameDesc(filename)
{
	mLastSliderValue = 0.f;
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
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::postBuild()
{
	LLRect r;
	LLKeyframeMotion* motionp = NULL;
	LLBVHLoader* loaderp = NULL;

	if (!LLFloaterNameDesc::postBuild())
	{
		return FALSE;
	}

	childSetCommitCallback("name_form", onCommitName, this);

	childSetLabelArg("ok_btn", "[AMOUNT]", llformat("%d",sUploadAmount));
	childSetAction("ok_btn", onBtnOK, this);
	setDefaultBtn();

	mPreviewRect.set(PREVIEW_HPAD, 
		PREVIEW_TEXTURE_HEIGHT,
		getRect().getWidth() - PREVIEW_HPAD, 
		PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
	mPreviewImageRect.set(0.f, 1.f, 1.f, 0.f);

	S32 y = mPreviewRect.mTop + BTN_HEIGHT;
	S32 btn_left = PREVIEW_HPAD;

	r.set( btn_left, y, btn_left + 32, y - BTN_HEIGHT );
	mPlayButton = LLViewerUICtrlFactory::getButtonByName(this, "play_btn");
	if (!mPlayButton)
	{
		mPlayButton = new LLButton("play_btn", LLRect(0,0,0,0));
	}
	mPlayButton->setClickedCallback(onBtnPlay);
	mPlayButton->setCallbackUserData(this);

	mPlayButton->setImages("button_anim_play.tga",
						   "button_anim_play_selected.tga");
	mPlayButton->setDisabledImages("","");

	mPlayButton->setScaleImage(TRUE);
	mPlayButton->setFixedBorder(0, 0);

	mStopButton = LLViewerUICtrlFactory::getButtonByName(this, "stop_btn");
	if (!mStopButton)
	{
		mStopButton = new LLButton("stop_btn", LLRect(0,0,0,0));
	}
	mStopButton->setClickedCallback(onBtnStop);
	mStopButton->setCallbackUserData(this);

	mStopButton->setImages("button_anim_stop.tga",
						   "button_anim_stop_selected.tga");
	mStopButton->setDisabledImages("","");

	mStopButton->setScaleImage(TRUE);
	mStopButton->setFixedBorder(0, 0);

	r.set(r.mRight + PREVIEW_HPAD, y, getRect().getWidth() - PREVIEW_HPAD, y - BTN_HEIGHT);
	childSetCommitCallback("playback_slider", onSliderMove, this);

	childHide("bad_animation_text");

	childSetCommitCallback("preview_base_anim", onCommitBaseAnim, this);
	childSetValue("preview_base_anim", "Standing");

	childSetCommitCallback("priority", onCommitPriority, this);
	childSetCommitCallback("loop_check", onCommitLoop, this);
	childSetCommitCallback("loop_in_point", onCommitLoopIn, this);
	childSetValidate("loop_in_point", validateLoopIn);
	childSetCommitCallback("loop_out_point", onCommitLoopOut, this);
	childSetValidate("loop_out_point", validateLoopOut);

	childSetCommitCallback("hand_pose_combo", onCommitHandPose, this);
	
	childSetCommitCallback("emote_combo", onCommitEmote, this);
	childSetValue("emote_combo", "[None]");

	childSetCommitCallback("ease_in_time", onCommitEaseIn, this);
	childSetValidate("ease_in_time", validateEaseIn);
	childSetCommitCallback("ease_out_time", onCommitEaseOut, this);
	childSetValidate("ease_out_time", validateEaseOut);

	if (!stricmp(strrchr(mFilename.c_str(), '.'), ".bvh"))
	{
		// loading a bvh file

		// now load bvh file
		S32 file_size;
		apr_file_t* fp = ll_apr_file_open(mFilenameAndPath, LL_APR_RB, &file_size);

		if (!fp)
		{
			llwarns << "Can't open BVH file:" << mFilename << llendl;	
		}
		else
		{
			char*	file_buffer;

			file_buffer = new char[file_size + 1];

			if (file_size == ll_apr_file_read(fp, file_buffer, file_size))
			{
				file_buffer[file_size] = '\0';
				llinfos << "Loading BVH file " << mFilename << llendl;
				loaderp = new LLBVHLoader(file_buffer);
			}

			apr_file_close(fp);
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
			const LLBBoxLocal &pelvis_bbox = motionp->getPelvisBBox();

			LLVector3 temp = pelvis_bbox.getCenter();
			// only consider XY?
			//temp.mV[VZ] = 0.f;
			F32 pelvis_offset = temp.magVec();

			temp = pelvis_bbox.getExtent();
			//temp.mV[VZ] = 0.f;
			F32 pelvis_max_displacement = pelvis_offset + (temp.magVec() * 0.5f) + 1.f;
			
			F32 camera_zoom = gCamera->getDefaultFOV() / (2.f * atan(pelvis_max_displacement / PREVIEW_CAMERA_DISTANCE));
		
			mAnimPreview->setZoom(camera_zoom);

			motionp->setName(childGetValue("name_form").asString());
			mAnimPreview->getDummyAvatar()->startMotion(mMotionID);
			childSetMinValue("playback_slider", 0.0);
			childSetMaxValue("playback_slider", 1.0);

			childSetValue("loop_check", LLSD(motionp->getLoop()));
			childSetValue("loop_in_point", LLSD(motionp->getLoopIn() / motionp->getDuration() * 100.f));
			childSetValue("loop_out_point", LLSD(motionp->getLoopOut() / motionp->getDuration() * 100.f));
			childSetValue("priority", LLSD((F32)motionp->getPriority()));
			childSetValue("hand_pose_combo", LLHandMotion::getHandPoseName(motionp->getHandPose()));
			childSetValue("ease_in_time", LLSD(motionp->getEaseInDuration()));
			childSetValue("ease_out_time", LLSD(motionp->getEaseOutDuration()));
			mEnabled = TRUE;
			char seconds_string[128];		/*Flawfinder: ignore*/
			snprintf(seconds_string, sizeof(seconds_string), " - %.2f seconds", motionp->getDuration());	/* Flawfinder: ignore */

			setTitle(mFilename + LLString(seconds_string));
		}
		else
		{
			delete mAnimPreview;
			mAnimPreview = NULL;
			mMotionID.setNull();
			// XUI:translate
			childSetValue("bad_animation_text", LLSD("Failed to initialize motion."));
			mEnabled = FALSE;
		}
	}
	else
	{
		if ( loaderp )
		{
			if (loaderp->getDuration() > MAX_ANIM_DURATION)
			{
				char output_str[256];	/*Flawfinder: ignore*/

				snprintf(output_str, sizeof(output_str), "Animation file is %.1f seconds in length.\n\nMaximum animation length is %.1f seconds.\n",	/* Flawfinder: ignore */
					loaderp->getDuration(), MAX_ANIM_DURATION);
				childSetValue("bad_animation_text", LLSD(output_str));
			}
			else
			{
				char* status = loaderp->getStatus();
				LLString error_string("Unable to read animation file.\n\n");
				error_string += LLString(status);
				childSetValue("bad_animation_text", LLSD(error_string));
			}
		}

		mEnabled = FALSE;
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
	delete mAnimPreview;
	mAnimPreview = NULL;

	mEnabled = FALSE;
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
		glColor3f(1.f, 1.f, 1.f);
		mAnimPreview->bindTexture();

		glBegin( GL_QUADS );
		{
			glTexCoord2f(0.f, 1.f);
			glVertex2i(PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
			glTexCoord2f(0.f, 0.f);
			glVertex2i(PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
			glTexCoord2f(1.f, 0.f);
			glVertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
			glTexCoord2f(1.f, 1.f);
			glVertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
		}
		glEnd();

		mAnimPreview->unbindTexture();

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

	mPauseRequest = NULL;
	
	LLUUID anim_id = mIDList[childGetValue("preview_base_anim").asString()];
	avatarp->stopMotion(anim_id, TRUE);
	avatarp->stopMotion(mMotionID, TRUE);
	avatarp->startMotion(anim_id, 5.f);
	avatarp->startMotion(mMotionID);
	childSetValue("playback_slider", 0.0);
	mLastSliderValue = 0.0f;

	if (paused)
	{
		mPauseRequest = avatarp->requestPause();
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
		gViewerWindow->setMouseCapture(this);
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
	gViewerWindow->setMouseCapture(FALSE);
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

		LLUI::setCursorPositionLocal(this, mLastMouseX, mLastMouseY);
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
	if (!previewp->mEnabled) return;

	if (previewp->mMotionID.notNull() && previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();

		if(!avatarp->isMotionActive(previewp->mMotionID))
		{
			previewp->resetMotion();
			previewp->mPauseRequest = NULL;
		}
		else
		{
			if (avatarp->areAnimationsPaused())
			{
				previewp->mPauseRequest = NULL;
			}
			else
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
	if (!previewp->mEnabled) return;

	if (previewp->mMotionID.notNull() && previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();

		// is the motion looping and have we passed the loop in point?
		if (previewp->childGetValue("loop_check").asBoolean() && 
			(F32)previewp->childGetValue("loop_in_point").asReal() <= (F32)previewp->childGetValue("playback_slider").asReal() * 100.f)
		{
			avatarp->stopMotion(previewp->mMotionID, FALSE);
		}
		else
		{
			avatarp->stopMotion(previewp->mMotionID, FALSE);
		}
	}
}

//-----------------------------------------------------------------------------
// onSliderMove()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onSliderMove(LLUICtrl* ctrl, void*user_data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)user_data;
	if (!previewp->mEnabled) return;

	if (previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
		LLMotion* motionp = avatarp->findMotion(previewp->mMotionID);
		LLMotion* base_motionp = 
			avatarp->findMotion(previewp->mIDList[previewp->childGetValue("preview_base_anim").asString()]);

		if (motionp && base_motionp)
		{
			if (!avatarp->isMotionActive(previewp->mMotionID))
			{ 
				previewp->resetMotion();
			}

			previewp->mPauseRequest = avatarp->requestPause();
			F32 original_activation_time = motionp->mActivationTimestamp;
			motionp->mActivationTimestamp -= ((F32)previewp->childGetValue("playback_slider").asReal() - previewp->mLastSliderValue) * 
				motionp->getDuration();
			base_motionp->mActivationTimestamp -= ((F32)previewp->childGetValue("playback_slider").asReal() - previewp->mLastSliderValue) * 
				base_motionp->getDuration();

			if (motionp->mSendStopTimestamp != F32_MIN)
			{
				motionp->mSendStopTimestamp = motionp->mSendStopTimestamp - original_activation_time + motionp->mActivationTimestamp;
			}

			if (motionp->mStopTimestamp != F32_MIN)
			{
				motionp->mStopTimestamp = motionp->mStopTimestamp - original_activation_time + motionp->mActivationTimestamp;
			}
			previewp->refresh();
		}
	}

	previewp->mLastSliderValue = (F32)previewp->childGetValue("playback_slider").asReal();
}

//-----------------------------------------------------------------------------
// onCommitBaseAnim()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitBaseAnim(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;

	if (!previewp->mEnabled) return;

	if (previewp->mAnimPreview)
	{
		LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();

		BOOL paused = avatarp->areAnimationsPaused();

		// stop all other possible base motions
		avatarp->stopMotion(ANIM_AGENT_STAND, TRUE);
		avatarp->stopMotion(ANIM_AGENT_WALK, TRUE);
		avatarp->stopMotion(ANIM_AGENT_SIT, TRUE);
		avatarp->stopMotion(ANIM_AGENT_HOVER, TRUE);

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

	if (!previewp->mEnabled) return;
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
	if (!previewp->mEnabled) return;

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
	if (!previewp->mEnabled) return;

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
	if (!previewp->mEnabled) return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	if (motionp)
	{
		motionp->setName(previewp->childGetValue("name_form").asString());
	}

	LLFloaterNameDesc::doCommit(ctrl, data);
}

//-----------------------------------------------------------------------------
// onCommitHandPose()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitHandPose(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->mEnabled) return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	motionp->setHandPose(LLHandMotion::getHandPose(previewp->childGetValue("hand_pose_combo").asString()));
	previewp->resetMotion();
}

//-----------------------------------------------------------------------------
// onCommitEmote()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitEmote(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->mEnabled) return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	motionp->setEmote(previewp->mIDList[previewp->childGetValue("emote_combo").asString()]);
	previewp->resetMotion();
}

//-----------------------------------------------------------------------------
// onCommitPriority()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onCommitPriority(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	if (!previewp->mEnabled) return;

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
	if (!previewp->mEnabled) return;

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
	if (!previewp->mEnabled) return;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);

	motionp->setEaseOut((F32)previewp->childGetValue("ease_out_time").asReal());
	previewp->resetMotion();
}

//-----------------------------------------------------------------------------
// validateEaseIn()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::validateEaseIn(LLUICtrl* spin, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;
	
	if (!previewp->mEnabled) return FALSE;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);
	
	if (!motionp->getLoop())
	{
		F32 new_ease_in = llclamp((F32)previewp->childGetValue("ease_in_time").asReal(), 0.f, motionp->getDuration() - motionp->getEaseOutDuration());
		previewp->childSetValue("ease_in_time", LLSD(new_ease_in));
	}
	
	return TRUE;
}

//-----------------------------------------------------------------------------
// validateEaseOut()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::validateEaseOut(LLUICtrl* spin, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;

	if (!previewp->mEnabled) return FALSE;

	LLVOAvatar* avatarp = previewp->mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(previewp->mMotionID);
	
	if (!motionp->getLoop())
	{
		F32 new_ease_out = llclamp((F32)previewp->childGetValue("ease_out_time").asReal(), 0.f, motionp->getDuration() - motionp->getEaseInDuration());
		previewp->childSetValue("ease_out_time", LLSD(new_ease_out));
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// validateLoopIn()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::validateLoopIn(LLUICtrl* ctrl, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;

	if (!previewp->mEnabled) return FALSE;

	F32 loop_in_value = (F32)previewp->childGetValue("loop_in_point").asReal();
	F32 loop_out_value = (F32)previewp->childGetValue("loop_out_point").asReal();

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

	previewp->childSetValue("loop_in_point", LLSD(loop_in_value));
	return TRUE;
}

//-----------------------------------------------------------------------------
// validateLoopOut()
//-----------------------------------------------------------------------------
BOOL LLFloaterAnimPreview::validateLoopOut(LLUICtrl* spin, void* data)
{
	LLFloaterAnimPreview* previewp = (LLFloaterAnimPreview*)data;

	if (!previewp->mEnabled) return FALSE;

	F32 loop_out_value = (F32)previewp->childGetValue("loop_out_point").asReal();
	F32 loop_in_value = (F32)previewp->childGetValue("loop_in_point").asReal();

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

	previewp->childSetValue("loop_out_point", LLSD(loop_out_value));
	return TRUE;
}


//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::refresh()
{
	if (!mAnimPreview)
	{
		childShow("bad_animation_text");
		mPlayButton->setEnabled(FALSE);
		mStopButton->setEnabled(FALSE);
		childDisable("ok_btn");
	}
	else
	{
		childHide("bad_animation_text");
		mPlayButton->setEnabled(TRUE);
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		if (avatarp->isMotionActive(mMotionID))
		{
			mStopButton->setEnabled(TRUE);
			LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
			if (avatarp->areAnimationsPaused())
			{

				mPlayButton->setImages("button_anim_play.tga",
									   "button_anim_play_selected.tga");

			}
			else
			{
				if (motionp)
				{
					F32 fraction_complete;
					fraction_complete = motionp->getLastUpdateTime() / motionp->getDuration();

					childSetValue("playback_slider", fraction_complete);
					mLastSliderValue = fraction_complete;
				}
				mPlayButton->setImages("button_anim_pause.tga",
									   "button_anim_pause_selected.tga");

			}
		}
		else
		{
			mPauseRequest = avatarp->requestPause();
			mPlayButton->setImages("button_anim_play.tga",
								   "button_anim_play_selected.tga");

			mStopButton->setEnabled(FALSE);
		}
		childEnable("ok_btn");
		mAnimPreview->requestUpdate();
	}
}

//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterAnimPreview::onBtnOK(void* userdata)
{
	LLFloaterAnimPreview* floaterp = (LLFloaterAnimPreview*)userdata;
	if (!floaterp->mEnabled) return;

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
				upload_new_resource(floaterp->mTransactionID, // tid
									LLAssetType::AT_ANIMATION,
									name,
									desc,
									0,
									LLAssetType::AT_NONE,
									LLInventoryType::IT_ANIMATION,
									PERM_NONE,
									name);
			}
			else
			{
				llwarns << "Failure writing animation data." << llendl;
				gViewerWindow->alertXml("WriteAnimationFail");
			}
		}

		delete [] buffer;
		// clear out cache for motion data
		floaterp->mAnimPreview->getDummyAvatar()->removeMotion(floaterp->mMotionID);
		LLKeyframeDataCache::removeKeyframeData(floaterp->mMotionID);
	}

	floaterp->onClose(false);
}

//-----------------------------------------------------------------------------
// LLPreviewAnimation
//-----------------------------------------------------------------------------
LLPreviewAnimation::LLPreviewAnimation(S32 width, S32 height) : LLDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE)
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
	mDummyAvatar->startMotion(ANIM_AGENT_STAND, 5.f);
	mDummyAvatar->mSkirtLOD.setVisible(FALSE, TRUE);
	gPipeline.markVisible(mDummyAvatar->mDrawable, *gCamera);

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
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLSUIDefault def;
	LLGLSNoTexture gls_no_texture;
	glColor4f(0.15f, 0.2f, 0.3f, 1.f);

	gl_rect_2d_simple( mWidth, mHeight );

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	LLVector3 target_pos = avatarp->mRoot.getWorldPosition();

	LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) * 
		LLQuaternion(mCameraYaw, LLVector3::z_axis);

	LLQuaternion av_rot = avatarp->mRoot.getWorldRotation() * camera_rot;
	gCamera->setOriginAndLookAt(
		target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + mCameraOffset) * av_rot),		// camera
		LLVector3::z_axis,																	// up
		target_pos + (mCameraOffset  * av_rot) );											// point of interest

	gCamera->setView(gCamera->getDefaultFOV() / mCameraZoom);
	gCamera->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mWidth, mHeight, FALSE);

	mCameraRelPos = gCamera->getOrigin() - avatarp->mHeadp->getWorldPosition();

	//avatarp->setAnimationData("LookAtPoint", (void *)&mCameraRelPos);

	//RN: timestep must be zero, because paused animations will never initialize
	// av skeleton otherwise
	avatarp->setTimeStep(0.f);
	if (avatarp->areAnimationsPaused())
	{
		avatarp->updateMotion(TRUE);
	}
	else
	{
		avatarp->updateMotion();
	}
	
	LLVertexBuffer::stopRender();
	avatarp->updateLOD();
	LLVertexBuffer::startRender();

	avatarp->mRoot.updateWorldMatrixChildren();

	stop_glerror();

	LLGLDepthTest gls_depth(GL_TRUE);

	if (avatarp->mDrawable.notNull())
	{
		LLDrawPoolAvatar *avatarPoolp = (LLDrawPoolAvatar *)avatarp->mDrawable->getFace(0)->getPool();
		avatarPoolp->renderAvatars(avatarp);  // renders only one avatar
	}
	
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

