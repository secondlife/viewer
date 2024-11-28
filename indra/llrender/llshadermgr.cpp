/**
 * @file llshadermgr.cpp
 * @brief Shader manager implementation.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "llshadermgr.h"
#include "llrender.h"
#include "llfile.h"
#include "lldir.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "hbxxh.h"

#if LL_DARWIN
#include "OpenGL/OpenGL.h"
#endif

// Lots of STL stuff in here, using namespace std to keep things more readable
using std::vector;
using std::pair;
using std::make_pair;
using std::string;

LLShaderMgr * LLShaderMgr::sInstance = NULL;

LLShaderMgr::LLShaderMgr()
{
}


LLShaderMgr::~LLShaderMgr()
{
}

// static
LLShaderMgr * LLShaderMgr::instance()
{
    if(NULL == sInstance)
    {
        LL_ERRS("Shaders") << "LLShaderMgr should already have been instantiated by the application!" << LL_ENDL;
    }

    return sInstance;
}

bool LLShaderMgr::attachShaderFeatures(LLGLSLShader * shader)
{
    llassert_always(shader != NULL);
    LLShaderFeatures *features = & shader->mFeatures;

    if (features->attachNothing)
    {
        return true;
    }
    //////////////////////////////////////
    // Attach Vertex Shader Features First
    //////////////////////////////////////

    // NOTE order of shader object attaching is VERY IMPORTANT!!!
    if (features->calculatesAtmospherics)
    {
        if (!shader->attachVertexObject("windlight/atmosphericsVarsV.glsl"))
        {
            return false;
        }
    }

    if (features->calculatesLighting || features->calculatesAtmospherics)
    {
        if (!shader->attachVertexObject("windlight/atmosphericsHelpersV.glsl"))
        {
            return false;
        }
    }

    if (features->calculatesLighting)
    {
        if (features->isSpecular)
        {
            if (!shader->attachVertexObject("lighting/lightFuncSpecularV.glsl"))
            {
                return false;
            }

            if (!features->isAlphaLighting)
            {
                if (!shader->attachVertexObject("lighting/sumLightsSpecularV.glsl"))
                {
                    return false;
                }
            }

            if (!shader->attachVertexObject("lighting/lightSpecularV.glsl"))
            {
                return false;
            }
        }
        else
        {
            if (!shader->attachVertexObject("lighting/lightFuncV.glsl"))
            {
                return false;
            }

            if (!features->isAlphaLighting)
            {
                if (!shader->attachVertexObject("lighting/sumLightsV.glsl"))
                {
                    return false;
                }
            }

            if (!shader->attachVertexObject("lighting/lightV.glsl"))
            {
                return false;
            }
        }
    }

    // NOTE order of shader object attaching is VERY IMPORTANT!!!
    if (features->calculatesAtmospherics)
    {
        if (!shader->attachVertexObject("environment/srgbF.glsl")) // NOTE -- "F" suffix is superfluous here, there is nothing fragment specific in srgbF
        {
            return false;
        }

        if (!shader->attachVertexObject("windlight/atmosphericsFuncs.glsl")) {
            return false;
        }

        if (!shader->attachVertexObject("windlight/atmosphericsV.glsl"))
        {
            return false;
        }
    }

    if (features->hasSkinning)
    {
        if (!shader->attachVertexObject("avatar/avatarSkinV.glsl"))
        {
            return false;
        }
    }

    if (features->hasObjectSkinning)
    {
        shader->mRiggedVariant = shader;
        if (!shader->attachVertexObject("avatar/objectSkinV.glsl"))
        {
            return false;
        }
    }

    if (!shader->attachVertexObject("deferred/textureUtilV.glsl"))
    {
        return false;
    }

    ///////////////////////////////////////
    // Attach Fragment Shader Features Next
    ///////////////////////////////////////

    // NOTE order of shader object attaching is VERY IMPORTANT!!!

    if (!shader->attachFragmentObject("deferred/globalF.glsl"))
    {
        return false;
    }

    if (features->hasSrgb || features->hasAtmospherics || features->calculatesAtmospherics || features->isDeferred)
    {
        if (!shader->attachFragmentObject("environment/srgbF.glsl"))
        {
            return false;
        }
    }

    if(features->calculatesAtmospherics || features->hasGamma || features->isDeferred)
    {
        if (!shader->attachFragmentObject("windlight/atmosphericsVarsF.glsl"))
        {
            return false;
        }
    }

    if (features->calculatesLighting || features->calculatesAtmospherics)
    {
        if (!shader->attachFragmentObject("windlight/atmosphericsHelpersF.glsl"))
        {
            return false;
        }
    }

    // we want this BEFORE shadows and AO because those facilities use pos/norm access
    if (features->isDeferred || features->hasReflectionProbes)
    {
        if (!shader->attachFragmentObject("deferred/deferredUtil.glsl"))
        {
            return false;
        }
    }

    if (features->hasFullGBuffer)
    {
        if (!shader->attachFragmentObject("deferred/gbufferUtil.glsl"))
        {
            return false;
        }
    }

    if (features->hasScreenSpaceReflections || features->hasReflectionProbes)
    {
        if (!shader->attachFragmentObject("deferred/screenSpaceReflUtil.glsl"))
        {
            return false;
        }
    }

    if (features->hasShadows)
    {
        if (!shader->attachFragmentObject("deferred/shadowUtil.glsl"))
        {
            return false;
        }
    }

    if (features->hasReflectionProbes)
    {
        if (!shader->attachFragmentObject("deferred/reflectionProbeF.glsl"))
        {
            return false;
        }
    }

    if (features->hasAmbientOcclusion)
    {
        if (!shader->attachFragmentObject("deferred/aoUtil.glsl"))
        {
            return false;
        }
    }

    if (features->hasGamma || features->isDeferred)
    {
        if (!shader->attachFragmentObject("windlight/gammaF.glsl"))
        {
            return false;
        }
    }

    if (features->hasAtmospherics || features->isDeferred)
    {
        if (!shader->attachFragmentObject("windlight/atmosphericsFuncs.glsl")) {
            return false;
        }

        if (!shader->attachFragmentObject("windlight/atmosphericsF.glsl"))
        {
            return false;
        }
    }

    if (features->isPBRTerrain)
    {
        if (!shader->attachFragmentObject("deferred/pbrterrainUtilF.glsl"))
        {
            return false;
        }
    }

    // NOTE order of shader object attaching is VERY IMPORTANT!!!
    if (features->hasAtmospherics)
    {
        if (!shader->attachFragmentObject("environment/waterFogF.glsl"))
        {
            return false;
        }
    }

    if (features->hasLighting)
    {
        if (features->mIndexedTextureChannels <= 1)
        {
            if (features->hasAlphaMask)
            {
                if (!shader->attachFragmentObject("lighting/lightAlphaMaskNonIndexedF.glsl"))
                {
                    return false;
                }
            }
            else
            {
                if (!shader->attachFragmentObject("lighting/lightNonIndexedF.glsl"))
                {
                    return false;
                }
            }
        }
        else
        {
            if (features->hasAlphaMask)
            {
                if (!shader->attachFragmentObject("lighting/lightAlphaMaskF.glsl"))
                {
                    return false;
                }
            }
            else
            {
                if (!shader->attachFragmentObject("lighting/lightF.glsl"))
                {
                    return false;
                }
            }
            shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels, 1);
        }
    }

    if (features->mIndexedTextureChannels <= 1)
    {
        if (!shader->attachVertexObject("objects/nonindexedTextureV.glsl"))
        {
            return false;
        }
    }
    else
    {
        if (!shader->attachVertexObject("objects/indexedTextureV.glsl"))
        {
            return false;
        }
    }

    return true;
}

//============================================================================
// Load Shader

static std::string get_shader_log(GLuint ret)
{
    std::string res;

    //get log length
    GLint length;
    glGetShaderiv(ret, GL_INFO_LOG_LENGTH, &length);
    if (length > 0)
    {
        //the log could be any size, so allocate appropriately
        GLchar* log = new GLchar[length];
        glGetShaderInfoLog(ret, length, &length, log);
        res = std::string((char *)log);
        delete[] log;
    }
    return res;
}

static std::string get_program_log(GLuint ret)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    std::string res;

    //get log length
    GLint length;
    glGetProgramiv(ret, GL_INFO_LOG_LENGTH, &length);
    if (length > 0)
    {
        //the log could be any size, so allocate appropriately
        GLchar* log = new GLchar[length];
        glGetProgramInfoLog(ret, length, &length, log);
        res = std::string((char*)log);
        delete[] log;
    }
    return res;
}

// get the info log for the given object, be it a shader or program object
// NOTE: ret MUST be a shader OR a program object
static std::string get_object_log(GLuint ret)
{
    if (glIsProgram(ret))
    {
        return get_program_log(ret);
    }
    else
    {
        llassert(glIsShader(ret));
        return get_shader_log(ret);
    }
}

//dump shader source for debugging
void LLShaderMgr::dumpShaderSource(U32 shader_code_count, GLchar** shader_code_text)
{
    char num_str[16]; // U32 = max 10 digits

    LL_SHADER_LOADING_WARNS() << "\n";

    for (U32 i = 0; i < shader_code_count; i++)
    {
        snprintf(num_str, sizeof(num_str), "%4d: ", i+1);
        std::string line_number(num_str);
        LL_CONT << line_number << shader_code_text[i];
    }
    LL_CONT << LL_ENDL;
}

void LLShaderMgr::dumpObjectLog(GLuint ret, bool warns, const std::string& filename)
{
    std::string log;
    log = get_object_log(ret);
    std::string fname = filename;
    if (filename.empty())
    {
        fname = "unknown shader file";
    }

    if (log.length() > 0)
    {
        LL_SHADER_LOADING_WARNS() << "Shader loading from " << fname << LL_ENDL;
        LL_SHADER_LOADING_WARNS() << "\n" << log << LL_ENDL;
    }
 }

GLuint LLShaderMgr::loadShaderFile(const std::string& filename, S32 & shader_level, GLenum type, std::map<std::string, std::string>* defines, S32 texture_index_channels)
{

// endsure work-around for missing GLSL funcs gets propogated to feature shader files (e.g. srgbF.glsl)
#if LL_DARWIN
    if (defines)
    {
        (*defines)["OLD_SELECT"] = "1";
    }
#endif

    GLenum error = GL_NO_ERROR;

    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LL_SHADER_LOADING_WARNS() << "GL ERROR entering loadShaderFile(): " << error << " for file: " << filename << LL_ENDL;
    }

    if (filename.empty())
    {
        return 0;
    }


    //read in from file
    LLFILE* file = NULL;

    S32 try_gpu_class = shader_level;
    S32 gpu_class;

    std::string open_file_name;

#if 0  // WIP -- try to come up with a way to fallback to an error shader without needing debug stubs all over the place in the shader tree
    if (shader_level == -1)
    {
        // use "error" fallback
        if (type == GL_VERTEX_SHADER)
        {
            open_file_name = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "shaders/errorV.glsl");
        }
        else
        {
            llassert(type == GL_FRAGMENT_SHADER);  // type must be vertex or fragment shader
            open_file_name = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "shaders/errorF.glsl");
        }

        file = LLFile::fopen(open_file_name, "r");
    }
    else
#endif
    {
        //find the most relevant file
        for (gpu_class = try_gpu_class; gpu_class > 0; gpu_class--)
        {   //search from the current gpu class down to class 1 to find the most relevant shader
            std::stringstream fname;
            fname << getShaderDirPrefix();
            fname << gpu_class << "/" << filename;

            open_file_name = fname.str();

            /*
            Would be awesome, if we didn't have shaders that re-use files
            with different environments to say, add skinning, etc
            can't depend on cached version to have evaluate ifdefs identically...
            if we can define a deterministic hash for the shader based on
            all the inputs, maybe we can save some time here.
            if (mShaderObjects.count(filename) > 0)
            {
                return mShaderObjects[filename];
            }

            */

            LL_DEBUGS("ShaderLoading") << "Looking in " << open_file_name << LL_ENDL;
            file = LLFile::fopen(open_file_name, "r");      /* Flawfinder: ignore */
            if (file)
            {
                LL_DEBUGS("ShaderLoading") << "Loading file: " << open_file_name << " (Want class " << gpu_class << ")" << LL_ENDL;
                break; // done
            }
        }
    }

    if (file == NULL)
    {
        LL_WARNS("ShaderLoading") << "GLSL Shader file not found: " << open_file_name << LL_ENDL;
        return 0;
    }

    //we can't have any lines longer than 1024 characters
    //or any shaders longer than 4096 lines... deal - DaveP
    GLchar buff[1024];
    GLchar *extra_code_text[1024];
    GLchar *shader_code_text[4096 + LL_ARRAY_SIZE(extra_code_text)] = { NULL };
    GLuint extra_code_count = 0, shader_code_count = 0;
    BOOST_STATIC_ASSERT(LL_ARRAY_SIZE(extra_code_text) < LL_ARRAY_SIZE(shader_code_text));


    S32 major_version = gGLManager.mGLSLVersionMajor;
    S32 minor_version = gGLManager.mGLSLVersionMinor;

    if (major_version == 1 && minor_version < 30)
    {
        llassert(false); // GL 3.1 or later required
    }
    else
    {
        if (major_version >= 4)
        {
            //set version to 400 or 420
            if (minor_version >= 20)
            {
                shader_code_text[shader_code_count++] = strdup("#version 420\n");
            }
            else
            {
                shader_code_text[shader_code_count++] = strdup("#version 400\n");
            }
        }
        else if (major_version == 3)
        {
            if (minor_version <= 29)
            {
                // OpenGL 3.2 had GLSL version 1.50.  anything after that the version numbers match.
                // https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#OpenGL_and_GLSL_versions
                shader_code_text[shader_code_count++] = strdup("#version 150\n");
            }
            else
            {
                shader_code_text[shader_code_count++] = strdup("#version 330\n");
            }
        }
        else
        {
            if (type == GL_GEOMETRY_SHADER)
            {
                //set version to 1.50
                shader_code_text[shader_code_count++] = strdup("#version 150\n");
                //some implementations of GLSL 1.30 require integer precision be explicitly declared
                extra_code_text[extra_code_count++] = strdup("precision mediump int;\n");
                extra_code_text[extra_code_count++] = strdup("precision highp float;\n");
            }
            else
            {
                //set version to 1.40
                shader_code_text[shader_code_count++] = strdup("#version 140\n");
                //some implementations of GLSL 1.30 require integer precision be explicitly declared
                extra_code_text[extra_code_count++] = strdup("precision mediump int;\n");
                extra_code_text[extra_code_count++] = strdup("precision highp float;\n");
            }
        }
    }

    if (type == GL_FRAGMENT_SHADER)
    {
        extra_code_text[extra_code_count++] = strdup("#define FRAGMENT_SHADER 1\n");
    }
    else
    {
        extra_code_text[extra_code_count++] = strdup("#define VERTEX_SHADER 1\n");
    }

    // Use alpha float to store bit flags
    // See: C++: addDeferredAttachment(), shader: frag_data[2]
    extra_code_text[extra_code_count++] = strdup("#define GBUFFER_FLAG_SKIP_ATMOS   0.0 \n"); // atmo kill
    extra_code_text[extra_code_count++] = strdup("#define GBUFFER_FLAG_HAS_ATMOS    0.34\n"); // bit 0
    extra_code_text[extra_code_count++] = strdup("#define GBUFFER_FLAG_HAS_PBR      0.67\n"); // bit 1
    extra_code_text[extra_code_count++] = strdup("#define GBUFFER_FLAG_HAS_HDRI      1.0\n");  // bit 2
    extra_code_text[extra_code_count++] = strdup("#define GET_GBUFFER_FLAG(data, flag)    (abs(data-flag)< 0.1)\n");

    if (defines)
    {
        for (auto iter = defines->begin(); iter != defines->end(); ++iter)
        {
            std::string define = "#define " + iter->first + " " + iter->second + "\n";
            extra_code_text[extra_code_count++] = (GLchar *) strdup(define.c_str());
        }
    }

    if( gGLManager.mIsAMD )
    {
        extra_code_text[extra_code_count++] = strdup( "#define IS_AMD_CARD 1\n" );
    }

    if (texture_index_channels > 0 && type == GL_FRAGMENT_SHADER)
    {
        //use specified number of texture channels for indexed texture rendering

        /* prepend shader code that looks like this:

        uniform sampler2D tex0;
        uniform sampler2D tex1;
        uniform sampler2D tex2;
        .
        .
        .
        uniform sampler2D texN;

        flat in int vary_texture_index;

        vec4 ret = vec4(1,0,1,1);

        vec4 diffuseLookup(vec2 texcoord)
        {
            switch (vary_texture_index)
            {
                case 0: ret = texture(tex0, texcoord); break;
                case 1: ret = texture(tex1, texcoord); break;
                case 2: ret = texture(tex2, texcoord); break;
                .
                .
                .
                case N: return texture(texN, texcoord); break;
            }

            return ret;
        }
        */

        extra_code_text[extra_code_count++] = strdup("#define HAS_DIFFUSE_LOOKUP\n");

        //uniform declartion
        for (S32 i = 0; i < texture_index_channels; ++i)
        {
            std::string decl = llformat("uniform sampler2D tex%d;\n", i);
            extra_code_text[extra_code_count++] = strdup(decl.c_str());
        }

        if (texture_index_channels > 1)
        {
            extra_code_text[extra_code_count++] = strdup("flat in int vary_texture_index;\n");
        }

        extra_code_text[extra_code_count++] = strdup("vec4 diffuseLookup(vec2 texcoord)\n");
        extra_code_text[extra_code_count++] = strdup("{\n");


        if (texture_index_channels == 1)
        { //don't use flow control, that's silly
            extra_code_text[extra_code_count++] = strdup("return texture(tex0, texcoord);\n");
            extra_code_text[extra_code_count++] = strdup("}\n");
        }
        else if (major_version > 1 || minor_version >= 30)
        {  //switches are supported in GLSL 1.30 and later
            if (gGLManager.mIsNVIDIA)
            { //switches are unreliable on some NVIDIA drivers
                for (S32 i = 0; i < texture_index_channels; ++i)
                {
                    std::string if_string = llformat("\t%sif (vary_texture_index == %d) { return texture(tex%d, texcoord); }\n", i > 0 ? "else " : "", i, i);
                    extra_code_text[extra_code_count++] = strdup(if_string.c_str());
                }
                extra_code_text[extra_code_count++] = strdup("\treturn vec4(1,0,1,1);\n");
                extra_code_text[extra_code_count++] = strdup("}\n");
            }
            else
            {
                extra_code_text[extra_code_count++] = strdup("\tvec4 ret = vec4(1,0,1,1);\n");
                extra_code_text[extra_code_count++] = strdup("\tswitch (vary_texture_index)\n");
                extra_code_text[extra_code_count++] = strdup("\t{\n");

                //switch body
                for (S32 i = 0; i < texture_index_channels; ++i)
                {
                    std::string case_str = llformat("\t\tcase %d: return texture(tex%d, texcoord);\n", i, i);
                    extra_code_text[extra_code_count++] = strdup(case_str.c_str());
                }

                extra_code_text[extra_code_count++] = strdup("\t}\n");
                extra_code_text[extra_code_count++] = strdup("\treturn ret;\n");
                extra_code_text[extra_code_count++] = strdup("}\n");
            }
        }
        else
        { //should never get here.  Indexed texture rendering requires GLSL 1.30 or later
            // (for passing integers between vertex and fragment shaders)
            LL_ERRS() << "Indexed texture rendering requires GLSL 1.30 or later." << LL_ENDL;
        }
    }

    // Master definition can be found in deferredUtil.glsl
    extra_code_text[extra_code_count++] = strdup("struct GBufferInfo { vec4 albedo; vec4 specular; vec3 normal; vec4 emissive; float gbufferFlag; float envIntensity; };\n");

    //copy file into memory
    enum {
          flag_write_to_out_of_extra_block_area = 0x01
        , flag_extra_block_marker_was_found = 0x02
    };

    unsigned char flags = flag_write_to_out_of_extra_block_area;

    GLuint out_of_extra_block_counter = 0, start_shader_code = shader_code_count, file_lines_count = 0;

