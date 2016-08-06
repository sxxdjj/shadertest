//
//  ShaderManager.cpp
//  ProjectSLG
//
//  Created by shexinxing on 6/12/16.
//
//

#include "ShaderManager.hpp"

#define STRINGIFY(A)  #A

#include "../Shaders/Shader_PositionTextureColor_ETC1.frag"
#include "../Shaders/Shader_PositionTextureColor_ETC1_GrayScale.frag"
#include "../Shaders/Shader_Scrub.frag"
#include "../Shaders/Shader_Scrub_ETC1.frag"

ShaderManager::ShaderManager(){
}

ShaderManager::~ShaderManager(){
    
}

void ShaderManager::loadGLPrograms(){
    cocos2d::GLProgram *p = new (std::nothrow) cocos2d::GLProgram();
    loadDefaultGLProgram(p, ShaderType::kShaderType_PositionTextureColorETC1);
    cocos2d::GLProgramCache::getInstance()->addGLProgram(p, cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_ETC1);
    
    p = new (std::nothrow) cocos2d::GLProgram();
    loadDefaultGLProgram(p, ShaderType::kShaderType_PositionTextureColorETC1_GrayScale);
    cocos2d::GLProgramCache::getInstance()->addGLProgram(p, cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_ETC1_GRAY_SCALE);
    
    p = new (std::nothrow) cocos2d::GLProgram();
    loadDefaultGLProgram(p, ShaderType::kShaderType_PositionTextureColorETC1_NO_MVP);
    cocos2d::GLProgramCache::getInstance()->addGLProgram(p, cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR_ETC1_NO_MVP);
    
    p = new (std::nothrow) cocos2d::GLProgram();
    loadDefaultGLProgram(p, ShaderType::kShaderType_Scrub);
    cocos2d::GLProgramCache::getInstance()->addGLProgram(p, cocos2d::GLProgram::SHADER_NAME_SCRUB);
    
    p = new (std::nothrow) cocos2d::GLProgram();
    loadDefaultGLProgram(p, ShaderType::kShaderType_Scrub_ETC1);
    cocos2d::GLProgramCache::getInstance()->addGLProgram(p, cocos2d::GLProgram::SHADER_NAME_SCRUB_ETC1);
}

void ShaderManager::loadDefaultGLProgram(cocos2d::GLProgram *program, int type){
    switch (type) {
        case ShaderType::kShaderType_PositionTextureColorETC1:
            program->initWithByteArrays(cocos2d::ccPositionTextureColor_vert, positionTextureColorETC1_frag);
        break;
        case ShaderType::kShaderType_PositionTextureColorETC1_GrayScale:
            program->initWithByteArrays(cocos2d::ccPositionTextureColor_noMVP_vert, positionTextureColorETC1_GrayScale_frag);
        break;
        case ShaderType::kShaderType_PositionTextureColorETC1_NO_MVP:
            program->initWithByteArrays(cocos2d::ccPositionTextureColor_noMVP_vert, positionTextureColorETC1_frag);
        break;
        case ShaderType::kShaderType_Scrub:
            program->initWithByteArrays(cocos2d::ccPositionTextureColor_noMVP_vert, scrub_frag);
        break;
        case ShaderType::kShaderType_Scrub_ETC1:
            program->initWithByteArrays(cocos2d::ccPositionTextureColor_noMVP_vert, scrub_frag_etc1);
        break;
        default:
            CCLOG("cocos2d: %s:%d, error shader type", __FUNCTION__, __LINE__);
            return;
    }
    
    program->link();
    program->updateUniforms();
    
    CHECK_GL_ERROR_DEBUG();
}

cocos2d::GLProgramState* ShaderManager::createProgramState(ShaderType type, cocos2d::Texture2D* texture){
    switch (type) {
        case ShaderType::kShaderType_Scrub:
        case ShaderType::kShaderType_Scrub_ETC1:
        {
            cocos2d::GLProgramState* pState;
            if(texture && texture->getAlphaTexture())
            {
                pState = cocos2d::GLProgramState::create(cocos2d::GLProgramCache::getInstance()->getGLProgram(cocos2d::GLProgram::SHADER_NAME_SCRUB_ETC1));
                pState->setUniformTexture("u_texture", texture->getName());
                pState->setUniformTexture("alpha_texture", texture->getAlphaTexture()->getName());
            }
            else
            {
                pState = cocos2d::GLProgramState::create(cocos2d::GLProgramCache::getInstance()->getGLProgram(cocos2d::GLProgram::SHADER_NAME_SCRUB));
            }
            
            if(texture)
            {
                auto pSize = texture->getContentSizeInPixels();
                float pOffset = MAX(1.0f / pSize.width, 1.0f / pSize.height) * 3;
                pState->setUniformFloat("offset", pOffset);
                pState->getGLProgram()->updateUniforms();
            }
            return pState;
        }
        break;
        default:
            return nullptr;
    }
}