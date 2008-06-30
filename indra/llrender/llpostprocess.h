/** 
 * @file llpostprocess.h
 * @brief LLPostProcess class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2007, Linden Research, Inc.
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

#ifndef LL_POSTPROCESS_H
#define LL_POSTPROCESS_H

#include <map>
#include <fstream>
#include "llgl.h"
#include "llglheaders.h"

class LLPostProcess 
{
public:

	typedef enum _QuadType {
		QUAD_NORMAL,
		QUAD_NOISE,
		QUAD_BLOOM_EXTRACT,
		QUAD_BLOOM_COMBINE
	} QuadType;

	/// GLSL Shader Encapsulation Struct
	typedef std::map<const char *, GLuint> glslUniforms;

	struct PostProcessTweaks : public LLSD {
		inline PostProcessTweaks() : LLSD(LLSD::emptyMap())
		{
		}

		inline LLSD & brightMult() {
			return (*this)["brightness_multiplier"];
		}

		inline LLSD & noiseStrength() {
			return (*this)["noise_strength"];
		}

		inline LLSD & noiseSize() {
			return (*this)["noise_size"];
		}

		inline LLSD & extractLow() {
			return (*this)["extract_low"];
		}

		inline LLSD & extractHigh() {
			return (*this)["extract_high"];
		}

		inline LLSD & bloomWidth() {
			return (*this)["bloom_width"];
		}

		inline LLSD & bloomStrength() {
			return (*this)["bloom_strength"];
		}

		inline LLSD & brightness() {
			return (*this)["brightness"];
		}

		inline LLSD & contrast() {
			return (*this)["contrast"];
		}

		inline LLSD & contrastBaseR() {
			return (*this)["contrast_base"][0];
		}

		inline LLSD & contrastBaseG() {
			return (*this)["contrast_base"][1];
		}

		inline LLSD & contrastBaseB() {
			return (*this)["contrast_base"][2];
		}

		inline LLSD & contrastBaseIntensity() {
			return (*this)["contrast_base"][3];
		}

		inline LLSD & saturation() {
			return (*this)["saturation"];
		}

		inline LLSD & useNightVisionShader() {
			return (*this)["enable_night_vision"];
		}

		inline LLSD & useBloomShader() {
			return (*this)["enable_bloom"];
		}

		inline LLSD & useColorFilter() {
			return (*this)["enable_color_filter"];
		}


		inline F32 getBrightMult() const {
			return F32((*this)["brightness_multiplier"].asReal());
		}

		inline F32 getNoiseStrength() const {
			return F32((*this)["noise_strength"].asReal());
		}

		inline F32 getNoiseSize() const {
			return F32((*this)["noise_size"].asReal());
		}

		inline F32 getExtractLow() const {
			return F32((*this)["extract_low"].asReal());
		}

		inline F32 getExtractHigh() const {
			return F32((*this)["extract_high"].asReal());
		}

		inline F32 getBloomWidth() const {
			return F32((*this)["bloom_width"].asReal());
		}

		inline F32 getBloomStrength() const {
			return F32((*this)["bloom_strength"].asReal());
		}

		inline F32 getBrightness() const {
			return F32((*this)["brightness"].asReal());
		}

		inline F32 getContrast() const {
			return F32((*this)["contrast"].asReal());
		}

		inline F32 getContrastBaseR() const {
			return F32((*this)["contrast_base"][0].asReal());
		}

		inline F32 getContrastBaseG() const {
			return F32((*this)["contrast_base"][1].asReal());
		}

		inline F32 getContrastBaseB() const {
			return F32((*this)["contrast_base"][2].asReal());
		}

		inline F32 getContrastBaseIntensity() const {
			return F32((*this)["contrast_base"][3].asReal());
		}

		inline F32 getSaturation() const {
			return F32((*this)["saturation"].asReal());
		}

	};
	
	GLuint sceneRenderTexture;
	GLuint noiseTexture;
	GLuint tempBloomTexture;
	bool initialized;
	PostProcessTweaks tweaks;

	// the map of all availible effects
	LLSD mAllEffects;

public:
	LLPostProcess(void);

	~LLPostProcess(void);

	void apply(unsigned int width, unsigned int height);

	/// Perform global initialization for this class.
	static void initClass(void);

	// Cleanup of global data that's only inited once per class.
	static void cleanupClass();

	void setSelectedEffect(std::string const & effectName);

	inline std::string const & getSelectedEffect(void) const {
		return mSelectedEffectName;
	}

	void saveEffect(std::string const & effectName);

private:
		/// read in from file
	std::string mShaderErrorString;
	unsigned int screenW;
	unsigned int screenH;

	float noiseTextureScale;
	
	/// Shader Uniforms
	glslUniforms nightVisionUniforms;
	glslUniforms bloomExtractUniforms;
	glslUniforms bloomBlurUniforms;
	glslUniforms colorFilterUniforms;

	// the name of currently selected effect in mAllEffects
	// invariant: tweaks == mAllEffects[mSelectedEffectName]
	std::string mSelectedEffectName;

	/// General functions
	void initialize(unsigned int width, unsigned int height);
	void doEffects(void);
	void applyShaders(void);
	bool shadersEnabled(void);

	/// Night Vision Functions
	void createNightVisionShader(void);
	void applyNightVisionShader(void);

	/// Bloom Functions
	void createBloomShader(void);
	void applyBloomShader(void);

	/// Color Filter Functions
	void createColorFilterShader(void);
	void applyColorFilterShader(void);

	/// OpenGL Helper Functions
	void getShaderUniforms(glslUniforms & uniforms, GLhandleARB & prog);
	void createTexture(GLuint & texture, unsigned int width, unsigned int height);
	void copyFrameBuffer(GLuint & texture, unsigned int width, unsigned int height);
	void createNoiseTexture(GLuint & texture);
	bool checkError(void);
	void checkShaderError(GLhandleARB shader);
	void drawOrthoQuad(unsigned int width, unsigned int height, QuadType type);
	void viewOrthogonal(unsigned int width, unsigned int height);
	void changeOrthogonal(unsigned int width, unsigned int height);
	void viewPerspective(void);
};

extern LLPostProcess * gPostProcess;


#endif // LL_POSTPROCESS_H
