/**
 * @file llpostprocess.cpp
 * @brief LLPostProcess class implementation
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

#include "linden_common.h"

#include "llpostprocess.h"
#include "llglslshader.h"
#include "llsdserialize.h"
#include "llrender.h"

static LLStaticHashedString sRenderTexture("RenderTexture");
static LLStaticHashedString sBrightness("brightness");
static LLStaticHashedString sContrast("contrast");
static LLStaticHashedString sContrastBase("contrastBase");
static LLStaticHashedString sSaturation("saturation");
static LLStaticHashedString sLumWeights("lumWeights");
static LLStaticHashedString sNoiseTexture("NoiseTexture");
static LLStaticHashedString sBrightMult("brightMult");
static LLStaticHashedString sNoiseStrength("noiseStrength");
static LLStaticHashedString sExtractLow("extractLow");
static LLStaticHashedString sExtractHigh("extractHigh");
static LLStaticHashedString sBloomStrength("bloomStrength");
static LLStaticHashedString sTexelSize("texelSize");
static LLStaticHashedString sBlurDirection("blurDirection");
static LLStaticHashedString sBlurWidth("blurWidth");

LLPostProcess * gPostProcess = NULL;

static const unsigned int NOISE_SIZE = 512;

LLPostProcess::LLPostProcess(void) :
                    initialized(false),
                    mAllEffects(LLSD::emptyMap()),
                    screenW(1), screenH(1)
{
    mSceneRenderTexture = NULL ;
    mNoiseTexture = NULL ;
    mTempBloomTexture = NULL ;

    noiseTextureScale = 1.0f;

    /*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.
    std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight", XML_FILENAME));
    LL_DEBUGS("AppInit", "Shaders") << "Loading PostProcess Effects settings from " << pathName << LL_ENDL;

    llifstream effectsXML(pathName);

    if (effectsXML)
    {
        LLPointer<LLSDParser> parser = new LLSDXMLParser();

        parser->parse(effectsXML, mAllEffects, LLSDSerialize::SIZE_UNLIMITED);
    }

    if (!mAllEffects.has("default"))
    {
        LLSD & defaultEffect = (mAllEffects["default"] = LLSD::emptyMap());

        defaultEffect["enable_night_vision"] = LLSD::Boolean(false);
        defaultEffect["enable_bloom"] = LLSD::Boolean(false);
        defaultEffect["enable_color_filter"] = LLSD::Boolean(false);

        /// NVG Defaults
        defaultEffect["brightness_multiplier"] = 3.0;
        defaultEffect["noise_size"] = 25.0;
        defaultEffect["noise_strength"] = 0.4;

        // TODO BTest potentially add this to tweaks?
        noiseTextureScale = 1.0f;

        /// Bloom Defaults
        defaultEffect["extract_low"] = 0.95;
        defaultEffect["extract_high"] = 1.0;
        defaultEffect["bloom_width"] = 2.25;
        defaultEffect["bloom_strength"] = 1.5;

        /// Color Filter Defaults
        defaultEffect["brightness"] = 1.0;
        defaultEffect["contrast"] = 1.0;
        defaultEffect["saturation"] = 1.0;

        LLSD& contrastBase = (defaultEffect["contrast_base"] = LLSD::emptyArray());
        contrastBase.append(1.0);
        contrastBase.append(1.0);
        contrastBase.append(1.0);
        contrastBase.append(0.5);
    }

    setSelectedEffect("default");
    */
}

LLPostProcess::~LLPostProcess(void)
{
    invalidate() ;
}

// static
void LLPostProcess::initClass(void)
{
    //this will cause system to crash at second time login
    //if first time login fails due to network connection --- bao
    //***llassert_always(gPostProcess == NULL);
    //replaced by the following line:
    if(gPostProcess)
        return ;


    gPostProcess = new LLPostProcess();
}

// static
void LLPostProcess::cleanupClass()
{
    delete gPostProcess;
    gPostProcess = NULL;
}

void LLPostProcess::setSelectedEffect(std::string const & effectName)
{
    mSelectedEffectName = effectName;
    static_cast<LLSD &>(tweaks) = mAllEffects[effectName];
}