#define TOUCH_SHADERS 0

#if TOUCH_SHADERS
    const char* marker = "// touched";
    bool touched = false;
#endif

    while(NULL != fgets((char *)buff, 1024, file)
          && shader_code_count < (LL_ARRAY_SIZE(shader_code_text) - LL_ARRAY_SIZE(extra_code_text)))
    {
        file_lines_count++;

        bool extra_block_area_found = NULL != strstr((const char*)buff, "[EXTRA_CODE_HERE]");

#if TOUCH_SHADERS
        if (NULL != strstr((const char*)buff, marker))
        {
            touched = true;
        }
#endif

        if(extra_block_area_found && !(flag_extra_block_marker_was_found & flags))
        {
            if(!(flag_write_to_out_of_extra_block_area & flags))
            {
                //shift
                for(GLuint to = start_shader_code, from = extra_code_count + start_shader_code;
                    from < shader_code_count; ++to, ++from)
                {
                    shader_code_text[to] = shader_code_text[from];
                }

                shader_code_count -= extra_code_count;
            }

            //copy extra code
            for(GLuint n = 0; n < extra_code_count
                && shader_code_count < (LL_ARRAY_SIZE(shader_code_text) - LL_ARRAY_SIZE(extra_code_text)); ++n)
            {
                shader_code_text[shader_code_count++] = extra_code_text[n];
            }

            extra_code_count = 0;

            flags &= ~flag_write_to_out_of_extra_block_area;
            flags |= flag_extra_block_marker_was_found;
        }
        else
        {
            shader_code_text[shader_code_count] = (GLchar *)strdup((char *)buff);

            if(flag_write_to_out_of_extra_block_area & flags)
            {
                shader_code_text[extra_code_count + start_shader_code + out_of_extra_block_counter]
                    = shader_code_text[shader_code_count];
                out_of_extra_block_counter++;

                if(out_of_extra_block_counter == extra_code_count)
                {
                    shader_code_count += extra_code_count;
                    flags &= ~flag_write_to_out_of_extra_block_area;
                }
            }

            ++shader_code_count;
        }
    } //while

    if(!(flag_extra_block_marker_was_found & flags))
    {
        for(GLuint n = start_shader_code; n < extra_code_count + start_shader_code; ++n)
        {
            shader_code_text[n] = extra_code_text[n - start_shader_code];
        }

        if (file_lines_count < extra_code_count)
        {
            shader_code_count += extra_code_count;
        }

        extra_code_count = 0;
    }

