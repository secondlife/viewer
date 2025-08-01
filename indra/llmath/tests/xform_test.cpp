/**
 * @file xform_test.cpp
 * @author Adroit
 * @date March 2007
 * @brief Test cases for LLXform
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

#include "linden_common.h"
#include "../test/lldoctest.h"

#include "../xform.h"

TEST_SUITE("LLXForm") {

TEST_CASE("test_1")
{

        LLXform xform_obj;
        LLVector3 emptyVec(0.f,0.f,0.f);
        LLVector3 initialScaleVec(1.f,1.f,1.f);

        CHECK_MESSAGE(!xform_obj.getParent(, "LLXform empty constructor failed: ") && !xform_obj.isChanged() &&
            xform_obj.getPosition() == emptyVec &&
            (xform_obj.getRotation()).isIdentity() &&
            xform_obj.getScale() == initialScaleVec &&
            xform_obj.getPositionW() == emptyVec &&
            (xform_obj.getWorldRotation()).isIdentity() &&
            !xform_obj.getScaleChildOffset());
    
}

TEST_CASE("test_2")
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
        CHECK_MESSAGE(xform_obj.getScale(, "setScale failed: ") == vec);

        vec.setVec(x, y, posz);
        CHECK_MESSAGE(xform_obj.getPosition(, "getPosition failed: ") == vec);

        x = x * 2.f;
        y = y + 2.3f;
        z = posz * 4.f;
        vec.setVec(x, y, z);
        xform_obj.setPositionX(x);
        xform_obj.setPositionY(y);
        xform_obj.setPositionZ(z);
        CHECK_MESSAGE(xform_obj.getPosition(, "setPositionX/Y/Z failed: ") == vec);

        xform_obj.setScaleChildOffset(true);
        CHECK_MESSAGE(xform_obj.getScaleChildOffset(, "setScaleChildOffset failed: "));

        vec.setVec(x, y, z);

        xform_obj.addPosition(vec);
        vec += vec;
        CHECK_MESSAGE(xform_obj.getPosition(, "addPosition failed: ") == vec);

        xform_obj.setScale(vec);
        CHECK_MESSAGE(xform_obj.getScale(, "setScale vector failed: ") == vec);

        LLQuaternion quat(x, y, z, w);
        xform_obj.setRotation(quat);
        CHECK_MESSAGE(xform_obj.getRotation(, "setRotation quat failed: ") == quat);

        xform_obj.setRotation(x, y, z, w);
        CHECK_MESSAGE(xform_obj.getRotation(, "getRotation 2 failed: ") == quat);

        xform_obj.setRotation(x, y, z);
        quat.setQuat(x,y,z);
        CHECK_MESSAGE(xform_obj.getRotation(, "setRotation xyz failed: ") == quat);

        // LLXform::setRotation(const F32 x, const F32 y, const F32 z)
        //      Does normalization
        // LLXform::setRotation(const F32 x, const F32 y, const F32 z, const F32 s)
        //      Simply copies the individual values - does not do any normalization.
        // Is that the expected behavior?
    
}

TEST_CASE("test_3")
{

        LLXform xform_obj;
        LLXform par;
        LLXform grandpar;
        xform_obj.setParent(&par);
        par.setParent(&grandpar);
        CHECK_MESSAGE(&par == xform_obj.getParent(, "setParent/getParent failed: "));
        CHECK_MESSAGE(&grandpar == xform_obj.getRoot(, "getRoot failed: "));
        CHECK_MESSAGE(grandpar.isRoot(, "isRoot failed: ") && !par.isRoot() && !xform_obj.isRoot());
        CHECK_MESSAGE(grandpar.isRootEdit(, "isRootEdit failed: ") && !par.isRootEdit() && !xform_obj.isRootEdit());
    
}

TEST_CASE("test_4")
{

        LLXform xform_obj;
        xform_obj.setChanged(LLXform::TRANSLATED | LLXform::ROTATED | LLXform::SCALED);
        CHECK_MESSAGE(xform_obj.isChanged(, "setChanged/isChanged failed: "));

        xform_obj.clearChanged(LLXform::TRANSLATED | LLXform::ROTATED | LLXform::SCALED);
        CHECK_MESSAGE(!xform_obj.isChanged(, "clearChanged failed: "));

        LLVector3 llvect3(12.4f, -5.6f, 0.34f);
        xform_obj.setScale(llvect3);
        CHECK_MESSAGE(xform_obj.isChanged(LLXform::SCALED, "setScale did not set SCALED flag: "));
        xform_obj.setPosition(1.2f, 2.3f, 3.4f);
        CHECK_MESSAGE(xform_obj.isChanged(LLXform::TRANSLATED, "setScale did not set TRANSLATED flag: "));
        CHECK_MESSAGE(xform_obj.isChanged(LLXform::TRANSLATED | LLXform::SCALED, "TRANSLATED reset SCALED flag: "));
        xform_obj.clearChanged(LLXform::SCALED);
        CHECK_MESSAGE(!xform_obj.isChanged(LLXform::SCALED, "reset SCALED failed: "));
        xform_obj.setRotation(1, 2, 3, 4);
        CHECK_MESSAGE(xform_obj.isChanged(LLXform::TRANSLATED | LLXform::ROTATED, "ROTATION flag not set "));
        xform_obj.setScale(llvect3);
        CHECK_MESSAGE(xform_obj.isChanged(LLXform::MOVED, "ROTATION flag not set "));
    
}

TEST_CASE("test_5")
{

        LLXformMatrix formMatrix_obj;
        formMatrix_obj.init();
        LLMatrix4 mat4_obj;

        CHECK_MESSAGE(1.f == formMatrix_obj.getWorldMatrix(, "1. The value is not NULL").mMatrix[0][0]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "2. The value is not NULL").mMatrix[0][1]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "3. The value is not NULL").mMatrix[0][2]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "4. The value is not NULL").mMatrix[0][3]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "5. The value is not NULL").mMatrix[1][0]);
        CHECK_MESSAGE(1.f == formMatrix_obj.getWorldMatrix(, "6. The value is not NULL").mMatrix[1][1]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "7. The value is not NULL").mMatrix[1][2]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "8. The value is not NULL").mMatrix[1][3]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "9. The value is not NULL").mMatrix[2][0]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "10. The value is not NULL").mMatrix[2][1]);
        CHECK_MESSAGE(1.f == formMatrix_obj.getWorldMatrix(, "11. The value is not NULL").mMatrix[2][2]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "12. The value is not NULL").mMatrix[2][3]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "13. The value is not NULL").mMatrix[3][0]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "14. The value is not NULL").mMatrix[3][1]);
        CHECK_MESSAGE(0.f == formMatrix_obj.getWorldMatrix(, "15. The value is not NULL").mMatrix[3][2]);
        CHECK_MESSAGE(1.f == formMatrix_obj.getWorldMatrix(, "16. The value is not NULL").mMatrix[3][3]);
    
}

} // TEST_SUITE
