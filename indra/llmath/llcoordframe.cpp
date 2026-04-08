/**
 * @file llcoordframe.cpp
 * @brief LLCoordFrame class implementation.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "v3math.h"
#include "m3math.h"
#include "v4math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "llcoordframe.h"

namespace
{
inline bool glm_is_finite(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
}

#define CHECK_FINITE(var)                                            \
    if (!glm_is_finite(var))                                         \
    {                                                                \
        LL_WARNS() << "Non Finite " << std::string(#var) << LL_ENDL; \
        reset();                                                     \
    }

#define CHECK_FINITE_OBJ()                                       \
    if (!isFinite())                                             \
    {                                                            \
        LL_WARNS() << "Non Finite in LLCoordFrame " << LL_ENDL;  \
        reset();                                                 \
    }

#ifndef X_AXIS
    #define X_AXIS 1.0f,0.0f,0.0f
    #define Y_AXIS 0.0f,1.0f,0.0f
    #define Z_AXIS 0.0f,0.0f,1.0f
#endif

// Constructors

LLCoordFrame::LLCoordFrame() :
    mOrigin(0.f, 0.f, 0.f),
    mXAxis(X_AXIS),
    mYAxis(Y_AXIS),
    mZAxis(Z_AXIS)
{
}

LLCoordFrame::LLCoordFrame(const glm::vec3 &origin) :
    mOrigin(origin),
    mXAxis(X_AXIS),
    mYAxis(Y_AXIS),
    mZAxis(Z_AXIS)
{
    CHECK_FINITE(mOrigin);
}

LLCoordFrame::LLCoordFrame(const glm::vec3 &origin, const glm::vec3 &direction) :
    mOrigin(origin)
{
    lookDir(direction);

    CHECK_FINITE_OBJ();
}

LLCoordFrame::LLCoordFrame(const glm::vec3 &x_axis,
                           const glm::vec3 &y_axis,
                           const glm::vec3 &z_axis) :
    mOrigin(0.f, 0.f, 0.f),
    mXAxis(x_axis),
    mYAxis(y_axis),
    mZAxis(z_axis)
{
    CHECK_FINITE_OBJ();
}

LLCoordFrame::LLCoordFrame(const glm::vec3 &origin,
                           const glm::vec3 &x_axis,
                           const glm::vec3 &y_axis,
                           const glm::vec3 &z_axis) :
    mOrigin(origin),
    mXAxis(x_axis),
    mYAxis(y_axis),
    mZAxis(z_axis)
{
    CHECK_FINITE_OBJ();
}


LLCoordFrame::LLCoordFrame(const glm::vec3 &origin,
                           const LLMatrix3 &rotation) :
    mOrigin(origin),
    mXAxis(rotation.mMatrix[VX][0], rotation.mMatrix[VX][1], rotation.mMatrix[VX][2]),
    mYAxis(rotation.mMatrix[VY][0], rotation.mMatrix[VY][1], rotation.mMatrix[VY][2]),
    mZAxis(rotation.mMatrix[VZ][0], rotation.mMatrix[VZ][1], rotation.mMatrix[VZ][2])
{
    CHECK_FINITE_OBJ();
}

LLCoordFrame::LLCoordFrame(const LLQuaternion &q) :
    mOrigin(0.f, 0.f, 0.f)
{
    LLMatrix3 rotation_matrix(q);
    mXAxis = glm::vec3(rotation_matrix.mMatrix[VX][0], rotation_matrix.mMatrix[VX][1], rotation_matrix.mMatrix[VX][2]);
    mYAxis = glm::vec3(rotation_matrix.mMatrix[VY][0], rotation_matrix.mMatrix[VY][1], rotation_matrix.mMatrix[VY][2]);
    mZAxis = glm::vec3(rotation_matrix.mMatrix[VZ][0], rotation_matrix.mMatrix[VZ][1], rotation_matrix.mMatrix[VZ][2]);

    CHECK_FINITE_OBJ();
}

LLCoordFrame::LLCoordFrame(const glm::vec3 &origin, const LLQuaternion &q) :
    mOrigin(origin)
{
    LLMatrix3 rotation_matrix(q);
    mXAxis = glm::vec3(rotation_matrix.mMatrix[VX][0], rotation_matrix.mMatrix[VX][1], rotation_matrix.mMatrix[VX][2]);
    mYAxis = glm::vec3(rotation_matrix.mMatrix[VY][0], rotation_matrix.mMatrix[VY][1], rotation_matrix.mMatrix[VY][2]);
    mZAxis = glm::vec3(rotation_matrix.mMatrix[VZ][0], rotation_matrix.mMatrix[VZ][1], rotation_matrix.mMatrix[VZ][2]);

    CHECK_FINITE_OBJ();
}

LLCoordFrame::LLCoordFrame(const LLMatrix4 &mat) :
    mOrigin(mat.mMatrix[VW][0], mat.mMatrix[VW][1], mat.mMatrix[VW][2]),
    mXAxis(mat.mMatrix[VX][0], mat.mMatrix[VX][1], mat.mMatrix[VX][2]),
    mYAxis(mat.mMatrix[VY][0], mat.mMatrix[VY][1], mat.mMatrix[VY][2]),
    mZAxis(mat.mMatrix[VZ][0], mat.mMatrix[VZ][1], mat.mMatrix[VZ][2])
{
    CHECK_FINITE_OBJ();
}


// The folowing two constructors are dangerous due to implicit casting and have been disabled - SJB
/*
LLCoordFrame::LLCoordFrame(const F32 *origin, const F32 *rotation) :
    mOrigin(origin),
    mXAxis(rotation+3*VX),
    mYAxis(rotation+3*VY),
    mZAxis(rotation+3*VZ)
{
    CHECK_FINITE_OBJ();
}
*/

