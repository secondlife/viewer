/** 
 * @file llwlparammanager_test.cpp
 * @brief LLWLParamManager tests
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

// Precompiled headers
#include "../llviewerprecompiledheaders.h"

// Class to test
#include "../llwlparammanager.h"

// Dependencies
#include "linden_common.h"

// TUT header
#include "lltut.h"

// Stubs
#include "llwldaycycle_stub.cpp"
#include "llwlparamset_stub.cpp"
#include "llwlanimator_stub.cpp"
#include "llglslshader_stub.cpp"
#include "lldir_stub.cpp"
#include "llsky_stub.cpp"
#include "llpipeline_stub.cpp"
#include "llviewershadermgr_stub.cpp"

void assert_glerror(void) {}
LLViewerCamera::LLViewerCamera() {}
void LLViewerCamera::setView(F32 vertical_fov_rads) {}
std::string LLTrans::getString(const std::string &xml_desc, const LLStringUtil::format_map_t& args) { return std::string(""); }

char* curl_unescape(const char* c_str, int length)
{
	char* copy = new char[length+4];
	memcpy(copy, c_str, length);
	copy[length+0] = 'E';
	copy[length+1] = 'S';
	copy[length+2] = 'C';
	copy[length+3] = '\0';
	return copy;
}
void curl_free(void* p) {delete[] ((char*)p);}
char* curl_escape(const char* c_str, int length) {
	char* copy = new char[length+6];
	memcpy(copy, c_str, length);
	copy[length+0] = 'U';
	copy[length+1] = 'N';
	copy[length+2] = 'E';
	copy[length+3] = 'S';
	copy[length+4] = 'C';
	copy[length+5] = '\0';
	return copy;
}

namespace tut
{
	// Main Setup
	struct LLWLParamManagerFixture
	{
		class LLWLParamManagerTest
		{
		};

		LLWLParamManager* mTestManager;

		LLWLParamManagerFixture()
			: mTestManager(LLWLParamManager::getInstance())
		{
		}

		~LLWLParamManagerFixture()
		{
		}
	};
	typedef test_group<LLWLParamManagerFixture> factory;
	typedef factory::object object;
	factory tf("LLWLParamManager test");

	// Tests
	template<> template<>
	void object::test<1>()
	{
		try
		{
			std::string preset =
			"<llsd>\
				<map>\
				<key>ambient</key>\
					<array>\
						<real>1.0499999523162842</real>\
						<real>1.0499999523162842</real>\
						<real>1.0499999523162842</real>\
						<real>0.34999999403953552</real>\
					</array>\
				<key>blue_density</key>\
					<array>\
						<real>0.2447581488182351</real>\
						<real>0.44872328639030457</real>\
						<real>0.75999999046325684</real>\
						<real>0.38000004053115788</real>\
					</array>\
				<key>blue_horizon</key>\
					<array>\
						<real>0.49548382097675159</real>\
						<real>0.49548381382419748</real>\
						<real>0.63999999284744291</real>\
						<real>0.31999999642372146</real>\
					</array>\
				<key>cloud_color</key>\
					<array>\
						<real>0.40999999165535073</real>\
						<real>0.40999999165535073</real>\
						<real>0.40999999165535073</real>\
						<real>0.40999999165535073</real>\
					</array>\
				<key>cloud_pos_density1</key>\
					<array>\
						<real>1.6884100437164307</real>\
						<real>0.52609699964523315</real>\
						<real>0.99999999999999289</real>\
						<real>1</real>\
					</array>\
				<key>cloud_pos_density2</key>\
					<array>\
						<real>1.6884100437164307</real>\
						<real>0.52609699964523315</real>\
						<real>0.125</real>\
						<real>1</real>\
					</array>\
				<key>cloud_scale</key>\
					<array>\
						<real>0.4199999868869746</real>\
						<real>0</real>\
						<real>0</real>\
						<real>1</real>\
					</array>\
				<key>cloud_scroll_rate</key>\
					<array>\
						<real>10.199999809265137</real>\
						<real>10.01099967956543</real>\
					</array>\
				<key>cloud_shadow</key>\
					<array>\
						<real>0.26999998092651367</real>\
						<real>0</real>\
						<real>0</real>\
						<real>1</real>\
					</array>\
				<key>density_multiplier</key>\
					<array>\
						<real>0.00017999998817685818</real>\
						<real>0</real>\
						<real>0</real>\
						<real>1</real>\
					</array>\
				<key>distance_multiplier</key>\
					<array>\
						<real>0.80000001192093606</real>\
						<real>0</real>\
						<real>0</real>\
						<real>1</real>\
					</array>\
				<key>east_angle</key>\
					<real>0</real>\
				<key>enable_cloud_scroll</key>\
					<array>\
						<boolean>1</boolean>\
						<boolean>1</boolean>\
					</array>\
				<key>gamma</key>\
					<array>\
						<real>1</real>\
						<real>0</real>\
						<real>0</real>\
						<real>1</real>\
					</array>\
				<key>glow</key>\
					<array>\
						<real>5</real>\
						<real>0.0010000000474974513</real>\
						<real>-0.47999998927116394</real>\
						<real>1</real>\
					</array>\
				<key>haze_density</key>\
					<array>\
						<real>0.69999998807907104</real>\
						<real>0</real>\
						<real>0</real>\
						<real>1</real>\
					</array>\
				<key>haze_horizon</key>\
					<array>\
						<real>0.18999999761581243</real>\
						<real>0.19915600121021271</real>\
						<real>0.19915600121021271</real>\
						<real>1</real>\
					</array>\
				<key>lightnorm</key>\
					<array>\
						<real>0</real>\
						<real>0.70710659027099609</real>\
						<real>-0.70710694789886475</real>\
						<real>0</real>\
					</array>\
				<key>max_y</key>\
					<array>\
						<real>1605</real>\
						<real>0</real>\
						<real>0</real>\
						<real>1</real>\
					</array>\
				<key>preset_num</key>\
					<integer>22</integer>\
				<key>star_brightness</key>\
					<real>0</real>\
				<key>sun_angle</key>\
					<real>2.3561947345733643</real>\
				<key>sunlight_color</key>\
					<array>\
						<real>0.73421055078505759</real>\
						<real>0.78157895803450828</real>\
						<real>0.89999997615813498</real>\
						<real>0.29999998211860301</real>\
					</array>\
				</map>\
			</llsd>";

			std::stringstream preset_stream(preset);
			mTestManager->loadPresetFromXML(LLWLParamKey("test1", LLWLParamKey::SCOPE_LOCAL), preset_stream);
			LLWLParamSet dummy;
			ensure("Couldn't get ParamSet after loading it", mTestManager->getParamSet(LLWLParamKey("test1", LLWLParamKey::SCOPE_LOCAL), dummy));
		}
		catch (...)
		{
			fail("loadPresetFromXML test crashed!");
		}
	}

	template<> template<>
	void object::test<2>()
	{
		mTestManager->propagateParameters();
		ensure_equals("Wrong value from getDomeOffset()", mTestManager->getDomeOffset(), 0.96f);
		ensure_equals("Wrong value from getDomeRadius()", mTestManager->getDomeRadius(), 15000.f);
		ensure_equals("Wrong value from getLightDir()", mTestManager->getLightDir(), LLVector4(-0,0,1,0));
		ensure_equals("Wrong value from getClampedLightDir()", mTestManager->getClampedLightDir(), LLVector4(-0,0,1,0));
		ensure_equals("Wrong value from getRotatedLightDir()", mTestManager->getRotatedLightDir(), LLVector4(0,0,0,1));
	}
}