#if TOUCH_SHADERS
    if (!touched)
    {
        fprintf(file, "\n%s\n", marker);
    }
#endif

    fclose(file);

    //create shader object
    GLuint ret = glCreateShader(type);

    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LL_WARNS("ShaderLoading") << "GL ERROR in glCreateShader: " << error << " for file: " << open_file_name << LL_ENDL;
        if (ret)
        {
            glDeleteShader(ret); //no longer need handle
            ret = 0;
        }
    }

    //load source
    if (ret)
    {
        glShaderSource(ret, shader_code_count, (const GLchar**)shader_code_text, NULL);

        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            LL_WARNS("ShaderLoading") << "GL ERROR in glShaderSource: " << error << " for file: " << open_file_name << LL_ENDL;
            glDeleteShader(ret); //no longer need handle
            ret = 0;
        }
    }

    //compile source
    if (ret)
    {
        glCompileShader(ret);

        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            LL_WARNS("ShaderLoading") << "GL ERROR in glCompileShader: " << error << " for file: " << open_file_name << LL_ENDL;
            glDeleteShader(ret); //no longer need handle
            ret = 0;
        }
    }

    if (error == GL_NO_ERROR)
    {
        //check for errors
        GLint success = GL_TRUE;
        glGetShaderiv(ret, GL_COMPILE_STATUS, &success);

        error = glGetError();
        if (error != GL_NO_ERROR || success == GL_FALSE)
        {
            //an error occured, print log
            LL_WARNS("ShaderLoading") << "GLSL Compilation Error:" << LL_ENDL;
            dumpObjectLog(ret, true, open_file_name);
            dumpShaderSource(shader_code_count, shader_code_text);
            glDeleteShader(ret); //no longer need handle
            ret = 0;
        }
    }
    else
    {
        ret = 0;
    }
    stop_glerror();

    //free memory
    for (GLuint i = 0; i < shader_code_count; i++)
    {
        free(shader_code_text[i]);
    }

    //successfully loaded, save results
    if (ret)
    {
        // Add shader file to map
        if (type == GL_VERTEX_SHADER) {
            mVertexShaderObjects[filename] = ret;
        }
        else if (type == GL_FRAGMENT_SHADER) {
            mFragmentShaderObjects[filename] = ret;
        }
        shader_level = try_gpu_class;
    }
    else
    {
        if (shader_level > 1)
        {
            shader_level--;
            return loadShaderFile(filename, shader_level, type, defines, texture_index_channels);
        }
        LL_WARNS("ShaderLoading") << "Failed to load " << filename << LL_ENDL;
    }
    return ret;
}

