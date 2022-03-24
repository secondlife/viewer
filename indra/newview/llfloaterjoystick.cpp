/** 
 * @file llfloaterjoystick.cpp
 * @brief Joystick preferences panel
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

// file include
#include "llfloaterjoystick.h"

// linden library includes
#include "llerror.h"
#include "llrect.h"
#include "llstring.h"
#include "lltrace.h"

// project includes
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llviewerjoystick.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"

#if LL_WINDOWS && !LL_MESA_HEADLESS
// Require DirectInput version 8
#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#endif

static LLTrace::SampleStatHandle<>	sJoystickAxis0("Joystick axis 0"),
									sJoystickAxis1("Joystick axis 1"),
									sJoystickAxis2("Joystick axis 2"),
									sJoystickAxis3("Joystick axis 3"),
									sJoystickAxis4("Joystick axis 4"),
									sJoystickAxis5("Joystick axis 5");
static LLTrace::SampleStatHandle<>* sJoystickAxes[6] = 
{
	&sJoystickAxis0,
	&sJoystickAxis1,
	&sJoystickAxis2,
	&sJoystickAxis3,
	&sJoystickAxis4,
	&sJoystickAxis5
};


#if LL_WINDOWS && !LL_MESA_HEADLESS

BOOL CALLBACK di8_list_devices_callback(LPCDIDEVICEINSTANCE device_instance_ptr, LPVOID pvRef)
{
    // Note: If a single device can function as more than one DirectInput
    // device type, it is enumerated as each device type that it supports.
    // Capable of detecting devices like Oculus Rift
    if (device_instance_ptr && pvRef)
    {
        std::string product_name = utf16str_to_utf8str(llutf16string(device_instance_ptr->tszProductName));
        S32 size = sizeof(GUID);
        LLSD::Binary data; //just an std::vector
        data.resize(size);
        memcpy(&data[0], &device_instance_ptr->guidInstance /*POD _GUID*/, size);

        LLFloaterJoystick * floater = (LLFloaterJoystick*)pvRef;
        LLSD value = data;
        floater->addDevice(product_name, value);
    }
    return DIENUM_CONTINUE;
}
#endif

LLFloaterJoystick::LLFloaterJoystick(const LLSD& data)
	: LLFloater(data),
    mHasDeviceList(false)
{
    if (!LLViewerJoystick::getInstance()->isJoystickInitialized())
    {
        LLViewerJoystick::getInstance()->init(false);
    }

	initFromSettings();
}

void LLFloaterJoystick::draw()
{
    LLViewerJoystick* joystick(LLViewerJoystick::getInstance());
    bool joystick_inited = joystick->isJoystickInitialized();
    if (joystick_inited != mHasDeviceList)
    {
        refreshListOfDevices();
    }

	for (U32 i = 0; i < 6; i++)
	{
		F32 value = joystick->getJoystickAxis(i);
		sample(*sJoystickAxes[i], value * gFrameIntervalSeconds.value());
		if (mAxisStatsBar[i])
		{
			F32 minbar, maxbar;
			mAxisStatsBar[i]->getRange(minbar, maxbar);
			if (llabs(value) > maxbar)
			{
				F32 range = llabs(value);
				mAxisStatsBar[i]->setRange(-range, range);
			}
		}
	}

	LLFloater::draw();
}

BOOL LLFloaterJoystick::postBuild()
{		
	center();
	F32 range = gSavedSettings.getBOOL("Cursor3D") ? 128.f : 2.f;

	for (U32 i = 0; i < 6; i++)
	{
		std::string stat_name(llformat("Joystick axis %d", i));
		std::string axisname = llformat("axis%d", i);
		mAxisStatsBar[i] = getChild<LLStatBar>(axisname);
		if (mAxisStatsBar[i])
		{
			mAxisStatsBar[i]->setStat(stat_name);
			mAxisStatsBar[i]->setRange(-range, range);
		}
	}
	
	mJoysticksCombo = getChild<LLComboBox>("joystick_combo");
	childSetCommitCallback("joystick_combo",onCommitJoystickEnabled,this);
	mCheckFlycamEnabled = getChild<LLCheckBoxCtrl>("JoystickFlycamEnabled");
	childSetCommitCallback("JoystickFlycamEnabled",onCommitJoystickEnabled,this);

	childSetAction("SpaceNavigatorDefaults", onClickRestoreSNDefaults, this);
	childSetAction("cancel_btn", onClickCancel, this);
	childSetAction("ok_btn", onClickOK, this);

	refresh();
	refreshListOfDevices();
	return TRUE;
}

