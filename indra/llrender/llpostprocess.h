/** 
 * @file llpostprocess.h
 * @brief LLPostProcess class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_POSTPROCESS_H
#define LL_POSTPROCESS_H

#include <map>
#include <fstream>
#include "llgl.h"
#include "llglheaders.h"
#include "llstaticstringtable.h"

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
    typedef LLStaticStringTable<GLuint> glslUniforms;

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
    
    bool initialized;
    PostProcessTweaks tweaks;

    // the map of all availible effects
    LLSD mAllEffects;

private:
    LLPointer<LLImageGL> mSceneRenderTexture ;
    LLPointer<LLImageGL> mNoiseTexture ;
    LLPointer<LLImageGL> mTempBloomTexture ;

public:
    LLPostProcess(void);

    ~LLPostProcess(void);

    void apply(unsigned int width, unsigned int height);
    void invalidate() ;

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
    void createTexture(LLPointer<LLImageGL>& texture, unsigned int width, unsigned int height);
    void copyFrameBuffer(U32 & texture, unsigned int width, unsigned int height);
    void createNoiseTexture(LLPointer<LLImageGL>& texture);
    bool checkError(void);
    void checkShaderError(GLhandleARB shader);
    void drawOrthoQuad(unsigned int width, unsigned int height, QuadType type);
    void viewOrthogonal(unsigned int width, unsigned int height);
    void changeOrthogonal(unsigned int width, unsigned int height);
    void viewPerspective(void);
};

extern LLPostProcess * gPostProcess;


#endif // LL_POSTPROCESS_H
