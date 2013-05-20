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


LLPostProcess * gPostProcess = NULL;


static const unsigned int NOISE_SIZE = 512;

/// CALCULATING LUMINANCE (Using NTSC lum weights)
/// http://en.wikipedia.org/wiki/Luma_%28video%29
static const float LUMINANCE_R = 0.299f;
static const float LUMINANCE_G = 0.587f;
static const float LUMINANCE_B = 0.114f;

static const char * const XML_FILENAME = "postprocesseffects.xml";

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
	LL_DEBUGS2("AppInit", "Shaders") << "Loading PostProcess Effects settings from " << pathName << LL_ENDL;

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
	//llinfos << "Saving PostProcess Effects settings to " << pathName << llendl;

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
	initialized = FALSE ;
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
	/*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.
	gPostColorFilterProgram.bind();

	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_RECT_TEXTURE);

	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, sceneRenderTexture);

	getShaderUniforms(colorFilterUniforms, gPostColorFilterProgram.mProgramObject);
	glUniform1iARB(colorFilterUniforms["RenderTexture"], 0);
	glUniform1fARB(colorFilterUniforms["brightness"], tweaks.getBrightness());
	glUniform1fARB(colorFilterUniforms["contrast"], tweaks.getContrast());
	float baseI = (tweaks.getContrastBaseR() + tweaks.getContrastBaseG() + tweaks.getContrastBaseB()) / 3.0f;
	baseI = tweaks.getContrastBaseIntensity() / ((baseI < 0.001f) ? 0.001f : baseI);
	float baseR = tweaks.getContrastBaseR() * baseI;
	float baseG = tweaks.getContrastBaseG() * baseI;
	float baseB = tweaks.getContrastBaseB() * baseI;
	glUniform3fARB(colorFilterUniforms["contrastBase"], baseR, baseG, baseB);
	glUniform1fARB(colorFilterUniforms["saturation"], tweaks.getSaturation());
	glUniform3fARB(colorFilterUniforms["lumWeights"], LUMINANCE_R, LUMINANCE_G, LUMINANCE_B);
	
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	LLGLDepthTest depth(GL_FALSE);
		
	/// Draw a screen space quad
	drawOrthoQuad(screenW, screenH, QUAD_NORMAL);
	gPostColorFilterProgram.unbind();
	*/
}

void LLPostProcess::createColorFilterShader(void)
{
	/// Define uniform names
	colorFilterUniforms["RenderTexture"] = 0;
	colorFilterUniforms["brightness"] = 0;
	colorFilterUniforms["contrast"] = 0;
	colorFilterUniforms["contrastBase"] = 0;
	colorFilterUniforms["saturation"] = 0;
	colorFilterUniforms["lumWeights"] = 0;
}

void LLPostProcess::applyNightVisionShader(void)
{	
	/*  Do nothing.  Needs to be updated to use our current shader system, and to work with the move into llrender.
	gPostNightVisionProgram.bind();

	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->enable(LLTexUnit::TT_RECT_TEXTURE);

	getShaderUniforms(nightVisionUniforms, gPostNightVisionProgram.mProgramObject);
	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, sceneRenderTexture);
	glUniform1iARB(nightVisionUniforms["RenderTexture"], 0);

	gGL.getTexUnit(1)->activate();
	gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);	

	gGL.getTexUnit(1)->bindManual(LLTexUnit::TT_TEXTURE, noiseTexture);
	glUniform1iARB(nightVisionUniforms["NoiseTexture"], 1);

	
	glUniform1fARB(nightVisionUniforms["brightMult"], tweaks.getBrightMult());
	glUniform1fARB(nightVisionUniforms["noiseStrength"], tweaks.getNoiseStrength());
	noiseTextureScale = 0.01f + ((101.f - tweaks.getNoiseSize()) / 100.f);
	noiseTextureScale *= (screenH / NOISE_SIZE);


	glUniform3fARB(nightVisionUniforms["lumWeights"], LUMINANCE_R, LUMINANCE_G, LUMINANCE_B);
	
	LLGLEnable blend(GL_BLEND);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	LLGLDepthTest depth(GL_FALSE);
		
	/// Draw a screen space quad
	drawOrthoQuad(screenW, screenH, QUAD_NOISE);
	gPostNightVisionProgram.unbind();
	gGL.getTexUnit(0)->activate();
	*/
}

void LLPostProcess::createNightVisionShader(void)
{
	/// Define uniform names
	nightVisionUniforms["RenderTexture"] = 0;
	nightVisionUniforms["NoiseTexture"] = 0;
	nightVisionUniforms["brightMult"] = 0;	
	nightVisionUniforms["noiseStrength"] = 0;
	nightVisionUniforms["lumWeights"] = 0;	

	createNoiseTexture(mNoiseTexture);
}

void LLPostProcess::applyBloomShader(void)
{

}