LLFloaterJoystick::~LLFloaterJoystick()
{
	// Children all cleaned up by default view destructor.
}


void LLFloaterJoystick::apply()
{
}

void LLFloaterJoystick::initFromSettings()
{
	mJoystickEnabled = gSavedSettings.getBOOL("JoystickEnabled");
	mJoystickId = gSavedSettings.getLLSD("JoystickDeviceUUID");

	mJoystickAxis[0] = gSavedSettings.getS32("JoystickAxis0");
	mJoystickAxis[1] = gSavedSettings.getS32("JoystickAxis1");
	mJoystickAxis[2] = gSavedSettings.getS32("JoystickAxis2");
	mJoystickAxis[3] = gSavedSettings.getS32("JoystickAxis3");
	mJoystickAxis[4] = gSavedSettings.getS32("JoystickAxis4");
	mJoystickAxis[5] = gSavedSettings.getS32("JoystickAxis5");
	mJoystickAxis[6] = gSavedSettings.getS32("JoystickAxis6");
	
	m3DCursor = gSavedSettings.getBOOL("Cursor3D");
	mAutoLeveling = gSavedSettings.getBOOL("AutoLeveling");
	mZoomDirect  = gSavedSettings.getBOOL("ZoomDirect");

	mAvatarEnabled = gSavedSettings.getBOOL("JoystickAvatarEnabled");
	mBuildEnabled = gSavedSettings.getBOOL("JoystickBuildEnabled");
	mFlycamEnabled = gSavedSettings.getBOOL("JoystickFlycamEnabled");
	
	mAvatarAxisScale[0] = gSavedSettings.getF32("AvatarAxisScale0");
	mAvatarAxisScale[1] = gSavedSettings.getF32("AvatarAxisScale1");
	mAvatarAxisScale[2] = gSavedSettings.getF32("AvatarAxisScale2");
	mAvatarAxisScale[3] = gSavedSettings.getF32("AvatarAxisScale3");
	mAvatarAxisScale[4] = gSavedSettings.getF32("AvatarAxisScale4");
	mAvatarAxisScale[5] = gSavedSettings.getF32("AvatarAxisScale5");

	mBuildAxisScale[0] = gSavedSettings.getF32("BuildAxisScale0");
	mBuildAxisScale[1] = gSavedSettings.getF32("BuildAxisScale1");
	mBuildAxisScale[2] = gSavedSettings.getF32("BuildAxisScale2");
	mBuildAxisScale[3] = gSavedSettings.getF32("BuildAxisScale3");
	mBuildAxisScale[4] = gSavedSettings.getF32("BuildAxisScale4");
	mBuildAxisScale[5] = gSavedSettings.getF32("BuildAxisScale5");

	mFlycamAxisScale[0] = gSavedSettings.getF32("FlycamAxisScale0");
	mFlycamAxisScale[1] = gSavedSettings.getF32("FlycamAxisScale1");
	mFlycamAxisScale[2] = gSavedSettings.getF32("FlycamAxisScale2");
	mFlycamAxisScale[3] = gSavedSettings.getF32("FlycamAxisScale3");
	mFlycamAxisScale[4] = gSavedSettings.getF32("FlycamAxisScale4");
	mFlycamAxisScale[5] = gSavedSettings.getF32("FlycamAxisScale5");
	mFlycamAxisScale[6] = gSavedSettings.getF32("FlycamAxisScale6");

	mAvatarAxisDeadZone[0] = gSavedSettings.getF32("AvatarAxisDeadZone0");
	mAvatarAxisDeadZone[1] = gSavedSettings.getF32("AvatarAxisDeadZone1");
	mAvatarAxisDeadZone[2] = gSavedSettings.getF32("AvatarAxisDeadZone2");
	mAvatarAxisDeadZone[3] = gSavedSettings.getF32("AvatarAxisDeadZone3");
	mAvatarAxisDeadZone[4] = gSavedSettings.getF32("AvatarAxisDeadZone4");
	mAvatarAxisDeadZone[5] = gSavedSettings.getF32("AvatarAxisDeadZone5");

	mBuildAxisDeadZone[0] = gSavedSettings.getF32("BuildAxisDeadZone0");
	mBuildAxisDeadZone[1] = gSavedSettings.getF32("BuildAxisDeadZone1");
	mBuildAxisDeadZone[2] = gSavedSettings.getF32("BuildAxisDeadZone2");
	mBuildAxisDeadZone[3] = gSavedSettings.getF32("BuildAxisDeadZone3");
	mBuildAxisDeadZone[4] = gSavedSettings.getF32("BuildAxisDeadZone4");
	mBuildAxisDeadZone[5] = gSavedSettings.getF32("BuildAxisDeadZone5");

	mFlycamAxisDeadZone[0] = gSavedSettings.getF32("FlycamAxisDeadZone0");
	mFlycamAxisDeadZone[1] = gSavedSettings.getF32("FlycamAxisDeadZone1");
	mFlycamAxisDeadZone[2] = gSavedSettings.getF32("FlycamAxisDeadZone2");
	mFlycamAxisDeadZone[3] = gSavedSettings.getF32("FlycamAxisDeadZone3");
	mFlycamAxisDeadZone[4] = gSavedSettings.getF32("FlycamAxisDeadZone4");
	mFlycamAxisDeadZone[5] = gSavedSettings.getF32("FlycamAxisDeadZone5");
	mFlycamAxisDeadZone[6] = gSavedSettings.getF32("FlycamAxisDeadZone6");

	mAvatarFeathering = gSavedSettings.getF32("AvatarFeathering");
	mBuildFeathering = gSavedSettings.getF32("BuildFeathering");
	mFlycamFeathering = gSavedSettings.getF32("FlycamFeathering");
}