/*
LLCoordFrame::LLCoordFrame(const F32 *origin_and_rotation) :
    mOrigin(origin_and_rotation),
    mXAxis(origin_and_rotation + 3*(VX+1)),
    mYAxis(origin_and_rotation + 3*(VY+1)),
    mZAxis(origin_and_rotation + 3*(VZ+1))
{
    CHECK_FINITE_OBJ();
}
*/


void LLCoordFrame::reset()
{
    mOrigin = glm::vec3(0.0f, 0.0f, 0.0f);
    resetAxes();
}


void LLCoordFrame::resetAxes()
{
    mXAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    mYAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    mZAxis = glm::vec3(0.0f, 0.0f, 1.0f);
}

// setOrigin() member functions set mOrigin

void LLCoordFrame::setOrigin(F32 x, F32 y, F32 z)
{
    mOrigin = glm::vec3(x, y, z);

    CHECK_FINITE(mOrigin);
}

void LLCoordFrame::setOrigin(const glm::vec3 &new_origin)
{
    mOrigin = new_origin;
    CHECK_FINITE(mOrigin);
}

void LLCoordFrame::setOrigin(const F32 *origin)
{
    mOrigin.x = *(origin + VX);
    mOrigin.y = *(origin + VY);
    mOrigin.z = *(origin + VZ);
    CHECK_FINITE(mOrigin);
}

void LLCoordFrame::setOrigin(const LLCoordFrame &frame)
{
    mOrigin = frame.getOrigin();
    CHECK_FINITE(mOrigin);
}

// setAxes()  member functions set the axes, and assume that
// the arguments are orthogonal and normalized.

void LLCoordFrame::setAxes(const glm::vec3 &x_axis,
                          const glm::vec3 &y_axis,
                          const glm::vec3 &z_axis)
{
    mXAxis = x_axis;
    mYAxis = y_axis;
    mZAxis = z_axis;
    CHECK_FINITE_OBJ();
}


void LLCoordFrame::setAxes(const LLMatrix3 &rotation_matrix)
{
    mXAxis = glm::vec3(rotation_matrix.mMatrix[VX][0], rotation_matrix.mMatrix[VX][1], rotation_matrix.mMatrix[VX][2]);
    mYAxis = glm::vec3(rotation_matrix.mMatrix[VY][0], rotation_matrix.mMatrix[VY][1], rotation_matrix.mMatrix[VY][2]);
    mZAxis = glm::vec3(rotation_matrix.mMatrix[VZ][0], rotation_matrix.mMatrix[VZ][1], rotation_matrix.mMatrix[VZ][2]);
    CHECK_FINITE_OBJ();
}


void LLCoordFrame::setAxes(const LLQuaternion &q )
{
    LLMatrix3 rotation_matrix(q);
    setAxes(rotation_matrix);
    CHECK_FINITE_OBJ();
}


