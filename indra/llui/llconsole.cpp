/** 
 * @file llconsole.cpp
 * @brief a scrolling console output device
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

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include "llconsole.h"

// linden library includes
#include "llmath.h"
//#include "llviewercontrol.h"
#include "llcriticaldamp.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llui.h"
#include "lluiimage.h"
//#include "llviewerimage.h"
//#include "llviewerimagelist.h"
//#include "llviewerwindow.h"
#include "llsd.h"
#include "llfontgl.h"
#include "llmath.h"

//#include "llstartup.h"

// Used for LCD display
extern void AddNewDebugConsoleToLCD(const LLWString &newLine);

LLConsole* gConsole = NULL;  // Created and destroyed in LLViewerWindow.

const F32 FADE_DURATION = 2.f;
const S32 MIN_CONSOLE_WIDTH = 200;
 
static LLDefaultChildRegistry::Register<LLConsole> r("console");

LLConsole::LLConsole(const LLConsole::Params& p) 
:	LLUICtrl(p),
	LLFixedBuffer(p.max_lines),
	mLinePersistTime(p.persist_time), // seconds
	mFont(p.font),
	mConsoleWidth(0),
	mConsoleHeight(0)
{
	if (p.font_size_index.isProvided())
	{
		setFontSize(p.font_size_index);
	}
	mFadeTime = mLinePersistTime - FADE_DURATION;
	setMaxLines(LLUI::sSettingGroups["config"]->getS32("ConsoleMaxLines"));
}

void LLConsole::setLinePersistTime(F32 seconds)
{
	mLinePersistTime = seconds;
	mFadeTime = mLinePersistTime - FADE_DURATION;
}

void LLConsole::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	S32 new_width = llmax(50, llmin(getRect().getWidth(), width));
	S32 new_height = llmax(llfloor(mFont->getLineHeight()) + 15, llmin(getRect().getHeight(), height));

	if (   mConsoleWidth == new_width
		&& mConsoleHeight == new_height )
	{
		return;
	}
	
	mConsoleWidth = new_width;
	mConsoleHeight= new_height;
	
	LLUICtrl::reshape(new_width, new_height, called_from_parent);
	
	for(paragraph_t::iterator paragraph_it = mParagraphs.begin(); paragraph_it != mParagraphs.end(); paragraph_it++)
	{
		(*paragraph_it).updateLines((F32)getRect().getWidth(), mFont, true);
	}
}

void LLConsole::setFontSize(S32 size_index)
{
	if (-1 == size_index)
	{
		mFont = LLFontGL::getFontMonospace();
	}
	else if (0 == size_index)
	{
		mFont = LLFontGL::getFontSansSerif();
	}
	else if (1 == size_index)
	{
		mFont = LLFontGL::getFontSansSerifBig();
	}
	else
	{
		mFont = LLFontGL::getFontSansSerifHuge();
	}
	// Make sure the font exists
	if (mFont == NULL)
	{
		mFont = LLFontGL::getFontDefault();
	}
	
	for(paragraph_t::iterator paragraph_it = mParagraphs.begin(); paragraph_it != mParagraphs.end(); paragraph_it++)
	{
		(*paragraph_it).updateLines((F32)getRect().getWidth(), mFont, true);
	}
}

void LLConsole::draw()
{
	// Units in pixels
	static const F32 padding_horizontal = 10;
	static const F32 padding_vertical = 3;
	LLGLSUIDefault gls_ui;

	// skip lines added more than mLinePersistTime ago
	F32 cur_time = mTimer.getElapsedTimeF32();
	
	F32 skip_time = cur_time - mLinePersistTime;
	F32 fade_time = cur_time - mFadeTime;

	if (mParagraphs.empty()) 	//No text to draw.
	{
		return;
	}

	U32 num_lines=0;

	paragraph_t::reverse_iterator paragraph_it;
	paragraph_it = mParagraphs.rbegin();
	U32 paragraph_num=mParagraphs.size();
	
	while (!mParagraphs.empty() && paragraph_it != mParagraphs.rend())
	{
		num_lines += (*paragraph_it).mLines.size();
		if(num_lines > mMaxLines 
			|| ( (mLinePersistTime > (F32)0.f) && ((*paragraph_it).mAddTime - skip_time)/(mLinePersistTime - mFadeTime) <= (F32)0.f)) 
		{							//All lines above here are done.  Lose them.
			for (U32 i=0;i<paragraph_num;i++)
			{
				if (!mParagraphs.empty())
					mParagraphs.pop_front();
			}
			break;
		}
		paragraph_num--;
		paragraph_it++;
	}

	if (mParagraphs.empty())
	{
		return;
	}
	
	// draw remaining lines
	F32 y_pos = 0.f;

	LLUIImagePtr imagep = LLUI::getUIImage("transparent");

	F32 console_opacity = llclamp(LLUI::sSettingGroups["config"]->getF32("ConsoleBackgroundOpacity"), 0.f, 1.f);
	LLColor4 color = LLUIColorTable::instance().getColor("ConsoleBackground");
	color.mV[VALPHA] *= console_opacity;

	F32 line_height = mFont->getLineHeight();

	for(paragraph_it = mParagraphs.rbegin(); paragraph_it != mParagraphs.rend(); paragraph_it++)
	{
		S32 target_height = llfloor( (*paragraph_it).mLines.size() * line_height + padding_vertical);
		S32 target_width =  llfloor( (*paragraph_it).mMaxWidth + padding_horizontal);

		y_pos += ((*paragraph_it).mLines.size()) * line_height;
		imagep->drawSolid(-14, (S32)(y_pos + line_height - target_height), target_width, target_height, color);

		F32 y_off=0;

		F32 alpha;

		if ((mLinePersistTime > 0.f) && ((*paragraph_it).mAddTime < fade_time))
		{
			alpha = ((*paragraph_it).mAddTime - skip_time)/(mLinePersistTime - mFadeTime);
		}
		else
		{
			alpha = 1.0f;
		}

		if( alpha > 0.f )
		{
			for (lines_t::iterator line_it=(*paragraph_it).mLines.begin(); 
					line_it != (*paragraph_it).mLines.end();
					line_it ++)
			{
				for (line_color_segments_t::iterator seg_it = (*line_it).mLineColorSegments.begin();
						seg_it != (*line_it).mLineColorSegments.end();
						seg_it++)
				{
					mFont->render((*seg_it).mText, 0, (*seg_it).mXPosition - 8, y_pos -  y_off,
						LLColor4(
							(*seg_it).mColor.mV[VRED], 
							(*seg_it).mColor.mV[VGREEN], 
							(*seg_it).mColor.mV[VBLUE], 
							(*seg_it).mColor.mV[VALPHA]*alpha),
						LLFontGL::LEFT, 
						LLFontGL::BASELINE,
						LLFontGL::NORMAL,
						LLFontGL::DROP_SHADOW,
						S32_MAX,
						target_width
						);
				}
				y_off += line_height;
			}
		}
		y_pos  += padding_vertical;
	}
}

//Generate highlight color segments for this paragraph.  Pass in default color of paragraph.
void LLConsole::Paragraph::makeParagraphColorSegments (const LLColor4 &color) 
{
	LLSD paragraph_color_segments;
	
	paragraph_color_segments[0]["text"] =wstring_to_utf8str(mParagraphText);
	LLSD color_sd = color.getValue();
	paragraph_color_segments[0]["color"]=color_sd;

	for(LLSD::array_const_iterator color_segment_it = paragraph_color_segments.beginArray();
		color_segment_it != paragraph_color_segments.endArray();
		++color_segment_it)
	{			
		LLSD color_llsd = (*color_segment_it)["color"];
		std::string color_str  = (*color_segment_it)["text"].asString();

		ParagraphColorSegment color_segment;
		
		color_segment.mColor.setValue(color_llsd);
		color_segment.mNumChars = color_str.length();
		
		mParagraphColorSegments.push_back(color_segment);
	}
}

//Called when a paragraph is added to the console or window is resized.
void LLConsole::Paragraph::updateLines(F32 screen_width, const LLFontGL* font, bool force_resize)
{
	if ( !force_resize )
	{
		if ( mMaxWidth >= 0.0f 
		 &&  mMaxWidth < screen_width )
		{
			return;					//No resize required.
		}
	}
	
	screen_width = screen_width - 30;	//Margin for small windows.
	
	if (	mParagraphText.empty() 
		|| mParagraphColorSegments.empty()
		|| font == NULL)
	{
		return;					//Not enough info to complete.
	}
	
	mLines.clear();				//Chuck everything.
	mMaxWidth = 0.0f;
	
	paragraph_color_segments_t::iterator current_color = mParagraphColorSegments.begin();
	U32 current_color_length = (*current_color).mNumChars;	
	
	S32 paragraph_offset = 0;			//Offset into the paragraph text.

	// Wrap lines that are longer than the view is wide.
	while( paragraph_offset < (S32)mParagraphText.length() &&
		   mParagraphText[paragraph_offset] != 0)
	{
		S32 skip_chars; // skip '\n'
		// Figure out if a word-wrapped line fits here.
		LLWString::size_type line_end = mParagraphText.find_first_of(llwchar('\n'), paragraph_offset);
		if (line_end != LLWString::npos)
		{
			skip_chars = 1; // skip '\n'
		}
		else
		{
			line_end = mParagraphText.size();
			skip_chars = 0;
		}

		U32 drawable = font->maxDrawableChars(mParagraphText.c_str()+paragraph_offset, screen_width, line_end - paragraph_offset, LLFontGL::WORD_BOUNDARY_IF_POSSIBLE);

		if (drawable != 0)
		{
			F32 x_position = 0;						//Screen X position of text.
			
			mMaxWidth = llmax( mMaxWidth, (F32)font->getWidth( mParagraphText.substr( paragraph_offset, drawable ).c_str() ) );
			Line line;
			
			U32 left_to_draw = drawable;
			U32 drawn = 0;
			
			while (left_to_draw >= current_color_length 
				&& current_color != mParagraphColorSegments.end() )
			{
				LLWString color_text = mParagraphText.substr( paragraph_offset + drawn, current_color_length );
				line.mLineColorSegments.push_back( LineColorSegment( color_text,			//Append segment to line.
												(*current_color).mColor, 
												x_position ) );
												
				x_position += font->getWidth( color_text.c_str() );	//Set up next screen position.
				
				drawn += current_color_length;
				left_to_draw -= current_color_length;
				
				current_color++;							//Goto next paragraph color record.
				
				if (current_color != mParagraphColorSegments.end())
				{
					current_color_length = (*current_color).mNumChars;
				}
			}
			
			if (left_to_draw > 0 && current_color != mParagraphColorSegments.end() )
			{
					LLWString color_text = mParagraphText.substr( paragraph_offset + drawn, left_to_draw );
					
					line.mLineColorSegments.push_back( LineColorSegment( color_text,		//Append segment to line.
													(*current_color).mColor, 
													x_position ) );
																		
					current_color_length -= left_to_draw;
			}
			mLines.push_back(line);								//Append line to paragraph line list.
		}
		paragraph_offset += (drawable + skip_chars);
	}
}

//Pass in the string and the default color for this block of text.
LLConsole::Paragraph::Paragraph (LLWString str, const LLColor4 &color, F32 add_time, const LLFontGL* font, F32 screen_width) 
:	mParagraphText(str), mAddTime(add_time), mMaxWidth(-1)
{
	makeParagraphColorSegments(color);
	updateLines( screen_width, font );
}
	
// called once per frame regardless of console visibility
// static
void LLConsole::updateClass()
{	
	for (instance_iter it = beginInstances(); it != endInstances(); ++it)
	{
		it->update();
	} 
}

void LLConsole::update()
{
	{
		LLMutexLock lock(&mMutex);

		while (!mLines.empty())
		{
			mParagraphs.push_back(
				Paragraph(	mLines.front(), 
							LLColor4::white, 
							mTimer.getElapsedTimeF32(), 
							mFont, 
							(F32)getRect().getWidth()));
			mLines.pop_front();
		}
	}

	// remove old paragraphs which can't possibly be visible any more.  ::draw() will do something similar but more conservative - we do this here because ::draw() isn't guaranteed to ever be called!  (i.e. the console isn't visible)
	while ((S32)mParagraphs.size() > llmax((S32)0, (S32)(mMaxLines)))
	{
			mParagraphs.pop_front();
	}
}