void LLPostProcess::saveEffect(std::string const & effectName)
{
    /*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.
    mAllEffects[effectName] = tweaks;

    std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight", XML_FILENAME));
    //LL_INFOS() << "Saving PostProcess Effects settings to " << pathName << LL_ENDL;

    llofstream effectsXML(pathName);

    LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();

    formatter->format(mAllEffects, effectsXML);
    */
}
void LLPostProcess::invalidate()
{
    mSceneRenderTexture = NULL ;
    mNoiseTexture = NULL ;
    mTempBloomTexture = NULL ;
    initialized = false ;
}

void LLPostProcess::apply(unsigned int width, unsigned int height)
{
    if (!initialized || width != screenW || height != screenH){
        initialize(width, height);
    }
    if (shadersEnabled()){
        doEffects();
    }
}

void LLPostProcess::initialize(unsigned int width, unsigned int height)
{
    screenW = width;
    screenH = height;
    createTexture(mSceneRenderTexture, screenW, screenH);
    initialized = true;

    checkError();
    createNightVisionShader();
    createBloomShader();
    createColorFilterShader();
    checkError();
}

inline bool LLPostProcess::shadersEnabled(void)
{
    return (tweaks.useColorFilter().asBoolean() ||
            tweaks.useNightVisionShader().asBoolean() ||
            tweaks.useBloomShader().asBoolean() );

}

void LLPostProcess::applyShaders(void)
{
    if (tweaks.useColorFilter()){
        applyColorFilterShader();
        checkError();
    }
    if (tweaks.useNightVisionShader()){
        /// If any of the above shaders have been called update the frame buffer;
        if (tweaks.useColorFilter())
        {
            U32 tex = mSceneRenderTexture->getTexName() ;
            copyFrameBuffer(tex, screenW, screenH);
        }
        applyNightVisionShader();
        checkError();
    }
    if (tweaks.useBloomShader()){
        /// If any of the above shaders have been called update the frame buffer;
        if (tweaks.useColorFilter().asBoolean() || tweaks.useNightVisionShader().asBoolean())
        {
            U32 tex = mSceneRenderTexture->getTexName() ;
            copyFrameBuffer(tex, screenW, screenH);
        }
        applyBloomShader();
        checkError();
    }
}

void LLPostProcess::applyColorFilterShader(void)
{

}

void LLPostProcess::createColorFilterShader(void)
{
    /// Define uniform names
    colorFilterUniforms[sRenderTexture] = 0;
    colorFilterUniforms[sBrightness] = 0;
    colorFilterUniforms[sContrast] = 0;
    colorFilterUniforms[sContrastBase] = 0;
    colorFilterUniforms[sSaturation] = 0;
    colorFilterUniforms[sLumWeights] = 0;
}

void LLPostProcess::applyNightVisionShader(void)
{

}

void LLPostProcess::createNightVisionShader(void)
{
    /// Define uniform names
    nightVisionUniforms[sRenderTexture] = 0;
    nightVisionUniforms[sNoiseTexture] = 0;
    nightVisionUniforms[sBrightMult] = 0;
    nightVisionUniforms[sNoiseStrength] = 0;
    nightVisionUniforms[sLumWeights] = 0;

    createNoiseTexture(mNoiseTexture);
}

void LLPostProcess::applyBloomShader(void)
{

}

void LLPostProcess::createBloomShader(void)
{
    createTexture(mTempBloomTexture, unsigned(screenW * 0.5), unsigned(screenH * 0.5));

    /// Create Bloom Extract Shader
    bloomExtractUniforms[sRenderTexture] = 0;
    bloomExtractUniforms[sExtractLow] = 0;
    bloomExtractUniforms[sExtractHigh] = 0;
    bloomExtractUniforms[sLumWeights] = 0;

    /// Create Bloom Blur Shader
    bloomBlurUniforms[sRenderTexture] = 0;
    bloomBlurUniforms[sBloomStrength] = 0;
    bloomBlurUniforms[sTexelSize] = 0;
    bloomBlurUniforms[sBlurDirection] = 0;
    bloomBlurUniforms[sBlurWidth] = 0;
}