void LLFloaterJoystick::refresh()
{
	LLFloater::refresh();

	initFromSettings();
}

void LLFloaterJoystick::addDevice(std::string &name, LLSD& value)
{
    mJoysticksCombo->add(name, value, ADD_BOTTOM, 1);
}

void LLFloaterJoystick::refreshListOfDevices()
{
    mJoysticksCombo->removeall();
    std::string no_device = getString("JoystickDisabled");
    LLSD value = LLSD::Integer(0);
    addDevice(no_device, value);

    mHasDeviceList = false;
    
    // di8_devices_callback callback is immediate and happens in scope of getInputDevices()
#if LL_WINDOWS && !LL_MESA_HEADLESS
    // space navigator is marked as DI8DEVCLASS_GAMECTRL in ndof lib
    U32 device_type = DI8DEVCLASS_GAMECTRL;
    void* callback = &di8_list_devices_callback;
#else
    // MAC doesn't support device search yet
    // On MAC there is an ndof_idsearch and it is possible to specify product
    // and manufacturer in NDOF_Device for ndof_init_first to pick specific one
    U32 device_type = 0;
    void* callback = NULL;
#endif
    if (gViewerWindow->getWindow()->getInputDevices(device_type, callback, this))
    {
        mHasDeviceList = true;
    }

    bool is_device_id_set = LLViewerJoystick::getInstance()->isDeviceUUIDSet();

    if (LLViewerJoystick::getInstance()->isJoystickInitialized() &&
        (!mHasDeviceList || !is_device_id_set))
    {
#if LL_WINDOWS && !LL_MESA_HEADLESS
        LL_WARNS() << "NDOF connected to device without using SL provided handle" << LL_ENDL;
#endif
        std::string desc = LLViewerJoystick::getInstance()->getDescription();
        if (!desc.empty())
        {
            LLSD value = LLSD::Integer(0);
            addDevice(desc, value);
            mHasDeviceList = true;
        }
    }

    if (gSavedSettings.getBOOL("JoystickEnabled") && mHasDeviceList)
    {
        if (is_device_id_set)
        {
            LLSD guid = LLViewerJoystick::getInstance()->getDeviceUUID();
            mJoysticksCombo->selectByValue(guid);
        }
        else
        {
            mJoysticksCombo->selectByValue(LLSD::Integer(1));
        }
    }
    else
    {
        mJoysticksCombo->selectByValue(LLSD::Integer(0));
    }
}

