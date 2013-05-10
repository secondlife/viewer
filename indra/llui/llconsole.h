/** 
 * @file llconsole.h
 * @brief a simple console-style output device
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

#ifndef LL_LLCONSOLE_H
#define LL_LLCONSOLE_H

#include "llfixedbuffer.h"
#include "lluictrl.h"
#include "v4color.h"
#include <deque>

class LLSD;

class LLConsole : public LLFixedBuffer, public LLUICtrl, public LLInstanceTracker<LLConsole>
{
public:
	typedef enum e_font_size
	{
		MONOSPACE = -1,
		SMALL = 0,
		BIG = 1
	} EFontSize;

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<U32>	max_lines;
		Optional<F32>	persist_time;
		Optional<S32>	font_size_index;
		Params()
		:	max_lines("max_lines", LLUI::sSettingGroups["config"]->getS32("ConsoleMaxLines")),
			persist_time("persist_time", 0.f), // forever
			font_size_index("font_size_index")
		{
			changeDefault(mouse_opaque, false);
		}
	};
protected:
	LLConsole(const Params&);
	friend class LLUICtrlFactory;

public:
	// call once per frame to pull data out of LLFixedBuffer
	static void updateClass();

	//A paragraph color segment defines the color of text in a line 
	//of text that was received for console display.  It has no 
	//notion of line wraps, screen position, or the text it contains.
	//It is only the number of characters that are a color and the
	//color.
	struct ParagraphColorSegment
	{
		S32		mNumChars;
		LLColor4 mColor;
	};
	
	//A line color segment is a chunk of text, the color associated
	//with it, and the X Position it was calculated to begin at 
	//on the screen.  X Positions are re-calculated if the 
	//screen changes size.
	class LineColorSegment
	{
		public:
			LineColorSegment(LLWString text, LLColor4 color, F32 xpos) : mText(text), mColor(color), mXPosition(xpos) {}
		public:
			LLWString mText;
			LLColor4  mColor;
			F32		  mXPosition;
	};
	 	
	typedef std::list<LineColorSegment> line_color_segments_t;
	
	//A line is composed of one or more color segments.
	class Line
	{
		public:
			line_color_segments_t mLineColorSegments;
	};
	
	typedef std::list<Line> lines_t;
	typedef std::list<ParagraphColorSegment> paragraph_color_segments_t;
	
	//A paragraph is a processed element containing the entire text of the
	//message (used for recalculating positions on screen resize)
	//The time this message was added to the console output
	//The visual screen width of the longest line in this block
	//And a list of one or more lines which are used to display this message.
	class Paragraph
	{
		public:
			Paragraph (LLWString str, const LLColor4 &color, F32 add_time, const LLFontGL* font, F32 screen_width);
			void makeParagraphColorSegments ( const LLColor4 &color);
			void updateLines ( F32 screen_width,  const LLFontGL* font, bool force_resize=false );
		public:
			LLWString mParagraphText;	//The entire text of the paragraph
			paragraph_color_segments_t	mParagraphColorSegments;
			F32 mAddTime;				//Time this paragraph was added to the display.
			F32 mMaxWidth;				//Width of the widest line of text in this paragraph.
			lines_t	mLines;
			
	};
		
	//The console contains a deque of paragraphs which represent the individual messages.
	typedef std::deque<Paragraph> paragraph_t;
	paragraph_t mParagraphs;

	~LLConsole(){};

	// each line lasts this long after being added
	void			setLinePersistTime(F32 seconds);

	void			reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	// -1 = monospace, 0 means small, font size = 1 means big
	void			setFontSize(S32 size_index);

	
	// Overrides
	/*virtual*/ void	draw();
private:
	void		update();

	F32			mLinePersistTime; // Age at which to stop drawing.
	F32			mFadeTime; // Age at which to start fading
	const LLFontGL*	mFont;
	S32			mConsoleWidth;
	S32			mConsoleHeight;

};

extern LLConsole* gConsole;

#endif
