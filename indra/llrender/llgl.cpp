/** 
 * @file llgl.cpp
 * @brief LLGL implementation
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

// This file sets some global GL parameters, and implements some 
// useful functions for GL operations.

#define GLH_EXT_SINGLE_FILE

#include "linden_common.h"

#include "boost/tokenizer.hpp"

#include "llsys.h"

#include "llgl.h"
#include "llglstates.h"
#include "llrender.h"

#include "llerror.h"
#include "llerrorcontrol.h"
#include "llquaternion.h"
#include "llmath.h"
#include "m4math.h"
#include "llstring.h"
#include "llstacktrace.h"

#include "llglheaders.h"
#include "llglslshader.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

#ifdef _DEBUG
//#define GL_STATE_VERIFY
#endif


BOOL gDebugSession = FALSE;
BOOL gDebugGLSession = FALSE;
BOOL gClothRipple = FALSE;
BOOL gHeadlessClient = FALSE;
BOOL gNonInteractive = FALSE;
BOOL gGLActive = FALSE;

static const std::string HEADLESS_VENDOR_STRING("Linden Lab");
static const std::string HEADLESS_RENDERER_STRING("Headless");
static const std::string HEADLESS_VERSION_STRING("1.0");

llofstream gFailLog;

#if GL_ARB_debug_output

#ifndef APIENTRY
#define APIENTRY
#endif

void APIENTRY gl_debug_callback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message,
                                GLvoid* userParam)
{
    if (severity != GL_DEBUG_SEVERITY_HIGH_ARB &&
        severity != GL_DEBUG_SEVERITY_MEDIUM_ARB &&
        severity != GL_DEBUG_SEVERITY_LOW_ARB)
    { //suppress out-of-spec messages sent by nvidia driver (mostly vertexbuffer hints)
        return;
    }

	if (severity == GL_DEBUG_SEVERITY_HIGH_ARB)
	{
		LL_WARNS() << "----- GL ERROR --------" << LL_ENDL;
	}
	else
	{
		LL_WARNS() << "----- GL WARNING -------" << LL_ENDL;
	}
	LL_WARNS() << "Type: " << std::hex << type << LL_ENDL;
	LL_WARNS() << "ID: " << std::hex << id << LL_ENDL;
	LL_WARNS() << "Severity: " << std::hex << severity << LL_ENDL;
	LL_WARNS() << "Message: " << message << LL_ENDL;
	LL_WARNS() << "-----------------------" << LL_ENDL;
	if (severity == GL_DEBUG_SEVERITY_HIGH_ARB)
	{
		LL_ERRS() << "Halting on GL Error" << LL_ENDL;
	}
}
#endif

void parse_glsl_version(S32& major, S32& minor);

void ll_init_fail_log(std::string filename)
{
	gFailLog.open(filename.c_str());
}


void ll_fail(std::string msg)
{
	
	if (gDebugSession)
	{
		std::vector<std::string> lines;

		gFailLog << LLError::utcTime() << " " << msg << std::endl;

		gFailLog << "Stack Trace:" << std::endl;

		ll_get_stack_trace(lines);
		
		for(size_t i = 0; i < lines.size(); ++i)
		{
			gFailLog << lines[i] << std::endl;
		}

		gFailLog << "End of Stack Trace." << std::endl << std::endl;

		gFailLog.flush();
	}
};

void ll_close_fail_log()
{
	gFailLog.close();
}

LLMatrix4 gGLObliqueProjectionInverse;

#define LL_GL_NAME_POOLING 0

std::list<LLGLUpdate*> LLGLUpdate::sGLQ;

#if (LL_WINDOWS || LL_LINUX)  && !LL_MESA_HEADLESS

#if LL_WINDOWS
PFNGLGETSTRINGIPROC glGetStringi = NULL;
#endif

// vertex blending prototypes
PFNGLWEIGHTPOINTERARBPROC			glWeightPointerARB = NULL;
PFNGLVERTEXBLENDARBPROC				glVertexBlendARB = NULL;
PFNGLWEIGHTFVARBPROC				glWeightfvARB = NULL;

// Vertex buffer object prototypes
PFNGLBINDBUFFERARBPROC				glBindBufferARB = NULL;
PFNGLDELETEBUFFERSARBPROC			glDeleteBuffersARB = NULL;
PFNGLGENBUFFERSARBPROC				glGenBuffersARB = NULL;
PFNGLISBUFFERARBPROC				glIsBufferARB = NULL;
PFNGLBUFFERDATAARBPROC				glBufferDataARB = NULL;
PFNGLBUFFERSUBDATAARBPROC			glBufferSubDataARB = NULL;
PFNGLGETBUFFERSUBDATAARBPROC		glGetBufferSubDataARB = NULL;
PFNGLMAPBUFFERARBPROC				glMapBufferARB = NULL;
PFNGLUNMAPBUFFERARBPROC				glUnmapBufferARB = NULL;
PFNGLGETBUFFERPARAMETERIVARBPROC	glGetBufferParameterivARB = NULL;
PFNGLGETBUFFERPOINTERVARBPROC		glGetBufferPointervARB = NULL;

//GL_ARB_vertex_array_object
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLISVERTEXARRAYPROC glIsVertexArray = NULL;

// GL_ARB_map_buffer_range
PFNGLMAPBUFFERRANGEPROC			glMapBufferRange = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC	glFlushMappedBufferRange = NULL;

// GL_ARB_sync
PFNGLFENCESYNCPROC				glFenceSync = NULL;
PFNGLISSYNCPROC					glIsSync = NULL;
PFNGLDELETESYNCPROC				glDeleteSync = NULL;
PFNGLCLIENTWAITSYNCPROC			glClientWaitSync = NULL;
PFNGLWAITSYNCPROC				glWaitSync = NULL;
PFNGLGETINTEGER64VPROC			glGetInteger64v = NULL;
PFNGLGETSYNCIVPROC				glGetSynciv = NULL;

// GL_APPLE_flush_buffer_range
PFNGLBUFFERPARAMETERIAPPLEPROC	glBufferParameteriAPPLE = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEAPPLEPROC glFlushMappedBufferRangeAPPLE = NULL;

// GL_ARB_occlusion_query
PFNGLGENQUERIESARBPROC glGenQueriesARB = NULL;
PFNGLDELETEQUERIESARBPROC glDeleteQueriesARB = NULL;
PFNGLISQUERYARBPROC glIsQueryARB = NULL;
PFNGLBEGINQUERYARBPROC glBeginQueryARB = NULL;
PFNGLENDQUERYARBPROC glEndQueryARB = NULL;
PFNGLGETQUERYIVARBPROC glGetQueryivARB = NULL;
PFNGLGETQUERYOBJECTIVARBPROC glGetQueryObjectivARB = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuivARB = NULL;

// GL_ARB_timer_query
PFNGLQUERYCOUNTERPROC glQueryCounter = NULL;
PFNGLGETQUERYOBJECTI64VPROC glGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v = NULL;

// GL_ARB_point_parameters
PFNGLPOINTPARAMETERFARBPROC glPointParameterfARB = NULL;
PFNGLPOINTPARAMETERFVARBPROC glPointParameterfvARB = NULL;

// GL_ARB_framebuffer_object
PFNGLISRENDERBUFFERPROC glIsRenderbuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = NULL;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = NULL;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv = NULL;
PFNGLISFRAMEBUFFERPROC glIsFramebuffer = NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer = NULL;

//GL_ARB_texture_multisample
PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glTexImage3DMultisample = NULL;
PFNGLGETMULTISAMPLEFVPROC glGetMultisamplefv = NULL;
PFNGLSAMPLEMASKIPROC glSampleMaski = NULL;

//transform feedback (4.0 core)
PFNGLBEGINTRANSFORMFEEDBACKPROC glBeginTransformFeedback = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC glEndTransformFeedback = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glTransformFeedbackVaryings = NULL;
PFNGLBINDBUFFERRANGEPROC glBindBufferRange = NULL;
PFNGLBINDBUFFERBASEPROC glBindBufferBase = NULL;

//GL_ARB_debug_output
PFNGLDEBUGMESSAGECONTROLARBPROC glDebugMessageControlARB = NULL;
PFNGLDEBUGMESSAGEINSERTARBPROC glDebugMessageInsertARB = NULL;
PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = NULL;
PFNGLGETDEBUGMESSAGELOGARBPROC glGetDebugMessageLogARB = NULL;

// GL_EXT_blend_func_separate
PFNGLBLENDFUNCSEPARATEEXTPROC glBlendFuncSeparateEXT = NULL;

// GL_ARB_draw_buffers
PFNGLDRAWBUFFERSARBPROC glDrawBuffersARB = NULL;

//shader object prototypes
PFNGLDELETEOBJECTARBPROC glDeleteObjectARB = NULL;
PFNGLGETHANDLEARBPROC glGetHandleARB = NULL;
PFNGLDETACHOBJECTARBPROC glDetachObjectARB = NULL;
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB = NULL;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB = NULL;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB = NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB = NULL;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB = NULL;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB = NULL;
PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB = NULL;
PFNGLUNIFORM1FARBPROC glUniform1fARB = NULL;
PFNGLUNIFORM2FARBPROC glUniform2fARB = NULL;
PFNGLUNIFORM3FARBPROC glUniform3fARB = NULL;
PFNGLUNIFORM4FARBPROC glUniform4fARB = NULL;
PFNGLUNIFORM1IARBPROC glUniform1iARB = NULL;
PFNGLUNIFORM2IARBPROC glUniform2iARB = NULL;
PFNGLUNIFORM3IARBPROC glUniform3iARB = NULL;
PFNGLUNIFORM4IARBPROC glUniform4iARB = NULL;
PFNGLUNIFORM1FVARBPROC glUniform1fvARB = NULL;
PFNGLUNIFORM2FVARBPROC glUniform2fvARB = NULL;
PFNGLUNIFORM3FVARBPROC glUniform3fvARB = NULL;
PFNGLUNIFORM4FVARBPROC glUniform4fvARB = NULL;
PFNGLUNIFORM1IVARBPROC glUniform1ivARB = NULL;
PFNGLUNIFORM2IVARBPROC glUniform2ivARB = NULL;
PFNGLUNIFORM3IVARBPROC glUniform3ivARB = NULL;
PFNGLUNIFORM4IVARBPROC glUniform4ivARB = NULL;
PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB = NULL;
PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB = NULL;
PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB = NULL;
PFNGLGETINFOLOGARBPROC glGetInfoLogARB = NULL;
PFNGLGETATTACHEDOBJECTSARBPROC glGetAttachedObjectsARB = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB = NULL;
PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB = NULL;
PFNGLGETUNIFORMFVARBPROC glGetUniformfvARB = NULL;
PFNGLGETUNIFORMIVARBPROC glGetUniformivARB = NULL;
PFNGLGETSHADERSOURCEARBPROC glGetShaderSourceARB = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer = NULL;

#if LL_WINDOWS
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
#endif

// vertex shader prototypes
#if LL_LINUX
PFNGLVERTEXATTRIB1DARBPROC glVertexAttrib1dARB = NULL;
PFNGLVERTEXATTRIB1DVARBPROC glVertexAttrib1dvARB = NULL;
PFNGLVERTEXATTRIB1FARBPROC glVertexAttrib1fARB = NULL;
PFNGLVERTEXATTRIB1FVARBPROC glVertexAttrib1fvARB = NULL;
PFNGLVERTEXATTRIB1SARBPROC glVertexAttrib1sARB = NULL;
PFNGLVERTEXATTRIB1SVARBPROC glVertexAttrib1svARB = NULL;
PFNGLVERTEXATTRIB2DARBPROC glVertexAttrib2dARB = NULL;
PFNGLVERTEXATTRIB2DVARBPROC glVertexAttrib2dvARB = NULL;
PFNGLVERTEXATTRIB2FARBPROC glVertexAttrib2fARB = NULL;
PFNGLVERTEXATTRIB2FVARBPROC glVertexAttrib2fvARB = NULL;
PFNGLVERTEXATTRIB2SARBPROC glVertexAttrib2sARB = NULL;
PFNGLVERTEXATTRIB2SVARBPROC glVertexAttrib2svARB = NULL;
PFNGLVERTEXATTRIB3DARBPROC glVertexAttrib3dARB = NULL;
PFNGLVERTEXATTRIB3DVARBPROC glVertexAttrib3dvARB = NULL;
PFNGLVERTEXATTRIB3FARBPROC glVertexAttrib3fARB = NULL;
PFNGLVERTEXATTRIB3FVARBPROC glVertexAttrib3fvARB = NULL;
PFNGLVERTEXATTRIB3SARBPROC glVertexAttrib3sARB = NULL;
PFNGLVERTEXATTRIB3SVARBPROC glVertexAttrib3svARB = NULL;
#endif // LL_LINUX
PFNGLVERTEXATTRIB4NBVARBPROC glVertexAttrib4nbvARB = NULL;
PFNGLVERTEXATTRIB4NIVARBPROC glVertexAttrib4nivARB = NULL;
PFNGLVERTEXATTRIB4NSVARBPROC glVertexAttrib4nsvARB = NULL;
PFNGLVERTEXATTRIB4NUBARBPROC glVertexAttrib4nubARB = NULL;
PFNGLVERTEXATTRIB4NUBVARBPROC glVertexAttrib4nubvARB = NULL;
PFNGLVERTEXATTRIB4NUIVARBPROC glVertexAttrib4nuivARB = NULL;
PFNGLVERTEXATTRIB4NUSVARBPROC glVertexAttrib4nusvARB = NULL;
#if LL_LINUX
PFNGLVERTEXATTRIB4BVARBPROC glVertexAttrib4bvARB = NULL;
PFNGLVERTEXATTRIB4DARBPROC glVertexAttrib4dARB = NULL;
PFNGLVERTEXATTRIB4DVARBPROC glVertexAttrib4dvARB = NULL;
PFNGLVERTEXATTRIB4FARBPROC glVertexAttrib4fARB = NULL;
PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB = NULL;
PFNGLVERTEXATTRIB4IVARBPROC glVertexAttrib4ivARB = NULL;
PFNGLVERTEXATTRIB4SARBPROC glVertexAttrib4sARB = NULL;
PFNGLVERTEXATTRIB4SVARBPROC glVertexAttrib4svARB = NULL;
PFNGLVERTEXATTRIB4UBVARBPROC glVertexAttrib4ubvARB = NULL;
PFNGLVERTEXATTRIB4UIVARBPROC glVertexAttrib4uivARB = NULL;
PFNGLVERTEXATTRIB4USVARBPROC glVertexAttrib4usvARB = NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB = NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB = NULL;
PFNGLPROGRAMSTRINGARBPROC glProgramStringARB = NULL;
PFNGLBINDPROGRAMARBPROC glBindProgramARB = NULL;
PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB = NULL;
PFNGLGENPROGRAMSARBPROC glGenProgramsARB = NULL;
PFNGLPROGRAMENVPARAMETER4DARBPROC glProgramEnvParameter4dARB = NULL;
PFNGLPROGRAMENVPARAMETER4DVARBPROC glProgramEnvParameter4dvARB = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC glProgramEnvParameter4fARB = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glProgramLocalParameter4dvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC glProgramLocalParameter4fARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC glGetProgramEnvParameterdvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC glGetProgramEnvParameterfvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glGetProgramLocalParameterfvARB = NULL;
PFNGLGETPROGRAMIVARBPROC glGetProgramivARB = NULL;
PFNGLGETPROGRAMSTRINGARBPROC glGetProgramStringARB = NULL;
PFNGLGETVERTEXATTRIBDVARBPROC glGetVertexAttribdvARB = NULL;
PFNGLGETVERTEXATTRIBFVARBPROC glGetVertexAttribfvARB = NULL;
PFNGLGETVERTEXATTRIBIVARBPROC glGetVertexAttribivARB = NULL;
PFNGLGETVERTEXATTRIBPOINTERVARBPROC glGetVertexAttribPointervARB = NULL;
PFNGLISPROGRAMARBPROC glIsProgramARB = NULL;
#endif // LL_LINUX
PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB = NULL;
PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB = NULL;
PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB = NULL;

#if LL_WINDOWS
PFNWGLGETGPUIDSAMDPROC				wglGetGPUIDsAMD = NULL;
PFNWGLGETGPUINFOAMDPROC				wglGetGPUInfoAMD = NULL;
PFNWGLSWAPINTERVALEXTPROC			wglSwapIntervalEXT = NULL;
#endif

#if LL_LINUX_NV_GL_HEADERS
// linux nvidia headers.  these define these differently to mesa's.  ugh.
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = NULL;
PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements = NULL;
#endif // LL_LINUX_NV_GL_HEADERS
#endif

LLGLManager gGLManager;

LLGLManager::LLGLManager() :
	mInited(FALSE),
	mIsDisabled(FALSE),

	mHasMultitexture(FALSE),
	mHasATIMemInfo(FALSE),
	mHasAMDAssociations(FALSE),
	mHasNVXMemInfo(FALSE),
	mNumTextureUnits(1),
	mHasMipMapGeneration(FALSE),
	mHasCompressedTextures(FALSE),
	mHasFramebufferObject(FALSE),
	mMaxSamples(0),
	mHasBlendFuncSeparate(FALSE),
	mHasSync(FALSE),
	mHasVertexBufferObject(FALSE),
	mHasVertexArrayObject(FALSE),
	mHasMapBufferRange(FALSE),
	mHasFlushBufferRange(FALSE),
	mHasPBuffer(FALSE),
	mNumTextureImageUnits(0),
	mHasOcclusionQuery(FALSE),
	mHasTimerQuery(FALSE),
	mHasOcclusionQuery2(FALSE),
	mHasPointParameters(FALSE),
	mHasDrawBuffers(FALSE),
	mHasTextureRectangle(FALSE),
	mHasTextureMultisample(FALSE),
	mHasTransformFeedback(FALSE),
	mMaxSampleMaskWords(0),
	mMaxColorTextureSamples(0),
	mMaxDepthTextureSamples(0),
	mMaxIntegerSamples(0),

	mHasAnisotropic(FALSE),
	mHasARBEnvCombine(FALSE),
	mHasCubeMap(FALSE),
	mHasDebugOutput(FALSE),

	mIsAMD(FALSE),
	mIsNVIDIA(FALSE),
	mIsIntel(FALSE),
#if LL_DARWIN
	mIsMobileGF(FALSE),
#endif
	mHasRequirements(TRUE),

	mHasSeparateSpecularColor(FALSE),

	mDriverVersionMajor(1),
	mDriverVersionMinor(0),
	mDriverVersionRelease(0),
	mGLVersion(1.0f),
	mGLSLVersionMajor(0),
	mGLSLVersionMinor(0),		
	mVRAM(0),
	mGLMaxVertexRange(0),
	mGLMaxIndexRange(0)
{
}

//---------------------------------------------------------------------
// Global initialization for GL
//---------------------------------------------------------------------
void LLGLManager::initWGL()
{
	mHasPBuffer = FALSE;
#if LL_WINDOWS && !LL_MESA_HEADLESS
	if (!glh_init_extensions("WGL_ARB_pixel_format"))
	{
		LL_WARNS("RenderInit") << "No ARB pixel format extensions" << LL_ENDL;
	}

	if (ExtensionExists("WGL_ARB_create_context",gGLHExts.mSysExts))
	{
		GLH_EXT_NAME(wglCreateContextAttribsARB) = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GLH_EXT_GET_PROC_ADDRESS("wglCreateContextAttribsARB");
	}
	else
	{
		LL_WARNS("RenderInit") << "No ARB create context extensions" << LL_ENDL;
	}

	// For retreiving information per AMD adapter, 
	// because we can't trust curently selected/default one when there are multiple
	mHasAMDAssociations = ExtensionExists("WGL_AMD_gpu_association", gGLHExts.mSysExts);
	if (mHasAMDAssociations)
	{
		GLH_EXT_NAME(wglGetGPUIDsAMD) = (PFNWGLGETGPUIDSAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetGPUIDsAMD");
		GLH_EXT_NAME(wglGetGPUInfoAMD) = (PFNWGLGETGPUINFOAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetGPUInfoAMD");
	}

	if (ExtensionExists("WGL_EXT_swap_control", gGLHExts.mSysExts))
	{
        GLH_EXT_NAME(wglSwapIntervalEXT) = (PFNWGLSWAPINTERVALEXTPROC)GLH_EXT_GET_PROC_ADDRESS("wglSwapIntervalEXT");
	}

	if( !glh_init_extensions("WGL_ARB_pbuffer") )
	{
		LL_WARNS("RenderInit") << "No ARB WGL PBuffer extensions" << LL_ENDL;
	}

	if( !glh_init_extensions("WGL_ARB_render_texture") )
	{
		LL_WARNS("RenderInit") << "No ARB WGL render texture extensions" << LL_ENDL;
	}

	mHasPBuffer = ExtensionExists("WGL_ARB_pbuffer", gGLHExts.mSysExts) &&
					ExtensionExists("WGL_ARB_render_texture", gGLHExts.mSysExts) &&
					ExtensionExists("WGL_ARB_pixel_format", gGLHExts.mSysExts);
#endif
}

// return false if unable (or unwilling due to old drivers) to init GL
bool LLGLManager::initGL()
{
	if (mInited)
	{
		LL_ERRS("RenderInit") << "Calling init on LLGLManager after already initialized!" << LL_ENDL;
	}

	stop_glerror();

#if LL_WINDOWS
	if (!glGetStringi)
	{
		glGetStringi = (PFNGLGETSTRINGIPROC) GLH_EXT_GET_PROC_ADDRESS("glGetStringi");
	}

	//reload extensions string (may have changed after using wglCreateContextAttrib)
	if (glGetStringi)
	{
		std::stringstream str;

		GLint count = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &count);
		for (GLint i = 0; i < count; ++i)
		{
			std::string ext = ll_safe_string((const char*) glGetStringi(GL_EXTENSIONS, i));
			str << ext << " ";
			LL_DEBUGS("GLExtensions") << ext << LL_ENDL;
		}
		
		{
			PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
			wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
			if(wglGetExtensionsStringARB)
			{
				str << (const char*) wglGetExtensionsStringARB(wglGetCurrentDC());
			}
		}

		free(gGLHExts.mSysExts);
		std::string extensions = str.str();
		gGLHExts.mSysExts = strdup(extensions.c_str());
	}
#endif
	
	stop_glerror();

	// Extract video card strings and convert to upper case to
	// work around driver-to-driver variation in capitalization.
	mGLVendor = ll_safe_string((const char *)glGetString(GL_VENDOR));
	LLStringUtil::toUpper(mGLVendor);

	mGLRenderer = ll_safe_string((const char *)glGetString(GL_RENDERER));
	LLStringUtil::toUpper(mGLRenderer);

	parse_gl_version( &mDriverVersionMajor, 
		&mDriverVersionMinor, 
		&mDriverVersionRelease, 
		&mDriverVersionVendorString,
		&mGLVersionString);

	mGLVersion = mDriverVersionMajor + mDriverVersionMinor * .1f;

	if (mGLVersion >= 2.f)
	{
		parse_glsl_version(mGLSLVersionMajor, mGLSLVersionMinor);

#if LL_DARWIN
		//never use GLSL greater than 1.20 on OSX
		if (mGLSLVersionMajor > 1 || mGLSLVersionMinor >= 30)
		{
			mGLSLVersionMajor = 1;
			mGLSLVersionMinor = 20;
		}
#endif
	}

	if (mGLVersion >= 2.1f && LLImageGL::sCompressTextures)
	{ //use texture compression
		glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
	}
	else
	{ //GL version is < 3.0, always disable texture compression
		LLImageGL::sCompressTextures = false;
	}
	
	// Trailing space necessary to keep "nVidia Corpor_ati_on" cards
	// from being recognized as ATI.
    // NOTE: AMD has been pretty good about not breaking this check, do not rename without good reason
	if (mGLVendor.substr(0,4) == "ATI ")
	{
		mGLVendorShort = "AMD";
		// *TODO: Fix this?
		mIsAMD = TRUE;
	}
	else if (mGLVendor.find("NVIDIA ") != std::string::npos)
	{
		mGLVendorShort = "NVIDIA";
		mIsNVIDIA = TRUE;
	}
	else if (mGLVendor.find("INTEL") != std::string::npos
#if LL_LINUX
		 // The Mesa-based drivers put this in the Renderer string,
		 // not the Vendor string.
		 || mGLRenderer.find("INTEL") != std::string::npos
#endif //LL_LINUX
		 )
	{
		mGLVendorShort = "INTEL";
		mIsIntel = TRUE;
	}
	else
	{
		mGLVendorShort = "MISC";
	}
	
	stop_glerror();
	// This is called here because it depends on the setting of mIsGF2or4MX, and sets up mHasMultitexture.
	initExtensions();
	stop_glerror();

	S32 old_vram = mVRAM;
	mVRAM = 0;

#if LL_WINDOWS
	if (mHasAMDAssociations)
	{
		GLuint gl_gpus_count = wglGetGPUIDsAMD(0, 0);
		if (gl_gpus_count > 0)
		{
			GLuint* ids = new GLuint[gl_gpus_count];
			wglGetGPUIDsAMD(gl_gpus_count, ids);

			GLuint mem_mb = 0;
			for (U32 i = 0; i < gl_gpus_count; i++)
			{
				wglGetGPUInfoAMD(ids[i],
					WGL_GPU_RAM_AMD,
					GL_UNSIGNED_INT,
					sizeof(GLuint),
					&mem_mb);
				if (mVRAM < mem_mb)
				{
					// basically pick the best AMD and trust driver/OS to know to switch
					mVRAM = mem_mb;
				}
			}
		}
		if (mVRAM != 0)
		{
			LL_WARNS("RenderInit") << "VRAM Detected (AMDAssociations):" << mVRAM << LL_ENDL;
		}
	}
#endif

	if (mHasATIMemInfo && mVRAM == 0)
	{ //ask the gl how much vram is free at startup and attempt to use no more than half of that
		S32 meminfo[4];
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, meminfo);

		mVRAM = meminfo[0] / 1024;
		LL_WARNS("RenderInit") << "VRAM Detected (ATIMemInfo):" << mVRAM << LL_ENDL;
	}

	if (mHasNVXMemInfo && mVRAM == 0)
	{
		S32 dedicated_memory;
		glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedicated_memory);
		mVRAM = dedicated_memory/1024;
		LL_WARNS("RenderInit") << "VRAM Detected (NVXMemInfo):" << mVRAM << LL_ENDL;
	}

#if LL_WINDOWS
	if (mVRAM < 256)
	{
		// Something likely went wrong using the above extensions
		// try WMI first and fall back to old method (from dxdiag) if all else fails
		// Function will check all GPUs WMI knows of and will pick up the one with most
		// memory. We need to check all GPUs because system can switch active GPU to
		// weaker one, to preserve power when not under load.
		S32 mem = LLDXHardware::getMBVideoMemoryViaWMI();
		if (mem != 0)
		{
			mVRAM = mem;
			LL_WARNS("RenderInit") << "VRAM Detected (WMI):" << mVRAM<< LL_ENDL;
		}
	}
#endif

	if (mVRAM < 256 && old_vram > 0)
	{
		// fall back to old method
		// Note: on Windows value will be from LLDXHardware.
		// Either received via dxdiag or via WMI by id from dxdiag.
		mVRAM = old_vram;
	}

	stop_glerror();

	GLint num_tex_image_units;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &num_tex_image_units);
	mNumTextureImageUnits = llmin(num_tex_image_units, 32);

    if (mHasMultitexture)
    {
        if (LLRender::sGLCoreProfile)
        {
            mNumTextureUnits = llmin(mNumTextureImageUnits, MAX_GL_TEXTURE_UNITS);
        }
        else
        {
            GLint num_tex_units;
            glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &num_tex_units);
            mNumTextureUnits = llmin(num_tex_units, (GLint)MAX_GL_TEXTURE_UNITS);
            if (mIsIntel)
            {
                mNumTextureUnits = llmin(mNumTextureUnits, 2);
            }
        }
    }
	else
	{
		mHasRequirements = FALSE;

		// We don't support cards that don't support the GL_ARB_multitexture extension
		LL_WARNS("RenderInit") << "GL Drivers do not support GL_ARB_multitexture" << LL_ENDL;
		return false;
	}

    if (!mHasFramebufferObject)
    {
        mHasRequirements = FALSE;

        LL_WARNS("RenderInit") << "GL Drivers do not support GL_ARB_framebuffer_object" << LL_ENDL;
        return false;
    }
	
	stop_glerror();

	if (mHasTextureMultisample)
	{
		glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &mMaxColorTextureSamples);
		glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &mMaxDepthTextureSamples);
		glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &mMaxIntegerSamples);
		glGetIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &mMaxSampleMaskWords);
	}

	stop_glerror();

	//HACK always disable texture multisample, use FXAA instead
	mHasTextureMultisample = FALSE;

	if (mHasFramebufferObject)
	{
		glGetIntegerv(GL_MAX_SAMPLES, &mMaxSamples);
	}

	stop_glerror();
	
	initGLStates();

	stop_glerror();

	return true;
}

void LLGLManager::getGLInfo(LLSD& info)
{
	if (gHeadlessClient)
	{
		info["GLInfo"]["GLVendor"] = HEADLESS_VENDOR_STRING;
		info["GLInfo"]["GLRenderer"] = HEADLESS_RENDERER_STRING;
		info["GLInfo"]["GLVersion"] = HEADLESS_VERSION_STRING;
		return;
	}
	else
	{
		info["GLInfo"]["GLVendor"] = ll_safe_string((const char *)glGetString(GL_VENDOR));
		info["GLInfo"]["GLRenderer"] = ll_safe_string((const char *)glGetString(GL_RENDERER));
		info["GLInfo"]["GLVersion"] = ll_safe_string((const char *)glGetString(GL_VERSION));
	}

#if !LL_MESA_HEADLESS
	std::string all_exts = ll_safe_string((const char *)gGLHExts.mSysExts);
	boost::char_separator<char> sep(" ");
	boost::tokenizer<boost::char_separator<char> > tok(all_exts, sep);
	for(boost::tokenizer<boost::char_separator<char> >::iterator i = tok.begin(); i != tok.end(); ++i)
	{
		info["GLInfo"]["GLExtensions"].append(*i);
	}
#endif
}

std::string LLGLManager::getGLInfoString()
{
	std::string info_str;

	if (gHeadlessClient)
	{
		info_str += std::string("GL_VENDOR      ") + HEADLESS_VENDOR_STRING + std::string("\n");
		info_str += std::string("GL_RENDERER    ") + HEADLESS_RENDERER_STRING + std::string("\n");
		info_str += std::string("GL_VERSION     ") + HEADLESS_VERSION_STRING + std::string("\n");
	}
	else
	{
		info_str += std::string("GL_VENDOR      ") + ll_safe_string((const char *)glGetString(GL_VENDOR)) + std::string("\n");
		info_str += std::string("GL_RENDERER    ") + ll_safe_string((const char *)glGetString(GL_RENDERER)) + std::string("\n");
		info_str += std::string("GL_VERSION     ") + ll_safe_string((const char *)glGetString(GL_VERSION)) + std::string("\n");
	}

#if !LL_MESA_HEADLESS 
	std::string all_exts= ll_safe_string(((const char *)gGLHExts.mSysExts));
	LLStringUtil::replaceChar(all_exts, ' ', '\n');
	info_str += std::string("GL_EXTENSIONS:\n") + all_exts + std::string("\n");
#endif
	
	return info_str;
}

void LLGLManager::printGLInfoString()
{
	if (gHeadlessClient)
	{
		LL_INFOS("RenderInit") << "GL_VENDOR:     " << HEADLESS_VENDOR_STRING << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_RENDERER:   " << HEADLESS_RENDERER_STRING << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_VERSION:    " << HEADLESS_VERSION_STRING << LL_ENDL;
	}
	else
	{
		LL_INFOS("RenderInit") << "GL_VENDOR:     " << ll_safe_string((const char *)glGetString(GL_VENDOR)) << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_RENDERER:   " << ll_safe_string((const char *)glGetString(GL_RENDERER)) << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_VERSION:    " << ll_safe_string((const char *)glGetString(GL_VERSION)) << LL_ENDL;
	}

#if !LL_MESA_HEADLESS
	std::string all_exts= ll_safe_string(((const char *)gGLHExts.mSysExts));
	LLStringUtil::replaceChar(all_exts, ' ', '\n');
	LL_DEBUGS("RenderInit") << "GL_EXTENSIONS:\n" << all_exts << LL_ENDL;
#endif
}

std::string LLGLManager::getRawGLString()
{
	std::string gl_string;
	if (gHeadlessClient)
	{
		gl_string = HEADLESS_VENDOR_STRING + " " + HEADLESS_RENDERER_STRING;
	}
	else
	{
		gl_string = ll_safe_string((char*)glGetString(GL_VENDOR)) + " " + ll_safe_string((char*)glGetString(GL_RENDERER));
	}
	return gl_string;
}

void LLGLManager::asLLSD(LLSD& info)
{
	// Currently these are duplicates of fields in "system".
	info["gpu_vendor"] = mGLVendorShort;
	info["gpu_version"] = mDriverVersionVendorString;
	info["opengl_version"] = mGLVersionString;

	info["vram"] = mVRAM;

	// Extensions used by everyone
	info["has_multitexture"] = mHasMultitexture;
	info["has_ati_mem_info"] = mHasATIMemInfo;
	info["has_nvx_mem_info"] = mHasNVXMemInfo;
	info["num_texture_units"] = mNumTextureUnits;
	info["has_mip_map_generation"] = mHasMipMapGeneration;
	info["has_compressed_textures"] = mHasCompressedTextures;
	info["has_framebuffer_object"] = mHasFramebufferObject;
	info["max_samples"] = mMaxSamples;
	info["has_blend_func_separate"] = mHasBlendFuncSeparate;

	// ARB Extensions
	info["has_vertex_buffer_object"] = mHasVertexBufferObject;
	info["has_vertex_array_object"] = mHasVertexArrayObject;
	info["has_sync"] = mHasSync;
	info["has_map_buffer_range"] = mHasMapBufferRange;
	info["has_flush_buffer_range"] = mHasFlushBufferRange;
	info["has_pbuffer"] = mHasPBuffer;
    info["has_shader_objects"] = std::string("Assumed TRUE");   // was mHasShaderObjects;
	info["has_vertex_shader"] = std::string("Assumed TRUE");    // was mHasVertexShader;
	info["has_fragment_shader"] = std::string("Assumed TRUE");  // was mHasFragmentShader;
	info["num_texture_image_units"] =  mNumTextureImageUnits;
	info["has_occlusion_query"] = mHasOcclusionQuery;
	info["has_timer_query"] = mHasTimerQuery;
	info["has_occlusion_query2"] = mHasOcclusionQuery2;
	info["has_point_parameters"] = mHasPointParameters;
	info["has_draw_buffers"] = mHasDrawBuffers;
	info["has_depth_clamp"] = mHasDepthClamp;
	info["has_texture_rectangle"] = mHasTextureRectangle;
	info["has_texture_multisample"] = mHasTextureMultisample;
	info["has_transform_feedback"] = mHasTransformFeedback;
	info["max_sample_mask_words"] = mMaxSampleMaskWords;
	info["max_color_texture_samples"] = mMaxColorTextureSamples;
	info["max_depth_texture_samples"] = mMaxDepthTextureSamples;
	info["max_integer_samples"] = mMaxIntegerSamples;

	// Other extensions.
	info["has_anisotropic"] = mHasAnisotropic;
	info["has_arb_env_combine"] = mHasARBEnvCombine;
	info["has_cube_map"] = mHasCubeMap;
	info["has_debug_output"] = mHasDebugOutput;
	info["has_srgb_texture"] = mHassRGBTexture;
	info["has_srgb_framebuffer"] = mHassRGBFramebuffer;
    info["has_texture_srgb_decode"] = mHasTexturesRGBDecode;

	// Vendor-specific extensions
	info["is_ati"] = mIsAMD;  // note, do not rename is_ati to is_amd without coordinating with DW
	info["is_nvidia"] = mIsNVIDIA;
	info["is_intel"] = mIsIntel;

	// Other fields
	info["has_requirements"] = mHasRequirements;
	info["has_separate_specular_color"] = mHasSeparateSpecularColor;
	info["max_vertex_range"] = mGLMaxVertexRange;
	info["max_index_range"] = mGLMaxIndexRange;
	info["max_texture_size"] = mGLMaxTextureSize;
	info["gl_renderer"] = mGLRenderer;
}

void LLGLManager::shutdownGL()
{
	if (mInited)
	{
		glFinish();
		stop_glerror();
		mInited = FALSE;
	}
}

// these are used to turn software blending on. They appear in the Debug/Avatar menu
// presence of vertex skinning/blending or vertex programs will set these to FALSE by default.

void LLGLManager::initExtensions()
{
#if LL_MESA_HEADLESS
# ifdef GL_ARB_multitexture
	mHasMultitexture = TRUE;
# else
	mHasMultitexture = FALSE;
# endif // GL_ARB_multitexture
# ifdef GL_ARB_texture_env_combine
	mHasARBEnvCombine = TRUE;	
# else
	mHasARBEnvCombine = FALSE;
# endif // GL_ARB_texture_env_combine
# ifdef GL_ARB_texture_compression
	mHasCompressedTextures = TRUE;
# else
	mHasCompressedTextures = FALSE;
# endif // GL_ARB_texture_compression
# ifdef GL_ARB_vertex_buffer_object
	mHasVertexBufferObject = TRUE;
# else
	mHasVertexBufferObject = FALSE;
# endif // GL_ARB_vertex_buffer_object
# ifdef GL_EXT_framebuffer_object
	mHasFramebufferObject = TRUE;
# else
	mHasFramebufferObject = FALSE;
# endif // GL_EXT_framebuffer_object
# ifdef GL_ARB_draw_buffers
	mHasDrawBuffers = TRUE;
#else
	mHasDrawBuffers = FALSE;
# endif // GL_ARB_draw_buffers
# if defined(GL_NV_depth_clamp) || defined(GL_ARB_depth_clamp)
	mHasDepthClamp = TRUE;
#else
	mHasDepthClamp = FALSE;
#endif // defined(GL_NV_depth_clamp) || defined(GL_ARB_depth_clamp)
# if GL_EXT_blend_func_separate
	mHasBlendFuncSeparate = TRUE;
#else
	mHasBlendFuncSeparate = FALSE;
# endif // GL_EXT_blend_func_separate
	mHasMipMapGeneration = FALSE;
	mHasSeparateSpecularColor = FALSE;
	mHasAnisotropic = FALSE;
	mHasCubeMap = FALSE;
	mHasOcclusionQuery = FALSE;
	mHasPointParameters = FALSE;
	mHasTextureRectangle = FALSE;
#else // LL_MESA_HEADLESS //important, gGLHExts.mSysExts is uninitialized until after glh_init_extensions is called
	mHasMultitexture = glh_init_extensions("GL_ARB_multitexture");
	mHasATIMemInfo = ExtensionExists("GL_ATI_meminfo", gGLHExts.mSysExts); //Basic AMD method, also see mHasAMDAssociations
	mHasNVXMemInfo = ExtensionExists("GL_NVX_gpu_memory_info", gGLHExts.mSysExts);
	mHasSeparateSpecularColor = glh_init_extensions("GL_EXT_separate_specular_color");
	mHasAnisotropic = glh_init_extensions("GL_EXT_texture_filter_anisotropic");
	glh_init_extensions("GL_ARB_texture_cube_map");
	mHasCubeMap = ExtensionExists("GL_ARB_texture_cube_map", gGLHExts.mSysExts);
	mHasARBEnvCombine = ExtensionExists("GL_ARB_texture_env_combine", gGLHExts.mSysExts);
	mHasCompressedTextures = glh_init_extensions("GL_ARB_texture_compression");
	mHasOcclusionQuery = ExtensionExists("GL_ARB_occlusion_query", gGLHExts.mSysExts);
	mHasTimerQuery = ExtensionExists("GL_ARB_timer_query", gGLHExts.mSysExts);
	mHasOcclusionQuery2 = ExtensionExists("GL_ARB_occlusion_query2", gGLHExts.mSysExts);
	mHasVertexBufferObject = ExtensionExists("GL_ARB_vertex_buffer_object", gGLHExts.mSysExts);
	mHasVertexArrayObject = ExtensionExists("GL_ARB_vertex_array_object", gGLHExts.mSysExts);
	mHasSync = ExtensionExists("GL_ARB_sync", gGLHExts.mSysExts);
	mHasMapBufferRange = ExtensionExists("GL_ARB_map_buffer_range", gGLHExts.mSysExts);
	mHasFlushBufferRange = ExtensionExists("GL_APPLE_flush_buffer_range", gGLHExts.mSysExts);
    // NOTE: Using extensions breaks reflections when Shadows are set to projector.  See: SL-16727
    //mHasDepthClamp = ExtensionExists("GL_ARB_depth_clamp", gGLHExts.mSysExts) || ExtensionExists("GL_NV_depth_clamp", gGLHExts.mSysExts);
    mHasDepthClamp = FALSE;
	// mask out FBO support when packed_depth_stencil isn't there 'cause we need it for LLRenderTarget -Brad
#ifdef GL_ARB_framebuffer_object
	mHasFramebufferObject = ExtensionExists("GL_ARB_framebuffer_object", gGLHExts.mSysExts);
#else
	mHasFramebufferObject = ExtensionExists("GL_EXT_framebuffer_object", gGLHExts.mSysExts) &&
							ExtensionExists("GL_EXT_framebuffer_blit", gGLHExts.mSysExts) &&
							ExtensionExists("GL_EXT_framebuffer_multisample", gGLHExts.mSysExts) &&
							ExtensionExists("GL_EXT_packed_depth_stencil", gGLHExts.mSysExts);
#endif
#ifdef GL_EXT_texture_sRGB
	mHassRGBTexture = ExtensionExists("GL_EXT_texture_sRGB", gGLHExts.mSysExts);
#endif
	
#ifdef GL_ARB_framebuffer_sRGB
	mHassRGBFramebuffer = ExtensionExists("GL_ARB_framebuffer_sRGB", gGLHExts.mSysExts);
#else
	mHassRGBFramebuffer = ExtensionExists("GL_EXT_framebuffer_sRGB", gGLHExts.mSysExts);
#endif
	
#ifdef GL_EXT_texture_sRGB_decode
    mHasTexturesRGBDecode = ExtensionExists("GL_EXT_texture_sRGB_decode", gGLHExts.mSysExts);
#else
    mHasTexturesRGBDecode = ExtensionExists("GL_ARB_texture_sRGB_decode", gGLHExts.mSysExts);
#endif

	mHasMipMapGeneration = mHasFramebufferObject || mGLVersion >= 1.4f;

	mHasDrawBuffers = ExtensionExists("GL_ARB_draw_buffers", gGLHExts.mSysExts);
	mHasBlendFuncSeparate = ExtensionExists("GL_EXT_blend_func_separate", gGLHExts.mSysExts);
	mHasTextureRectangle = ExtensionExists("GL_ARB_texture_rectangle", gGLHExts.mSysExts);
	mHasTextureMultisample = ExtensionExists("GL_ARB_texture_multisample", gGLHExts.mSysExts);
	mHasDebugOutput = ExtensionExists("GL_ARB_debug_output", gGLHExts.mSysExts);
	mHasTransformFeedback = mGLVersion >= 4.f ? TRUE : FALSE;
#if !LL_DARWIN
	mHasPointParameters = ExtensionExists("GL_ARB_point_parameters", gGLHExts.mSysExts);
#endif
#endif

#if LL_LINUX
	LL_INFOS() << "initExtensions() checking shell variables to adjust features..." << LL_ENDL;
	// Our extension support for the Linux Client is very young with some
	// potential driver gotchas, so offer a semi-secret way to turn it off.
	if (getenv("LL_GL_NOEXT"))
	{
		//mHasMultitexture = FALSE; // NEEDED!
		mHasDepthClamp = FALSE;
		mHasARBEnvCombine = FALSE;
		mHasCompressedTextures = FALSE;
		mHasVertexBufferObject = FALSE;
		mHasFramebufferObject = FALSE;
		mHasDrawBuffers = FALSE;
		mHasBlendFuncSeparate = FALSE;
		mHasMipMapGeneration = FALSE;
		mHasSeparateSpecularColor = FALSE;
		mHasAnisotropic = FALSE;
		mHasCubeMap = FALSE;
		mHasOcclusionQuery = FALSE;
		mHasPointParameters = FALSE;
		LL_WARNS("RenderInit") << "GL extension support DISABLED via LL_GL_NOEXT" << LL_ENDL;
	}
	else if (getenv("LL_GL_BASICEXT"))	/* Flawfinder: ignore */
	{
		// This switch attempts to turn off all support for exotic
		// extensions which I believe correspond to fatal driver
		// bug reports.  This should be the default until we get a
		// proper blacklist/whitelist on Linux.
		mHasMipMapGeneration = FALSE;
		mHasAnisotropic = FALSE;
		//mHasCubeMap = FALSE; // apparently fatal on Intel 915 & similar
		//mHasOcclusionQuery = FALSE; // source of many ATI system hangs
		mHasBlendFuncSeparate = FALSE;
		LL_WARNS("RenderInit") << "GL extension support forced to SIMPLE level via LL_GL_BASICEXT" << LL_ENDL;
	}
	if (getenv("LL_GL_BLACKLIST"))	/* Flawfinder: ignore */
	{
		// This lets advanced troubleshooters disable specific
		// GL extensions to isolate problems with their hardware.
		// SL-28126
		const char *const blacklist = getenv("LL_GL_BLACKLIST");	/* Flawfinder: ignore */
		LL_WARNS("RenderInit") << "GL extension support partially disabled via LL_GL_BLACKLIST: " << blacklist << LL_ENDL;
		if (strchr(blacklist,'a')) mHasARBEnvCombine = FALSE;
		if (strchr(blacklist,'b')) mHasCompressedTextures = FALSE;
		if (strchr(blacklist,'c')) mHasVertexBufferObject = FALSE;
		if (strchr(blacklist,'d')) mHasMipMapGeneration = FALSE;//S
// 		if (strchr(blacklist,'f')) mHasNVVertexArrayRange = FALSE;//S
// 		if (strchr(blacklist,'g')) mHasNVFence = FALSE;//S
		if (strchr(blacklist,'h')) mHasSeparateSpecularColor = FALSE;
		if (strchr(blacklist,'i')) mHasAnisotropic = FALSE;//S
		if (strchr(blacklist,'j')) mHasCubeMap = FALSE;//S
// 		if (strchr(blacklist,'k')) mHasATIVAO = FALSE;//S
		if (strchr(blacklist,'l')) mHasOcclusionQuery = FALSE;
		if (strchr(blacklist,'p')) mHasPointParameters = FALSE;//S
		if (strchr(blacklist,'q')) mHasFramebufferObject = FALSE;//S
		if (strchr(blacklist,'r')) mHasDrawBuffers = FALSE;//S
		if (strchr(blacklist,'s')) mHasTextureRectangle = FALSE;
		if (strchr(blacklist,'t')) mHasBlendFuncSeparate = FALSE;//S
		if (strchr(blacklist,'u')) mHasDepthClamp = FALSE;
		
	}
