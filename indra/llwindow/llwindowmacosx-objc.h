/** 
 * @file llwindowmacosx-objc.h
 * @brief Prototypes for functions shared between llwindowmacosx.cpp
 * and llwindowmacosx-objc.mm.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */


// This will actually hold an NSCursor*, but that type is only available in objective C.
typedef void *CursorRef;

/* Defined in llwindowmacosx-objc.mm: */
void setupCocoa();
CursorRef createImageCursor(const char *fullpath, int hotspotX, int hotspotY);
OSErr releaseImageCursor(CursorRef ref);
OSErr setImageCursor(CursorRef ref);

