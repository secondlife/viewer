/** 
 * @file llnotecard.cpp
 * @brief LLNotecard class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#include "llnotecard.h"
#include "llstreamtools.h"

LLNotecard::LLNotecard(S32 max_text)
	: mMaxText(max_text),
	  mVersion(0),
	  mEmbeddedVersion(0)
{
}

LLNotecard::~LLNotecard()
{
}

bool LLNotecard::importEmbeddedItemsStream(std::istream& str)
{
	// Version 1 format:
	//		LLEmbeddedItems version 1
	//		{
	//			count <number of entries being used and not deleted>
	//			{
	//				ext char index <index>
	//				<InventoryItem chunk>
	//			}
	//		}
	
	S32 i;
	S32 count = 0;

	str >> std::ws >> "LLEmbeddedItems version" >> mEmbeddedVersion >> "\n";
	if (str.fail())
	{
		LL_WARNS() << "Invalid Linden text file header" << LL_ENDL;
		goto import_file_failed;
	}

	if( 1 != mEmbeddedVersion )
	{
		LL_WARNS() << "Invalid LLEmbeddedItems version: " << mEmbeddedVersion << LL_ENDL;
		goto import_file_failed;
	}

	str >> std::ws >> "{\n";
	if(str.fail())
	{
		LL_WARNS() << "Invalid Linden text file format: missing {" << LL_ENDL;
		goto import_file_failed;
	}

	str >> std::ws >> "count " >> count >> "\n";
	if(str.fail())
	{
		LL_WARNS() << "Invalid LLEmbeddedItems count" << LL_ENDL;
		goto import_file_failed;
	}

	if((count < 0))
	{
		LL_WARNS() << "Invalid LLEmbeddedItems count value: " << count << LL_ENDL;
		goto import_file_failed;
	}

	for(i = 0; i < count; i++)
	{
		str >> std::ws >> "{\n";
		if(str.fail())
		{
			LL_WARNS() << "Invalid LLEmbeddedItems file format: missing {" << LL_ENDL;
			goto import_file_failed;
		}

		U32 index = 0;
		str >> std::ws >> "ext char index " >> index >> "\n";
		if(str.fail())
		{
			LL_WARNS() << "Invalid LLEmbeddedItems file format: missing ext char index" << LL_ENDL;
			goto import_file_failed;
		}

		str >> std::ws >> "inv_item\t0\n";
		if(str.fail())
		{
			LL_WARNS() << "Invalid LLEmbeddedItems file format: missing inv_item" << LL_ENDL;
			goto import_file_failed;
		}

		LLPointer<LLInventoryItem> item = new LLInventoryItem;
		if (!item->importLegacyStream(str))
		{
			LL_INFOS() << "notecard import failed" << LL_ENDL;
			goto import_file_failed;
		}		
		mItems.push_back(item);

		str >> std::ws >> "}\n";
		if(str.fail())
		{
			LL_WARNS() << "Invalid LLEmbeddedItems file format: missing }" << LL_ENDL;
			goto import_file_failed;
		}
	}

	str >> std::ws >> "}\n";
	if(str.fail())
	{
		LL_WARNS() << "Invalid LLEmbeddedItems file format: missing }" << LL_ENDL;
		goto import_file_failed;
	}

	return true;

	import_file_failed:
	return false;
}

bool LLNotecard::importStream(std::istream& str)
{
	// Version 1 format:
	//		Linden text version 1
	//		{
	//			<EmbeddedItemList chunk>
	//			Text length
	//			<ASCII text; 0x80 | index = embedded item>
	//		}
	
	// Version 2 format: (NOTE: Imports identically to version 1)
	//		Linden text version 2
	//		{
	//			<EmbeddedItemList chunk>
	//			Text length
	//			<UTF8 text; FIRST_EMBEDDED_CHAR + index = embedded item>
	//		}
	
	str >> std::ws >> "Linden text version " >> mVersion >> "\n";
	if(str.fail())
	{
		LL_WARNS() << "Invalid Linden text file header " << LL_ENDL;
		return false;
	}

	if( 1 != mVersion && 2 != mVersion)
	{
		LL_WARNS() << "Invalid Linden text file version: " << mVersion << LL_ENDL;
		return false;
	}

	str >> std::ws >> "{\n";
	if(str.fail())
	{
		LL_WARNS() << "Invalid Linden text file format" << LL_ENDL;
		return false;
	}

	if(!importEmbeddedItemsStream(str))
	{
		return false;
	}

	char line_buf[STD_STRING_BUF_SIZE];	/* Flawfinder: ignore */
	str.getline(line_buf, STD_STRING_BUF_SIZE);
	if(str.fail())
	{
		LL_WARNS() << "Invalid Linden text length field" << LL_ENDL;
		return false;
	}
	line_buf[STD_STRING_STR_LEN] = '\0';
	
	S32 text_len = 0;
	if( 1 != sscanf(line_buf, "Text length %d", &text_len) )
	{
		LL_WARNS() << "Invalid Linden text length field" << LL_ENDL;
		return false;
	}

	if(text_len > mMaxText || text_len < 0)
	{
		LL_WARNS() << "Invalid Linden text length: " << text_len << LL_ENDL;
		return false;
	}

	bool success = true;

	char* text = new char[text_len + 1];
	fullread(str, text, text_len);
	if(str.fail())
	{
		LL_WARNS() << "Invalid Linden text: text shorter than text length: " << text_len << LL_ENDL;
		success = false;
	}
	text[text_len] = '\0';

	if(success)
	{
		// Actually set the text
		mText = std::string(text);
	}

	delete[] text;

	return success;
}

////////////////////////////////////////////////////////////////////////////

bool LLNotecard::exportEmbeddedItemsStream( std::ostream& out_stream )
{
	out_stream << "LLEmbeddedItems version 1\n";
	out_stream << "{\n";

	out_stream << llformat("count %d\n", mItems.size() );

	S32 idx = 0;
	for (std::vector<LLPointer<LLInventoryItem> >::iterator iter = mItems.begin();
		 iter != mItems.end(); ++iter)
	{
		LLInventoryItem* item = *iter;
		if (item)
		{
			out_stream << "{\n";
			out_stream << llformat("ext char index %d\n", idx  );
			if( !item->exportLegacyStream( out_stream ) )
			{
				return false;
			}
			out_stream << "}\n";
		}
		++idx;
	}

	out_stream << "}\n";
	
	return true;
}

bool LLNotecard::exportStream( std::ostream& out_stream )
{
	out_stream << "Linden text version 2\n";
	out_stream << "{\n";

	if( !exportEmbeddedItemsStream( out_stream ) )
	{
		return false;
	}

	out_stream << llformat("Text length %d\n", mText.length() );
	out_stream << mText;
	out_stream << "}\n";

	return true;
}

////////////////////////////////////////////////////////////////////////////

void LLNotecard::setItems(const std::vector<LLPointer<LLInventoryItem> >& items)
{
	mItems = items;
}

void LLNotecard::setText(const std::string& text)
{
	mText = text;
}
