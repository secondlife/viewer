/** 
 * @file llglslshader_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "../llviewershadermgr.h"

LLShaderMgr::LLShaderMgr() {}
LLShaderMgr::~LLShaderMgr() {}

LLViewerShaderMgr::LLViewerShaderMgr() {}
LLViewerShaderMgr::~LLViewerShaderMgr() {}

LLViewerShaderMgr* stub_instance = NULL;

LLViewerShaderMgr* LLViewerShaderMgr::instance() {
	if(NULL == stub_instance)
	{
		stub_instance = new LLViewerShaderMgr();
	}

	return stub_instance;
}
LLViewerShaderMgr::shader_iter fake_iter;
LLViewerShaderMgr::shader_iter LLViewerShaderMgr::beginShaders() const {return fake_iter;}
LLViewerShaderMgr::shader_iter LLViewerShaderMgr::endShaders() const {return fake_iter;}

void LLViewerShaderMgr::updateShaderUniforms(LLGLSLShader* shader) {return;}
std::string LLViewerShaderMgr::getShaderDirPrefix() {return "SHADER_DIR_PREFIX-";}