bool LLShaderMgr::linkProgramObject(GLuint obj, bool suppress_errors)
{
    //check for errors
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_SHADER("glLinkProgram");
        glLinkProgram(obj);
    }

    GLint success = GL_TRUE;

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_SHADER("glsl check link status");
        glGetProgramiv(obj, GL_LINK_STATUS, &success);
        if (!suppress_errors && success == GL_FALSE)
        {
            //an error occured, print log
            LL_SHADER_LOADING_WARNS() << "GLSL Linker Error:" << LL_ENDL;
            dumpObjectLog(obj, true, "linker");
            return success;
        }
    }

    std::string log = get_program_log(obj);
    LLStringUtil::toLower(log);
    if (log.find("software") != std::string::npos)
    {
        LL_SHADER_LOADING_WARNS() << "GLSL Linker: Running in Software:" << LL_ENDL;
        success = GL_FALSE;
        suppress_errors = false;
    }
    return success;
}

bool LLShaderMgr::validateProgramObject(GLuint obj)
{
    //check program validity against current GL
    glValidateProgram(obj);
    GLint success = GL_TRUE;
    glGetProgramiv(obj, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        LL_SHADER_LOADING_WARNS() << "GLSL program not valid: " << LL_ENDL;
        dumpObjectLog(obj);
    }
    else
    {
        dumpObjectLog(obj, false);
    }

    return success;
}