#endif // LL_LINUX
	
	if (!mHasMultitexture)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize multitexturing" << LL_ENDL;
	}
	if (!mHasMipMapGeneration)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize mipmap generation" << LL_ENDL;
	}
	if (!mHasARBEnvCombine)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize GL_ARB_texture_env_combine" << LL_ENDL;
	}
	if (!mHasSeparateSpecularColor)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize separate specular color" << LL_ENDL;
	}
	if (!mHasAnisotropic)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize anisotropic filtering" << LL_ENDL;
	}
	if (!mHasCompressedTextures)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize GL_ARB_texture_compression" << LL_ENDL;
	}
	if (!mHasOcclusionQuery)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize GL_ARB_occlusion_query" << LL_ENDL;
	}
	if (!mHasOcclusionQuery2)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize GL_ARB_occlusion_query2" << LL_ENDL;
	}
	if (!mHasPointParameters)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize GL_ARB_point_parameters" << LL_ENDL;
	}
	if (!mHasBlendFuncSeparate)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize GL_EXT_blend_func_separate" << LL_ENDL;
	}
	if (!mHasDrawBuffers)
	{
		LL_INFOS("RenderInit") << "Couldn't initialize GL_ARB_draw_buffers" << LL_ENDL;
	}

	// Disable certain things due to known bugs
	if (mIsIntel && mHasMipMapGeneration)
	{
		LL_INFOS("RenderInit") << "Disabling mip-map generation for Intel GPUs" << LL_ENDL;
		mHasMipMapGeneration = FALSE;
	}

	// Misc
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, (GLint*) &mGLMaxVertexRange);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, (GLint*) &mGLMaxIndexRange);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*) &mGLMaxTextureSize);