void LLPostProcess::getShaderUniforms(glslUniforms & uniforms, GLuint & prog)
{
    /// Find uniform locations and insert into map
    glslUniforms::iterator i;
    for (i  = uniforms.begin(); i != uniforms.end(); ++i){
        i->second = glGetUniformLocation(prog, i->first.String().c_str());
    }
}

void LLPostProcess::doEffects(void)
{
    /// Save GL State
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_ALL_ATTRIB_BITS);

    /// Copy the screen buffer to the render texture
    {
        U32 tex = mSceneRenderTexture->getTexName() ;
        copyFrameBuffer(tex, screenW, screenH);
    }

    /// Clear the frame buffer.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /// Change to an orthogonal view
    viewOrthogonal(screenW, screenH);

    checkError();
    applyShaders();

    LLGLSLShader::unbind();
    checkError();

    /// Change to a perspective view
    viewPerspective();

    /// Reset GL State
    glPopClientAttrib();
    glPopAttrib();
    checkError();
}

void LLPostProcess::copyFrameBuffer(U32 & texture, unsigned int width, unsigned int height)
{
    gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, texture);
    glCopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, 0, 0, width, height, 0);
}

void LLPostProcess::drawOrthoQuad(unsigned int width, unsigned int height, QuadType type)
{

}

void LLPostProcess::viewOrthogonal(unsigned int width, unsigned int height)
{
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGL.ortho( 0.f, (GLfloat) width , (GLfloat) height , 0.f, -1.f, 1.f );
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadIdentity();
}

void LLPostProcess::viewPerspective(void)
{
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();
}

void LLPostProcess::changeOrthogonal(unsigned int width, unsigned int height)
{
    viewPerspective();
    viewOrthogonal(width, height);
}

void LLPostProcess::createTexture(LLPointer<LLImageGL>& texture, unsigned int width, unsigned int height)
{
    std::vector<GLubyte> data(width * height * 4, 0) ;

    texture = new LLImageGL(false) ;
    if(texture->createGLTexture())
    {
        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, texture->getTexName());
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, 4, width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
    }
}

void LLPostProcess::createNoiseTexture(LLPointer<LLImageGL>& texture)
{
    std::vector<GLubyte> buffer(NOISE_SIZE * NOISE_SIZE);
    for (unsigned int i = 0; i < NOISE_SIZE; i++){
        for (unsigned int k = 0; k < NOISE_SIZE; k++){
            buffer[(i * NOISE_SIZE) + k] = (GLubyte)((double) rand() / ((double) RAND_MAX + 1.f) * 255.f);
        }
    }

    texture = new LLImageGL(false) ;
    if(texture->createGLTexture())
    {
        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, texture->getTexName());
        LLImageGL::setManualImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, NOISE_SIZE, NOISE_SIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, &buffer[0]);
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_WRAP);
    }
}

bool LLPostProcess::checkError(void)
{
    GLenum glErr;
    bool    retCode = false;

    glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
        // shaderErrorLog << (const char *) gluErrorString(glErr) << std::endl;
        char const * err_str_raw = (const char *) gluErrorString(glErr);

        if(err_str_raw == NULL)
        {
            std::ostringstream err_builder;
            err_builder << "unknown error number " << glErr;
            mShaderErrorString = err_builder.str();
        }
        else
        {
            mShaderErrorString = err_str_raw;
        }

        retCode = true;
        glErr = glGetError();
    }
    return retCode;
}

void LLPostProcess::checkShaderError(GLuint shader)
{
    GLint infologLength = 0;
    GLint charsWritten  = 0;
    GLchar *infoLog;

    checkError();  // Check for OpenGL errors

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);

    checkError();  // Check for OpenGL errors

    if (infologLength > 0)
    {
        infoLog = (GLchar *)malloc(infologLength);
        if (infoLog == NULL)
        {
            /// Could not allocate infolog buffer
            return;
        }
       glGetProgramInfoLog(shader, infologLength, &charsWritten, infoLog);
        // shaderErrorLog << (char *) infoLog << std::endl;
        mShaderErrorString = (char *) infoLog;
        free(infoLog);
    }
    checkError();  // Check for OpenGL errors
}