void LLFloaterJoystick::cancel()
{
	gSavedSettings.setBOOL("JoystickEnabled", mJoystickEnabled);
	gSavedSettings.setLLSD("JoystickDeviceUUID", mJoystickId);

	gSavedSettings.setS32("JoystickAxis0", mJoystickAxis[0]);
	gSavedSettings.setS32("JoystickAxis1", mJoystickAxis[1]);
	gSavedSettings.setS32("JoystickAxis2", mJoystickAxis[2]);
	gSavedSettings.setS32("JoystickAxis3", mJoystickAxis[3]);
	gSavedSettings.setS32("JoystickAxis4", mJoystickAxis[4]);
	gSavedSettings.setS32("JoystickAxis5", mJoystickAxis[5]);
	gSavedSettings.setS32("JoystickAxis6", mJoystickAxis[6]);

	gSavedSettings.setBOOL("Cursor3D", m3DCursor);
	gSavedSettings.setBOOL("AutoLeveling", mAutoLeveling);
	gSavedSettings.setBOOL("ZoomDirect", mZoomDirect );

	gSavedSettings.setBOOL("JoystickAvatarEnabled", mAvatarEnabled);
	gSavedSettings.setBOOL("JoystickBuildEnabled", mBuildEnabled);
	gSavedSettings.setBOOL("JoystickFlycamEnabled", mFlycamEnabled);
	
	gSavedSettings.setF32("AvatarAxisScale0", mAvatarAxisScale[0]);
	gSavedSettings.setF32("AvatarAxisScale1", mAvatarAxisScale[1]);
	gSavedSettings.setF32("AvatarAxisScale2", mAvatarAxisScale[2]);
	gSavedSettings.setF32("AvatarAxisScale3", mAvatarAxisScale[3]);
	gSavedSettings.setF32("AvatarAxisScale4", mAvatarAxisScale[4]);
	gSavedSettings.setF32("AvatarAxisScale5", mAvatarAxisScale[5]);

	gSavedSettings.setF32("BuildAxisScale0", mBuildAxisScale[0]);
	gSavedSettings.setF32("BuildAxisScale1", mBuildAxisScale[1]);
	gSavedSettings.setF32("BuildAxisScale2", mBuildAxisScale[2]);
	gSavedSettings.setF32("BuildAxisScale3", mBuildAxisScale[3]);
	gSavedSettings.setF32("BuildAxisScale4", mBuildAxisScale[4]);
	gSavedSettings.setF32("BuildAxisScale5", mBuildAxisScale[5]);

	gSavedSettings.setF32("FlycamAxisScale0", mFlycamAxisScale[0]);
	gSavedSettings.setF32("FlycamAxisScale1", mFlycamAxisScale[1]);
	gSavedSettings.setF32("FlycamAxisScale2", mFlycamAxisScale[2]);
	gSavedSettings.setF32("FlycamAxisScale3", mFlycamAxisScale[3]);
	gSavedSettings.setF32("FlycamAxisScale4", mFlycamAxisScale[4]);
	gSavedSettings.setF32("FlycamAxisScale5", mFlycamAxisScale[5]);
	gSavedSettings.setF32("FlycamAxisScale6", mFlycamAxisScale[6]);

	gSavedSettings.setF32("AvatarAxisDeadZone0", mAvatarAxisDeadZone[0]);
	gSavedSettings.setF32("AvatarAxisDeadZone1", mAvatarAxisDeadZone[1]);
	gSavedSettings.setF32("AvatarAxisDeadZone2", mAvatarAxisDeadZone[2]);
	gSavedSettings.setF32("AvatarAxisDeadZone3", mAvatarAxisDeadZone[3]);
	gSavedSettings.setF32("AvatarAxisDeadZone4", mAvatarAxisDeadZone[4]);
	gSavedSettings.setF32("AvatarAxisDeadZone5", mAvatarAxisDeadZone[5]);

	gSavedSettings.setF32("BuildAxisDeadZone0", mBuildAxisDeadZone[0]);
	gSavedSettings.setF32("BuildAxisDeadZone1", mBuildAxisDeadZone[1]);
	gSavedSettings.setF32("BuildAxisDeadZone2", mBuildAxisDeadZone[2]);
	gSavedSettings.setF32("BuildAxisDeadZone3", mBuildAxisDeadZone[3]);
	gSavedSettings.setF32("BuildAxisDeadZone4", mBuildAxisDeadZone[4]);
	gSavedSettings.setF32("BuildAxisDeadZone5", mBuildAxisDeadZone[5]);

	gSavedSettings.setF32("FlycamAxisDeadZone0", mFlycamAxisDeadZone[0]);
	gSavedSettings.setF32("FlycamAxisDeadZone1", mFlycamAxisDeadZone[1]);
	gSavedSettings.setF32("FlycamAxisDeadZone2", mFlycamAxisDeadZone[2]);
	gSavedSettings.setF32("FlycamAxisDeadZone3", mFlycamAxisDeadZone[3]);
	gSavedSettings.setF32("FlycamAxisDeadZone4", mFlycamAxisDeadZone[4]);
	gSavedSettings.setF32("FlycamAxisDeadZone5", mFlycamAxisDeadZone[5]);
	gSavedSettings.setF32("FlycamAxisDeadZone6", mFlycamAxisDeadZone[6]);

	gSavedSettings.setF32("AvatarFeathering", mAvatarFeathering);
	gSavedSettings.setF32("BuildFeathering", mBuildFeathering);
	gSavedSettings.setF32("FlycamFeathering", mFlycamFeathering);
}

