/**
* @file llsnapshotmodel.h
* @brief Snapshot model for storing snapshot data etc.
*
* $LicenseInfo:firstyear=2004&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2016, Linden Research, Inc.
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

#ifndef LL_LLSNAPSHOTMODEL_H
#define LL_LLSNAPSHOTMODEL_H

class LLSnapshotModel
{
public:
	enum ESnapshotType
	{
		SNAPSHOT_POSTCARD,
		SNAPSHOT_TEXTURE,
		SNAPSHOT_LOCAL,
		SNAPSHOT_WEB
	};

	typedef enum e_snapshot_format
	{
		SNAPSHOT_FORMAT_PNG,
		SNAPSHOT_FORMAT_JPEG,
		SNAPSHOT_FORMAT_BMP
	} ESnapshotFormat;

	typedef enum
	{
		SNAPSHOT_TYPE_COLOR,
		SNAPSHOT_TYPE_DEPTH
	} ESnapshotLayerType;
};

#endif // LL_LLSNAPSHOTMODEL_H