void LLCoordFrame::setAxes(  const F32 *rotation_matrix )
{
    mXAxis.x = *(rotation_matrix + 3*VX + VX);
    mXAxis.y = *(rotation_matrix + 3*VX + VY);
    mXAxis.z = *(rotation_matrix + 3*VX + VZ);
    mYAxis.x = *(rotation_matrix + 3*VY + VX);
    mYAxis.y = *(rotation_matrix + 3*VY + VY);
    mYAxis.z = *(rotation_matrix + 3*VY + VZ);
    mZAxis.x = *(rotation_matrix + 3*VZ + VX);
    mZAxis.y = *(rotation_matrix + 3*VZ + VY);
    mZAxis.z = *(rotation_matrix + 3*VZ + VZ);

    CHECK_FINITE_OBJ();
}


void LLCoordFrame::setAxes(const LLCoordFrame &frame)
{
    mXAxis = frame.getXAxis();
    mYAxis = frame.getYAxis();
    mZAxis = frame.getZAxis();
    CHECK_FINITE_OBJ();
}

// translate() member functions move mOrigin to a relative position
void LLCoordFrame::translate(F32 x, F32 y, F32 z)
{
    mOrigin.x += x;
    mOrigin.y += y;
    mOrigin.z += z;
    CHECK_FINITE(mOrigin);
}

void LLCoordFrame::translate(const glm::vec3 &v)
{
    mOrigin += v;
    CHECK_FINITE(mOrigin);
}


void LLCoordFrame::translate(const F32 *origin)
{
    mOrigin.x += *(origin + VX);
    mOrigin.y += *(origin + VY);
    mOrigin.z += *(origin + VZ);
    CHECK_FINITE(mOrigin);
}


// Rotate move the axes to a relative rotation

void LLCoordFrame::rotate(F32 angle, F32 x, F32 y, F32 z)
{
    LLQuaternion q(angle, LLVector3(x,y,z));
    rotate(q);
    CHECK_FINITE_OBJ();
}


void LLCoordFrame::rotate(F32 angle, const glm::vec3 &rotation_axis)
{
    LLQuaternion q(angle, LLVector3(rotation_axis.x, rotation_axis.y, rotation_axis.z));
    rotate(q);
    CHECK_FINITE_OBJ();
}


void LLCoordFrame::rotate(const LLQuaternion &q)
{
    LLMatrix3 rotation_matrix(q);
    rotate(rotation_matrix);
    CHECK_FINITE_OBJ();
}


void LLCoordFrame::rotate(const LLMatrix3 &rotation_matrix)
{
    LLVector3 ll_x(mXAxis.x, mXAxis.y, mXAxis.z);
    LLVector3 ll_y(mYAxis.x, mYAxis.y, mYAxis.z);
    ll_x.rotVec(rotation_matrix);
    ll_y.rotVec(rotation_matrix);
    mXAxis = glm::vec3(ll_x.mV[VX], ll_x.mV[VY], ll_x.mV[VZ]);
    mYAxis = glm::vec3(ll_y.mV[VX], ll_y.mV[VY], ll_y.mV[VZ]);
    orthonormalize();
    CHECK_FINITE_OBJ();
}


// Rotate 2 normalized orthogonal vectors in direction from `source` to `target`
static void rotate2(glm::vec3& source, glm::vec3& target, F32 angle)
{
    F32 sx = source.x, sy = source.y, sz = source.z;
    F32 tx = target.x, ty = target.y, tz = target.z;
    F32 c = cosf(angle), s = sinf(angle);

    source = glm::vec3(sx * c + tx * s, sy * c + ty * s, sz * c + tz * s);
    target = glm::vec3(tx * c - sx * s, ty * c - sy * s, tz * c - sz * s);
}

void LLCoordFrame::roll(F32 angle)
{
    rotate2(mYAxis, mZAxis, angle);
}

void LLCoordFrame::pitch(F32 angle)
{
    rotate2(mZAxis, mXAxis, angle);
}

void LLCoordFrame::yaw(F32 angle)
{
    rotate2(mXAxis, mYAxis, angle);
}

// get*() routines


LLQuaternion LLCoordFrame::getQuaternion() const
{
    LLQuaternion quat(LLVector3(mXAxis.x, mXAxis.y, mXAxis.z),
                      LLVector3(mYAxis.x, mYAxis.y, mYAxis.z),
                      LLVector3(mZAxis.x, mZAxis.y, mZAxis.z));
    return quat;
}


