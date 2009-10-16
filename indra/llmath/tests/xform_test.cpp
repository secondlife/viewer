/** 
 * @file xform_test.cpp
 * @author Adroit
 * @date March 2007 
 * @brief Test cases for LLXform
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "linden_common.h"
#include "../test/lltut.h"

#include "../xform.h"

namespace tut
{
	struct xform_test
	{
	};
	typedef test_group<xform_test> xform_test_t;
	typedef xform_test_t::object xform_test_object_t;
	tut::xform_test_t tut_xform_test("xform_test");

	//test case for init(), getParent(), getRotation(), getPositionW(), getWorldRotation() fns.
	template<> template<>
	void xform_test_object_t::test<1>()
	{
		LLXform xform_obj;
		LLVector3 emptyVec(0.f,0.f,0.f);
		LLVector3 initialScaleVec(1.f,1.f,1.f);

		ensure("LLXform empty constructor failed: ", !xform_obj.getParent() && !xform_obj.isChanged() &&
			xform_obj.getPosition() == emptyVec && 
			(xform_obj.getRotation()).isIdentity() &&
			xform_obj.getScale() == initialScaleVec && 
			xform_obj.getPositionW() == emptyVec && 
			(xform_obj.getWorldRotation()).isIdentity() &&
			!xform_obj.getScaleChildOffset());
	}

	// test cases for 
	// setScale(const LLVector3& scale) 
	// setScale(const F32 x, const F32 y, const F32 z)
	// setRotation(const F32 x, const F32 y, const F32 z) 
	// setPosition(const F32 x, const F32 y, const F32 z) 
	// getLocalMat4(LLMatrix4 &mat)
	template<> template<>
	void xform_test_object_t::test<2>()	
	{
		LLMatrix4 llmat4;
		LLXform xform_obj;

		F32 x = 3.6f;
		F32 y = 5.5f;
		F32 z = 4.2f;
		F32 w = 0.f;
		F32 posz = z + 2.122f;
		LLVector3 vec(x, y, z);
		xform_obj.setScale(x, y, z);
		xform_obj.setPosition(x, y, posz);
		ensure("setScale failed: ", xform_obj.getScale() == vec);

		vec.setVec(x, y, posz);
		ensure("getPosition failed: ", xform_obj.getPosition() == vec);

		x = x * 2.f;
		y = y + 2.3f;
		z = posz * 4.f; 
		vec.setVec(x, y, z);
		xform_obj.setPositionX(x);
		xform_obj.setPositionY(y);
		xform_obj.setPositionZ(z);
		ensure("setPositionX/Y/Z failed: ", xform_obj.getPosition() == vec);

		xform_obj.setScaleChildOffset(TRUE);
		ensure("setScaleChildOffset failed: ", xform_obj.getScaleChildOffset());

		vec.setVec(x, y, z);

		xform_obj.addPosition(vec);
		vec += vec;
		ensure("addPosition failed: ", xform_obj.getPosition() == vec);

		xform_obj.setScale(vec);
		ensure("setScale vector failed: ", xform_obj.getScale() == vec);

		LLQuaternion quat(x, y, z, w);
		xform_obj.setRotation(quat);
		ensure("setRotation quat failed: ", xform_obj.getRotation() == quat);

		xform_obj.setRotation(x, y, z, w);
		ensure("getRotation 2 failed: ", xform_obj.getRotation() == quat);

		xform_obj.setRotation(x, y, z);
		quat.setQuat(x,y,z); 
		ensure("setRotation xyz failed: ", xform_obj.getRotation() == quat);

		// LLXform::setRotation(const F32 x, const F32 y, const F32 z) 
		//		Does normalization
		// LLXform::setRotation(const F32 x, const F32 y, const F32 z, const F32 s) 
		//		Simply copies the individual values - does not do any normalization. 
		// Is that the expected behavior?
	}

	// test cases for inline BOOL setParent(LLXform *parent) and getParent() fn.
	template<> template<>
	void xform_test_object_t::test<3>()	
	{		
		LLXform xform_obj;
		LLXform par;
		LLXform grandpar;
		xform_obj.setParent(&par); 
		par.setParent(&grandpar); 
		ensure("setParent/getParent failed: ", &par == xform_obj.getParent());
		ensure("getRoot failed: ", &grandpar == xform_obj.getRoot());
		ensure("isRoot failed: ", grandpar.isRoot() && !par.isRoot() && !xform_obj.isRoot());
		ensure("isRootEdit failed: ", grandpar.isRootEdit() && !par.isRootEdit() && !xform_obj.isRootEdit());
	}

	template<> template<>
	void xform_test_object_t::test<4>()	
	{
		LLXform xform_obj;
		xform_obj.setChanged(LLXform::TRANSLATED | LLXform::ROTATED | LLXform::SCALED);
		ensure("setChanged/isChanged failed: ", xform_obj.isChanged());

		xform_obj.clearChanged(LLXform::TRANSLATED | LLXform::ROTATED | LLXform::SCALED);
		ensure("clearChanged failed: ", !xform_obj.isChanged());
		
		LLVector3 llvect3(12.4f, -5.6f, 0.34f);
		xform_obj.setScale(llvect3);
		ensure("setScale did not set SCALED flag: ", xform_obj.isChanged(LLXform::SCALED));
		xform_obj.setPosition(1.2f, 2.3f, 3.4f);
		ensure("setScale did not set TRANSLATED flag: ", xform_obj.isChanged(LLXform::TRANSLATED));
		ensure("TRANSLATED reset SCALED flag: ", xform_obj.isChanged(LLXform::TRANSLATED | LLXform::SCALED));
		xform_obj.clearChanged(LLXform::SCALED);
		ensure("reset SCALED failed: ", !xform_obj.isChanged(LLXform::SCALED));
		xform_obj.setRotation(1, 2, 3, 4);
		ensure("ROTATION flag not set ", xform_obj.isChanged(LLXform::TRANSLATED | LLXform::ROTATED));
		xform_obj.setScale(llvect3);
		ensure("ROTATION flag not set ", xform_obj.isChanged(LLXform::MOVED));
	}

	//to test init() and getWorldMatrix() fns.
	template<> template<>
	void xform_test_object_t::test<5>()	
	{
		LLXformMatrix formMatrix_obj;
		formMatrix_obj.init();
		LLMatrix4 mat4_obj;
		
		ensure("1. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[0][0]);
		ensure("2. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[0][1]);
		ensure("3. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[0][2]);
		ensure("4. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[0][3]);
		ensure("5. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[1][0]);
		ensure("6. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[1][1]);
		ensure("7. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[1][2]);
		ensure("8. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[1][3]);
		ensure("9. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[2][0]);
		ensure("10. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[2][1]);
		ensure("11. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[2][2]);
		ensure("12. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[2][3]);
		ensure("13. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[3][0]);
		ensure("14. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[3][1]);
		ensure("15. The value is not NULL", 0.f == formMatrix_obj.getWorldMatrix().mMatrix[3][2]);
		ensure("16. The value is not NULL", 1.f == formMatrix_obj.getWorldMatrix().mMatrix[3][3]);
	}

	//to test mMin.clearVec() and mMax.clearVec() fns
	template<> template<>
	void xform_test_object_t::test<6>()	
	{
		LLXformMatrix formMatrix_obj;
		formMatrix_obj.init();
		LLVector3 llmin_vec3;
		LLVector3 llmax_vec3;
		formMatrix_obj.getMinMax(llmin_vec3, llmax_vec3);
		ensure("1. The value is not NULL", 0.f == llmin_vec3.mV[0]);
		ensure("2. The value is not NULL", 0.f == llmin_vec3.mV[1]);
		ensure("3. The value is not NULL", 0.f == llmin_vec3.mV[2]);
		ensure("4. The value is not NULL", 0.f == llmin_vec3.mV[0]);
		ensure("5. The value is not NULL", 0.f == llmin_vec3.mV[1]);
		ensure("6. The value is not NULL", 0.f == llmin_vec3.mV[2]);
	}

	//test case of update() fn.
	template<> template<>
	void xform_test_object_t::test<7>()	
	{
		LLXformMatrix formMatrix_obj;

		LLXformMatrix parent;
		LLVector3 llvecpos(1.0, 2.0, 3.0);
		LLVector3 llvecpospar(10.0, 20.0, 30.0);
		formMatrix_obj.setPosition(llvecpos);
		parent.setPosition(llvecpospar);

		LLVector3 llvecparentscale(1.0, 2.0, 0);
		parent.setScaleChildOffset(TRUE);
		parent.setScale(llvecparentscale);

		LLQuaternion quat(1, 2, 3, 4);
		LLQuaternion quatparent(5, 6, 7, 8);
		formMatrix_obj.setRotation(quat);
		parent.setRotation(quatparent);
		formMatrix_obj.setParent(&parent);

		parent.update();
		formMatrix_obj.update();

		LLVector3 worldPos = llvecpos;
		worldPos.scaleVec(llvecparentscale);
		worldPos *= quatparent;
		worldPos += llvecpospar;

		LLQuaternion worldRot = quat * quatparent; 

		ensure("getWorldPosition failed: ", formMatrix_obj.getWorldPosition() == worldPos);
		ensure("getWorldRotation failed: ", formMatrix_obj.getWorldRotation() == worldRot);

		ensure("getWorldPosition for parent failed: ", parent.getWorldPosition() == llvecpospar);
		ensure("getWorldRotation for parent failed: ", parent.getWorldRotation() == quatparent);
	}
}	