void LLFloaterJoystick::onCommitJoystickEnabled(LLUICtrl*, void *joy_panel)
{
	LLFloaterJoystick* self = (LLFloaterJoystick*)joy_panel;

    LLSD value = self->mJoysticksCombo->getValue();
    bool joystick_enabled = true;
    if (value.isInteger())
    {
        // ndof already has a device selected, we are just setting it enabled or disabled
        joystick_enabled = value.asInteger();
    }
    else
    {
        LLViewerJoystick::getInstance()->initDevice(value);
        // else joystick is enabled, because combobox holds id of device
        joystick_enabled = true;
    }
    gSavedSettings.setBOOL("JoystickEnabled", joystick_enabled);
	BOOL flycam_enabled = self->mCheckFlycamEnabled->get();

	if (!joystick_enabled || !flycam_enabled)
	{
		// Turn off flycam
		LLViewerJoystick* joystick(LLViewerJoystick::getInstance());
		if (joystick->getOverrideCamera())
		{
			joystick->toggleFlycam();
		}
	}

    std::string device_id = LLViewerJoystick::getInstance()->getDeviceUUIDString();
    gSavedSettings.setString("JoystickDeviceUUID", device_id);
    LL_DEBUGS("Joystick") << "Selected " << device_id << " as joystick." << LL_ENDL;

    self->refreshListOfDevices();
}

void LLFloaterJoystick::onClickRestoreSNDefaults(void *joy_panel)
{
	setSNDefaults();
}

void LLFloaterJoystick::onClickCancel(void *joy_panel)
{
	if (joy_panel)
	{
		LLFloaterJoystick* self = (LLFloaterJoystick*)joy_panel;

		if (self)
		{
			self->cancel();
			self->closeFloater();
		}
	}
}

void LLFloaterJoystick::onClickOK(void *joy_panel)
{
	if (joy_panel)
	{
		LLFloaterJoystick* self = (LLFloaterJoystick*)joy_panel;

		if (self)
		{
			self->closeFloater();
		}
	}
}

void LLFloaterJoystick::onClickCloseBtn(bool app_quitting)
{
	cancel();
	closeFloater(app_quitting);
}

void LLFloaterJoystick::setSNDefaults()
{
	LLViewerJoystick::getInstance()->setSNDefaults();
}

void LLFloaterJoystick::onClose(bool app_quitting)
{
	if (app_quitting)
	{
		cancel();
	}
}