void LLCoordFrame::getMatrixToLocal(LLMatrix4& mat) const
{
    LLVector3 ll_x(mXAxis.x, mXAxis.y, mXAxis.z);
    LLVector3 ll_y(mYAxis.x, mYAxis.y, mYAxis.z);
    LLVector3 ll_z(mZAxis.x, mZAxis.y, mZAxis.z);
    mat.setFwdCol(ll_x);
    mat.setLeftCol(ll_y);
    mat.setUpCol(ll_z);

    glm::vec3 c0(mat.mMatrix[0][0], mat.mMatrix[1][0], mat.mMatrix[2][0]);
    glm::vec3 c1(mat.mMatrix[0][1], mat.mMatrix[1][1], mat.mMatrix[2][1]);
    glm::vec3 c2(mat.mMatrix[0][2], mat.mMatrix[1][2], mat.mMatrix[2][2]);
    mat.mMatrix[3][0] = -glm::dot(mOrigin, c0);
    mat.mMatrix[3][1] = -glm::dot(mOrigin, c1);
    mat.mMatrix[3][2] = -glm::dot(mOrigin, c2);
}


void LLCoordFrame::getRotMatrixToParent(LLMatrix4& mat) const
{
    // Note: moves into CFR
    LLVector3 ll_x(mXAxis.x, mXAxis.y, mXAxis.z);
    LLVector3 ll_y(mYAxis.x, mYAxis.y, mYAxis.z);
    LLVector3 ll_z(mZAxis.x, mZAxis.y, mZAxis.z);
    mat.setFwdRow(  -ll_y );
    mat.setLeftRow(  ll_z );
    mat.setUpRow(   -ll_x );
}

size_t LLCoordFrame::writeOrientation(char *buffer) const
{
    memcpy(buffer, glm::value_ptr(mOrigin), 3*sizeof(F32)); /*Flawfinder: ignore */
    buffer += 3*sizeof(F32);
    memcpy(buffer, glm::value_ptr(mXAxis), 3*sizeof(F32)); /*Flawfinder: ignore */
    buffer += 3*sizeof(F32);
    memcpy(buffer, glm::value_ptr(mYAxis), 3*sizeof(F32));/*Flawfinder: ignore */
    buffer += 3*sizeof(F32);
    memcpy(buffer, glm::value_ptr(mZAxis), 3*sizeof(F32));   /*Flawfinder: ignore */
    return 12*sizeof(F32);
}


size_t LLCoordFrame::readOrientation(const char *buffer)
{
    memcpy(glm::value_ptr(mOrigin), buffer, 3*sizeof(F32));  /*Flawfinder: ignore */
    buffer += 3*sizeof(F32);
    memcpy(glm::value_ptr(mXAxis), buffer, 3*sizeof(F32));   /*Flawfinder: ignore */
    buffer += 3*sizeof(F32);
    memcpy(glm::value_ptr(mYAxis), buffer, 3*sizeof(F32));   /*Flawfinder: ignore */
    buffer += 3*sizeof(F32);
    memcpy(glm::value_ptr(mZAxis), buffer, 3*sizeof(F32));   /*Flawfinder: ignore */

    if( !isFinite() )
    {
        reset();
        LL_WARNS() << "Non Finite in LLCoordFrame::readOrientation()" << LL_ENDL;
    }

    return 12*sizeof(F32);
}


// rotation and transform vectors between reference frames

glm::vec3 LLCoordFrame::rotateToLocal(const glm::vec3 &absolute_vector) const
{
    return glm::vec3(glm::dot(mXAxis, absolute_vector),
                     glm::dot(mYAxis, absolute_vector),
                     glm::dot(mZAxis, absolute_vector));
}


LLVector4 LLCoordFrame::rotateToLocal(const LLVector4 &absolute_vector) const
{
    LLVector4 local_vector(mXAxis.x * absolute_vector.mV[VX] +
                               mXAxis.y * absolute_vector.mV[VY] +
                               mXAxis.z * absolute_vector.mV[VZ],
                           mYAxis.x * absolute_vector.mV[VX] +
                               mYAxis.y * absolute_vector.mV[VY] +
                               mYAxis.z * absolute_vector.mV[VZ],
                           mZAxis.x * absolute_vector.mV[VX] +
                               mZAxis.y * absolute_vector.mV[VY] +
                               mZAxis.z * absolute_vector.mV[VZ],
                           absolute_vector.mV[VW]);
    return local_vector;
}


glm::vec3 LLCoordFrame::rotateToAbsolute(const glm::vec3 &local_vector) const
{
    return glm::vec3(mXAxis.x * local_vector.x +
                         mYAxis.x * local_vector.y +
                         mZAxis.x * local_vector.z,
                     mXAxis.y * local_vector.x +
                         mYAxis.y * local_vector.y +
                         mZAxis.y * local_vector.z,
                     mXAxis.z * local_vector.x +
                         mYAxis.z * local_vector.y +
                         mZAxis.z * local_vector.z);
}