void LLPostProcess::createBloomShader(void)
{
	createTexture(mTempBloomTexture, unsigned(screenW * 0.5), unsigned(screenH * 0.5));

	/// Create Bloom Extract Shader
	bloomExtractUniforms["RenderTexture"] = 0;
	bloomExtractUniforms["extractLow"] = 0;
	bloomExtractUniforms["extractHigh"] = 0;	
	bloomExtractUniforms["lumWeights"] = 0;	
	
	/// Create Bloom Blur Shader
	bloomBlurUniforms["RenderTexture"] = 0;
	bloomBlurUniforms["bloomStrength"] = 0;	
	bloomBlurUniforms["texelSize"] = 0;
	bloomBlurUniforms["blurDirection"] = 0;
	bloomBlurUniforms["blurWidth"] = 0;
}

void LLPostProcess::getShaderUniforms(glslUniforms & uniforms, GLhandleARB & prog)
{
	/// Find uniform locations and insert into map	
	std::map<const char *, GLuint>::iterator i;
	for (i  = uniforms.begin(); i != uniforms.end(); ++i){
		i->second = glGetUniformLocationARB(prog, i->first);
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
	
	LLGLSLShader::bindNoShader();
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
	gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, texture);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, 0, 0, width, height, 0);
}

void LLPostProcess::drawOrthoQuad(unsigned int width, unsigned int height, QuadType type)
{
#if 0
	float noiseX = 0.f;
	float noiseY = 0.f;
	float screenRatio = 1.0f;

	if (type == QUAD_NOISE){
		noiseX = ((float) rand() / (float) RAND_MAX);
		noiseY = ((float) rand() / (float) RAND_MAX);
		screenRatio = (float) width / (float) height;
	}
	

	glBegin(GL_QUADS);
		if (type != QUAD_BLOOM_EXTRACT){
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.f, (GLfloat) height);
		} else {
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.f, (GLfloat) height * 2.0f);
		}
		if (type == QUAD_NOISE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,
									noiseX,
									noiseTextureScale + noiseY);
		} else if (type == QUAD_BLOOM_COMBINE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.f, (GLfloat) height * 0.5f);
		}
		glVertex2f(0.f, (GLfloat) screenH - height);

		if (type != QUAD_BLOOM_EXTRACT){
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.f, 0.f);
		} else {
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.f, 0.f);
		}
		if (type == QUAD_NOISE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,
									noiseX,
									noiseY);
		} else if (type == QUAD_BLOOM_COMBINE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.f, 0.f);
		}
		glVertex2f(0.f, (GLfloat) height + (screenH - height));

		
		if (type != QUAD_BLOOM_EXTRACT){
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, (GLfloat) width, 0.f);
		} else {
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, (GLfloat) width * 2.0f, 0.f);
		}
		if (type == QUAD_NOISE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,
									screenRatio * noiseTextureScale + noiseX,
									noiseY);
		} else if (type == QUAD_BLOOM_COMBINE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, (GLfloat) width * 0.5f, 0.f);
		}
		glVertex2f((GLfloat) width, (GLfloat) height + (screenH - height));

		
		if (type != QUAD_BLOOM_EXTRACT){
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, (GLfloat) width, (GLfloat) height);
		} else {
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB, (GLfloat) width * 2.0f, (GLfloat) height * 2.0f);
		}
		if (type == QUAD_NOISE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,
									screenRatio * noiseTextureScale + noiseX,
									noiseTextureScale + noiseY);
		} else if (type == QUAD_BLOOM_COMBINE){
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, (GLfloat) width * 0.5f, (GLfloat) height * 0.5f);
		}
		glVertex2f((GLfloat) width, (GLfloat) screenH - height);
	glEnd();
#endif
}

void LLPostProcess::viewOrthogonal(unsigned int width, unsigned int height)
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho( 0.f, (GLdouble) width , (GLdouble) height , 0.f, -1.f, 1.f );
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

	texture = new LLImageGL(FALSE) ;	
	if(texture->createGLTexture())
	{
		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_RECT_TEXTURE, texture->getTexName());
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 4, width, height, 0,
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

	texture = new LLImageGL(FALSE) ;
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

void LLPostProcess::checkShaderError(GLhandleARB shader)
{
    GLint infologLength = 0;
    GLint charsWritten  = 0;
    GLchar *infoLog;

    checkError();  // Check for OpenGL errors

    glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infologLength);

    checkError();  // Check for OpenGL errors

    if (infologLength > 0)
    {
        infoLog = (GLchar *)malloc(infologLength);
        if (infoLog == NULL)
        {
            /// Could not allocate infolog buffer
            return;
        }
        glGetInfoLogARB(shader, infologLength, &charsWritten, infoLog);
		// shaderErrorLog << (char *) infoLog << std::endl;
		mShaderErrorString = (char *) infoLog;
        free(infoLog);
    }
    checkError();  // Check for OpenGL errors
}