void LLShaderMgr::initShaderCache(bool enabled, const LLUUID& old_cache_version, const LLUUID& current_cache_version)
{
    LL_INFOS() << "Initializing shader cache" << LL_ENDL;

    mShaderCacheEnabled = gGLManager.mGLVersion >= 4.09 && enabled;

    if(!mShaderCacheEnabled || mShaderCacheInitialized)
        return;

    mShaderCacheInitialized = true;

    mShaderCacheDir = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "shader_cache");
    LLFile::mkdir(mShaderCacheDir);

    {
        std::string meta_out_path = gDirUtilp->add(mShaderCacheDir, "shaderdata.llsd");
        if (gDirUtilp->fileExists(meta_out_path))
        {
            LL_INFOS() << "Loading shader cache metadata" << LL_ENDL;

            llifstream instream(meta_out_path);
            LLSD in_data;
            LLSDSerialize::fromNotation(in_data, instream, LLSDSerialize::SIZE_UNLIMITED);
            instream.close();

            if (old_cache_version == current_cache_version)
            {
                for (const auto& data_pair : llsd::inMap(in_data))
                {
                    ProgramBinaryData binary_info = ProgramBinaryData();
                    binary_info.mBinaryFormat = data_pair.second["binary_format"].asInteger();
                    binary_info.mBinaryLength = data_pair.second["binary_size"].asInteger();
                    binary_info.mLastUsedTime = (F32)data_pair.second["last_used"].asReal();
                    mShaderBinaryCache.insert_or_assign(LLUUID(data_pair.first), binary_info);
                }
            }
            else
            {
                LL_INFOS() << "Shader cache version mismatch detected. Purging." << LL_ENDL;
                clearShaderCache();
            }
        }
    }
}

void LLShaderMgr::clearShaderCache()
{
    std::string shader_cache = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "shader_cache");
    LL_INFOS() << "Removing shader cache at " << shader_cache << LL_ENDL;
    const std::string mask = "*";
    gDirUtilp->deleteFilesInDir(shader_cache, mask);
    mShaderBinaryCache.clear();
}

void LLShaderMgr::persistShaderCacheMetadata()
{
    if(!mShaderCacheEnabled) return;

    LL_INFOS() << "Persisting shader cache metadata to disk" << LL_ENDL;

    LLSD out = LLSD::emptyMap();

    static const F32 LRU_TIME = (60.f * 60.f) * 24.f * 7.f; // 14 days
    const F32 current_time = (F32)LLTimer::getTotalSeconds();
    for (auto it = mShaderBinaryCache.begin(); it != mShaderBinaryCache.end();)
    {
        const ProgramBinaryData& shader_metadata = it->second;
        if ((shader_metadata.mLastUsedTime + LRU_TIME) < current_time)
        {
            std::string shader_path = gDirUtilp->add(mShaderCacheDir, it->first.asString() + ".shaderbin");
            LLFile::remove(shader_path);
            it = mShaderBinaryCache.erase(it);
        }
        else
        {
            LLSD data = LLSD::emptyMap();
            data["binary_format"] = LLSD::Integer(shader_metadata.mBinaryFormat);
            data["binary_size"] = LLSD::Integer(shader_metadata.mBinaryLength);
            data["last_used"] = LLSD::Real(shader_metadata.mLastUsedTime);
            out[it->first.asString()] = data;
            ++it;
        }
    }

    std::string meta_out_path = gDirUtilp->add(mShaderCacheDir, "shaderdata.llsd");
    llofstream outstream(meta_out_path);
    LLSDSerialize::toNotation(out, outstream);
    outstream.close();
}