#if (LL_WINDOWS || LL_LINUX) && !LL_MESA_HEADLESS
	LL_DEBUGS("RenderInit") << "GL Probe: Getting symbols" << LL_ENDL;
	if (mHasVertexBufferObject)
	{
		glBindBufferARB = (PFNGLBINDBUFFERARBPROC)GLH_EXT_GET_PROC_ADDRESS("glBindBufferARB");
		if (glBindBufferARB)
		{
			glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteBuffersARB");
			glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGenBuffersARB");
			glIsBufferARB = (PFNGLISBUFFERARBPROC)GLH_EXT_GET_PROC_ADDRESS("glIsBufferARB");
			glBufferDataARB = (PFNGLBUFFERDATAARBPROC)GLH_EXT_GET_PROC_ADDRESS("glBufferDataARB");
			glBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)GLH_EXT_GET_PROC_ADDRESS("glBufferSubDataARB");
			glGetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBufferSubDataARB");
			glMapBufferARB = (PFNGLMAPBUFFERARBPROC)GLH_EXT_GET_PROC_ADDRESS("glMapBufferARB");
			glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)GLH_EXT_GET_PROC_ADDRESS("glUnmapBufferARB");
			glGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBufferParameterivARB");
			glGetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBufferPointervARB");
		}
		else
		{
			mHasVertexBufferObject = FALSE;
		}
	}
	if (mHasVertexArrayObject)
	{
		glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) GLH_EXT_GET_PROC_ADDRESS("glBindVertexArray");
		glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) GLH_EXT_GET_PROC_ADDRESS("glDeleteVertexArrays");
		glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) GLH_EXT_GET_PROC_ADDRESS("glGenVertexArrays");
		glIsVertexArray = (PFNGLISVERTEXARRAYPROC) GLH_EXT_GET_PROC_ADDRESS("glIsVertexArray");
	}
	if (mHasSync)
	{
		glFenceSync = (PFNGLFENCESYNCPROC) GLH_EXT_GET_PROC_ADDRESS("glFenceSync");
		glIsSync = (PFNGLISSYNCPROC) GLH_EXT_GET_PROC_ADDRESS("glIsSync");
		glDeleteSync = (PFNGLDELETESYNCPROC) GLH_EXT_GET_PROC_ADDRESS("glDeleteSync");
		glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC) GLH_EXT_GET_PROC_ADDRESS("glClientWaitSync");
		glWaitSync = (PFNGLWAITSYNCPROC) GLH_EXT_GET_PROC_ADDRESS("glWaitSync");
		glGetInteger64v = (PFNGLGETINTEGER64VPROC) GLH_EXT_GET_PROC_ADDRESS("glGetInteger64v");
		glGetSynciv = (PFNGLGETSYNCIVPROC) GLH_EXT_GET_PROC_ADDRESS("glGetSynciv");
	}
	if (mHasMapBufferRange)
	{
		glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC) GLH_EXT_GET_PROC_ADDRESS("glMapBufferRange");
		glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC) GLH_EXT_GET_PROC_ADDRESS("glFlushMappedBufferRange");
	}
	if (mHasFramebufferObject)
	{
		LL_INFOS() << "initExtensions() FramebufferObject-related procs..." << LL_ENDL;
		glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) GLH_EXT_GET_PROC_ADDRESS("glIsRenderbuffer");
		glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) GLH_EXT_GET_PROC_ADDRESS("glBindRenderbuffer");
		glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) GLH_EXT_GET_PROC_ADDRESS("glDeleteRenderbuffers");
		glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) GLH_EXT_GET_PROC_ADDRESS("glGenRenderbuffers");
		glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) GLH_EXT_GET_PROC_ADDRESS("glRenderbufferStorage");
		glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) GLH_EXT_GET_PROC_ADDRESS("glGetRenderbufferParameteriv");
		glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) GLH_EXT_GET_PROC_ADDRESS("glIsFramebuffer");
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) GLH_EXT_GET_PROC_ADDRESS("glBindFramebuffer");
		glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) GLH_EXT_GET_PROC_ADDRESS("glDeleteFramebuffers");
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) GLH_EXT_GET_PROC_ADDRESS("glGenFramebuffers");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) GLH_EXT_GET_PROC_ADDRESS("glCheckFramebufferStatus");
		glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) GLH_EXT_GET_PROC_ADDRESS("glFramebufferTexture1D");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) GLH_EXT_GET_PROC_ADDRESS("glFramebufferTexture2D");
		glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) GLH_EXT_GET_PROC_ADDRESS("glFramebufferTexture3D");
		glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) GLH_EXT_GET_PROC_ADDRESS("glFramebufferRenderbuffer");
		glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) GLH_EXT_GET_PROC_ADDRESS("glGetFramebufferAttachmentParameteriv");
		glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) GLH_EXT_GET_PROC_ADDRESS("glGenerateMipmap");
		glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC) GLH_EXT_GET_PROC_ADDRESS("glBlitFramebuffer");
		glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) GLH_EXT_GET_PROC_ADDRESS("glRenderbufferStorageMultisample");
		glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) GLH_EXT_GET_PROC_ADDRESS("glFramebufferTextureLayer");
	}
	if (mHasDrawBuffers)
	{
		glDrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDrawBuffersARB");
	}
	if (mHasBlendFuncSeparate)
	{
		glBlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC) GLH_EXT_GET_PROC_ADDRESS("glBlendFuncSeparateEXT");
	}
	if (mHasTextureMultisample)
	{
		glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC) GLH_EXT_GET_PROC_ADDRESS("glTexImage2DMultisample");
		glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC) GLH_EXT_GET_PROC_ADDRESS("glTexImage3DMultisample");
		glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC) GLH_EXT_GET_PROC_ADDRESS("glGetMultisamplefv");
		glSampleMaski = (PFNGLSAMPLEMASKIPROC) GLH_EXT_GET_PROC_ADDRESS("glSampleMaski");
	}
	if (mHasTransformFeedback)
	{
		glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC) GLH_EXT_GET_PROC_ADDRESS("glBeginTransformFeedback");
		glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC) GLH_EXT_GET_PROC_ADDRESS("glEndTransformFeedback");
		glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC) GLH_EXT_GET_PROC_ADDRESS("glTransformFeedbackVaryings");
		glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC) GLH_EXT_GET_PROC_ADDRESS("glBindBufferRange");
		glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) GLH_EXT_GET_PROC_ADDRESS("glBindBufferBase");
	}
	if (mHasDebugOutput)
	{
		glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDebugMessageControlARB");
		glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDebugMessageInsertARB");
		glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDebugMessageCallbackARB");
		glGetDebugMessageLogARB = (PFNGLGETDEBUGMESSAGELOGARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetDebugMessageLogARB");
	}
#if (!LL_LINUX) || LL_LINUX_NV_GL_HEADERS
	// This is expected to be a static symbol on Linux GL implementations, except if we use the nvidia headers - bah
	glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawRangeElements");
	if (!glDrawRangeElements)
	{
		mGLMaxVertexRange = 0;
		mGLMaxIndexRange = 0;
	}
#endif // !LL_LINUX || LL_LINUX_NV_GL_HEADERS
#if LL_LINUX_NV_GL_HEADERS
	// nvidia headers are critically different from mesa-esque
 	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)GLH_EXT_GET_PROC_ADDRESS("glActiveTextureARB");
 	glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)GLH_EXT_GET_PROC_ADDRESS("glClientActiveTextureARB");
