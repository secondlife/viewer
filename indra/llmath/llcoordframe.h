/**
 * @file llcoordframe.h
 * @brief LLCoordFrame class header file.
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

#pragma once

#include "glm/vec3.hpp"
#include "v3math.h"
#include "v4math.h"
#include "llerror.h"

// XXX : The constructors of the LLCoordFrame class assume that all vectors
//       and quaternion being passed as arguments are normalized, and all matrix
//       arguments are unitary.  VERY BAD things will happen if these assumptions fail.
//       Also, segfault hazzards exist in methods that accept F32* arguments.


class LLCoordFrame
{
public:
    LLCoordFrame();                                         // Inits at zero with identity rotation
    explicit LLCoordFrame(const glm::vec3 &origin);         // Sets origin, and inits rotation = Identity
    LLCoordFrame(const glm::vec3 &x_axis,
                 const glm::vec3 &y_axis,
                 const glm::vec3 &z_axis);                  // Sets coordinate axes and inits origin at zero
    LLCoordFrame(const glm::vec3 &origin,
                 const glm::vec3 &x_axis,
                 const glm::vec3 &y_axis,
                 const glm::vec3 &z_axis);                  // Sets the origin and coordinate axes
    LLCoordFrame(const glm::vec3 &origin,
                 const LLMatrix3 &rotation);                // Sets axes to 3x3 matrix
    LLCoordFrame(const glm::vec3 &origin,
                 const glm::vec3 &direction);               // Sets origin and calls lookDir(direction)
    explicit LLCoordFrame(const LLQuaternion &q);           // Sets axes using q and inits mOrigin to zero
    LLCoordFrame(const glm::vec3 &origin,
                 const LLQuaternion &q);                    // Uses quaternion to init axes
    explicit LLCoordFrame(const LLMatrix4 &mat);            // Extracts frame from a 4x4 matrix
    // The folowing two constructors are dangerous due to implicit casting and have been disabled - SJB
    //LLCoordFrame(const F32 *origin, const F32 *rotation); // Assumes "origin" is 1x3 and "rotation" is 1x9 array
    //LLCoordFrame(const F32 *origin_and_rotation);         // Assumes "origin_and_rotation" is 1x12 array

    [[nodiscard]] bool isFinite() const
    {
        return std::isfinite(mOrigin.x) && std::isfinite(mOrigin.y) && std::isfinite(mOrigin.z)
            && std::isfinite(mXAxis.x) && std::isfinite(mXAxis.y) && std::isfinite(mXAxis.z)
            && std::isfinite(mYAxis.x) && std::isfinite(mYAxis.y) && std::isfinite(mYAxis.z)
            && std::isfinite(mZAxis.x) && std::isfinite(mZAxis.y) && std::isfinite(mZAxis.z);
    }

    void reset();
    void resetAxes();

    void setOrigin(F32 x, F32 y, F32 z);                    // Set mOrigin
    void setOrigin(const glm::vec3 &origin);
    void setOrigin(const F32 *origin);
    void setOrigin(const LLCoordFrame &frame);

    inline void setOriginX(F32 x) { mOrigin.x = x; }
    inline void setOriginY(F32 y) { mOrigin.y = y; }
    inline void setOriginZ(F32 z) { mOrigin.z = z; }

    void setAxes(const glm::vec3 &x_axis,                   // Set axes
                 const glm::vec3 &y_axis,
                 const glm::vec3 &z_axis);
    void setAxes(const LLMatrix3 &rotation_matrix);
    void setAxes(const glm::quat &q);
    void setAxes(const F32 *rotation_matrix);
    void setAxes(const LLCoordFrame &frame);

    void translate(F32 x, F32 y, F32 z);                    // Move mOrgin
    void translate(const glm::vec3 &v);
    void translate(const F32 *origin);

    void rotate(F32 angle, F32 x, F32 y, F32 z);            // Move axes
    void rotate(F32 angle, const glm::vec3 &rotation_axis);
    void rotate(const glm::quat &q);
    void rotate(const LLMatrix3 &m);

    void orthonormalize();  // Makes sure axes are unitary and orthogonal.

    // These methods allow rotations in the LLCoordFrame's frame
    void roll(F32 angle);       // RH rotation about mXAxis, radians
    void pitch(F32 angle);      // RH rotation about mYAxis, radians
    void yaw(F32 angle);        // RH rotation about mZAxis, radians

    inline const glm::vec3 &getOrigin() const { return mOrigin; }

    inline const glm::vec3 &getXAxis() const  { return mXAxis; }
    inline const glm::vec3 &getYAxis() const  { return mYAxis; }
    inline const glm::vec3 &getZAxis() const  { return mZAxis; }

    inline const glm::vec3 &getAtAxis() const   { return mXAxis; }
    inline const glm::vec3 &getLeftAxis() const { return mYAxis; }
    inline const glm::vec3 &getUpAxis() const   { return mZAxis; }

    // These return representations of the rotation or orientation of the LLFrame
    // it its absolute frame.  That is, these rotations acting on the X-axis {1,0,0}
    // will produce the mXAxis.
    //      LLMatrix3 getMatrix3() const;               // Returns axes in 3x3 matrix
    LLQuaternion getQuaternion() const;         // Returns axes in quaternion form

    // Same as above, except it also includes the translation of the LLFrame
    //      LLMatrix4 getMatrix4() const;               // Returns position and axes in 4x4 matrix

    // Returns matrix which expresses point in local frame in the parent frame
    void getMatrixToParent(LLMatrix4 &mat) const;
    // Returns matrix which expresses point in parent frame in the local frame
    void getMatrixToLocal(LLMatrix4 &mat) const; // Returns matrix which expresses point in parent frame in the local frame

    void getRotMatrixToParent(LLMatrix4 &mat) const;

    // Copies mOrigin, then the three axes to buffer, returns number of bytes copied.
    size_t writeOrientation(char *buffer) const;

    // Copies mOrigin, then the three axes from buffer, returns the number of bytes copied.
    // Assumes the data in buffer is correct.
    size_t readOrientation(const char *buffer);

    glm::vec3 rotateToLocal(const glm::vec3 &v) const;      // Returns v' rotated to local
    LLVector4 rotateToLocal(const LLVector4 &v) const;      // Returns v' rotated to local
    glm::vec3 rotateToAbsolute(const glm::vec3 &v) const;   // Returns v' rotated to absolute
    LLVector4 rotateToAbsolute(const LLVector4 &v) const;   // Returns v' rotated to absolute

    glm::vec3 transformToLocal(const glm::vec3 &v) const;       // Returns v' in local coord
    LLVector4 transformToLocal(const LLVector4 &v) const;       // Returns v' in local coord
    glm::vec3 transformToAbsolute(const glm::vec3 &v) const;    // Returns v' in absolute coord
    LLVector4 transformToAbsolute(const LLVector4 &v) const;    // Returns v' in absolute coord

    // Write coord frame orientation into provided array in OpenGL matrix format.
    void getOpenGLTranslation(F32 *ogl_matrix) const;
    void getOpenGLRotation(F32 *ogl_matrix) const;
    void getOpenGLTransform(F32 *ogl_matrix) const;

    // lookDir orients to (xuv, presumed normalized) and does not affect origin
    void lookDir(const glm::vec3 &xuv, const glm::vec3 &up);
    void lookDir(const glm::vec3 &xuv); // up = 0,0,1
    // lookAt orients to (point_of_interest - origin) and sets origin
    void lookAt(const glm::vec3 &origin, const glm::vec3 &point_of_interest, const glm::vec3 &up);
    void lookAt(const glm::vec3 &origin, const glm::vec3 &point_of_interest); // up = 0,0,1

    // deprecated
    void setOriginAndLookAt(const glm::vec3 &origin, const glm::vec3 &up, const glm::vec3 &point_of_interest)
    {
        lookAt(origin, point_of_interest, up);
    }

    friend std::ostream& operator<<(std::ostream &s, const LLCoordFrame &C);

    // These vectors are in absolute frame
    glm::vec3 mOrigin;
    glm::vec3 mXAxis;
    glm::vec3 mYAxis;
    glm::vec3 mZAxis;
};