bool LLShaderMgr::loadCachedProgramBinary(LLGLSLShader* shader)
{
    if (!mShaderCacheEnabled) return false;

    glProgramParameteri(shader->mProgramObject, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

    auto binary_iter = mShaderBinaryCache.find(shader->mShaderHash);
    if (binary_iter != mShaderBinaryCache.end())
    {
        std::string in_path = gDirUtilp->add(mShaderCacheDir, shader->mShaderHash.asString() + ".shaderbin");
        auto& shader_info = binary_iter->second;
        if (shader_info.mBinaryLength > 0)
        {
            std::vector<U8> in_data;
            in_data.resize(shader_info.mBinaryLength);

            LLUniqueFile filep = LLFile::fopen(in_path, "rb");
            if (filep)
            {
                size_t result = fread(in_data.data(), sizeof(U8), in_data.size(), filep);
                filep.close();

                if (result == in_data.size())
                {
                    GLenum error = glGetError(); // Clear current error
                    glProgramBinary(shader->mProgramObject, shader_info.mBinaryFormat, in_data.data(), shader_info.mBinaryLength);

                    error = glGetError();
                    GLint success = GL_TRUE;
                    glGetProgramiv(shader->mProgramObject, GL_LINK_STATUS, &success);
                    if (error == GL_NO_ERROR && success == GL_TRUE)
                    {
                        binary_iter->second.mLastUsedTime = (F32)LLTimer::getTotalSeconds();
                        LL_INFOS() << "Loaded cached binary for shader: " << shader->mName << LL_ENDL;
                        return true;
                    }
                }
            }
        }
        //an error occured, normally we would print log but in this case it means the shader needs recompiling.
        LL_INFOS() << "Failed to load cached binary for shader: " << shader->mName << " falling back to compilation" << LL_ENDL;
        LLFile::remove(in_path);
        mShaderBinaryCache.erase(binary_iter);
    }
    return false;
}

bool LLShaderMgr::saveCachedProgramBinary(LLGLSLShader* shader)
{
    if (!mShaderCacheEnabled) return true;

    ProgramBinaryData binary_info = ProgramBinaryData();
    glGetProgramiv(shader->mProgramObject, GL_PROGRAM_BINARY_LENGTH, &binary_info.mBinaryLength);
    if (binary_info.mBinaryLength > 0)
    {
        std::vector<U8> program_binary;
        program_binary.resize(binary_info.mBinaryLength);

        GLenum error = glGetError(); // Clear current error
        glGetProgramBinary(shader->mProgramObject, static_cast<GLsizei>(program_binary.size() * sizeof(U8)), nullptr, &binary_info.mBinaryFormat, program_binary.data());
        error = glGetError();
        if (error == GL_NO_ERROR)
        {
            std::string out_path = gDirUtilp->add(mShaderCacheDir, shader->mShaderHash.asString() + ".shaderbin");
            LLUniqueFile outfile = LLFile::fopen(out_path, "wb");
            if (outfile)
            {
                fwrite(program_binary.data(), sizeof(U8), program_binary.size(), outfile);
                outfile.close();

                binary_info.mLastUsedTime = (F32)LLTimer::getTotalSeconds();

                mShaderBinaryCache.insert_or_assign(shader->mShaderHash, binary_info);
                return true;
            }
        }
    }
    return false;
}

//virtual
void LLShaderMgr::initAttribsAndUniforms()
{
    //MUST match order of enum in LLVertexBuffer.h
    mReservedAttribs.push_back("position");
    mReservedAttribs.push_back("normal");
    mReservedAttribs.push_back("texcoord0");
    mReservedAttribs.push_back("texcoord1");
    mReservedAttribs.push_back("texcoord2");
    mReservedAttribs.push_back("texcoord3");
    mReservedAttribs.push_back("diffuse_color");
    mReservedAttribs.push_back("emissive");
    mReservedAttribs.push_back("tangent");
    mReservedAttribs.push_back("weight");
    mReservedAttribs.push_back("weight4");
    mReservedAttribs.push_back("clothing");
    mReservedAttribs.push_back("joint");
    mReservedAttribs.push_back("texture_index");

    //matrix state
    mReservedUniforms.push_back("modelview_matrix");
    mReservedUniforms.push_back("projection_matrix");
    mReservedUniforms.push_back("inv_proj");
    mReservedUniforms.push_back("modelview_projection_matrix");
    mReservedUniforms.push_back("inv_modelview");
    mReservedUniforms.push_back("identity_matrix");
    mReservedUniforms.push_back("normal_matrix");
    mReservedUniforms.push_back("texture_matrix0");
    mReservedUniforms.push_back("texture_matrix1");
    mReservedUniforms.push_back("texture_matrix2");
    mReservedUniforms.push_back("texture_matrix3");
    mReservedUniforms.push_back("object_plane_s");
    mReservedUniforms.push_back("object_plane_t");

    mReservedUniforms.push_back("texture_base_color_transform"); // (GLTF)
    mReservedUniforms.push_back("texture_normal_transform"); // (GLTF)
    mReservedUniforms.push_back("texture_metallic_roughness_transform"); // (GLTF)
    mReservedUniforms.push_back("texture_occlusion_transform"); // (GLTF)
    mReservedUniforms.push_back("texture_emissive_transform"); // (GLTF)
    mReservedUniforms.push_back("base_color_texcoord"); // (GLTF)
    mReservedUniforms.push_back("emissive_texcoord"); // (GLTF)
    mReservedUniforms.push_back("normal_texcoord"); // (GLTF)
    mReservedUniforms.push_back("metallic_roughness_texcoord"); // (GLTF)
    mReservedUniforms.push_back("occlusion_texcoord"); // (GLTF)
    mReservedUniforms.push_back("gltf_node_id"); // (GLTF)
    mReservedUniforms.push_back("gltf_material_id"); // (GLTF)

    mReservedUniforms.push_back("terrain_texture_transforms"); // (GLTF)
    mReservedUniforms.push_back("terrain_stamp_scale");

    llassert(mReservedUniforms.size() == LLShaderMgr::TERRAIN_STAMP_SCALE +1);

    mReservedUniforms.push_back("viewport");

    mReservedUniforms.push_back("light_position");
    mReservedUniforms.push_back("light_direction");
    mReservedUniforms.push_back("light_attenuation");
    mReservedUniforms.push_back("light_deferred_attenuation");
    mReservedUniforms.push_back("light_diffuse");
    mReservedUniforms.push_back("light_ambient");
    mReservedUniforms.push_back("light_count");
    mReservedUniforms.push_back("light");
    mReservedUniforms.push_back("light_col");
    mReservedUniforms.push_back("far_z");

    llassert(mReservedUniforms.size() == LLShaderMgr::MULTI_LIGHT_FAR_Z+1);

    //NOTE: MUST match order in eGLSLReservedUniforms
    mReservedUniforms.push_back("proj_mat");
    mReservedUniforms.push_back("proj_near");
    mReservedUniforms.push_back("proj_p");
    mReservedUniforms.push_back("proj_n");
    mReservedUniforms.push_back("proj_origin");
    mReservedUniforms.push_back("proj_range");
    mReservedUniforms.push_back("proj_ambiance");
    mReservedUniforms.push_back("proj_shadow_idx");
    mReservedUniforms.push_back("shadow_fade");
    mReservedUniforms.push_back("proj_focus");
    mReservedUniforms.push_back("proj_lod");
    mReservedUniforms.push_back("proj_ambient_lod");

    llassert(mReservedUniforms.size() == LLShaderMgr::PROJECTOR_AMBIENT_LOD+1);

    mReservedUniforms.push_back("color");
    mReservedUniforms.push_back("emissiveColor");
    mReservedUniforms.push_back("metallicFactor");
    mReservedUniforms.push_back("roughnessFactor");
    mReservedUniforms.push_back("mirror_flag");
    mReservedUniforms.push_back("clipPlane");
    mReservedUniforms.push_back("clipSign");

    mReservedUniforms.push_back("diffuseMap");
    mReservedUniforms.push_back("altDiffuseMap");
    mReservedUniforms.push_back("specularMap");
    mReservedUniforms.push_back("metallicRoughnessMap");
    mReservedUniforms.push_back("normalMap");
    mReservedUniforms.push_back("occlusionMap");
    mReservedUniforms.push_back("emissiveMap");
    mReservedUniforms.push_back("bumpMap");
    mReservedUniforms.push_back("bumpMap2");
    mReservedUniforms.push_back("environmentMap");
    mReservedUniforms.push_back("sceneMap");
    mReservedUniforms.push_back("sceneDepth");
    mReservedUniforms.push_back("reflectionProbes");
    mReservedUniforms.push_back("irradianceProbes");
    mReservedUniforms.push_back("heroProbes");
    mReservedUniforms.push_back("cloud_noise_texture");
    mReservedUniforms.push_back("cloud_noise_texture_next");
    mReservedUniforms.push_back("lightnorm");
    mReservedUniforms.push_back("sunlight_color");
    mReservedUniforms.push_back("ambient_color");
    mReservedUniforms.push_back("sky_hdr_scale");
    mReservedUniforms.push_back("sky_sunlight_scale");
    mReservedUniforms.push_back("sky_ambient_scale");
    mReservedUniforms.push_back("classic_mode");
    mReservedUniforms.push_back("blue_horizon");
    mReservedUniforms.push_back("blue_density");
    mReservedUniforms.push_back("haze_horizon");
    mReservedUniforms.push_back("haze_density");
    mReservedUniforms.push_back("cloud_shadow");
    mReservedUniforms.push_back("density_multiplier");
    mReservedUniforms.push_back("distance_multiplier");
    mReservedUniforms.push_back("max_y");
    mReservedUniforms.push_back("glow");
    mReservedUniforms.push_back("cloud_color");
    mReservedUniforms.push_back("cloud_pos_density1");
    mReservedUniforms.push_back("cloud_pos_density2");
    mReservedUniforms.push_back("cloud_scale");
    mReservedUniforms.push_back("gamma");
    mReservedUniforms.push_back("scene_light_strength");

    llassert(mReservedUniforms.size() == LLShaderMgr::SCENE_LIGHT_STRENGTH+1);

    mReservedUniforms.push_back("center");
    mReservedUniforms.push_back("size");
    mReservedUniforms.push_back("falloff");

    mReservedUniforms.push_back("box_center");
    mReservedUniforms.push_back("box_size");


    mReservedUniforms.push_back("minLuminance");
    mReservedUniforms.push_back("maxExtractAlpha");
    mReservedUniforms.push_back("lumWeights");
    mReservedUniforms.push_back("warmthWeights");
    mReservedUniforms.push_back("warmthAmount");
    mReservedUniforms.push_back("glowStrength");
    mReservedUniforms.push_back("glowDelta");
    mReservedUniforms.push_back("glowNoiseMap");

    llassert(mReservedUniforms.size() == LLShaderMgr::GLOW_NOISE_MAP+1);


    mReservedUniforms.push_back("minimum_alpha");
    mReservedUniforms.push_back("emissive_brightness");

    // Deferred
    mReservedUniforms.push_back("shadow_matrix");
    mReservedUniforms.push_back("env_mat");
    mReservedUniforms.push_back("shadow_clip");
    mReservedUniforms.push_back("ssao_radius");
    mReservedUniforms.push_back("ssao_max_radius");
    mReservedUniforms.push_back("ssao_factor");
    mReservedUniforms.push_back("ssao_factor_inv");
    mReservedUniforms.push_back("ssao_effect_mat");
    mReservedUniforms.push_back("screen_res");
    mReservedUniforms.push_back("near_clip");
    mReservedUniforms.push_back("shadow_offset");
    mReservedUniforms.push_back("shadow_bias");
    mReservedUniforms.push_back("spot_shadow_bias");
    mReservedUniforms.push_back("spot_shadow_offset");
    mReservedUniforms.push_back("sun_dir");
    mReservedUniforms.push_back("moon_dir");
    mReservedUniforms.push_back("shadow_res");
    mReservedUniforms.push_back("proj_shadow_res");
    mReservedUniforms.push_back("shadow_target_width");

    llassert(mReservedUniforms.size() == LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH + 1);

    mReservedUniforms.push_back("iterationCount");
    mReservedUniforms.push_back("rayStep");
    mReservedUniforms.push_back("distanceBias");
    mReservedUniforms.push_back("depthRejectBias");
    mReservedUniforms.push_back("glossySampleCount");
    mReservedUniforms.push_back("noiseSine");
    mReservedUniforms.push_back("adaptiveStepMultiplier");

    mReservedUniforms.push_back("modelview_delta");
    mReservedUniforms.push_back("inv_modelview_delta");
    mReservedUniforms.push_back("cube_snapshot");

    mReservedUniforms.push_back("tc_scale");
    mReservedUniforms.push_back("rcp_screen_res");
    mReservedUniforms.push_back("rcp_frame_opt");
    mReservedUniforms.push_back("rcp_frame_opt2");

    mReservedUniforms.push_back("focal_distance");
    mReservedUniforms.push_back("blur_constant");
    mReservedUniforms.push_back("tan_pixel_angle");
    mReservedUniforms.push_back("magnification");
    mReservedUniforms.push_back("max_cof");
    mReservedUniforms.push_back("res_scale");
    mReservedUniforms.push_back("dof_width");
    mReservedUniforms.push_back("dof_height");

    mReservedUniforms.push_back("depthMap");
    mReservedUniforms.push_back("shadowMap0");
    mReservedUniforms.push_back("shadowMap1");
    mReservedUniforms.push_back("shadowMap2");
    mReservedUniforms.push_back("shadowMap3");
    mReservedUniforms.push_back("shadowMap4");
    mReservedUniforms.push_back("shadowMap5");

    llassert(mReservedUniforms.size() == LLShaderMgr::DEFERRED_SHADOW5+1);

    mReservedUniforms.push_back("positionMap");
    mReservedUniforms.push_back("diffuseRect");
    mReservedUniforms.push_back("specularRect");
    mReservedUniforms.push_back("emissiveRect");
    mReservedUniforms.push_back("exposureMap");
    mReservedUniforms.push_back("brdfLut");
    mReservedUniforms.push_back("noiseMap");
    mReservedUniforms.push_back("lightFunc");
    mReservedUniforms.push_back("lightMap");
    mReservedUniforms.push_back("projectionMap");

    mReservedUniforms.push_back("specular_color");
    mReservedUniforms.push_back("env_intensity");

    mReservedUniforms.push_back("matrixPalette");
    mReservedUniforms.push_back("translationPalette");

    mReservedUniforms.push_back("screenTex");
    mReservedUniforms.push_back("screenDepth");
    mReservedUniforms.push_back("refTex");
    mReservedUniforms.push_back("eyeVec");
    mReservedUniforms.push_back("time");
    mReservedUniforms.push_back("waveDir1");
    mReservedUniforms.push_back("waveDir2");
    mReservedUniforms.push_back("lightDir");
    mReservedUniforms.push_back("specular");
    mReservedUniforms.push_back("lightExp");
    mReservedUniforms.push_back("waterFogColor");
    mReservedUniforms.push_back("waterFogColorLinear");
    mReservedUniforms.push_back("waterFogDensity");
    mReservedUniforms.push_back("waterFogKS");
    mReservedUniforms.push_back("refScale");
    mReservedUniforms.push_back("waterHeight");
    mReservedUniforms.push_back("waterPlane");
    mReservedUniforms.push_back("normScale");
    mReservedUniforms.push_back("fresnelScale");
    mReservedUniforms.push_back("fresnelOffset");
    mReservedUniforms.push_back("blurMultiplier");
    mReservedUniforms.push_back("sunAngle");
    mReservedUniforms.push_back("scaledAngle");
    mReservedUniforms.push_back("sunAngle2");

    mReservedUniforms.push_back("camPosLocal");

    mReservedUniforms.push_back("gWindDir");
    mReservedUniforms.push_back("gSinWaveParams");
    mReservedUniforms.push_back("gGravity");

    mReservedUniforms.push_back("detail_0");
    mReservedUniforms.push_back("detail_1");
    mReservedUniforms.push_back("detail_2");
    mReservedUniforms.push_back("detail_3");

    mReservedUniforms.push_back("alpha_ramp");
    mReservedUniforms.push_back("paint_map");

    mReservedUniforms.push_back("detail_0_base_color");
    mReservedUniforms.push_back("detail_1_base_color");
    mReservedUniforms.push_back("detail_2_base_color");
    mReservedUniforms.push_back("detail_3_base_color");
    mReservedUniforms.push_back("detail_0_normal");
    mReservedUniforms.push_back("detail_1_normal");
    mReservedUniforms.push_back("detail_2_normal");
    mReservedUniforms.push_back("detail_3_normal");
    mReservedUniforms.push_back("detail_0_metallic_roughness");
    mReservedUniforms.push_back("detail_1_metallic_roughness");
    mReservedUniforms.push_back("detail_2_metallic_roughness");
    mReservedUniforms.push_back("detail_3_metallic_roughness");
    mReservedUniforms.push_back("detail_0_emissive");
    mReservedUniforms.push_back("detail_1_emissive");
    mReservedUniforms.push_back("detail_2_emissive");
    mReservedUniforms.push_back("detail_3_emissive");

    mReservedUniforms.push_back("baseColorFactors");
    mReservedUniforms.push_back("metallicFactors");
    mReservedUniforms.push_back("roughnessFactors");
    mReservedUniforms.push_back("emissiveColors");
    mReservedUniforms.push_back("minimum_alphas");

    mReservedUniforms.push_back("region_scale");

    mReservedUniforms.push_back("origin");
    mReservedUniforms.push_back("display_gamma");

    mReservedUniforms.push_back("inscatter");
    mReservedUniforms.push_back("sun_size");
    mReservedUniforms.push_back("fog_color");

    mReservedUniforms.push_back("blend_factor");
    mReservedUniforms.push_back("moisture_level");
    mReservedUniforms.push_back("droplet_radius");
    mReservedUniforms.push_back("ice_level");
    mReservedUniforms.push_back("rainbow_map");
    mReservedUniforms.push_back("halo_map");
    mReservedUniforms.push_back("moon_brightness");
    mReservedUniforms.push_back("cloud_variance");
    mReservedUniforms.push_back("reflection_probe_ambiance");
    mReservedUniforms.push_back("max_probe_lod");
    mReservedUniforms.push_back("probe_strength");

    mReservedUniforms.push_back("sh_input_r");
    mReservedUniforms.push_back("sh_input_g");
    mReservedUniforms.push_back("sh_input_b");

    mReservedUniforms.push_back("sun_moon_glow_factor");
    mReservedUniforms.push_back("water_edge");
    mReservedUniforms.push_back("sun_up_factor");
    mReservedUniforms.push_back("moonlight_color");

    mReservedUniforms.push_back("debug_normal_draw_length");

    mReservedUniforms.push_back("edgesTex");
    mReservedUniforms.push_back("areaTex");
    mReservedUniforms.push_back("searchTex");
    mReservedUniforms.push_back("blendTex");

    llassert(mReservedUniforms.size() == END_RESERVED_UNIFORMS);

    std::set<std::string> dupe_check;

    for (U32 i = 0; i < mReservedUniforms.size(); ++i)
    {
        if (dupe_check.find(mReservedUniforms[i]) != dupe_check.end())
        {
            LL_ERRS() << "Duplicate reserved uniform name found: " << mReservedUniforms[i] << LL_ENDL;
        }
        dupe_check.insert(mReservedUniforms[i]);
    }
}