#endif // LL_LINUX_NV_GL_HEADERS

	if (mHasOcclusionQuery)
	{
		LL_INFOS() << "initExtensions() OcclusionQuery-related procs..." << LL_ENDL;
		glGenQueriesARB = (PFNGLGENQUERIESARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGenQueriesARB");
		glDeleteQueriesARB = (PFNGLDELETEQUERIESARBPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteQueriesARB");
		glIsQueryARB = (PFNGLISQUERYARBPROC)GLH_EXT_GET_PROC_ADDRESS("glIsQueryARB");
		glBeginQueryARB = (PFNGLBEGINQUERYARBPROC)GLH_EXT_GET_PROC_ADDRESS("glBeginQueryARB");
		glEndQueryARB = (PFNGLENDQUERYARBPROC)GLH_EXT_GET_PROC_ADDRESS("glEndQueryARB");
		glGetQueryivARB = (PFNGLGETQUERYIVARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryivARB");
		glGetQueryObjectivARB = (PFNGLGETQUERYOBJECTIVARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjectivARB");
		glGetQueryObjectuivARB = (PFNGLGETQUERYOBJECTUIVARBPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjectuivARB");
	}
	if (mHasTimerQuery)
	{
		LL_INFOS() << "initExtensions() TimerQuery-related procs..." << LL_ENDL;
		glQueryCounter = (PFNGLQUERYCOUNTERPROC) GLH_EXT_GET_PROC_ADDRESS("glQueryCounter");
		glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC) GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjecti64v");
		glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC) GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjectui64v");
	}
	if (mHasPointParameters)
	{
		LL_INFOS() << "initExtensions() PointParameters-related procs..." << LL_ENDL;
		glPointParameterfARB = (PFNGLPOINTPARAMETERFARBPROC)GLH_EXT_GET_PROC_ADDRESS("glPointParameterfARB");
		glPointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)GLH_EXT_GET_PROC_ADDRESS("glPointParameterfvARB");
	}

    // Assume shader capabilities
    glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDeleteObjectARB");
    glGetHandleARB            = (PFNGLGETHANDLEARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetHandleARB");
    glDetachObjectARB         = (PFNGLDETACHOBJECTARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDetachObjectARB");
    glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC) GLH_EXT_GET_PROC_ADDRESS("glCreateShaderObjectARB");
    glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC) GLH_EXT_GET_PROC_ADDRESS("glShaderSourceARB");
    glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC) GLH_EXT_GET_PROC_ADDRESS("glCompileShaderARB");
    glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC) GLH_EXT_GET_PROC_ADDRESS("glCreateProgramObjectARB");
    glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC) GLH_EXT_GET_PROC_ADDRESS("glAttachObjectARB");
    glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC) GLH_EXT_GET_PROC_ADDRESS("glLinkProgramARB");
    glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUseProgramObjectARB");
    glValidateProgramARB      = (PFNGLVALIDATEPROGRAMARBPROC) GLH_EXT_GET_PROC_ADDRESS("glValidateProgramARB");
    glUniform1fARB            = (PFNGLUNIFORM1FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform1fARB");
    glUniform2fARB            = (PFNGLUNIFORM2FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform2fARB");
    glUniform3fARB            = (PFNGLUNIFORM3FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform3fARB");
    glUniform4fARB            = (PFNGLUNIFORM4FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform4fARB");
    glUniform1iARB            = (PFNGLUNIFORM1IARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform1iARB");
    glUniform2iARB            = (PFNGLUNIFORM2IARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform2iARB");
    glUniform3iARB            = (PFNGLUNIFORM3IARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform3iARB");
    glUniform4iARB            = (PFNGLUNIFORM4IARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform4iARB");
    glUniform1fvARB           = (PFNGLUNIFORM1FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform1fvARB");
    glUniform2fvARB           = (PFNGLUNIFORM2FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform2fvARB");
    glUniform3fvARB           = (PFNGLUNIFORM3FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform3fvARB");
    glUniform4fvARB           = (PFNGLUNIFORM4FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform4fvARB");
    glUniform1ivARB           = (PFNGLUNIFORM1IVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform1ivARB");
    glUniform2ivARB           = (PFNGLUNIFORM2IVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform2ivARB");
    glUniform3ivARB           = (PFNGLUNIFORM3IVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform3ivARB");
    glUniform4ivARB           = (PFNGLUNIFORM4IVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniform4ivARB");
    glUniformMatrix2fvARB     = (PFNGLUNIFORMMATRIX2FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix2fvARB");
    glUniformMatrix3fvARB     = (PFNGLUNIFORMMATRIX3FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3fvARB");
    glUniformMatrix3x4fv      = (PFNGLUNIFORMMATRIX3X4FVPROC) GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3x4fv");
    glUniformMatrix4fvARB     = (PFNGLUNIFORMMATRIX4FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix4fvARB");
    glGetObjectParameterfvARB = (PFNGLGETOBJECTPARAMETERFVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetObjectParameterfvARB");
    glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetObjectParameterivARB");
    glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetInfoLogARB");
    glGetAttachedObjectsARB   = (PFNGLGETATTACHEDOBJECTSARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetAttachedObjectsARB");
    glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetUniformLocationARB");
    glGetActiveUniformARB     = (PFNGLGETACTIVEUNIFORMARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetActiveUniformARB");
    glGetUniformfvARB         = (PFNGLGETUNIFORMFVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetUniformfvARB");
    glGetUniformivARB         = (PFNGLGETUNIFORMIVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetUniformivARB");
    glGetShaderSourceARB      = (PFNGLGETSHADERSOURCEARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetShaderSourceARB");

    LL_INFOS() << "initExtensions() VertexShader-related procs..." << LL_ENDL;

    // nSight doesn't support use of ARB funcs that have been normalized in the API
    if (!LLRender::sNsightDebugSupport)
    {
        glGetAttribLocationARB  = (PFNGLGETATTRIBLOCATIONARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetAttribLocationARB");
        glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC) GLH_EXT_GET_PROC_ADDRESS("glBindAttribLocationARB");
    }
    else
    {
        glGetAttribLocationARB  = (PFNGLGETATTRIBLOCATIONARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetAttribLocation");
        glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC) GLH_EXT_GET_PROC_ADDRESS("glBindAttribLocation");
    }

    glGetActiveAttribARB            = (PFNGLGETACTIVEATTRIBARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetActiveAttribARB");
    glVertexAttrib1dARB             = (PFNGLVERTEXATTRIB1DARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1dARB");
    glVertexAttrib1dvARB            = (PFNGLVERTEXATTRIB1DVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1dvARB");
    glVertexAttrib1fARB             = (PFNGLVERTEXATTRIB1FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1fARB");
    glVertexAttrib1fvARB            = (PFNGLVERTEXATTRIB1FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1fvARB");
    glVertexAttrib1sARB             = (PFNGLVERTEXATTRIB1SARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1sARB");
    glVertexAttrib1svARB            = (PFNGLVERTEXATTRIB1SVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1svARB");
    glVertexAttrib2dARB             = (PFNGLVERTEXATTRIB2DARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2dARB");
    glVertexAttrib2dvARB            = (PFNGLVERTEXATTRIB2DVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2dvARB");
    glVertexAttrib2fARB             = (PFNGLVERTEXATTRIB2FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2fARB");
    glVertexAttrib2fvARB            = (PFNGLVERTEXATTRIB2FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2fvARB");
    glVertexAttrib2sARB             = (PFNGLVERTEXATTRIB2SARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2sARB");
    glVertexAttrib2svARB            = (PFNGLVERTEXATTRIB2SVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2svARB");
    glVertexAttrib3dARB             = (PFNGLVERTEXATTRIB3DARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3dARB");
    glVertexAttrib3dvARB            = (PFNGLVERTEXATTRIB3DVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3dvARB");
    glVertexAttrib3fARB             = (PFNGLVERTEXATTRIB3FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3fARB");
    glVertexAttrib3fvARB            = (PFNGLVERTEXATTRIB3FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3fvARB");
    glVertexAttrib3sARB             = (PFNGLVERTEXATTRIB3SARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3sARB");
    glVertexAttrib3svARB            = (PFNGLVERTEXATTRIB3SVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3svARB");
    glVertexAttrib4nbvARB           = (PFNGLVERTEXATTRIB4NBVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4nbvARB");
    glVertexAttrib4nivARB           = (PFNGLVERTEXATTRIB4NIVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4nivARB");
    glVertexAttrib4nsvARB           = (PFNGLVERTEXATTRIB4NSVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4nsvARB");
    glVertexAttrib4nubARB           = (PFNGLVERTEXATTRIB4NUBARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4nubARB");
    glVertexAttrib4nubvARB          = (PFNGLVERTEXATTRIB4NUBVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4nubvARB");
    glVertexAttrib4nuivARB          = (PFNGLVERTEXATTRIB4NUIVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4nuivARB");
    glVertexAttrib4nusvARB          = (PFNGLVERTEXATTRIB4NUSVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4nusvARB");
    glVertexAttrib4bvARB            = (PFNGLVERTEXATTRIB4BVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4bvARB");
    glVertexAttrib4dARB             = (PFNGLVERTEXATTRIB4DARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4dARB");
    glVertexAttrib4dvARB            = (PFNGLVERTEXATTRIB4DVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4dvARB");
    glVertexAttrib4fARB             = (PFNGLVERTEXATTRIB4FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4fARB");
    glVertexAttrib4fvARB            = (PFNGLVERTEXATTRIB4FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4fvARB");
    glVertexAttrib4ivARB            = (PFNGLVERTEXATTRIB4IVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4ivARB");
    glVertexAttrib4sARB             = (PFNGLVERTEXATTRIB4SARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4sARB");
    glVertexAttrib4svARB            = (PFNGLVERTEXATTRIB4SVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4svARB");
    glVertexAttrib4ubvARB           = (PFNGLVERTEXATTRIB4UBVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4ubvARB");
    glVertexAttrib4uivARB           = (PFNGLVERTEXATTRIB4UIVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4uivARB");
    glVertexAttrib4usvARB           = (PFNGLVERTEXATTRIB4USVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4usvARB");
    glVertexAttribPointerARB        = (PFNGLVERTEXATTRIBPOINTERARBPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttribPointerARB");
    glVertexAttribIPointer          = (PFNGLVERTEXATTRIBIPOINTERPROC) GLH_EXT_GET_PROC_ADDRESS("glVertexAttribIPointer");
    glEnableVertexAttribArrayARB    = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC) GLH_EXT_GET_PROC_ADDRESS("glEnableVertexAttribArrayARB");
    glDisableVertexAttribArrayARB   = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDisableVertexAttribArrayARB");
    glProgramStringARB              = (PFNGLPROGRAMSTRINGARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramStringARB");
    glBindProgramARB                = (PFNGLBINDPROGRAMARBPROC) GLH_EXT_GET_PROC_ADDRESS("glBindProgramARB");
    glDeleteProgramsARB             = (PFNGLDELETEPROGRAMSARBPROC) GLH_EXT_GET_PROC_ADDRESS("glDeleteProgramsARB");
    glGenProgramsARB                = (PFNGLGENPROGRAMSARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGenProgramsARB");
    glProgramEnvParameter4dARB      = (PFNGLPROGRAMENVPARAMETER4DARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramEnvParameter4dARB");
    glProgramEnvParameter4dvARB     = (PFNGLPROGRAMENVPARAMETER4DVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramEnvParameter4dvARB");
    glProgramEnvParameter4fARB      = (PFNGLPROGRAMENVPARAMETER4FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramEnvParameter4fARB");
    glProgramEnvParameter4fvARB     = (PFNGLPROGRAMENVPARAMETER4FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramEnvParameter4fvARB");
    glProgramLocalParameter4dARB    = (PFNGLPROGRAMLOCALPARAMETER4DARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramLocalParameter4dARB");
    glProgramLocalParameter4dvARB   = (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramLocalParameter4dvARB");
    glProgramLocalParameter4fARB    = (PFNGLPROGRAMLOCALPARAMETER4FARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramLocalParameter4fARB");
    glProgramLocalParameter4fvARB   = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glProgramLocalParameter4fvARB");
    glGetProgramEnvParameterdvARB   = (PFNGLGETPROGRAMENVPARAMETERDVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetProgramEnvParameterdvARB");
    glGetProgramEnvParameterfvARB   = (PFNGLGETPROGRAMENVPARAMETERFVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetProgramEnvParameterfvARB");
    glGetProgramLocalParameterdvARB = (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetProgramLocalParameterdvARB");
    glGetProgramLocalParameterfvARB = (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetProgramLocalParameterfvARB");
    glGetProgramivARB               = (PFNGLGETPROGRAMIVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetProgramivARB");
    glGetProgramStringARB           = (PFNGLGETPROGRAMSTRINGARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetProgramStringARB");
    glGetVertexAttribdvARB          = (PFNGLGETVERTEXATTRIBDVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribdvARB");
    glGetVertexAttribfvARB          = (PFNGLGETVERTEXATTRIBFVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribfvARB");
    glGetVertexAttribivARB          = (PFNGLGETVERTEXATTRIBIVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribivARB");
    glGetVertexAttribPointervARB    = (PFNGLGETVERTEXATTRIBPOINTERVARBPROC) GLH_EXT_GET_PROC_ADDRESS("glgetVertexAttribPointervARB");
    glIsProgramARB                  = (PFNGLISPROGRAMARBPROC) GLH_EXT_GET_PROC_ADDRESS("glIsProgramARB");

    LL_DEBUGS("RenderInit") << "GL Probe: Got symbols" << LL_ENDL;
#endif

    mInited = TRUE;
}

void rotate_quat(LLQuaternion& rotation)
{
	F32 angle_radians, x, y, z;
	rotation.getAngleAxis(&angle_radians, &x, &y, &z);
	gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);
}

void flush_glerror()
{
	glGetError();
}

//this function outputs gl error to the log file, does not crash the code.
void log_glerror()
{
	if (LL_UNLIKELY(!gGLManager.mInited))
	{
		return ;
	}
	//  Create or update texture to be used with this data 
	GLenum error;
	error = glGetError();
	while (LL_UNLIKELY(error))
	{
		GLubyte const * gl_error_msg = gluErrorString(error);
		if (NULL != gl_error_msg)
		{
			LL_WARNS() << "GL Error: " << error << " GL Error String: " << gl_error_msg << LL_ENDL ;			
		}
		else
		{
			// gluErrorString returns NULL for some extensions' error codes.
			// you'll probably have to grep for the number in glext.h.
			LL_WARNS() << "GL Error: UNKNOWN 0x" << std::hex << error << std::dec << LL_ENDL;
		}
		error = glGetError();
	}
}

void do_assert_glerror()
{
	//  Create or update texture to be used with this data 
	GLenum error;
	error = glGetError();
	BOOL quit = FALSE;
	while (LL_UNLIKELY(error))
	{
		quit = TRUE;
		GLubyte const * gl_error_msg = gluErrorString(error);
		if (NULL != gl_error_msg)
		{
			LL_WARNS("RenderState") << "GL Error:" << error<< LL_ENDL;
			LL_WARNS("RenderState") << "GL Error String:" << gl_error_msg << LL_ENDL;

			if (gDebugSession)
			{
				gFailLog << "GL Error:" << gl_error_msg << std::endl;
			}
		}
		else
		{
			// gluErrorString returns NULL for some extensions' error codes.
			// you'll probably have to grep for the number in glext.h.
			LL_WARNS("RenderState") << "GL Error: UNKNOWN 0x" << std::hex << error << std::dec << LL_ENDL;

			if (gDebugSession)
			{
				gFailLog << "GL Error: UNKNOWN 0x" << std::hex << error << std::dec << std::endl;
			}
		}
		error = glGetError();
	}

	if (quit)
	{
		if (gDebugSession)
		{
			ll_fail("assert_glerror failed");
		}
		else
		{
			LL_ERRS() << "One or more unhandled GL errors." << LL_ENDL;
		}
	}
}

void assert_glerror()
{
/*	if (!gGLActive)
	{
		//LL_WARNS() << "GL used while not active!" << LL_ENDL;

		if (gDebugSession)
		{
			//ll_fail("GL used while not active");
		}
	}
*/

	if (!gDebugGL) 
	{
		//funny looking if for branch prediction -- gDebugGL is almost always false and assert_glerror is called often
	}
	else
	{
		do_assert_glerror();
	}
}
	

void clear_glerror()
{
	glGetError();
	glGetError();
}

///////////////////////////////////////////////////////////////
//
// LLGLState
//

// Static members
boost::unordered_map<LLGLenum, LLGLboolean> LLGLState::sStateMap;

GLboolean LLGLDepthTest::sDepthEnabled = GL_FALSE; // OpenGL default
GLenum LLGLDepthTest::sDepthFunc = GL_LESS; // OpenGL default
GLboolean LLGLDepthTest::sWriteEnabled = GL_TRUE; // OpenGL default

//static
void LLGLState::initClass() 
{
	sStateMap[GL_DITHER] = GL_TRUE;
	// sStateMap[GL_TEXTURE_2D] = GL_TRUE;
	
	//make sure multisample defaults to disabled
	sStateMap[GL_MULTISAMPLE_ARB] = GL_FALSE;
	glDisable(GL_MULTISAMPLE_ARB);
}

//static
void LLGLState::restoreGL()
{
	sStateMap.clear();
	initClass();
}

//static
// Really shouldn't be needed, but seems we sometimes do.
void LLGLState::resetTextureStates()
{
	gGL.flush();
	GLint maxTextureUnits;
	
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &maxTextureUnits);
	for (S32 j = maxTextureUnits-1; j >=0; j--)
	{
		gGL.getTexUnit(j)->activate();
		glClientActiveTextureARB(GL_TEXTURE0_ARB+j);
		j == 0 ? gGL.getTexUnit(j)->enable(LLTexUnit::TT_TEXTURE) : gGL.getTexUnit(j)->disable();
	}
}

void LLGLState::dumpStates() 
{
	LL_INFOS("RenderState") << "GL States:" << LL_ENDL;
	for (boost::unordered_map<LLGLenum, LLGLboolean>::iterator iter = sStateMap.begin();
		 iter != sStateMap.end(); ++iter)
	{
		LL_INFOS("RenderState") << llformat(" 0x%04x : %s",(S32)iter->first,iter->second?"TRUE":"FALSE") << LL_ENDL;
	}
}

void LLGLState::checkStates(const std::string& msg)  
{
	if (!gDebugGL)
	{
		return;
	}

	stop_glerror();

	GLint src;
	GLint dst;
	glGetIntegerv(GL_BLEND_SRC, &src);
	glGetIntegerv(GL_BLEND_DST, &dst);
	
	stop_glerror();

	BOOL error = FALSE;

	if (src != GL_SRC_ALPHA || dst != GL_ONE_MINUS_SRC_ALPHA)
	{
		if (gDebugSession)
		{
			gFailLog << "Blend function corrupted: " << std::hex << src << " " << std::hex << dst << "  " << msg << std::dec << std::endl;
			error = TRUE;
		}
		else
		{
			LL_GL_ERRS << "Blend function corrupted: " << std::hex << src << " " << std::hex << dst << "  " << msg << std::dec << LL_ENDL;
		}
	}
	
	for (boost::unordered_map<LLGLenum, LLGLboolean>::iterator iter = sStateMap.begin();
		 iter != sStateMap.end(); ++iter)
	{
		LLGLenum state = iter->first;
		LLGLboolean cur_state = iter->second;
		stop_glerror();
		LLGLboolean gl_state = glIsEnabled(state);
		stop_glerror();
		if(cur_state != gl_state)
		{
			dumpStates();
			if (gDebugSession)
			{
				gFailLog << llformat("LLGLState error. State: 0x%04x",state) << std::endl;
				error = TRUE;
			}
			else
			{
				LL_GL_ERRS << llformat("LLGLState error. State: 0x%04x",state) << LL_ENDL;
			}
		}
	}
	
	if (error)
	{
		ll_fail("LLGLState::checkStates failed.");
	}
	stop_glerror();
}

void LLGLState::checkTextureChannels(const std::string& msg)
{
#if 0
	if (!gDebugGL)
	{
		return;
	}
	stop_glerror();

	GLint activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &activeTexture);
	stop_glerror();

	BOOL error = FALSE;

	if (activeTexture == GL_TEXTURE0_ARB)
	{
		GLint tex_env_mode = 0;

		glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &tex_env_mode);
		stop_glerror();

		if (tex_env_mode != GL_MODULATE)
		{
			error = TRUE;
			LL_WARNS("RenderState") << "GL_TEXTURE_ENV_MODE invalid: " << std::hex << tex_env_mode << std::dec << LL_ENDL;
			if (gDebugSession)
			{
				gFailLog << "GL_TEXTURE_ENV_MODE invalid: " << std::hex << tex_env_mode << std::dec << std::endl;
			}
		}
	}

	static const char* label[] =
	{
		"GL_TEXTURE_2D",
		"GL_TEXTURE_COORD_ARRAY",
		"GL_TEXTURE_1D",
		"GL_TEXTURE_CUBE_MAP_ARB",
		"GL_TEXTURE_GEN_S",
		"GL_TEXTURE_GEN_T",
		"GL_TEXTURE_GEN_Q",
		"GL_TEXTURE_GEN_R",
		"GL_TEXTURE_RECTANGLE_ARB",
		"GL_TEXTURE_2D_MULTISAMPLE"
	};

	static GLint value[] =
	{
		GL_TEXTURE_2D,
		GL_TEXTURE_COORD_ARRAY,
		GL_TEXTURE_1D,
		GL_TEXTURE_CUBE_MAP_ARB,
		GL_TEXTURE_GEN_S,
		GL_TEXTURE_GEN_T,
		GL_TEXTURE_GEN_Q,
		GL_TEXTURE_GEN_R,
		GL_TEXTURE_RECTANGLE_ARB,
		GL_TEXTURE_2D_MULTISAMPLE
	};

	GLint stackDepth = 0;

	glh::matrix4f mat;
	glh::matrix4f identity;
	identity.identity();

	for (GLint i = 1; i < gGLManager.mNumTextureUnits; i++)
	{
		gGL.getTexUnit(i)->activate();

		if (i < gGLManager.mNumTextureUnits)
		{
			glClientActiveTextureARB(GL_TEXTURE0_ARB+i);
			stop_glerror();
			glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &stackDepth);
			stop_glerror();

			if (stackDepth != 1)
			{
				error = TRUE;
				LL_WARNS("RenderState") << "Texture matrix stack corrupted." << LL_ENDL;

				if (gDebugSession)
				{
					gFailLog << "Texture matrix stack corrupted." << std::endl;
				}
			}

			glGetFloatv(GL_TEXTURE_MATRIX, (GLfloat*) mat.m);
			stop_glerror();

			if (mat != identity)
			{
				error = TRUE;
				LL_WARNS("RenderState") << "Texture matrix in channel " << i << " corrupt." << LL_ENDL;
				if (gDebugSession)
				{
					gFailLog << "Texture matrix in channel " << i << " corrupt." << std::endl;
				}
			}
				
			for (S32 j = (i == 0 ? 1 : 0); 
				j < 9; j++)
			{
				if (j == 8 && !gGLManager.mHasTextureRectangle ||
					j == 9 && !gGLManager.mHasTextureMultisample)
				{
					continue;
				}
				
				if (glIsEnabled(value[j]))
				{
					error = TRUE;
					LL_WARNS("RenderState") << "Texture channel " << i << " still has " << label[j] << " enabled." << LL_ENDL;
					if (gDebugSession)
					{
						gFailLog << "Texture channel " << i << " still has " << label[j] << " enabled." << std::endl;
					}
				}
				stop_glerror();
			}

			glGetFloatv(GL_TEXTURE_MATRIX, mat.m);
			stop_glerror();

			if (mat != identity)
			{
				error = TRUE;
				LL_WARNS("RenderState") << "Texture matrix " << i << " is not identity." << LL_ENDL;
				if (gDebugSession)
				{
					gFailLog << "Texture matrix " << i << " is not identity." << std::endl;
				}
			}
		}

		{
			GLint tex = 0;
			stop_glerror();
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
			stop_glerror();

			if (tex != 0)
			{
				error = TRUE;
				LL_WARNS("RenderState") << "Texture channel " << i << " still has texture " << tex << " bound." << LL_ENDL;

				if (gDebugSession)
				{
					gFailLog << "Texture channel " << i << " still has texture " << tex << " bound." << std::endl;
				}
			}
		}
	}

	stop_glerror();
	gGL.getTexUnit(0)->activate();
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	stop_glerror();

	if (error)
	{
		if (gDebugSession)
		{
			ll_fail("LLGLState::checkTextureChannels failed.");
		}
		else
		{
			LL_GL_ERRS << "GL texture state corruption detected.  " << msg << LL_ENDL;
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////

LLGLState::LLGLState(LLGLenum state, S32 enabled) :
	mState(state), mWasEnabled(FALSE), mIsEnabled(FALSE)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	switch (state)
	{
		case GL_ALPHA_TEST:
		case GL_NORMALIZE:
		case GL_TEXTURE_GEN_R:
		case GL_TEXTURE_GEN_S:
		case GL_TEXTURE_GEN_T:
		case GL_TEXTURE_GEN_Q:
		case GL_LIGHTING:
		case GL_COLOR_MATERIAL:
		case GL_FOG:
		case GL_LINE_STIPPLE:
		case GL_POLYGON_STIPPLE:
			mState = 0;
			break;
	}


	stop_glerror();
	if (mState)
	{
		mWasEnabled = sStateMap[state];
        // we can't actually assert on this as queued changes to state are not reflected by glIsEnabled
		//llassert(mWasEnabled == glIsEnabled(state));
		setEnabled(enabled);
		stop_glerror();
	}
}

void LLGLState::setEnabled(S32 enabled)
{
	if (!mState)
	{
		return;
	}
	if (enabled == CURRENT_STATE)
	{
		enabled = sStateMap[mState] == GL_TRUE ? TRUE : FALSE;
	}
	else if (enabled == TRUE && sStateMap[mState] != GL_TRUE)
	{
		gGL.flush();
		glEnable(mState);
		sStateMap[mState] = GL_TRUE;
	}
	else if (enabled == FALSE && sStateMap[mState] != GL_FALSE)
	{
		gGL.flush();
		glDisable(mState);
		sStateMap[mState] = GL_FALSE;
	}
	mIsEnabled = enabled;
}

LLGLState::~LLGLState() 
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	stop_glerror();
	if (mState)
	{
		if (gDebugGL)
		{
			if (!gDebugSession)
			{
				llassert_always(sStateMap[mState] == glIsEnabled(mState));
			}
			else
			{
				if (sStateMap[mState] != glIsEnabled(mState))
				{
					ll_fail("GL enabled state does not match expected");
				}
			}
		}

		if (mIsEnabled != mWasEnabled)
		{
			gGL.flush();
			if (mWasEnabled)
			{
				glEnable(mState);
				sStateMap[mState] = GL_TRUE;
			}
			else
			{
				glDisable(mState);
				sStateMap[mState] = GL_FALSE;
			}
		}
	}
	stop_glerror();
}

////////////////////////////////////////////////////////////////////////////////

void LLGLManager::initGLStates()
{
	//gl states moved to classes in llglstates.h
	LLGLState::initClass();
}

////////////////////////////////////////////////////////////////////////////////

void parse_gl_version( S32* major, S32* minor, S32* release, std::string* vendor_specific, std::string* version_string )
{
	// GL_VERSION returns a null-terminated string with the format: 
	// <major>.<minor>[.<release>] [<vendor specific>]

	const char* version = (const char*) glGetString(GL_VERSION);
	*major = 0;
	*minor = 0;
	*release = 0;
	vendor_specific->assign("");

	if( !version )
	{
		return;
	}

	version_string->assign(version);

	std::string ver_copy( version );
	S32 len = (S32)strlen( version );	/* Flawfinder: ignore */
	S32 i = 0;
	S32 start;
	// Find the major version
	start = i;
	for( ; i < len; i++ )
	{
		if( '.' == version[i] )
		{
			break;
		}
	}
	std::string major_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(major_str, *major);

	if( '.' == version[i] )
	{
		i++;
	}

	// Find the minor version
	start = i;
	for( ; i < len; i++ )
	{
		if( ('.' == version[i]) || isspace(version[i]) )
		{
			break;
		}
	}
	std::string minor_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(minor_str, *minor);

	// Find the release number (optional)
	if( '.' == version[i] )
	{
		i++;

		start = i;
		for( ; i < len; i++ )
		{
			if( isspace(version[i]) )
			{
				break;
			}
		}

		std::string release_str = ver_copy.substr(start,i-start);
		LLStringUtil::convertToS32(release_str, *release);
	}

	// Skip over any white space
	while( version[i] && isspace( version[i] ) )
	{
		i++;
	}

	// Copy the vendor-specific string (optional)
	if( version[i] )
	{
		vendor_specific->assign( version + i );
	}
}


void parse_glsl_version(S32& major, S32& minor)
{
	// GL_SHADING_LANGUAGE_VERSION returns a null-terminated string with the format: 
	// <major>.<minor>[.<release>] [<vendor specific>]

	const char* version = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
	major = 0;
	minor = 0;
	
	if( !version )
	{
		return;
	}

	std::string ver_copy( version );
	S32 len = (S32)strlen( version );	/* Flawfinder: ignore */
	S32 i = 0;
	S32 start;
	// Find the major version
	start = i;
	for( ; i < len; i++ )
	{
		if( '.' == version[i] )
		{
			break;
		}
	}
	std::string major_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(major_str, major);

	if( '.' == version[i] )
	{
		i++;
	}

	// Find the minor version
	start = i;
	for( ; i < len; i++ )
	{
		if( ('.' == version[i]) || isspace(version[i]) )
		{
			break;
		}
	}
	std::string minor_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(minor_str, minor);
}

LLGLUserClipPlane::LLGLUserClipPlane(const LLPlane& p, const glh::matrix4f& modelview, const glh::matrix4f& projection, bool apply)
{
	mApply = apply;

	if (mApply)
	{
		mModelview = modelview;
		mProjection = projection;

        //flip incoming LLPlane to get consistent behavior compared to frustum culling
		setPlane(-p[0], -p[1], -p[2], -p[3]);
	}
}

void LLGLUserClipPlane::disable()
{
    if (mApply)
	{
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.popMatrix();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
    mApply = false;
}

void LLGLUserClipPlane::setPlane(F32 a, F32 b, F32 c, F32 d)
{
	glh::matrix4f& P = mProjection;
	glh::matrix4f& M = mModelview;
    
	glh::matrix4f invtrans_MVP = (P * M).inverse().transpose();
    glh::vec4f oplane(a,b,c,d);
    glh::vec4f cplane;
    invtrans_MVP.mult_matrix_vec(oplane, cplane);

    cplane /= fabs(cplane[2]); // normalize such that depth is not scaled
    cplane[3] -= 1;

    if(cplane[2] < 0)
        cplane *= -1;

    glh::matrix4f suffix;
    suffix.set_row(2, cplane);
    glh::matrix4f newP = suffix * P;
    gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
    gGL.loadMatrix(newP.m);
	gGLObliqueProjectionInverse = LLMatrix4(newP.inverse().transpose().m);
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}

LLGLUserClipPlane::~LLGLUserClipPlane()
{
	disable();
}

LLGLDepthTest::LLGLDepthTest(GLboolean depth_enabled, GLboolean write_enabled, GLenum depth_func)
: mPrevDepthEnabled(sDepthEnabled), mPrevDepthFunc(sDepthFunc), mPrevWriteEnabled(sWriteEnabled)
{
	stop_glerror();
	
	checkState();

	if (!depth_enabled)
	{ // always disable depth writes if depth testing is disabled
	  // GL spec defines this as a requirement, but some implementations allow depth writes with testing disabled
	  // The proper way to write to depth buffer with testing disabled is to enable testing and use a depth_func of GL_ALWAYS
		write_enabled = FALSE;
	}

	if (depth_enabled != sDepthEnabled)
	{
		gGL.flush();
		if (depth_enabled) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
		sDepthEnabled = depth_enabled;
	}
	if (depth_func != sDepthFunc)
	{
		gGL.flush();
		glDepthFunc(depth_func);
		sDepthFunc = depth_func;
	}
	if (write_enabled != sWriteEnabled)
	{
		gGL.flush();
		glDepthMask(write_enabled);
		sWriteEnabled = write_enabled;
	}
}

LLGLDepthTest::~LLGLDepthTest()
{
	checkState();
	if (sDepthEnabled != mPrevDepthEnabled )
	{
		gGL.flush();
		if (mPrevDepthEnabled) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
		sDepthEnabled = mPrevDepthEnabled;
	}
	if (sDepthFunc != mPrevDepthFunc)
	{
		gGL.flush();
		glDepthFunc(mPrevDepthFunc);
		sDepthFunc = mPrevDepthFunc;
	}
	if (sWriteEnabled != mPrevWriteEnabled )
	{
		gGL.flush();
		glDepthMask(mPrevWriteEnabled);
		sWriteEnabled = mPrevWriteEnabled;
	}
}

void LLGLDepthTest::checkState()
{
	if (gDebugGL)
	{
		GLint func = 0;
		GLboolean mask = FALSE;

		glGetIntegerv(GL_DEPTH_FUNC, &func);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);

		if (glIsEnabled(GL_DEPTH_TEST) != sDepthEnabled ||
			sWriteEnabled != mask ||
			sDepthFunc != func)
		{
			if (gDebugSession)
			{
				gFailLog << "Unexpected depth testing state." << std::endl;
			}
			else
			{
				LL_GL_ERRS << "Unexpected depth testing state." << LL_ENDL;
			}
		}
	}
}

LLGLSquashToFarClip::LLGLSquashToFarClip()
{
    glh::matrix4f proj = get_current_projection();
    setProjectionMatrix(proj, 0);
}

LLGLSquashToFarClip::LLGLSquashToFarClip(glh::matrix4f& P, U32 layer)
{
    setProjectionMatrix(P, layer);
}


void LLGLSquashToFarClip::setProjectionMatrix(glh::matrix4f& projection, U32 layer)
{

	F32 depth = 0.99999f - 0.0001f * layer;

	for (U32 i = 0; i < 4; i++)
	{
		projection.element(2, i) = projection.element(3, i) * depth;
	}

    LLRender::eMatrixMode last_matrix_mode = gGL.getMatrixMode();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadMatrix(projection.m);

	gGL.matrixMode(last_matrix_mode);
}

LLGLSquashToFarClip::~LLGLSquashToFarClip()
{
    LLRender::eMatrixMode last_matrix_mode = gGL.getMatrixMode();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();

	gGL.matrixMode(last_matrix_mode);
}


	
LLGLSyncFence::LLGLSyncFence()
{
#ifdef GL_ARB_sync
	mSync = 0;
#endif
}

LLGLSyncFence::~LLGLSyncFence()
{
#ifdef GL_ARB_sync
	if (mSync)
	{
		glDeleteSync(mSync);
	}
#endif
}

void LLGLSyncFence::placeFence()
{
#ifdef GL_ARB_sync
	if (mSync)
	{
		glDeleteSync(mSync);
	}
	mSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#endif
}

bool LLGLSyncFence::isCompleted()
{
	bool ret = true;
#ifdef GL_ARB_sync
	if (mSync)
	{
		GLenum status = glClientWaitSync(mSync, 0, 1);
		if (status == GL_TIMEOUT_EXPIRED)
		{
			ret = false;
		}
	}
#endif
	return ret;
}

void LLGLSyncFence::wait()
{
#ifdef GL_ARB_sync
	if (mSync)
	{
		while (glClientWaitSync(mSync, 0, FENCE_WAIT_TIME_NANOSECONDS) == GL_TIMEOUT_EXPIRED)
		{
		}
	}
#endif
}

LLGLSPipelineSkyBox::LLGLSPipelineSkyBox()
: mAlphaTest(GL_ALPHA_TEST)
, mCullFace(GL_CULL_FACE)
, mSquashClip()
{ 
}

LLGLSPipelineSkyBox::~LLGLSPipelineSkyBox()
{
}

LLGLSPipelineDepthTestSkyBox::LLGLSPipelineDepthTestSkyBox(bool depth_test, bool depth_write)
: LLGLSPipelineSkyBox()
, mDepth(depth_test ? GL_TRUE : GL_FALSE, depth_write ? GL_TRUE : GL_FALSE, GL_LEQUAL)
{

}

LLGLSPipelineBlendSkyBox::LLGLSPipelineBlendSkyBox(bool depth_test, bool depth_write)
: LLGLSPipelineDepthTestSkyBox(depth_test, depth_write)    
, mBlend(GL_BLEND)
{ 
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

#if LL_WINDOWS
// Expose desired use of high-performance graphics processor to Optimus driver and to AMD driver
// https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
extern "C" 
{ 
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


