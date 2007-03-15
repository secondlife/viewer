/** 
 * @file llassettype.h
 * @brief Declaration of LLAssetType.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLASSETTYPE
#define LL_LLASSETTYPE

#include <string>

#include "stdenums.h" 	// for EDragAndDropType

class LLAssetType
{
public:
	enum EType
	{
		// Used for painting the faces of geometry.
		// Stored in typical j2c stream format
		AT_TEXTURE = 0,

		// Used to fill the aural spectrum.
		AT_SOUND = 1, 

		// Links instant message access to the user on the card. eg, a
		// card for yourself, a card for linden support, a card for
		// the guy you were talking to in the coliseum.
		AT_CALLINGCARD = 2,

		// Links to places in the world with location and a screen
		// shot or image saved. eg, home, linden headquarters, the
		// coliseum, or destinations where we want to increase
		// traffic.
		AT_LANDMARK = 3,

		// Valid scripts that can be attached to an object. eg. open a
		// door, jump into the air.
		AT_SCRIPT = 4,

		// A collection of textures and parameters that can be worn
		// by an avatar.
		AT_CLOTHING = 5,

		// Any combination of textures, sounds, and scripts that are
		// associated with a fixed piece of geometry. eg, a hot tub, a
		// house with working door.
		AT_OBJECT = 6,

		// Just text
		AT_NOTECARD = 7,

		// A category holds a collection of inventory items. It's
		// treated as an item in the inventory, and therefore needs a
		// type.
		AT_CATEGORY = 8,

		// A root category is a user's root inventory category. We
		// decided to expose it visually, so it seems logical to fold
		// it into the asset types.
		AT_ROOT_CATEGORY = 9,

		// The LSL is the brand spanking new scripting language. We've
		// split it into a text and bytecode representation.
		AT_LSL_TEXT = 10,
		AT_LSL_BYTECODE = 11,
		
		// uncompressed TGA texture
		AT_TEXTURE_TGA = 12,

		// A collection of textures and parameters that can be worn
		// by an avatar.
		AT_BODYPART = 13,

		// This asset type is meant to only be used as a marker for a
		// category preferred type. Using this, we can throw things in
		// the trash before completely deleting.
		AT_TRASH = 14,

		// This is a marker for a folder meant for snapshots. No
		// actual assets will be snapshots, though if there were, you
		// could interpret them as textures.
		AT_SNAPSHOT_CATEGORY = 15,

		// This is used to stuff lost&found items into
		AT_LOST_AND_FOUND = 16,

		// uncompressed sound
		AT_SOUND_WAV = 17,

		// uncompressed image, non-square, and not appropriate for use
		// as a texture.
		AT_IMAGE_TGA = 18,

		// compressed image, non-square, and not appropriate for use
		// as a texture.
		AT_IMAGE_JPEG = 19,

		// animation
		AT_ANIMATION = 20,

		// gesture, sequence of animations, sounds, chat, wait steps
		AT_GESTURE = 21,

		// simstate file
		AT_SIMSTATE = 22,

		// +*********************************************+
		// |  TO ADD AN ELEMENT TO THIS ENUM:            |
		// +*********************************************+
		// | 1. INSERT BEFORE AT_COUNT                   |
		// | 2. INCREMENT AT_COUNT BY 1                  |
		// | 3. ADD TO LLAssetType::mAssetTypeNames      |
		// | 4. ADD TO LLAssetType::mAssetTypeHumanNames |
		// +*********************************************+

		AT_COUNT = 23,

		AT_NONE = -1
	};

	// machine transation between type and strings
	static EType lookup(const char* name);
	static const char* lookup(EType type);

	// translation from a type to a human readable form.
	static EType lookupHumanReadable( const char* name );
	static const char* lookupHumanReadable(EType type);

	static EDragAndDropType lookupDragAndDropType( EType );

	// Generate a good default description. You may want to add a verb
	// or agent name after this depending on your application.
	static void generateDescriptionFor(LLAssetType::EType type,
									   std::string& desc);

	static EType getType(const std::string& sin);
	static std::string getDesc(EType type);
	
private:
	// don't instantiate or derive one of these objects
	LLAssetType( void ) {}
	~LLAssetType( void ) {}

private:
	static const char* mAssetTypeNames[];
	static const char* mAssetTypeHumanNames[];
};

#endif // LL_LLASSETTYPE
