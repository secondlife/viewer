/** 
 * @file llvoicevisualizer.h
 * @brief Draws in-world speaking indicators.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

//--------------------------------------------------------------------
//
// VOICE VISUALIZER
// author: JJ Ventrella, Linden Lab
// (latest update to this info: Jan 18, 2007)
//
// The Voice Visualizer is responsible for taking realtime signals from actual users speaking and 
// visualizing this speech in two forms: 
//
// (1) as a dynamic sound symbol (also referred to as the "voice indicator" that appears over the avatar's head
// (2) as gesticulation events that are used to trigger avatr gestures 
//
// The input for the voice visualizer is a continual stream of voice amplitudes. 

//-----------------------------------------------------------------------------
#ifndef LL_VOICE_VISUALIZER_H
#define LL_VOICE_VISUALIZER_H

#include "llhudeffect.h"

//-----------------------------------------------------------------------------------------------
// The values of voice gesticulation represent energy levels for avatar animation, based on 
// amplitude surge events parsed from the voice signal. These are made available so that 
// the appropriate kind of avatar animation can be triggered, and thereby simulate the physical
// motion effects of speech. It is recommended that multiple body parts be animated as well as 
// lips, such as head, shoulders, and hands, with large gestures used when the energy level is high.
//-----------------------------------------------------------------------------------------------
enum VoiceGesticulationLevel
{
	VOICE_GESTICULATION_LEVEL_OFF = -1,
	VOICE_GESTICULATION_LEVEL_LOW = 0,
	VOICE_GESTICULATION_LEVEL_MEDIUM,
	VOICE_GESTICULATION_LEVEL_HIGH,
	NUM_VOICE_GESTICULATION_LEVELS
};

const static int NUM_VOICE_SYMBOL_WAVES = 7;

//----------------------------------------------------
// LLVoiceVisualizer class 
//----------------------------------------------------
class LLVoiceVisualizer : public LLHUDEffect
{
	//---------------------------------------------------
	// public methods 
	//---------------------------------------------------
	public:
		LLVoiceVisualizer ( const U8 type );	//constructor
		~LLVoiceVisualizer();					//destructor
		
		friend class LLHUDObject;

		void					setVoiceSourceWorldPosition( const LLVector3 &p );		// this should be the position of the speaking avatar's head
		void					setMinGesticulationAmplitude( F32 );					// the lower range of meaningful amplitude for setting gesticulation level 
		void					setMaxGesticulationAmplitude( F32 );					// the upper range of meaningful amplitude for setting gesticulation level 
		void					setStartSpeaking();										// tell me when the av starts speaking
		void					setVoiceEnabled( bool );								// tell me whether or not the user is voice enabled
		void					setSpeakingAmplitude( F32 );							// tell me how loud the av is speaking (ranges from 0 to 1)
		void					setStopSpeaking();										// tell me when the av stops speaking
		bool					getCurrentlySpeaking();									// the get for the above set
		VoiceGesticulationLevel	getCurrentGesticulationLevel();							// based on voice amplitude, I'll give you the current "energy level" of avatar speech
		static void				setPreferences( );
		static void				lipStringToF32s ( std::string& in_string, F32*& out_F32s, U32& count_F32s ); // convert a string of digits to an array of floats
		void					lipSyncOohAah( F32& ooh, F32& aah );
		void					render();												// inherited from HUD Effect
		void 					packData(LLMessageSystem *mesgsys);						// inherited from HUD Effect
		void 					unpackData(LLMessageSystem *mesgsys, S32 blocknum);		// inherited from HUD Effect
		void					markDead();											// inherited from HUD Effect
		
		//----------------------------------------------------------------------------------------------
		// "setMaxGesticulationAmplitude" and "setMinGesticulationAmplitude" allow for the tuning of the 
		// gesticulation level detector to be responsive to different kinds of signals. For instance, we 
		// may find that the average voice amplitude rarely exceeds 0.7 (in a range from 0 to 1), and 
		// therefore we may want to set 0.7 as the max, so we can more easily catch all the variance 
		// within that range. Also, we may find that there is often noise below a certain range like 0.1, 
		// and so we would want to set 0.1 as the min so as not to accidentally use this as signal.
		//----------------------------------------------------------------------------------------------
		void setMaxGesticulationAmplitude(); 
		void setMinGesticulationAmplitude(); 

	//---------------------------------------------------
	// private members 
	//---------------------------------------------------
	private:
	
		struct SoundSymbol
		{
			F32						mWaveExpansion			[ NUM_VOICE_SYMBOL_WAVES ];
			bool					mWaveActive				[ NUM_VOICE_SYMBOL_WAVES ];
			F64						mWaveFadeOutStartTime	[ NUM_VOICE_SYMBOL_WAVES ];
			F32						mWaveOpacity			[ NUM_VOICE_SYMBOL_WAVES ];
			LLPointer<LLImageGL>	mTexture				[ NUM_VOICE_SYMBOL_WAVES ];
			bool					mActive;
			LLVector3				mPosition;
		};

		LLFrameTimer			mTimer;							// so I can ask the current time in seconds
		F64						mStartTime;						// time in seconds when speaking started
		F64						mCurrentTime;					// current time in seconds, captured every step
		F64						mPreviousTime;					// copy of "current time" from last frame
		SoundSymbol				mSoundSymbol;					// the sound symbol that appears over the avatar's head
		bool					mVoiceEnabled;					// if off, no rendering should happen
		bool					mCurrentlySpeaking;				// is the user currently speaking?
		LLVector3				mVoiceSourceWorldPosition;		// give this to me every step - I need it to update the sound symbol
		F32						mSpeakingAmplitude;				// this should be set as often as possible when the user is speaking
		F32						mMaxGesticulationAmplitude;		// this is the upper-limit of the envelope of detectable gesticulation leves
		F32						mMinGesticulationAmplitude;		// this is the lower-limit of the envelope of detectable gesticulation leves
		
	//---------------------------------------------------
	// private static members 
	//---------------------------------------------------

		static U32	  sLipSyncEnabled;		 // 0 disabled, 1 babble loop
		static bool	  sPrefsInitialized;	 // the first instance will initialize the static members
		static F32*	  sOoh;					 // the babble loop of amplitudes for the ooh morph
		static F32*	  sAah;					 // the babble loop of amplitudes for the ooh morph
		static U32	  sOohs;				 // the number of entries in the ooh loop
		static U32	  sAahs;				 // the number of entries in the aah loop
		static F32	  sOohAahRate;			 // frames per second for the babble loop
		static F32*	  sOohPowerTransfer;	 // the power transfer characteristics for the ooh amplitude
		static U32	  sOohPowerTransfers;	 // the number of entries in the ooh transfer characteristics
		static F32	  sOohPowerTransfersf;	 // the number of entries in the ooh transfer characteristics as a float
		static F32*	  sAahPowerTransfer;	 // the power transfer characteristics for the aah amplitude
		static U32	  sAahPowerTransfers;	 // the number of entries in the aah transfer characteristics
		static F32	  sAahPowerTransfersf;	 // the number of entries in the aah transfer characteristics as a float

};//-----------------------------------------------------------------
 //   end of LLVoiceVisualizer class
//------------------------------------------------------------------

#endif //LL_VOICE_VISUALIZER_H

