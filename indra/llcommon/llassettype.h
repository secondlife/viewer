/** 
 * @file llassettype.h
 * @brief Declaration of LLAssetType.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLASSETTYPE_H
#define LL_LLASSETTYPE_H

#include <string>

#include "stdenums.h" 	// for EDragAndDropType

class LL_COMMON_API LLAssetType
{
public:
	enum EType
	{
		AT_TEXTURE = 0,
			// Used for painting the faces of geometry.
			// Stored in typical j2c stream format.

		AT_SOUND = 1, 
			// Used to fill the aural spectrum.

		AT_CALLINGCARD = 2,
		    // Links instant message access to the user on the card.
			// : E.G. A card for yourself, for linden support, for
			// : the guy you were talking to in the coliseum.

		AT_LANDMARK = 3,
			// Links to places in the world with location and a screen shot or image saved.
			// : E.G. Home, linden headquarters, the coliseum, destinations where 
			// : we want to increase traffic.

		AT_SCRIPT = 4,
			// Valid scripts that can be attached to an object.
			// : E.G. Open a door, jump into the air.

		AT_CLOTHING = 5,
			// A collection of textures and parameters that can be worn by an avatar.

		AT_OBJECT = 6,
			// Any combination of textures, sounds, and scripts that are
			// associated with a fixed piece of geometry.
			// : E.G. A hot tub, a house with working door.

		AT_NOTECARD = 7,
			// Just text.

		AT_CATEGORY = 8,
			// Holds a collection of inventory items.
			// It's treated as an item in the inventory and therefore needs a type.

		AT_LSL_TEXT = 10,
		AT_LSL_BYTECODE = 11,
			// The LSL is the scripting language. 
			// We've split it into a text and bytecode representation.
		
		AT_TEXTURE_TGA = 12,
			// Uncompressed TGA texture.

		AT_BODYPART = 13,
			// A collection of textures and parameters that can be worn by an avatar.

		AT_SOUND_WAV = 17,
			// Uncompressed sound.

		AT_IMAGE_TGA = 18,
			// Uncompressed image, non-square.
			// Not appropriate for use as a texture.

		AT_IMAGE_JPEG = 19,
			// Compressed image, non-square.
			// Not appropriate for use as a texture.

		AT_ANIMATION = 20,
			// Animation.

		AT_GESTURE = 21,
			// Gesture, sequence of animations, sounds, chat, wait steps.

		AT_SIMSTATE = 22,
			// Simstate file.

		AT_LINK = 24,
			// Inventory symbolic link

		AT_LINK_FOLDER = 25,
			// Inventory folder link
		
		AT_WIDGET = 40,
			// UI Widget: this is *not* an inventory asset type, only a viewer side asset (e.g. button, other ui items...)
		
		AT_MESH = 49,
			// Mesh data in our proprietary SLM format
		
		AT_COUNT = 50,

			// +*********************************************************+
			// |  TO ADD AN ELEMENT TO THIS ENUM:                        |
			// +*********************************************************+
			// | 1. INSERT BEFORE AT_COUNT                               |
			// | 2. INCREMENT AT_COUNT BY 1                              |
			// | 3. ADD TO LLAssetType.cpp                               |
			// | 4. ADD TO LLViewerAssetType.cpp                         |
			// | 5. ADD TO DEFAULT_ASSET_FOR_INV in LLInventoryType.cpp  |
			// +*********************************************************+

		AT_NONE = -1
	};

	// machine transation between type and strings
	static EType 				lookup(const char* name); // safe conversion to std::string, *TODO: deprecate
	static EType 				lookup(const std::string& type_name);
	static const char*			lookup(EType asset_type);

	// translation from a type to a human readable form.
	static EType 				lookupHumanReadable(const char* desc_name); // safe conversion to std::string, *TODO: deprecate
	static EType 				lookupHumanReadable(const std::string& readable_name);
	static const char*			lookupHumanReadable(EType asset_type);

	static EType 				getType(const std::string& desc_name);
	static const std::string&	getDesc(EType asset_type);

	static bool 				lookupCanLink(EType asset_type);
	static bool 				lookupIsLinkType(EType asset_type);

	static bool 				lookupIsAssetFetchByIDAllowed(EType asset_type); // the asset allows direct download
	static bool 				lookupIsAssetIDKnowable(EType asset_type); // asset data can be known by the viewer
	
	static const std::string&	badLookup(); // error string when a lookup fails

protected:
	LLAssetType() {}
	~LLAssetType() {}
};

#endif // LL_LLASSETTYPE_H