LLVector4 LLCoordFrame::rotateToAbsolute(const LLVector4 &local_vector) const
{
    LLVector4 absolute_vector(mXAxis.x * local_vector.mV[VX] +
                                  mYAxis.x * local_vector.mV[VY] +
                                  mZAxis.x * local_vector.mV[VZ],
                              mXAxis.y * local_vector.mV[VX] +
                                  mYAxis.y * local_vector.mV[VY] +
                                  mZAxis.y * local_vector.mV[VZ],
                              mXAxis.z * local_vector.mV[VX] +
                                  mYAxis.z * local_vector.mV[VY] +
                                  mZAxis.z * local_vector.mV[VZ],
                              local_vector[VW]);
    return absolute_vector;
}


void LLCoordFrame::orthonormalize()
// Makes sure the axes are orthogonal and normalized.
{
    mXAxis = glm::normalize(mXAxis);                          // X is renormalized
    mYAxis -= mXAxis * glm::dot(mXAxis, mYAxis);              // Y remains in X-Y plane
    mYAxis = glm::normalize(mYAxis);                          // Y is normalized
    mZAxis = glm::cross(mXAxis, mYAxis);                      // Z = X cross Y
}


glm::vec3 LLCoordFrame::transformToLocal(const glm::vec3 &absolute_vector) const
{
    return rotateToLocal(absolute_vector - mOrigin);
}


LLVector4 LLCoordFrame::transformToLocal(const LLVector4 &absolute_vector) const
{
    LLVector4 local_vector(absolute_vector);
    local_vector.mV[VX] -= mOrigin.x;
    local_vector.mV[VY] -= mOrigin.y;
    local_vector.mV[VZ] -= mOrigin.z;
    return rotateToLocal(local_vector);
}


glm::vec3 LLCoordFrame::transformToAbsolute(const glm::vec3 &local_vector) const
{
    return (rotateToAbsolute(local_vector) + mOrigin);
}


LLVector4 LLCoordFrame::transformToAbsolute(const LLVector4 &local_vector) const
{
    LLVector4 absolute_vector;
    absolute_vector = rotateToAbsolute(local_vector);
    absolute_vector.mV[VX] += mOrigin.x;
    absolute_vector.mV[VY] += mOrigin.y;
    absolute_vector.mV[VZ] += mOrigin.z;
    return absolute_vector;
}


// This is how you combine a translation and rotation of a
// coordinate frame to get an OpenGL transformation matrix:
//
//     translation   *   rotation      =          transformation matrix
//
//     (i)->
// (j)| 1  0  0  0 |   | a  d  g  0 |     |     a            d            g          0 |
//  | | 0  1  0  0 | * | b  e  h  0 |  =  |     b            e            h          0 |
//  V | 0  0  1  0 |   | c  f  i  0 |     |     c            f            i          0 |
//    |-x -y -z  1 |   | 0  0  0  1 |     |-(ax+by+cz)  -(dx+ey+fz)  -(gx+hy+iz)     1 |
//
// where {a,b,c} = x-axis
//       {d,e,f} = y-axis
//       {g,h,i} = z-axis
//       {x,y,z} = origin

void LLCoordFrame::getOpenGLTranslation(F32 *ogl_matrix) const
{
    *(ogl_matrix + 0)  = 1.0f;
    *(ogl_matrix + 1)  = 0.0f;
    *(ogl_matrix + 2)  = 0.0f;
    *(ogl_matrix + 3)  = 0.0f;

    *(ogl_matrix + 4)  = 0.0f;
    *(ogl_matrix + 5)  = 1.0f;
    *(ogl_matrix + 6)  = 0.0f;
    *(ogl_matrix + 7)  = 0.0f;

    *(ogl_matrix + 8)  = 0.0f;
    *(ogl_matrix + 9)  = 0.0f;
    *(ogl_matrix + 10) = 1.0f;
    *(ogl_matrix + 11) = 0.0f;

    *(ogl_matrix + 12) = -mOrigin.x;
    *(ogl_matrix + 13) = -mOrigin.y;
    *(ogl_matrix + 14) = -mOrigin.z;
    *(ogl_matrix + 15) = 1.0f;
}


