/** 
 * @file llmapimagetype.h
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMAPIMAGETYPE_H
#define LL_LLMAPIMAGETYPE_H

typedef enum e_map_image_type
{
	MIT_TERRAIN = 0,
	MIT_POPULAR = 1,
	MIT_OBJECTS = 2,
	MIT_OBJECTS_FOR_SALE = 3,
	MIT_LAND_TO_BUY = 4,
	MIT_OBJECT_NEW = 5,
	MIT_EOF = 6
} EMapImageType;

#endif