void LLCoordFrame::getOpenGLRotation(F32 *ogl_matrix) const
{
    *(ogl_matrix + 0)  = mXAxis.x;
    *(ogl_matrix + 4)  = mXAxis.y;
    *(ogl_matrix + 8)  = mXAxis.z;

    *(ogl_matrix + 1)  = mYAxis.x;
    *(ogl_matrix + 5)  = mYAxis.y;
    *(ogl_matrix + 9)  = mYAxis.z;

    *(ogl_matrix + 2)  = mZAxis.x;
    *(ogl_matrix + 6)  = mZAxis.y;
    *(ogl_matrix + 10) = mZAxis.z;

    *(ogl_matrix + 3)  = 0.0f;
    *(ogl_matrix + 7)  = 0.0f;
    *(ogl_matrix + 11) = 0.0f;

    *(ogl_matrix + 12) = 0.0f;
    *(ogl_matrix + 13) = 0.0f;
    *(ogl_matrix + 14) = 0.0f;
    *(ogl_matrix + 15) = 1.0f;
}


void LLCoordFrame::getOpenGLTransform(F32 *ogl_matrix) const
{
    *(ogl_matrix + 0)  = mXAxis.x;
    *(ogl_matrix + 4)  = mXAxis.y;
    *(ogl_matrix + 8)  = mXAxis.z;
    *(ogl_matrix + 12) = -glm::dot(mOrigin, mXAxis);

    *(ogl_matrix + 1)  = mYAxis.x;
    *(ogl_matrix + 5)  = mYAxis.y;
    *(ogl_matrix + 9)  = mYAxis.z;
    *(ogl_matrix + 13) = -glm::dot(mOrigin, mYAxis);

    *(ogl_matrix + 2)  = mZAxis.x;
    *(ogl_matrix + 6)  = mZAxis.y;
    *(ogl_matrix + 10) = mZAxis.z;
    *(ogl_matrix + 14) = -glm::dot(mOrigin, mZAxis);

    *(ogl_matrix + 3)  = 0.0f;
    *(ogl_matrix + 7)  = 0.0f;
    *(ogl_matrix + 11) = 0.0f;
    *(ogl_matrix + 15) = 1.0f;
}


// at and up_direction are presumed to be normalized
void LLCoordFrame::lookDir(const glm::vec3 &at, const glm::vec3 &up_direction)
{
    // Make sure 'at' and 'up_direction' are not parallel
    // and that neither are zero-length vectors
    glm::vec3 left(glm::cross(up_direction, at));
    if (glm::dot(left, left) < F_APPROXIMATELY_ZERO)
    {
        //tweak lookat pos so we don't get a degenerate matrix
        glm::vec3 tempat(at.x + 0.01f, at.y, at.z);
        tempat = glm::normalize(tempat);
        left = glm::cross(up_direction, tempat);
    }
    left = glm::normalize(left);

    glm::vec3 up = glm::cross(at, left);

    if (glm_is_finite(at) && glm_is_finite(left) && glm_is_finite(up))
    {
        setAxes(at, left, up);
    }
}

void LLCoordFrame::lookDir(const glm::vec3 &xuv)
{
    static const glm::vec3 up_direction(0.0f, 0.0f, 1.0f);
    lookDir(xuv, up_direction);
}

void LLCoordFrame::lookAt(const glm::vec3 &origin, const glm::vec3 &point_of_interest, const glm::vec3 &up_direction)
{
    setOrigin(origin);
    glm::vec3 at(glm::normalize(point_of_interest - origin));
    lookDir(at, up_direction);
}

void LLCoordFrame::lookAt(const glm::vec3 &origin, const glm::vec3 &point_of_interest)
{
    static const glm::vec3 up_direction(0.0f, 0.0f, 1.0f);

    setOrigin(origin);
    glm::vec3 at(glm::normalize(point_of_interest - origin));
    lookDir(at, up_direction);
}


// Operators and friends

std::ostream& operator<<(std::ostream &s, const LLCoordFrame &C)
{
    s << "{ "
      << " origin = { " << C.mOrigin.x << ", " << C.mOrigin.y << ", " << C.mOrigin.z << " }"
      << " x_axis = { " << C.mXAxis.x << ", " << C.mXAxis.y << ", " << C.mXAxis.z << " }"
      << " y_axis = { " << C.mYAxis.x << ", " << C.mYAxis.y << ", " << C.mYAxis.z << " }"
      << " z_axis = { " << C.mZAxis.x << ", " << C.mZAxis.y << ", " << C.mZAxis.z << " }"
    << " }";
    return s;
}



// Private member functions


//EOF
